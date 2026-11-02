#include "EIHeroCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EIDataFile.h"
#include "EIHeroMovementComponent.h"
#include "EIVisuals.h"
#include "Engine/World.h"

namespace
{
	constexpr float EyeHeightCm = 64.0f;
}

AEIHeroCharacter::AEIHeroCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UEIHeroMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = true;
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeightCm));
	Camera->bUsePawnControlRotation = true;
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
