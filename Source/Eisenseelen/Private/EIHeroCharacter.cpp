#include "EIHeroCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Dom/JsonObject.h"
#include "EICasterComponent.h"
#include "EIDataFile.h"
#include "EIHealthComponent.h"
#include "EIHeroMovementComponent.h"
#include "EIOmenComponent.h"
#include "EIPlayerController.h"
#include "EISkyBarge.h"
#include "EISwordComponent.h"
#include "EIViewModelComponent.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float EyeHeightCm = 64.0f;
	constexpr float CameraKickSeconds = 0.14f;
	constexpr float CameraKickRollShare = 0.6f;
	constexpr float HurtFlashSeconds = 0.35f;
	constexpr float HurtKickDegrees = 2.0f;
	constexpr float DeadEyeDropCm = 110.0f;
	constexpr float HardLandingSpeedCm = 1500.0f;
	constexpr float HardLandingKickDegrees = 4.0f;
}

bool FEIClassValues::LoadFromProjectFile(EEIHeroClass HeroClass, FEIClassValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("klassen.json"), OutError);
	if (!Root.IsValid())
	{
		return false;
	}
	const TCHAR* Section = HeroClass == EEIHeroClass::Arcanist ? TEXT("arkanist") : TEXT("klingenwacht");
	const TSharedPtr<FJsonObject>* Object = nullptr;
	if (!Root->TryGetObjectField(Section, Object))
	{
		OutError = FString::Printf(TEXT("Der Abschnitt %s fehlt."), Section);
		return false;
	}
	FEIClassValues Loaded;
	if (!FEIDataFile::ReadNumber(**Object, TEXT("leben"), 1.0, 100000.0, Loaded.Life, OutError)
		|| !FEIDataFile::ReadNumber(**Object, TEXT("zauber_abklingzeit_faktor"), 0.05, 10.0, Loaded.SpellCooldownFactor, OutError)
		|| !FEIDataFile::ReadNumber(**Object, TEXT("mana"), 0.0, 100000.0, Loaded.Mana, OutError)
		|| !FEIDataFile::ReadNumber(**Object, TEXT("mana_pro_s"), 0.0, 100000.0, Loaded.ManaPerSecond, OutError))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}

AEIHeroCharacter::AEIHeroCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UEIHeroMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = true;
	BaseEyeHeight = EyeHeightCm;
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeightCm));
	Camera->bUsePawnControlRotation = true;

	ViewModel = CreateDefaultSubobject<UEIViewModelComponent>(TEXT("ViewModel"));
	ViewModel->SetupAttachment(Camera);

	Health = CreateDefaultSubobject<UEIHealthComponent>(TEXT("Health"));
	Sword = CreateDefaultSubobject<UEISwordComponent>(TEXT("Sword"));
	Caster = CreateDefaultSubobject<UEICasterComponent>(TEXT("Caster"));
	Omens = CreateDefaultSubobject<UEIOmenComponent>(TEXT("Omens"));
}

void AEIHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	FEIHeroValues Loaded;
	FString Error;
	if (!FEIHeroValues::LoadFromProjectFile(Loaded, Error))
	{
		FEIDataFile::ReportError(TEXT("held.json"), Error);
	}
	ApplyValues(Loaded);
	if (!FEIPoseLibrary::LoadFromProjectFile(Poses, Error))
	{
		FEIDataFile::ReportError(TEXT("posen.json"), Error);
	}
	Sword->LoadValues();
	Caster->LoadValues();
	Omens->LoadValues();
	LoadClassValues();
	Caster->RefillMana();
	if (HasAuthority())
	{
		Health->Reset(ClassValues.Life);
		Health->OnDied.AddUObject(this, &AEIHeroCharacter::HandleDied);
	}
}

void AEIHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIHeroCharacter, HeroClass);
}

void AEIHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (const AEIPlayerController* Player = Cast<AEIPlayerController>(NewController))
	{
		SetHeroClass(Player->GetSelectedClass());
	}
}

void AEIHeroCharacter::LoadClassValues()
{
	FString Error;
	if (!FEIClassValues::LoadFromProjectFile(HeroClass, ClassValues, Error))
	{
		FEIDataFile::ReportError(TEXT("klassen.json"), Error);
	}
}

void AEIHeroCharacter::SetHeroClass(EEIHeroClass NewClass)
{
	const float LifeShare = Health->GetHealth() / FMath::Max(Health->GetMaxHealth(), 1.0f);
	HeroClass = NewClass;
	LoadClassValues();
	const float LifeWithOmens = ClassValues.Life + Omens->GetBonus(EEIOmenEffect::Life);
	Health->Reset(LifeWithOmens);
	Health->ApplyDamage(LifeWithOmens * (1.0f - LifeShare));
	OnRep_HeroClass();
}

void AEIHeroCharacter::OnRep_HeroClass()
{
	LoadClassValues();
	Caster->RefillMana();
	Sword->Deactivate();
	Caster->Deactivate();
	ApplyClassToView();
}

void AEIHeroCharacter::ApplyClassToView()
{
	if (GetViewModel() != nullptr)
	{
		GetViewModel()->ShowStaff(HeroClass == EEIHeroClass::Arcanist);
	}
}

void AEIHeroCharacter::PressPrimary()
{
	if (HeroClass == EEIHeroClass::Arcanist)
	{
		Caster->FireBolt();
	}
	else
	{
		Sword->PressPrimary();
	}
}

void AEIHeroCharacter::ReleasePrimary()
{
	if (HeroClass != EEIHeroClass::Arcanist)
	{
		Sword->ReleasePrimary();
	}
}

