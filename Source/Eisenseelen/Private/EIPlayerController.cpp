#include "EIPlayerController.h"

#include "EIBoss.h"
#include "EIEnemy.h"
#include "EIForge.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHordeDirector.h"
#include "EIProfileComponent.h"
#include "EIRuneComponent.h"
#include "EISkyBarge.h"
#include "EISoulKettle.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

AEIPlayerController::AEIPlayerController()
{
	Profile = CreateDefaultSubobject<UEIProfileComponent>(TEXT("Profile"));
}

void AEIPlayerController::SetMenuOpen(bool bOpen)
{
	if (bOpen == bMenuOpen || (bOpen && !Profile->CanChangeGear()))
	{
		return;
	}
	bMenuOpen = bOpen;
	bShowMouseCursor = bOpen;
	if (bOpen)
	{
		Menu.ShowLootBook(false);
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
	if (AEIHeroCharacter* Hero = GetPawn<AEIHeroCharacter>())
	{
		// What was held down when the page opened would never get its release.
		Hero->SetSprintKey(false);
		Hero->ReleasePrimary();
		Hero->ReleaseSecondary();
	}
}

void AEIPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// Somebody else may have jumped off the barge in the meantime.
	if (bMenuOpen && !Profile->CanChangeGear())
	{
		SetMenuOpen(false);
	}
}

void AEIPlayerController::HandleMenu()
{
	SetMenuOpen(!bMenuOpen);
}

void AEIPlayerController::HandleMap()
{
	bMapOpen = !bMapOpen;
}

void AEIPlayerController::PreviewMenuSlot(int32 Slot)
{
	Menu.SelectSlot(static_cast<EEISlot>(FMath::Clamp(Slot, 0, EISlotCount - 1)));
}

void AEIPlayerController::PreviewLootBook(bool bShow)
{
	Menu.ShowLootBook(bShow);
}

void AEIPlayerController::PreviewMenuPage(int32 Page)
{
	Menu.ShowPage(static_cast<EEIMenuPage>(FMath::Clamp(Page, 0, static_cast<int32>(EEIMenuPage::Engravings))));
}

void AEIPlayerController::PreviewBuy(const FString& Name)
{
	Profile->BuyByName(Name);
}

void AEIPlayerController::PreviewPurse(int32 Gold, int32 Experience)
{
	Profile->GetSave().Gold = FMath::Max(Gold, 0);
	Profile->GetSave().Experience = FMath::Max(Experience, 0);
}

AEIForge* AEIPlayerController::FindForgeInReach() const
{
	const AEIHeroCharacter* Hero = GetPawn<AEIHeroCharacter>();
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (Hero == nullptr || Hero->IsDead() || Hero->IsAboard() || State == nullptr || State->GetRunPhase() != EEIRunPhase::AtDock)
	{
		return nullptr;
	}
	// Getting into the barge goes first, should both be in reach.
	for (TActorIterator<AEISkyBarge> It(GetWorld()); It; ++It)
	{
		if (It->IsInBoardingZone(Hero))
		{
			return nullptr;
		}
	}
	for (TActorIterator<AEIForge> It(GetWorld()); It; ++It)
	{
		if (It->IsInReach(Hero))
		{
			return *It;
		}
	}
	return nullptr;
}

void AEIPlayerController::PreviewEquip(const FString& Name)
{
	Profile->EquipByName(Name);
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

void AEIPlayerController::PreviewBloodMoon(bool bStands)
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->GetRun()->ForceBloodMoon(bStands);
	}
}

void AEIPlayerController::PreviewBeginRun()
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->GetRun()->BeginRunNow();
	}
}

void AEIPlayerController::PreviewSkipBannbruch()
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->GetRun()->SkipBannbruch();
	}
}

void AEIPlayerController::PreviewChooseBoss(int32 BossIndex)
{
	AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>();
	if (GameMode == nullptr)
	{
		return;
	}
	// The letter of that boss, if one hangs at the board, so that the screen says the same.
	const TArray<FEIHuntLetter>& Letters = GetWorld()->GetGameState<AEIGameState>()->GetHuntLetters();
	const int32 Letter = Letters.IndexOfByPredicate([BossIndex](const FEIHuntLetter& Each) { return Each.BossIndex == BossIndex; });
	if (Letter == INDEX_NONE || !GameMode->GetRun()->ChooseLetter(Letter))
	{
		GameMode->GetRun()->ForceBoss(BossIndex);
	}
}

