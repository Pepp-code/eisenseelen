#include "EIHeroCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Eisenseelen.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float EyeHeightCm = 64.0f;
}

AEIHeroCharacter::AEIHeroCharacter()
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
		// Editing the file by hand must not stop the game: say what is wrong and play on with the defaults.
		const FString Message = FString::Printf(TEXT("Data/held.json: %s Das Spiel nutzt die Standardwerte."), *Error);
		UE_LOG(LogEisenseelen, Error, TEXT("%s"), *Message);
		if (GEngine != nullptr)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 30.0f, FColor::Red, Message);
		}
	}
	ApplyValues(Loaded);
}

void AEIHeroCharacter::ApplyValues(const FEIHeroValues& NewValues)
{
	Values = NewValues;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = Values.RunSpeedMetersPerSecond * CentimetersPerMeter;
	Movement->AirControl = Values.AirControl;
	// A jump of height h starts at v = sqrt(2 * g * h).
	Movement->JumpZVelocity = FMath::Sqrt(2.0f * FMath::Abs(Movement->GetGravityZ()) * Values.JumpHeightMeters * CentimetersPerMeter);
}
