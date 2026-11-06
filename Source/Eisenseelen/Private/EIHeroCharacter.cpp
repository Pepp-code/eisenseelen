#include "EIHeroCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Dom/JsonObject.h"
#include "EICasterComponent.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIGearComponent.h"
#include "EIHealthComponent.h"
#include "EIHeroBodyComponent.h"
#include "EIHeroMovementComponent.h"
#include "EIPlayerController.h"
#include "EIProfileComponent.h"
#include "EIProjectile.h"
#include "EIRuneComponent.h"
#include "EISkyBarge.h"
#include "EISoulKettle.h"
#include "EISwordComponent.h"
#include "EIViewModelComponent.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float EyeHeightCm = 64.0f;
	constexpr float CameraKickSeconds = 0.14f;
	constexpr float CameraKickRollShare = 0.6f;
	constexpr float HurtFlashSeconds = 0.35f;
	constexpr float HitNoteSeconds = 3.0f;
	constexpr float HurtKickDegrees = 2.0f;
	constexpr float DeadEyeDropCm = 110.0f;
	constexpr float HardLandingSpeedCm = 1500.0f;
	constexpr float HardLandingKickDegrees = 4.0f;
	constexpr float FieldOfViewBlendSpeed = 5.0f;
	constexpr float SprintFromRunShare = 1.15f;
	constexpr float CapsuleRadiusCm = 34.0f;
	constexpr float CapsuleHalfHeightCm = 88.0f;
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
	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadiusCm, CapsuleHalfHeightCm);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeightCm));
	Camera->bUsePawnControlRotation = true;

	ViewModel = CreateDefaultSubobject<UEIViewModelComponent>(TEXT("ViewModel"));
	ViewModel->SetupAttachment(Camera);

	// The body stands on the ground, so it starts at the bottom of the capsule.
	Body = CreateDefaultSubobject<UEIHeroBodyComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetRelativeLocation(FVector(0.0f, 0.0f, -CapsuleHalfHeightCm));

	Health = CreateDefaultSubobject<UEIHealthComponent>(TEXT("Health"));
	Sword = CreateDefaultSubobject<UEISwordComponent>(TEXT("Sword"));
	Caster = CreateDefaultSubobject<UEICasterComponent>(TEXT("Caster"));
	Runes = CreateDefaultSubobject<UEIRuneComponent>(TEXT("Runes"));
	Gear = CreateDefaultSubobject<UEIGearComponent>(TEXT("Gear"));
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
	if (!FEIHandMotion::LoadFromProjectFile(HandMotion, Error))
	{
		FEIDataFile::ReportError(TEXT("animation.json"), Error);
	}
	Sword->LoadValues();
	Caster->LoadValues();
	Runes->LoadValues();
	Gear->LoadValues();
	LoadClassValues();
	Gear->Recalculate();
	Caster->RefillMana();
	Body->Refresh();
	if (HasAuthority())
	{
		Health->Reset(ClassValues.Life + Gear->GetStat(EEIStat::Life));
		Health->OnDied.AddUObject(this, &AEIHeroCharacter::HandleDied);
	}
}

void AEIHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIHeroCharacter, HeroClass);
	DOREPLIFETIME(AEIHeroCharacter, bAboard);
	DOREPLIFETIME(AEIHeroCharacter, bCarriesSoulStone);
	DOREPLIFETIME(AEIHeroCharacter, CarriedCargoId);
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
	HeroClass = NewClass;
	LoadClassValues();
	if (AEIPlayerController* Player = GetController<AEIPlayerController>())
	{
		Player->GetProfile()->ApplyTo(this);
	}
	else
	{
		RefreshLife();
	}
	OnRep_HeroClass();
}

void AEIHeroCharacter::RefreshLife()
{
	if (IsDead())
	{
		return;
	}
	const float LifeShare = Health->GetHealth() / FMath::Max(Health->GetMaxHealth(), 1.0f);
	const float NewLife = ClassValues.Life + Gear->GetStat(EEIStat::Life);
	Health->Reset(NewLife);
	Health->ApplyDamage(NewLife * (1.0f - LifeShare));
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
	ApplyWeaponToView();
}

void AEIHeroCharacter::ApplyWeaponToView()
{
	const bool bStaff = HeroClass == EEIHeroClass::Arcanist;
	if (GetViewModel() != nullptr)
	{
		const FEIWeaponForm* Form = Gear->GetWeaponForm();
		GetViewModel()->SetWeaponLook(Form != nullptr ? Form->LengthFactor : 1.0f, EIElementColor(Gear->GetWeaponElement()));
		GetViewModel()->SetModels(TEXT("hand.") + EIHeroClassLookName(HeroClass), EIWeaponLookKeys(Gear, bStaff), bStaff);
	}
	Body->Refresh();
}