void AEIHeroCharacter::PressSecondary()
{
	if (HeroClass == EEIHeroClass::Arcanist)
	{
		Caster->PressCharge();
	}
	else
	{
		Sword->PressBlock();
	}
}

void AEIHeroCharacter::ReleaseSecondary()
{
	Caster->ReleaseCharge();
	Sword->ReleaseBlock();
}

void AEIHeroCharacter::PressSpell()
{
	Caster->CastFireball();
}

bool AEIHeroCharacter::IsDead() const
{
	return Health->IsDead();
}

void AEIHeroCharacter::HandleDied()
{
	MulticastDied();
}

void AEIHeroCharacter::MulticastDied_Implementation()
{
	Sword->Deactivate();
	Caster->Deactivate();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	// No model yet, so the view simply drops to the floor and the hand is gone.
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeightCm - DeadEyeDropCm));
	ViewModel->SetVisibility(false, true);
}

void AEIHeroCharacter::ClientHurt_Implementation()
{
	HurtFlashRemaining = HurtFlashSeconds;
	AddCameraKick(HurtKickDegrees);
	EIVisuals::PlaySound2D(this, TEXT("GegnerSchlag"));
}

void AEIHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// A guest only learns that this hero is theirs some time after BeginPlay.
	if (!bViewModelBuilt && IsLocallyControlled())
	{
		bViewModelBuilt = true;
		ViewModel->Build();
		ApplyClassToView();
	}
	// Set every frame, because replaying a corrected move overwrites the wish with an older one.
	if (IsLocallyControlled())
	{
		GetHeroMovement()->SetWantsToSprint(bSprintKeyHeld);
	}
	UpdateCameraKick(DeltaSeconds);
	HurtFlashRemaining = FMath::Max(0.0f, HurtFlashRemaining - DeltaSeconds);
}

void AEIHeroCharacter::AddCameraKick(float Degrees)
{
	CameraKickDegrees = Degrees;
	CameraKickRemaining = CameraKickSeconds;
}

void AEIHeroCharacter::UpdateCameraKick(float DeltaSeconds)
{
	if (CameraKickRemaining <= 0.0f)
	{
		return;
	}
	CameraKickRemaining = FMath::Max(0.0f, CameraKickRemaining - DeltaSeconds);
	const float Strength = CameraKickDegrees * CameraKickRemaining / CameraKickSeconds;
	Camera->ClearAdditiveOffset();
	Camera->AddAdditiveOffset(FTransform(FRotator(-Strength, 0.0f, Strength * CameraKickRollShare)), 0.0f);
}

float AEIHeroCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Incoming = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	const float Applied = Health->ApplyDamage(Sword->BlockDamage(Incoming, DamageCauser));
	if (Applied > 0.0f)
	{
		ClientHurt();
	}
	return Applied;
}

void AEIHeroCharacter::ApplyValues(const FEIHeroValues& NewValues)
{
	Values = NewValues;
	GetHeroMovement()->ApplyValues(Values, Omens->GetFactor(EEIOmenEffect::Speed));
}

void AEIHeroCharacter::OnOmensChanged()
{
	GetHeroMovement()->ApplyValues(Values, Omens->GetFactor(EEIOmenEffect::Speed));
	Caster->SetManaFactor(Omens->GetFactor(EEIOmenEffect::ManaRegen));
}

UEIHeroMovementComponent* AEIHeroCharacter::GetHeroMovement() const
{
	return CastChecked<UEIHeroMovementComponent>(GetCharacterMovement());
}

bool AEIHeroCharacter::CanJumpWhileFalling() const
{
	return JumpCurrentCount == 0 && !GetHeroMovement()->IsDiving();
}

void AEIHeroCharacter::PressJump()
{
	if (CanJump())
	{
		Jump();
	}
	else
	{
		BufferedJumpUntil = GetWorld()->GetTimeSeconds() + Values.JumpBufferSeconds;
	}
}

void AEIHeroCharacter::BoardBarge(AEISkyBarge* Barge)
{
	bAboard = true;
	SetCanBeDamaged(false);
	MulticastBoardBarge(Barge);
}

void AEIHeroCharacter::MulticastBoardBarge_Implementation(AEISkyBarge* Barge)
{
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	SetActorLocation(Barge->GetDeckSpotNear(GetActorLocation(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	AttachToActor(Barge, FAttachmentTransformRules::KeepWorldTransform);
	if (IsLocallyControlled())
	{
		EIVisuals::PlaySound2D(this, TEXT("Sprungfeld"));
	}
}

void AEIHeroCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	// Landing from the barge hits hard for the eye and the ear, but it does no damage.
	if (IsLocallyControlled() && !bClientUpdating && GetVelocity().Z < -HardLandingSpeedCm)
	{
		AddCameraKick(HardLandingKickDegrees);
		EIVisuals::PlaySound2D(this, TEXT("TrefferSchwer"));
	}
	if (IsLocallyControlled() && GetWorld()->GetTimeSeconds() <= BufferedJumpUntil)
	{
		BufferedJumpUntil = -1.0;
		bJumpOnGround = true;
	}
}

void AEIHeroCharacter::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	// Landed comes before the switch to walking, and that switch clears any jump pressed earlier.
	if (bJumpOnGround && GetCharacterMovement()->IsMovingOnGround())
	{
		bJumpOnGround = false;
		Jump();
	}
}

void AEIHeroCharacter::OnDodgePerformed()
{
	if (IsLocallyControlled())
	{
		EIVisuals::PlaySound2D(this, TEXT("Dash"));
	}
}