void AEIPlayerController::PreviewKettleSouls(int32 Souls)
{
	AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>();
	AEISoulKettle* Kettle = GameMode != nullptr ? GameMode->GetRun()->GetKettle() : nullptr;
	if (Kettle != nullptr)
	{
		Kettle->SetSouls(FMath::Min(Souls, Kettle->GetCount() - 1));
		GetWorld()->GetGameState<AEIGameState>()->SetBannbruchProgress(Kettle->GetSouls(), Kettle->GetCount());
	}
}

void AEIPlayerController::PreviewHurryBoss()
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>(); GameMode != nullptr && GameMode->GetRun()->GetBoss() != nullptr)
	{
		GameMode->GetRun()->GetBoss()->HurryMoves();
	}
}

void AEIPlayerController::PreviewDefeatBoss()
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>(); GameMode != nullptr && GameMode->GetRun()->GetBoss() != nullptr)
	{
		GameMode->GetRun()->GetBoss()->DefeatSummons();
		GameMode->GetRun()->GetBoss()->TakeDamage(100000000.0f, FDamageEvent(), nullptr, nullptr);
	}
}

void AEIPlayerController::PreviewSkipContracts(int32 Count)
{
	if (AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->GetRun()->SkipContracts(Count);
	}
}

void AEIPlayerController::PreviewSpawnGroup(const FString& Name)
{
	constexpr float AheadCm = 1200.0f;
	constexpr float RadiusCm = 300.0f;
	AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>();
	const APawn* Hero = GetPawn();
	if (GameMode == nullptr || Hero == nullptr)
	{
		return;
	}
	for (const FEIEnemyGroup& Group : GameMode->GetRun()->GetGroups().Groups)
	{
		if (Group.Name == Name)
		{
			GameMode->GetHorde()->SpawnGroup(Group, 1.0f, Hero->GetActorLocation() + Hero->GetActorForwardVector() * AheadCm, RadiusCm);
		}
	}
}

void AEIPlayerController::PreviewDefeatGuards()
{
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		if (It->IsGuard())
		{
			It->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
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
	SprintAction = MakeAction(this, EInputActionValueType::Boolean);
	PrimaryAction = MakeAction(this, EInputActionValueType::Boolean);
	SecondaryAction = MakeAction(this, EInputActionValueType::Boolean);
	InteractAction = MakeAction(this, EInputActionValueType::Boolean);
	HordeAction = MakeAction(this, EInputActionValueType::Boolean);
	RestartAction = MakeAction(this, EInputActionValueType::Boolean);
	SpellAction = MakeAction(this, EInputActionValueType::Boolean);
	RuneSpellAction = MakeAction(this, EInputActionValueType::Boolean);
	ClassAction = MakeAction(this, EInputActionValueType::Boolean);
	MenuAction = MakeAction(this, EInputActionValueType::Boolean);
	MapAction = MakeAction(this, EInputActionValueType::Boolean);

	Mapping = NewObject<UInputMappingContext>(this);
	MapMoveKey(Mapping, MoveAction, EKeys::W, true, false);
	MapMoveKey(Mapping, MoveAction, EKeys::S, true, true);
	MapMoveKey(Mapping, MoveAction, EKeys::D, false, false);
	MapMoveKey(Mapping, MoveAction, EKeys::A, false, true);
	Mapping->MapKey(LookAction, EKeys::Mouse2D);
	Mapping->MapKey(JumpAction, EKeys::SpaceBar);
	Mapping->MapKey(SprintAction, EKeys::LeftShift);
	Mapping->MapKey(PrimaryAction, EKeys::LeftMouseButton);
	Mapping->MapKey(SecondaryAction, EKeys::RightMouseButton);
	Mapping->MapKey(InteractAction, EKeys::F);
	Mapping->MapKey(HordeAction, EKeys::H);
	Mapping->MapKey(RestartAction, EKeys::R);
	Mapping->MapKey(SpellAction, EKeys::Q);
	Mapping->MapKey(RuneSpellAction, EKeys::E);
	Mapping->MapKey(ClassAction, EKeys::C);
	Mapping->MapKey(MenuAction, EKeys::I);
	Mapping->MapKey(MapAction, EKeys::M);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEIPlayerController::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEIPlayerController::HandleLook);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleJumpStarted);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandleJumpEnded);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleSprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandleSprintEnded);
	Input->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AEIPlayerController::HandlePrimaryPressed);
	Input->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandlePrimaryReleased);
	Input->BindAction(SecondaryAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleSecondaryPressed);
	Input->BindAction(SecondaryAction, ETriggerEvent::Completed, this, &AEIPlayerController::HandleSecondaryReleased);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleInteract);
	Input->BindAction(HordeAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleHorde);
	Input->BindAction(RestartAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleRestart);
	Input->BindAction(SpellAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleSpell);
	Input->BindAction(RuneSpellAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleRuneSpell);
	Input->BindAction(ClassAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleClass);
	Input->BindAction(MenuAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleMenu);
	Input->BindAction(MapAction, ETriggerEvent::Started, this, &AEIPlayerController::HandleMap);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	Subsystem->AddMappingContext(Mapping, 0);
}