void AEIHeroCharacter::ShowLookEvent(FName Event)
{
	if (HasAuthority())
	{
		MulticastLookEvent(Event);
	}
}

void AEIHeroCharacter::MulticastLookEvent_Implementation(FName Event)
{
	Body->PlayEvent(Event);
	if (GetViewModel() != nullptr)
	{
		GetViewModel()->PlayLookEvent(Event);
	}
}

void AEIHeroCharacter::PressPrimary()
{
	if (IsCarryingSoulCargo())
	{
		DropSoulCargo(true);
		return;
	}
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
	if (IsCarryingSoulCargo())
	{
		return;
	}
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
	if (IsCarryingSoulCargo())
	{
		return;
	}
	Caster->CastSpell();
}

void AEIHeroCharacter::PressRuneSpell()
{
	if (IsCarryingSoulCargo())
	{
		return;
	}
	Runes->CastRune();
}

bool AEIHeroCharacter::PickUpSoulCargo(FName CargoId)
{
	if (IsCarryingSoulCargo())
	{
		return false;
	}

	const FEISoulCargoDef* Def = FEIDataFile::FindSoulCargoDef(CargoId);
	if (!Def)
	{
		return false;
	}

	CarriedCargoId = CargoId;
	CachedCargoDef = *Def;

	if (UEIHeroMovementComponent* MoveComp = Cast<UEIHeroMovementComponent>(GetCharacterMovement()))
	{
		MoveComp->SetCarrySpeedModifier(Def->CarrierSpeedFactor);
	}

	if (Body != nullptr)
	{
		Body->SetCarryingVisualState(true);
	}
	return true;
}

void AEIHeroCharacter::DropSoulCargo(bool bThrowForward)
{
	if (!IsCarryingSoulCargo())
	{
		return;
	}

	const FEISoulCargoDef CargoToRelease = CachedCargoDef;
	CarriedCargoId = NAME_None;

	if (UEIHeroMovementComponent* MoveComp = Cast<UEIHeroMovementComponent>(GetCharacterMovement()))
	{
		MoveComp->SetCarrySpeedModifier(1.0f);
	}

	if (Body != nullptr)
	{
		Body->SetCarryingVisualState(false);
	}

	if (bThrowForward)
	{
		const FVector Forward = GetActorForwardVector();
		const FVector SpawnLoc = GetActorLocation() + Forward * 120.0f + FVector(0.0f, 0.0f, 40.0f);
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AEIProjectile* Thrown = GetWorld()->SpawnActor<AEIProjectile>(AEIProjectile::StaticClass(), SpawnLoc, Forward.Rotation(), Params))
		{
			Thrown->InitAsThrownCargo(CargoToRelease.ThrowDamage, CargoToRelease.ThrowRadius, this);
			Thrown->FinishSpawning(FTransform(Forward.Rotation(), SpawnLoc));
		}
	}
	else
	{
		TArray<AActor*> OverlappingKettles;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEISoulKettle::StaticClass(), OverlappingKettles);
		for (AActor* Actor : OverlappingKettles)
		{
			if (AEISoulKettle* Kettle = Cast<AEISoulKettle>(Actor))
			{
				if (FVector::Dist(GetActorLocation(), Kettle->GetActorLocation()) <= 500.0f)
				{
					Kettle->DeliverSoulCargo(CargoToRelease, this);
					break;
				}
			}
		}
	}
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
	// The view simply drops to the floor and the hand is gone, while the others see the body fall.
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeightCm - DeadEyeDropCm));
	ViewModel->SetVisibility(false, true);
	Body->ShowDeath();
}

void AEIHeroCharacter::ClientHurt_Implementation(FVector_NetQuantize From, const FString& What, float Amount)
{
	HurtFlashRemaining = HurtFlashSeconds;
	LastHitFrom = From;
	LastHitNote = What.IsEmpty() ? FString::Printf(TEXT("-%.0f"), Amount) : FString::Printf(TEXT("-%.0f  %s"), Amount, *What);
	HitNoteRemaining = HitNoteSeconds;
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
		UpdateFieldOfView(DeltaSeconds);
	}
	UpdateCameraKick(DeltaSeconds);
	HurtFlashRemaining = FMath::Max(0.0f, HurtFlashRemaining - DeltaSeconds);
	HitNoteRemaining = FMath::Max(0.0f, HitNoteRemaining - DeltaSeconds);
	if (HasAuthority())
	{
		UpdateRecovery(DeltaSeconds);
	}
}

