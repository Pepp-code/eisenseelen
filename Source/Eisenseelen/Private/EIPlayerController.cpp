#include "EIPlayerController.h"

#include "EIGameMode.h"
#include "Engine/World.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EIRunDirector.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputKeyEventArgs.h"
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

void AEIPlayerController::PreviewKey(FName KeyName, bool bPressed)
{
	InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, FKey(KeyName), bPressed ? IE_Pressed : IE_Released, FPlatformTime::Cycles64()));
}

void AEIPlayerController::PreviewInvulnerable()
{
	if (APawn* Hero = GetPawn())
	{
		Hero->SetCanBeDamaged(false);
	}
}

void AEIPlayerController::PreviewCorruption(int32 Stage)
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->GetRun()->SetCorruptionStage(Stage);
	}
}

void AEIPlayerController::PreviewBreakAltar()
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->GetRun()->SkipAltarHold();
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
	SprintAction = MakeAction(this, EInputActionValueType::Boolean);
	DodgeAction = MakeAction(this, EInputActionValueType::Boolean);
	PrimaryAction = MakeAction(this, EInputActionValueType::Boolean);
	SecondaryAction = MakeAction(this, EInputActionValueType::Boolean);
	InteractAction = MakeAction(this, EInputActionValueType::Boolean);
	HordeAction = MakeAction(this, EInputActionValueType::Boolean);
	RestartAction = MakeAction(this, EInputActionValueType::Boolean);
	SpellAction = MakeAction(this, EInputActionValueType::Boolean);
	ClassAction = MakeAction(this, EInputActionValueType::Boolean);

	Mapping = NewObject<UInputMappingContext>(this);
	MapMoveKey(Mapping, MoveAction, EKeys::W, true, false);
	MapMoveKey(Mapping, MoveAction, EKeys::S, true, true);
	MapMoveKey(Mapping, MoveAction, EKeys::D, false, false);
	MapMoveKey(Mapping, MoveAction, EKeys::A, false, true);
	Mapping->MapKey(LookAction, EKeys::Mouse2D);
	Mapping->MapKey(JumpAction, EKeys::SpaceBar);
	Mapping->MapKey(SprintAction, EKeys::LeftShift);
	Mapping->MapKey(DodgeAction, EKeys::V);
	Mapping->MapKey(PrimaryAction, EKeys::LeftMouseButton);
	Mapping->MapKey(SecondaryAction, EKeys::RightMouseButton);
	Mapping->MapKey(InteractAction, EKeys::F);
	Mapping->MapKey(HordeAction, EKeys::H);
	Mapping->MapKey(RestartAction, EKeys::R);
	Mapping->MapKey(SpellAction, EKeys::Q);
	Mapping->MapKey(ClassAction, EKeys::C);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEIPlayerController::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEIPlayerController::HandleLook);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleJumpStarted);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandleJumpEnded);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleSprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandleSprintEnded);
	Input->BindAction(DodgeAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleDodge);
	Input->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AEIPlayerController::HandlePrimaryPressed);
	Input->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandlePrimaryReleased);
	Input->BindAction(SecondaryAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleSecondaryPressed);
	Input->BindAction(SecondaryAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandleSecondaryReleased);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleInteract);
	Input->BindAction(HordeAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleHorde);
	Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleRestart);
	Input->BindAction(SpellAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleSpell);
	Input->BindAction(ClassAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleClass);

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

void AEIPlayerController::HandleSprintStarted()
{
	if (AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->SetSprintKey(true);
	}
}

void AEIPlayerController::HandleSprintEnded()
{
	if (AEIHeroCharacter* Hero = GetPawn<AEIHeroCharacter>())
	{
		Hero->SetSprintKey(false);
	}
}

void AEIPlayerController::HandleDodge()
{
	if (const AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->GetHeroMovement()->RequestDodge();
	}
}

void AEIPlayerController::HandlePrimaryPressed()
{
	if (AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->PressPrimary();
	}
}

void AEIPlayerController::HandlePrimaryReleased()
{
	if (AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->ReleasePrimary();
	}
}

void AEIPlayerController::HandleSecondaryPressed()
{
	if (AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->PressSecondary();
	}
}

void AEIPlayerController::HandleSecondaryReleased()
{
	if (AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->ReleaseSecondary();
	}
}

void AEIPlayerController::HandleInteract()
{
	ServerInteract();
}

void AEIPlayerController::ServerInteract_Implementation()
{
	AEIHeroCharacter* Hero = GetLivingHero();
	AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>();
	if (Hero != nullptr && GameMode != nullptr)
	{
		GameMode->GetRun()->Interact(Hero);
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

void AEIPlayerController::HandleSpell()
{
	if (AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->PressSpell();
	}
}

void AEIPlayerController::HandleClass()
{
	ServerToggleClass();
}

void AEIPlayerController::ServerToggleClass_Implementation()
{
	SelectedClass = SelectedClass == EEIHeroClass::Arcanist ? EEIHeroClass::Bladewarden : EEIHeroClass::Arcanist;
	if (AEIHeroCharacter* Hero = GetLivingHero())
	{
		Hero->SetHeroClass(SelectedClass);
	}
}
