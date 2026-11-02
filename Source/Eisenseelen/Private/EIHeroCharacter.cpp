#include "EIHeroCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EIDataFile.h"
#include "EIHealthComponent.h"
#include "EIHeroMovementComponent.h"
#include "EIHordeDirector.h"
#include "EISwordComponent.h"
#include "EIViewModelComponent.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr float EyeHeightCm = 64.0f;
	constexpr float CameraKickSeconds = 0.14f;
	constexpr float CameraKickRollShare = 0.6f;
	constexpr float HurtFlashSeconds = 0.35f;
	constexpr float HurtKickDegrees = 2.0f;
	constexpr float DeadEyeDropCm = 110.0f;
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
	if (HasAuthority())
	{
		FEIHordeValues HordeValues;
		if (!FEIHordeValues::LoadFromProjectFile(HordeValues, Error))
		{
			FEIDataFile::ReportError(TEXT("gegner.json"), Error);
		}
		Health->Reset(HordeValues.HeroLife);
		Health->OnDied.AddUObject(this, &AEIHeroCharacter::HandleDied);
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
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	// No model yet, so the view simply drops to the floor.
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeightCm - DeadEyeDropCm));
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
	const float Applied = Health->ApplyDamage(Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser));
	if (Applied > 0.0f)
	{
		ClientHurt();
	}
	return Applied;
}

void AEIHeroCharacter::PressBattleCry()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (IsDead() || Now < NextBattleCryAt)
	{
		return;
	}
	NextBattleCryAt = Now + Values.BattleCryCooldownSeconds;
	ServerBattleCry();
}

void AEIHeroCharacter::ServerBattleCry_Implementation()
{
	MulticastBattleCry();
}

void AEIHeroCharacter::MulticastBattleCry_Implementation()
{
	if (IsLocallyControlled())
	{
		EIVisuals::PlaySound2D(this, TEXT("Schlachtruf"));
	}
	else
	{
		EIVisuals::PlaySoundAt(this, TEXT("Schlachtruf"), GetActorLocation());
	}
}

void AEIHeroCharacter::ApplyValues(const FEIHeroValues& NewValues)
{
	Values = NewValues;
	GetHeroMovement()->ApplyValues(Values);
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
		BufferedJumpUntil = GetWorld()->GetTimeSeconds() + Values.SpeedChainWindowSeconds;
	}
}

void AEIHeroCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
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

void AEIHeroCharacter::OnDashPerformed()
{
	if (IsLocallyControlled())
	{
		EIVisuals::PlaySound2D(this, TEXT("Dash"));
	}
}