void AEIHeroCharacter::UpdateFieldOfView(float DeltaSeconds)
{
	const UEIHeroMovementComponent* Movement = GetHeroMovement();
	float Target = HandMotion.FieldOfViewDegrees;
	if (Movement->IsMovingOnGround() && GetVelocity().Size2D() > Movement->MaxWalkSpeed * SprintFromRunShare)
	{
		Target += HandMotion.SprintFieldOfViewPlusDegrees;
	}
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, Target, DeltaSeconds, FieldOfViewBlendSpeed));
}

void AEIHeroCharacter::NoteCombat()
{
	LastCombatAt = GetWorld()->GetTimeSeconds();
}

void AEIHeroCharacter::UpdateRecovery(float DeltaSeconds)
{
	const bool bQuiet = GetWorld()->GetTimeSeconds() - LastCombatAt >= Values.LifeBackAfterSeconds;
	const bool bRecovering = bQuiet && !IsDead() && Health->GetHealth() < Health->GetMaxHealth();
	Health->SetRecovering(bRecovering);
	if (bRecovering)
	{
		Health->Heal(Health->GetMaxHealth() * Values.LifeBackSharePerSecond * DeltaSeconds);
	}
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
	if (Incoming > 0.0f)
	{
		NoteCombat();
	}
	const float Applied = Health->ApplyDamage(Sword->BlockDamage(Incoming, DamageCauser) * (1.0f - Gear->GetArmorShare()));
	if (Applied > 0.0f)
	{
		// A thrown bone tells of whoever threw it, so the screen points there.
		const AActor* Source = DamageCauser != nullptr && DamageCauser->GetInstigator() != nullptr ? DamageCauser->GetInstigator() : DamageCauser;
		ClientHurt(Source != nullptr ? Source->GetActorLocation() : GetActorLocation(), AEIEnemy::DescribeAttackOf(DamageCauser), Applied);
		if (!IsDead())
		{
			ShowLookEvent(TEXT("treffer"));
		}
	}
	return Applied;
}

void AEIHeroCharacter::ApplyValues(const FEIHeroValues& NewValues)
{
	Values = NewValues;
	GetHeroMovement()->ApplyValues(Values, Gear->GetSpeedFactor());
}

void AEIHeroCharacter::OnGearChanged()
{
	GetHeroMovement()->ApplyValues(Values, Gear->GetSpeedFactor());
	Caster->SetManaFactor(1.0f + Gear->GetStat(EEIStat::ManaRegen));
	Sword->ApplyGear(*Gear);
	Caster->ApplyGear(*Gear);
	const FEIItem* Rune = Gear->GetWorn(EEISlot::Rune);
	Runes->SetRune(Rune != nullptr ? Runes->GetTable().FindByName(Rune->Name) : INDEX_NONE);
	ApplyWeaponToView();
}

UEIHeroMovementComponent* AEIHeroCharacter::GetHeroMovement() const
{
	return CastChecked<UEIHeroMovementComponent>(GetCharacterMovement());
}

bool AEIHeroCharacter::CanJumpWhileFalling() const
{
	return JumpCurrentCount == 0;
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
	LocationBeforeBoarding = GetActorLocation();
	bDamageableBeforeBoarding = CanBeDamaged();
	SetCanBeDamaged(false);
	MulticastBoardBarge(Barge);
}

void AEIHeroCharacter::ArriveAt(const FVector& Location, float YawDegrees)
{
	if (bAboard)
	{
		bAboard = false;
		SetCanBeDamaged(bDamageableBeforeBoarding);
	}
	MulticastArriveAt(Location, YawDegrees);
}

void AEIHeroCharacter::MulticastArriveAt_Implementation(const FVector& Location, float YawDegrees)
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	const FRotator Facing(0.0f, YawDegrees, 0.0f);
	SetActorLocationAndRotation(Location, Facing, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (AController* Player = GetController())
	{
		Player->SetControlRotation(Facing);
	}
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
	if (IsLocallyControlled() && !bClientUpdating && GetViewModel() != nullptr)
	{
		GetViewModel()->NoteLanding(FMath::Clamp(static_cast<float>(-GetVelocity().Z) / HardLandingSpeedCm, 0.0f, 1.0f));
	}
	// Landing from high up hits hard for the eye and the ear, but it does no damage.
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
