#include "EIPlayerController.h"

#include "EIGameMode.h"
#include "Engine/World.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EISwordComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace
{
	UInputAction* MakeAction(UObject* Outer, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer);
		Action->ValueType = ValueType;
		return Action;
	}

	/** A key sends its value on X, so forward and backward are swizzled onto Y. */
	void MapMoveKey(UInputMappingContext* Mapping, const UInputAction* Action, const FKey& Key, bool bOntoY, bool bNegate)
	{
		FEnhancedActionKeyMapping& KeyMapping = Mapping->MapKey(Action, Key);
		if (bOntoY)
		{
			KeyMapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Mapping));
		}
		if (bNegate)
		{
			KeyMapping.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
		}
	}
}

void AEIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!IsLocalPlayerController())
	{
		return;
	}

	MoveAction = MakeAction(this, EInputActionValueType::Axis2D);
	LookAction = MakeAction(this, EInputActionValueType::Axis2D);
	JumpAction = MakeAction(this, EInputActionValueType::Boolean);
	DashAction = MakeAction(this, EInputActionValueType::Boolean);
	PrimaryAction = MakeAction(this, EInputActionValueType::Boolean);
	BattleCryAction = MakeAction(this, EInputActionValueType::Boolean);
	HordeAction = MakeAction(this, EInputActionValueType::Boolean);
	RestartAction = MakeAction(this, EInputActionValueType::Boolean);

	Mapping = NewObject<UInputMappingContext>(this);
	MapMoveKey(Mapping, MoveAction, EKeys::W, true, false);
	MapMoveKey(Mapping, MoveAction, EKeys::S, true, true);
	MapMoveKey(Mapping, MoveAction, EKeys::D, false, false);
	MapMoveKey(Mapping, MoveAction, EKeys::A, false, true);
	Mapping->MapKey(LookAction, EKeys::Mouse2D);
	Mapping->MapKey(JumpAction, EKeys::SpaceBar);
	Mapping->MapKey(DashAction, EKeys::LeftShift);
	Mapping->MapKey(PrimaryAction, EKeys::LeftMouseButton);
	Mapping->MapKey(BattleCryAction, EKeys::V);
	Mapping->MapKey(HordeAction, EKeys::H);
	Mapping->MapKey(RestartAction, EKeys::R);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEIPlayerController::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEIPlayerController::HandleLook);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleJumpStarted);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandleJumpEnded);
	Input->BindAction(DashAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleDash);
	Input->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AEIPlayerController::HandlePrimaryPressed);
	Input->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandlePrimaryReleased);
	Input->BindAction(BattleCryAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleBattleCry);
	Input->BindAction(HordeAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleHorde);
	Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleRestart);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	Subsystem->AddMappingContext(Mapping, 0);
}

AEIHeroCharacter* AEIPlayerController::GetLivingHero() const
{
	AEIHeroCharacter* Hero = GetPawn<AEIHeroCharacter>();
	return Hero != nullptr && !Hero->IsDead() ? Hero : nullptr;
}

void AEIPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (APawn* Hero = GetLivingHero())
	{
		const FVector2D Input = Value.Get<FVector2D>();
		Hero->AddMovementInput(Hero->GetActorForwardVector(), Input.Y);
		Hero->AddMovementInput(Hero->GetActorRightVector(), Input.X);
	}
}

void AEIPlayerController::HandleLook(const FInputActionValue& Value)
{
	ApplyLookInput(Value.Get<FVector2D>());
}

void AEIPlayerController::ApplyLookInput(const FVector2D& Input)
{
	const AEIHeroCharacter* Hero = GetPawn<AEIHeroCharacter>();
	const float MouseFactor = Hero != nullptr ? Hero->GetValues().MouseFactor : 1.0f;
	AddYawInput(Input.X * MouseFactor);
	// The engine's pitch scale is negative, so the sign flips here to make mouse up look up.
	AddPitchInput(-Input.Y * MouseFactor);
}

void AEIPlayerController::HandleJumpStarted()
{
	if (AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->PressJump();
	}
}

void AEIPlayerController::HandleJumpEnded()
{
	if (ACharacter* Hero = GetCharacter())
	{
		Hero->StopJumping();
	}
}

void AEIPlayerController::HandleDash()
{
	if (const AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->GetHeroMovement()->RequestDash();
	}
}

void AEIPlayerController::HandlePrimaryPressed()
{
	if (const AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->GetSword()->PressPrimary();
	}
}

void AEIPlayerController::HandlePrimaryReleased()
{
	if (const AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->GetSword()->ReleasePrimary();
	}
}

void AEIPlayerController::HandleBattleCry()
{
	if (AEIHeroCharacter* Hero = GetPawn<AEIHeroCharacter>())
	{
		Hero->PressBattleCry();
	}
}

void AEIPlayerController::HandleHorde()
{
	ServerToggleHorde();
}

void AEIPlayerController::HandleRestart()
{
	ServerRestartRun();
}

void AEIPlayerController::ServerToggleHorde_Implementation()
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->ToggleHorde();
	}
}

void AEIPlayerController::ServerRestartRun_Implementation()
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->RestartRun();
	}
}