AEIHeroCharacter* AEIPlayerController::GetLivingHero() const
{
	AEIHeroCharacter* Hero = GetPawn<AEIHeroCharacter>();
	return Hero != nullptr && !Hero->IsDead() ? Hero : nullptr;
}

AEIHeroCharacter* AEIPlayerController::GetHeroForControls() const
{
	return bMenuOpen ? nullptr : GetLivingHero();
}

void AEIPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (APawn* Hero = GetHeroForControls())
	{
		const FVector2D Input = Value.Get<FVector2D>();
		Hero->AddMovementInput(Hero->GetActorForwardVector(), Input.Y);
		Hero->AddMovementInput(Hero->GetActorRightVector(), Input.X);
	}
}

void AEIPlayerController::HandleLook(const FInputActionValue& Value)
{
	if (!bMenuOpen)
	{
		ApplyLookInput(Value.Get<FVector2D>());
	}
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
	if (AEIHeroCharacter* Hero = GetHeroForControls())
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
	if (AEIHeroCharacter* Hero = GetHeroForControls())
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

void AEIPlayerController::HandlePrimaryPressed()
{
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (bMenuOpen && GetMousePosition(MouseX, MouseY))
	{
		Menu.Click(MouseX, MouseY);
	}
	if (AEIHeroCharacter* Hero = GetHeroForControls())
	{
		Hero->PressPrimary();
	}
}

void AEIPlayerController::HandlePrimaryReleased()
{
	if (AEIHeroCharacter* Hero = GetHeroForControls())
	{
		Hero->ReleasePrimary();
	}
}

void AEIPlayerController::HandleSecondaryPressed()
{
	if (AEIHeroCharacter* Hero = GetHeroForControls())
	{
		Hero->PressSecondary();
	}
}

void AEIPlayerController::HandleSecondaryReleased()
{
	if (AEIHeroCharacter* Hero = GetHeroForControls())
	{
		Hero->ReleaseSecondary();
	}
}

void AEIPlayerController::HandleInteract()
{
	if (bMenuOpen)
	{
		return;
	}
	// The forge is only a page of the menu, so the server need not know about it.
	if (FindForgeInReach() != nullptr)
	{
		SetMenuOpen(true);
		if (bMenuOpen)
		{
			Menu.ShowPage(EEIMenuPage::Forge);
		}
		return;
	}
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
	SetMenuOpen(false);
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
	if (AEIHeroCharacter* Hero = GetHeroForControls())
	{
		Hero->PressSpell();
	}
}

void AEIPlayerController::HandleRuneSpell()
{
	if (AEIHeroCharacter* Hero = GetHeroForControls())
	{
		Hero->PressRuneSpell();
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
