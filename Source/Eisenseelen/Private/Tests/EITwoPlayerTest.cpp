#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EICasterComponent.h"
#include "EIAnchorStone.h"
#include "EIBoss.h"
#include "EIChest.h"
#include "EIEnemy.h"
#include "EIGearComponent.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EIProfileComponent.h"
#include "EISwordComponent.h"
#include "EIProjectile.h"
#include "EIRunDirector.h"
#include "EIRuneComponent.h"
#include "EISignalFire.h"
#include "EIRuneField.h"
#include "EISkyBarge.h"
#include "EITrainingDummy.h"
#include "EngineUtils.h"
#include "EIPlayerController.h"
#include "EITestKeys.h"
#include "Editor.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
	const TCHAR* ArenaMap = TEXT("/Game/Maps/L_Testarena");
	constexpr double TimeoutSeconds = 180.0;
	constexpr float MaxDriftCm = 25.0f;
	constexpr float LeaderScale = 1.25f;

	/** The worlds and heroes of a session with a host and one guest, once all of them exist. */
	struct FTwoPlayerSession
	{
		UWorld* ClientWorld = nullptr;
		AEIPlayerController* ClientPlayer = nullptr;
		AEIHeroCharacter* ClientHero = nullptr;
		AEIHeroCharacter* ServerCopyOfClientHero = nullptr;

		static FTwoPlayerSession Find()
		{
			FTwoPlayerSession Session;
			UWorld* ServerWorld = nullptr;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType != EWorldType::PIE || World == nullptr)
				{
					continue;
				}
				if (World->GetNetMode() == NM_ListenServer)
				{
					ServerWorld = World;
				}
				else if (World->GetNetMode() == NM_Client)
				{
					Session.ClientWorld = World;
				}
			}
			if (ServerWorld == nullptr || Session.ClientWorld == nullptr)
			{
				return Session;
			}
			Session.ClientPlayer = Cast<AEIPlayerController>(Session.ClientWorld->GetFirstPlayerController());
			Session.ClientHero = Session.ClientPlayer != nullptr ? Cast<AEIHeroCharacter>(Session.ClientPlayer->GetPawn()) : nullptr;
			for (FConstPlayerControllerIterator It = ServerWorld->GetPlayerControllerIterator(); It; ++It)
			{
				if (!It->Get()->IsLocalController())
				{
					Session.ServerCopyOfClientHero = Cast<AEIHeroCharacter>(It->Get()->GetPawn());
				}
			}
			return Session;
		}

		bool IsReady() const
		{
			return ClientHero != nullptr && ServerCopyOfClientHero != nullptr && ClientHero->HasActorBegunPlay()
				&& ClientHero->GetHeroMovement()->IsMovingOnGround();
		}
	};
}

/**
 * Plays as the guest of a real listen server: the guest sprints and jumps, the prediction must show it at once, and
 * afterwards guest and server must agree on where the hero stands.
 */
class FEITwoPlayerSprintCommand : public IAutomationLatentCommand
{
public:
	explicit FEITwoPlayerSprintCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - StartedAt > TimeoutSeconds)
		{
			Test->AddError(FString::Printf(TEXT("The two-player test timed out in step %d"), Step));
			return true;
		}
		const FTwoPlayerSession Session = FTwoPlayerSession::Find();
		if (!Session.IsReady() && Step == 0)
		{
			return false;
		}
		if (Session.ClientHero == nullptr || Session.ServerCopyOfClientHero == nullptr)
		{
			Test->AddError(TEXT("The guest disappeared from the session"));
			return true;
		}

		const double Now = Session.ClientWorld->GetTimeSeconds();
		const auto EnterStep = [this, Now](int32 NewStep)
		{
			Step = NewStep;
			StepAt = Now;
		};
		switch (Step)
		{
		case 0:
			Session.ClientPlayer->SetControlRotation(FRotator(0.0f, 180.0f, 0.0f));
			EnterStep(1);
			return false;
		case 1:
			if (Now - StepAt < 0.5)
			{
				return false;
			}
			StartLocation = Session.ClientHero->GetActorLocation();
			EISendTestKey(Session.ClientPlayer, EKeys::W, IE_Pressed);
			EnterStep(2);
			return false;
		case 2:
			if (Now - StepAt < 0.6)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::LeftShift, IE_Pressed);
			EnterStep(3);
			return false;
		case 3:
			if (Now - StepAt < 0.6)
			{
				return false;
			}
			GuestSpeed = static_cast<float>(Session.ClientHero->GetVelocity().Size2D());
			Test->AddInfo(FString::Printf(TEXT("The guest sprints at %.0f cm/s, the server sees %.0f cm/s"), GuestSpeed, Session.ServerCopyOfClientHero->GetVelocity().Size2D()));
			Test->TestTrue(TEXT("The guest feels the sprint at once"), GuestSpeed > Session.ClientHero->GetValues().RunSpeedMetersPerSecond * 100.0f + 100.0f);
			Test->TestTrue(TEXT("The server lets the guest sprint"), Session.ServerCopyOfClientHero->GetVelocity().Size2D() > Session.ClientHero->GetValues().RunSpeedMetersPerSecond * 100.0f + 100.0f);
			EISendTestKey(Session.ClientPlayer, EKeys::SpaceBar, IE_Pressed);
			EnterStep(4);
			return false;
		case 4:
			if (Now - StepAt < 0.08)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::SpaceBar, IE_Released);
			Test->TestTrue(TEXT("The guest feels the jump at once"), Session.ClientHero->GetHeroMovement()->IsFalling());
			// Steering in the air is part of the predicted move, so it must not make guest and server drift apart.
			Session.ClientPlayer->SetControlRotation(FRotator(0.0f, 230.0f, 0.0f));
			EnterStep(5);
			return false;
		case 5:
			if (Now - StepAt < 0.1)
			{
				return false;
			}
			Test->TestTrue(TEXT("The server lets the guest jump"), Session.ServerCopyOfClientHero->GetHeroMovement()->IsFalling());
			EISendTestKey(Session.ClientPlayer, EKeys::LeftShift, IE_Released);
			EISendTestKey(Session.ClientPlayer, EKeys::W, IE_Released);
			EnterStep(6);
			return false;
		case 6:
			if (Now - StepAt < 1.2)
			{
				return false;
			}
			CheckAgreement(Session);
			return true;
		default:
			return true;
		}
	}

private:
	void CheckAgreement(const FTwoPlayerSession& Session) const
	{
		const FVector GuestLocation = Session.ClientHero->GetActorLocation();
		const float Moved = static_cast<float>(FVector::Dist2D(StartLocation, GuestLocation));
		const float Drift = static_cast<float>(FVector::Dist(GuestLocation, Session.ServerCopyOfClientHero->GetActorLocation()));
		Test->AddInfo(FString::Printf(TEXT("The guest moved %.0f cm, guest and server differ by %.1f cm"), Moved, Drift));
		Test->TestTrue(TEXT("Sprint and jump really moved the guest"), Moved > 500.0f);
		Test->TestTrue(TEXT("Guest and server agree on the position"), Drift < MaxDriftCm);
	}

	FAutomationTestBase* Test;
	double StartedAt;
	int32 Step = 0;
	double StepAt = 0.0;
	FVector StartLocation = FVector::ZeroVector;
	float GuestSpeed = 0.0f;
};

/**
 * The guest fights: a sword swing, a staff bolt and a charged fireball must count on the server and show up on the
 * guest's machine, and the server has the last word on the guest's mana.
 */
class FEITwoPlayerCombatCommand : public IAutomationLatentCommand
{
public:
	explicit FEITwoPlayerCombatCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - StartedAt > TimeoutSeconds)
		{
			Test->AddError(FString::Printf(TEXT("The two-player combat test timed out in step %d"), Step));
			return true;
		}
		const FTwoPlayerSession Session = FTwoPlayerSession::Find();
		if (!Session.IsReady() && Step == 0)
		{
			return false;
		}
		if (Session.ClientHero == nullptr || Session.ServerCopyOfClientHero == nullptr)
		{
			Test->AddError(TEXT("The guest disappeared from the session"));
			return true;
		}

		const double Now = Session.ClientWorld->GetTimeSeconds();
		const auto EnterStep = [this, Now](int32 NewStep)
		{
			Step = NewStep;
			StepAt = Now;
		};
		AEITrainingDummy* ServerDummy = FindMiddleDummy(Session.ServerCopyOfClientHero->GetWorld());
		const AEITrainingDummy* GuestDummy = FindMiddleDummy(Session.ClientWorld);
		if (ServerDummy == nullptr || GuestDummy == nullptr)
		{
			return false;
		}
		switch (Step)
		{
		case 0:
			// The server moves the guest in front of the dummy, as a teleporter would.
			Session.ServerCopyOfClientHero->TeleportTo(ServerDummy->GetActorLocation() - FVector(180.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
			EnterStep(1);
			return false;
		case 1:
			if (Now - StepAt < 0.6)
			{
				return false;
			}
			Session.ClientPlayer->SetControlRotation(FRotator::ZeroRotator);
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Pressed);
			EnterStep(2);
			return false;
		case 2:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Released);
			EnterStep(3);
			return false;
		case 3:
			if (Now - StepAt < 0.8)
			{
				return false;
			}
			Test->AddInfo(FString::Printf(TEXT("The guest's sword: the server counts %d hits for %.0f damage, the guest sees %d hits"), ServerDummy->GetHitCount(), ServerDummy->GetLastDamage(), GuestDummy->GetHitCount()));
			Test->TestEqual(TEXT("The guest's swing counts on the server"), ServerDummy->GetHitCount(), 1);
			Test->TestEqual(TEXT("The guest sees the hit"), GuestDummy->GetHitCount(), 1);
			ServerDummy->ResetHits();
			EISendTestKey(Session.ClientPlayer, EKeys::C, IE_Pressed);
			EnterStep(4);
			return false;
		case 4:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::C, IE_Released);
			EnterStep(5);
			return false;
		case 5:
			if (Now - StepAt < 0.5)
			{
				return false;
			}
			Test->TestEqual(TEXT("The guest becomes an arcanist on the server"), Session.ServerCopyOfClientHero->GetHeroClass(), EEIHeroClass::Arcanist);
			Test->TestEqual(TEXT("The guest learns about it"), Session.ClientHero->GetHeroClass(), EEIHeroClass::Arcanist);
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Pressed);
			EnterStep(6);
			return false;
		case 6:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Released);
			EnterStep(7);
			return false;
		case 7:
		{
			if (Now - StepAt < 0.1)
			{
				return false;
			}
			const UEICasterComponent* GuestCaster = Session.ClientHero->GetCaster();
			const UEICasterComponent* ServerCaster = Session.ServerCopyOfClientHero->GetCaster();
			Test->AddInfo(FString::Printf(TEXT("After the bolt the guest has %.1f mana, the server says %.1f"), GuestCaster->GetMana(), ServerCaster->GetMana()));
			Test->TestTrue(TEXT("The bolt costs the guest mana"), GuestCaster->GetMana() < GuestCaster->GetMaxMana());
			Test->TestNearlyEqual(TEXT("Guest and server agree on the mana"), GuestCaster->GetMana(), ServerCaster->GetMana(), 3.0f);
			EnterStep(8);
			return false;
		}
		case 8:
			if (Now - StepAt < 0.7)
			{
				return false;
			}
			Test->AddInfo(FString::Printf(TEXT("The guest's bolt: the server counts %d hits for %.0f damage"), ServerDummy->GetHitCount(), ServerDummy->GetLastDamage()));
			Test->TestEqual(TEXT("The guest's bolt counts on the server"), ServerDummy->GetHitCount(), 1);
			ServerDummy->ResetHits();
			EISendTestKey(Session.ClientPlayer, EKeys::RightMouseButton, IE_Pressed);
			EnterStep(9);
			return false;
		case 9:
			if (Now - StepAt < 0.6)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::RightMouseButton, IE_Released);
			EnterStep(10);
			return false;
		case 10:
			if (Now - StepAt < 0.7)
			{
				return false;
			}
			Test->AddInfo(FString::Printf(TEXT("The guest's charged fireball: %d hits for %.0f damage at 180 cm"), ServerDummy->GetHitCount(), ServerDummy->GetLastDamage()));
			Test->TestEqual(TEXT("The guest's fireball counts on the server"), ServerDummy->GetLastDamage(), Session.ClientHero->GetCaster()->GetValues().Fireball.Damage * Session.ClientHero->GetGear()->GetShotFactor());
			ServerDummy->ResetHits();
			// Only the server's mana runs out, so the guest still fires and the server has to refuse.
			Session.ServerCopyOfClientHero->GetCaster()->SpendMana(Session.ServerCopyOfClientHero->GetCaster()->GetMaxMana());
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Pressed);
			EnterStep(11);
			return false;
		case 11:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Released);
			EnterStep(12);
			return false;
		case 12:
			if (Now - StepAt < 0.5)
			{
				return false;
			}
			Test->TestEqual(TEXT("The server refuses a bolt without mana"), ServerDummy->GetHitCount(), 0);
			return true;
		default:
			return true;
		}
	}

private:
	static AEITrainingDummy* FindMiddleDummy(UWorld* World)
	{
		TArray<AEITrainingDummy*> Dummies;
		for (TActorIterator<AEITrainingDummy> It(World); It; ++It)
		{
			Dummies.Add(*It);
		}
		Dummies.Sort([](const AEITrainingDummy& A, const AEITrainingDummy& B) { return A.GetActorLocation().Y < B.GetActorLocation().Y; });
		return Dummies.Num() == 3 ? Dummies[1] : nullptr;
	}

	FAutomationTestBase* Test;
	double StartedAt;
	int32 Step = 0;
	double StepAt = 0.0;
};

/**
 * Guest and host board together at the dock and arrive together. Then the guest plays alone, the host has fallen: a chest
 * calls its guards on the server, the use key opens it once they have fallen and lights the fire, the guest sees how the
 * run goes on, and the barge takes the guest home.
 */
class FEITwoPlayerRunCommand : public IAutomationLatentCommand
{
public:
	explicit FEITwoPlayerRunCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - StartedAt > TimeoutSeconds)
		{
			Test->AddError(FString::Printf(TEXT("The two-player run test timed out in step %d"), Step));
			return true;
		}
		const FTwoPlayerSession Session = FTwoPlayerSession::Find();
		if (!Session.IsReady() && Step == 0)
		{
			return false;
		}
		if (Session.ClientHero == nullptr || Session.ServerCopyOfClientHero == nullptr)
		{
			Test->AddError(TEXT("The guest disappeared from the session"));
			return true;
		}

		const double Now = Session.ClientWorld->GetTimeSeconds();
		const auto EnterStep = [this, Now](int32 NewStep)
		{
			Step = NewStep;
			StepAt = Now;
		};
		UWorld* ServerWorld = Session.ServerCopyOfClientHero->GetWorld();
		AEIGameMode* GameMode = ServerWorld->GetAuthGameMode<AEIGameMode>();
		const AEIGameState* GuestState = Session.ClientWorld->GetGameState<AEIGameState>();
		switch (Step)
		{
		case 0:
		{
			Test->TestEqual(TEXT("The guest knows the run has not begun"), GuestState->GetRunPhase(), EEIRunPhase::AtDock);
			FEIRunValues Short = GameMode->GetRun()->GetValues();
			Short.SignalHoldSeconds = 1.0f;
			Short.CampFactor = 0.0f;
			Short.CastOffSeconds = 0.3f;
			Short.CrossingSeconds = 0.3f;
			GameMode->GetRun()->ApplyValues(Short);
			GameMode->GetRun()->GetContractTable().Tasks.RemoveAll([](const FEIContractTask& Task) { return Task.Kind != EEIContractKind::Chest; });
			GameMode->GetRun()->GetBossTable().Bosses.Empty();
			GameMode->GetRun()->ResetRun();

			// Both walk up to the barge, and the guest goes first.
			const FVector Barge = GameMode->GetRun()->GetBarge()->GetActorLocation();
			Session.ServerCopyOfClientHero->TeleportTo(FVector(Barge.X - 300.0f, Barge.Y - 500.0f, Barge.Z + 100.0f), FRotator::ZeroRotator);
			for (FConstPlayerControllerIterator It = ServerWorld->GetPlayerControllerIterator(); It; ++It)
			{
				if (It->Get()->IsLocalController())
				{
					HostHero = It->Get()->GetPawn<AEIHeroCharacter>();
					HostHero->TeleportTo(FVector(Barge.X + 300.0f, Barge.Y - 500.0f, Barge.Z + 100.0f), FRotator::ZeroRotator);
				}
			}
			EnterStep(100);
			return false;
		}
		case 100:
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Pressed);
			EnterStep(101);
			return false;
		case 101:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Released);
			EnterStep(102);
			return false;
		case 102:
		{
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			Test->TestTrue(TEXT("The server takes the guest aboard"), Session.ServerCopyOfClientHero->IsAboard());
			Test->TestTrue(TEXT("The guest knows about it"), Session.ClientHero->IsAboard());
			Test->TestTrue(TEXT("The guest rides the barge"), Session.ClientHero->GetAttachParentActor() != nullptr);
			Test->TestEqual(TEXT("The barge waits for the host"), GuestState->GetRunPhase(), EEIRunPhase::AtDock);
			Test->TestFalse(TEXT("It does not cast off yet"), GuestState->IsCastingOff());

			// Now the host goes aboard too, and they leave together.
			GameMode->GetRun()->Interact(HostHero);
			EnterStep(103);
			return false;
		}
		case 103:
		{
			if (GuestState->GetRunPhase() != EEIRunPhase::Exploring)
			{
				return false;
			}
			if (Now - StepAt < 1.0)
			{
				return false;
			}
			Test->TestFalse(TEXT("The guest is off the barge"), Session.ClientHero->IsAboard());
			Test->TestNull(TEXT("The guest rides nothing any more"), Session.ClientHero->GetAttachParentActor());
			Test->TestTrue(TEXT("The team stands together"), FVector::Dist(Session.ServerCopyOfClientHero->GetActorLocation(), HostHero->GetActorLocation()) < 1000.0);
			Test->TestTrue(TEXT("The guest sees the same place as the server"), FVector::Dist(Session.ClientHero->GetActorLocation(), Session.ServerCopyOfClientHero->GetActorLocation()) < 200.0);
			Test->TestEqual(TEXT("The guest knows the side jobs"), GuestState->GetContracts().Num(), 3);

			// From here on the guest is alone: the host has fallen.
			HostHero->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
			Session.ServerCopyOfClientHero->SetCanBeDamaged(false);
			for (TActorIterator<AEIChest> Chest(ServerWorld); Chest; ++Chest)
			{
				if (Chest->IsInUse())
				{
					Session.ServerCopyOfClientHero->TeleportTo(Chest->GetActorLocation() + FVector(200.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
					OpenedChest = *Chest;
					break;
				}
			}
			EnterStep(1);
			return false;
		}
		case 1:
		{
			if (Now - StepAt < 0.6)
			{
				return false;
			}
			int32 GuardsTheGuestSees = 0;
			for (TActorIterator<AEIChest> Chest(Session.ClientWorld); Chest; ++Chest)
			{
				GuardsTheGuestSees += Chest->GetGuardsLeft();
			}
			Test->TestTrue(TEXT("The server has called the guards"), OpenedChest != nullptr && OpenedChest->GetGuardsLeft() > 0);
			Test->TestEqual(TEXT("The guest sees how many guards there are"), GuardsTheGuestSees, OpenedChest != nullptr ? OpenedChest->GetGuardsLeft() : -1);
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Pressed);
			EnterStep(2);
			return false;
		}
		case 2:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Released);
			EnterStep(3);
			return false;
		case 3:
		{
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			Test->TestEqual(TEXT("The seal holds while guards live"), GuestState->CountContractsDone(), 0);
			for (TActorIterator<AEIEnemy> Enemy(ServerWorld); Enemy; ++Enemy)
			{
				Enemy->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
			}
			EnterStep(4);
			return false;
		}
		case 4:
		{
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			int32 FreeChestsTheGuestSees = 0;
			for (TActorIterator<AEIChest> Chest(Session.ClientWorld); Chest; ++Chest)
			{
				FreeChestsTheGuestSees += Chest->GetState() == EEIChestState::Free ? 1 : 0;
			}
			Test->TestEqual(TEXT("The guest sees the seal break"), FreeChestsTheGuestSees, 1);
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Pressed);
			EnterStep(5);
			return false;
		}
		case 5:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Released);
			EnterStep(6);
			return false;
		case 6:
		{
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			Test->TestTrue(TEXT("The chest is open on the server"), OpenedChest != nullptr && OpenedChest->IsOpen());
			Test->TestEqual(TEXT("The guest sees the contract done"), GuestState->CountContractsDone(), 1);
			const UEIProfileComponent* GuestProfile = Session.ClientPlayer->GetProfile();
			Test->TestEqual(TEXT("The guest earns the gold"), GuestProfile->GetRunGold(), GuestProfile->GetContractTable().GetGold());
			Test->TestTrue(TEXT("The guest is told"), GuestProfile->GetRewardNote() > 0.0f);
			const AEIPlayerController* Host = Cast<AEIPlayerController>(ServerWorld->GetFirstPlayerController());
			Test->TestEqual(TEXT("The fallen host earns nothing"), Host->GetProfile()->GetRunGold(), 0);
			Session.ServerCopyOfClientHero->TeleportTo(GameMode->GetRun()->GetActiveFire()->GetActorLocation() + FVector(200.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
			EnterStep(7);
			return false;
		}
		case 7:
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Pressed);
			EnterStep(8);
			return false;
		case 8:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Released);
			EnterStep(9);
			return false;
		case 9:
		{
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			Test->TestEqual(TEXT("The guest's use key lights the fire"), GuestState->GetRunPhase(), EEIRunPhase::SignalLit);
			bool bAnyLit = false;
			for (TActorIterator<AEISignalFire> GuestFire(Session.ClientWorld); GuestFire; ++GuestFire)
			{
				bAnyLit = bAnyLit || GuestFire->IsLit();
			}
			Test->TestTrue(TEXT("The guest sees the fire"), bAnyLit);
			EnterStep(10);
			return false;
		}
		case 10:
			if (GuestState->GetRunPhase() != EEIRunPhase::BargeWaiting)
			{
				return false;
			}
			Session.ServerCopyOfClientHero->TeleportTo(GameMode->GetRun()->GetBarge()->GetActorLocation() + FVector(0.0f, 0.0f, -40.0f), FRotator::ZeroRotator);
			EnterStep(11);
			return false;
		case 11:
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Pressed);
			EnterStep(12);
			return false;
		case 12:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::F, IE_Released);
			EnterStep(13);
			return false;
		case 13:
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			Test->TestEqual(TEXT("The guest gets away"), GuestState->GetRunPhase(), EEIRunPhase::Escaped);
			Test->TestNotNull(TEXT("The guest stands on the barge"), Session.ClientHero->GetAttachParentActor());
			Test->TestTrue(TEXT("The guest's gold is safe on the guest's side"), Session.ClientPlayer->GetProfile()->GetSave().Gold > 0);
			HeightAtBoarding = Session.ClientHero->GetActorLocation().Z;
			EnterStep(14);
			return false;
		case 14:
			if (Now - StepAt < 0.6)
			{
				return false;
			}
			Test->TestTrue(TEXT("The barge carries the guest away"), Session.ClientHero->GetActorLocation().Z > HeightAtBoarding + 100.0);
			return true;
		default:
			return true;
		}
	}

private:
	FAutomationTestBase* Test;
	double StartedAt;
	int32 Step = 0;
	double StepAt = 0.0;
	double HeightAtBoarding = 0.0;
	const AEIChest* OpenedChest = nullptr;
	AEIHeroCharacter* HostHero = nullptr;
};

/**
 * Guest and host meet a boss: the guest sees it bound and big, breaks a stone with the use of its own feet, sees it break
 * free with more life for two, and each player gets their own loot on their own side.
 */
class FEITwoPlayerBossCommand : public IAutomationLatentCommand
{
public:
	explicit FEITwoPlayerBossCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - StartedAt > TimeoutSeconds)
		{
			Test->AddError(FString::Printf(TEXT("The two-player boss test timed out in step %d"), Step));
			return true;
		}
		const FTwoPlayerSession Session = FTwoPlayerSession::Find();
		if (!Session.IsReady() && Step == 0)
		{
			return false;
		}
		if (Session.ClientHero == nullptr || Session.ServerCopyOfClientHero == nullptr)
		{
			Test->AddError(TEXT("The guest disappeared from the session"));
			return true;
		}
		const double Now = Session.ClientWorld->GetTimeSeconds();
		const auto EnterStep = [this, Now](int32 NewStep)
		{
			Step = NewStep;
			StepAt = Now;
		};
		UWorld* ServerWorld = Session.ServerCopyOfClientHero->GetWorld();
		AEIGameMode* GameMode = ServerWorld->GetAuthGameMode<AEIGameMode>();
		const AEIGameState* GuestState = Session.ClientWorld->GetGameState<AEIGameState>();
		const auto FindGuestBoss = [&Session]() -> const AEIBoss*
		{
			TActorIterator<AEIBoss> It(Session.ClientWorld);
			return It ? *It : nullptr;
		};
		switch (Step)
		{
		case 0:
		{
			UEIRunDirector* Run = GameMode->GetRun();
			FEIRunValues Short = Run->GetValues();
			Short.CampFactor = 0.0f;
			Run->ApplyValues(Short);
			Run->GetContractTable().ContractsPerRun = 0.0f;
			Run->GetGroups().AltarWave.Size.Empty();
			Run->GetGroups().FireWave.Size.Empty();
			for (FEIBossValues& Boss : Run->GetBossTable().Bosses)
			{
				Boss.Bannbruch.HoldSeconds = 0.3f;
				for (FEILootItem& Item : Boss.Loot)
				{
					Item.Chance = 1.0f;
				}
			}
			Run->ResetRun();
			Run->BeginRunNow();
			Session.ServerCopyOfClientHero->SetCanBeDamaged(false);
			for (FConstPlayerControllerIterator It = ServerWorld->GetPlayerControllerIterator(); It; ++It)
			{
				if (It->Get()->IsLocalController())
				{
					Host = Cast<AEIPlayerController>(It->Get());
					Host->GetPawn()->SetCanBeDamaged(false);
				}
			}
			EnterStep(1);
			return false;
		}
		case 1:
		{
			if (Now - StepAt < 0.6)
			{
				return false;
			}
			const AEIBoss* Boss = FindGuestBoss();
			Test->TestNotNull(TEXT("The guest sees the boss"), Boss);
			if (Boss == nullptr || Boss->GetBossValues() == nullptr)
			{
				return true;
			}
			Test->TestEqual(TEXT("The guest knows it is sealed"), GuestState->GetBossStage(), EEIBossStage::Sealed);
			Test->TestTrue(TEXT("The guest sees it bound"), Boss->IsBound());
			Test->TestEqual(TEXT("The guest sees it big"), static_cast<float>(Boss->GetActorScale3D().X), Boss->GetBossValues()->Fight.Scale);
			Test->TestEqual(TEXT("The guest knows who it is"), Boss->GetBossValues()->Name, GameMode->GetRun()->GetBossTable().Bosses[0].Name);
			int32 Stones = 0;
			for (TActorIterator<AEIAnchorStone> It(Session.ClientWorld); It; ++It)
			{
				++Stones;
			}
			Test->TestEqual(TEXT("The guest sees the stones"), Stones, GuestState->GetBannbruchTotal());
			// Three stones, broken by the guest standing in their zones.
			Session.ServerCopyOfClientHero->TeleportTo(GameMode->GetRun()->GetAnchors()[0]->GetActorLocation() + FVector(200.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
			EnterStep(2);
			return false;
		}
		case 2:
		{
			if (GuestState->GetBannbruchDone() < 1)
			{
				return false;
			}
			Test->TestEqual(TEXT("The guest's feet break the stone"), GuestState->GetBannbruchDone(), 1);
			GameMode->GetRun()->SkipBannbruch();
			EnterStep(3);
			return false;
		}
		case 3:
		{
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			const AEIBoss* Boss = FindGuestBoss();
			Test->TestEqual(TEXT("The guest knows the fight is on"), GuestState->GetBossStage(), EEIBossStage::Awake);
			Test->TestFalse(TEXT("The guest sees it free"), Boss->IsBound());
			const FEIBossTable& Table = GameMode->GetRun()->GetBossTable();
			const float Expected = Table.Bosses[0].Fight.Life * (1.0f + Table.LifePerExtraPlayer);
			Test->TestEqual(TEXT("With two players it has more life, and the guest knows"), Boss->GetHealth()->GetMaxHealth(), Expected);
			GameMode->GetRun()->GetBoss()->TakeDamage(100000000.0f, FDamageEvent(), nullptr, nullptr);
			EnterStep(4);
			return false;
		}
		case 4:
		{
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			const FEIBossValues& Boss = GameMode->GetRun()->GetBossTable().Bosses[0];
			const UEIProfileComponent* Guest = Session.ClientPlayer->GetProfile();
			Test->TestEqual(TEXT("The guest knows the boss is down"), GuestState->GetBossStage(), EEIBossStage::Down);
			Test->TestTrue(TEXT("The guest knows the fire can burn"), GuestState->CanLightSignal());
			Test->TestTrue(TEXT("The guest gets gold"), Guest->GetRunGold() >= FMath::RoundToInt32(Boss.GoldFrom));
			Test->TestEqual(TEXT("The guest gets the ingredients"), Guest->GetRunIngredients().Num(), Boss.Loot.Num());
			Test->TestTrue(TEXT("The guest is told"), Guest->GetRewardText().Contains(Boss.Name));
			Test->TestEqual(TEXT("The host rolls for themselves"), Host->GetProfile()->GetRunIngredients().Num(), Boss.Loot.Num());
			return true;
		}
		default:
			return true;
		}
	}

private:
	FAutomationTestBase* Test;
	double StartedAt;
	int32 Step = 0;
	double StepAt = 0.0;
	AEIPlayerController* Host = nullptr;
};

/**
 * The guest blocks: the block breaks off a swing on the server too, the server must know about the block from the
 * guest's moves alone, and it decides about the hit.
 */
class FEITwoPlayerBlockCommand : public IAutomationLatentCommand
{
public:
	explicit FEITwoPlayerBlockCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - StartedAt > TimeoutSeconds)
		{
			Test->AddError(FString::Printf(TEXT("The two-player block test timed out in step %d"), Step));
			return true;
		}
		const FTwoPlayerSession Session = FTwoPlayerSession::Find();
		if (!Session.IsReady() && Step == 0)
		{
			return false;
		}
		if (Session.ClientHero == nullptr || Session.ServerCopyOfClientHero == nullptr)
		{
			Test->AddError(TEXT("The guest disappeared from the session"));
			return true;
		}

		constexpr float TestHit = 10.0f;
		const double Now = Session.ClientWorld->GetTimeSeconds();
		const auto EnterStep = [this, Now](int32 NewStep)
		{
			Step = NewStep;
			StepAt = Now;
		};
		AEIHeroCharacter* ServerHero = Session.ServerCopyOfClientHero;
		// In front of the guest, as the place a hit comes from.
		const FVector Ahead = ServerHero->GetActorLocation() + ServerHero->GetActorForwardVector() * 300.0f;
		switch (Step)
		{
		case 0:
			Session.ClientPlayer->SetControlRotation(FRotator::ZeroRotator);
			EnterStep(1);
			return false;
		case 1:
			if (Now - StepAt < 0.5)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Pressed);
			EnterStep(2);
			return false;
		case 2:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Released);
			EnterStep(3);
			return false;
		case 3:
			if (Now - StepAt < 0.06)
			{
				return false;
			}
			Test->TestTrue(TEXT("The guest's swing runs on the server"), ServerHero->GetSword()->IsSwinging());
			EISendTestKey(Session.ClientPlayer, EKeys::RightMouseButton, IE_Pressed);
			EnterStep(4);
			return false;
		case 4:
			if (Now - StepAt < 0.1)
			{
				return false;
			}
			Test->TestFalse(TEXT("The guest's block breaks off the swing on the guest"), Session.ClientHero->GetSword()->IsSwinging());
			Test->TestFalse(TEXT("The guest's block breaks off the swing on the server"), ServerHero->GetSword()->IsSwinging());
			EnterStep(5);
			return false;
		case 5:
		{
			if (Now - StepAt < 0.6)
			{
				return false;
			}
			Test->TestTrue(TEXT("The guest's block shows on the guest"), Session.ClientHero->GetHeroMovement()->IsBlocking());
			Test->TestTrue(TEXT("The server knows about the guest's block"), ServerHero->GetHeroMovement()->IsBlocking());
			AActor* Causer = ServerHero->GetWorld()->SpawnActor<AEITrainingDummy>(AEITrainingDummy::StaticClass(), Ahead, FRotator::ZeroRotator);
			const float Before = ServerHero->GetHealth()->GetHealth();
			ServerHero->TakeDamage(TestHit, FDamageEvent(), nullptr, Causer);
			const float Expected = TestHit * (1.0f - ServerHero->GetSword()->GetValues().Block.DamageReduction);
			Test->TestNearlyEqual(TEXT("The server reduces the hit"), Before - ServerHero->GetHealth()->GetHealth(), Expected, 0.01f);
			Causer->Destroy();
			EnterStep(6);
			return false;
		}
		case 6:
			if (Now - StepAt < 0.1)
			{
				return false;
			}
			Test->TestTrue(TEXT("The guest sees the block catch the hit"), Session.ClientHero->GetSword()->GetBlockFlash() > 0.0f);
			EISendTestKey(Session.ClientPlayer, EKeys::RightMouseButton, IE_Released);
			EnterStep(7);
			return false;
		case 7:
		{
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			Test->TestFalse(TEXT("The server sees the block end"), ServerHero->GetHeroMovement()->IsBlocking());
			AActor* Causer = ServerHero->GetWorld()->SpawnActor<AEITrainingDummy>(AEITrainingDummy::StaticClass(), Ahead, FRotator::ZeroRotator);
			const float Before = ServerHero->GetHealth()->GetHealth();
			ServerHero->TakeDamage(TestHit, FDamageEvent(), nullptr, Causer);
			Test->TestNearlyEqual(TEXT("Without the block the whole hit counts"), Before - ServerHero->GetHealth()->GetHealth(), TestHit, 0.01f);
			Causer->Destroy();
			return true;
		}
		default:
			return true;
		}
	}

private:
	FAutomationTestBase* Test;
	double StartedAt;
	int32 Step = 0;
	double StepAt = 0.0;
};

/**
 * The guest equips a rune and casts its spell: the server has the last word on the cooldown, and everyone sees the spell.
 */
class FEITwoPlayerRuneCommand : public IAutomationLatentCommand
{
public:
	explicit FEITwoPlayerRuneCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - StartedAt > TimeoutSeconds)
		{
			Test->AddError(FString::Printf(TEXT("The two-player rune test timed out in step %d"), Step));
			return true;
		}
		const FTwoPlayerSession Session = FTwoPlayerSession::Find();
		if (!Session.IsReady() && Step == 0)
		{
			return false;
		}
		if (Session.ClientHero == nullptr || Session.ServerCopyOfClientHero == nullptr)
		{
			Test->AddError(TEXT("The guest disappeared from the session"));
			return true;
		}

		const double Now = Session.ClientWorld->GetTimeSeconds();
		const auto EnterStep = [this, Now](int32 NewStep)
		{
			Step = NewStep;
			StepAt = Now;
		};
		const auto CountFields = [](UWorld* World)
		{
			int32 Count = 0;
			for (TActorIterator<AEIRuneField> It(World); It; ++It)
			{
				++Count;
			}
			return Count;
		};
		UWorld* ServerWorld = Session.ServerCopyOfClientHero->GetWorld();
		switch (Step)
		{
		case 0:
		{
			Session.ServerCopyOfClientHero->SetCanBeDamaged(false);
			Session.ClientPlayer->SetControlRotation(FRotator(-20.0f, 0.0f, 0.0f));
			const FVector Spot = Session.ServerCopyOfClientHero->GetActorLocation() + FVector(420.0f, 0.0f, 0.0f);
			if (AEIEnemy* Enemy = ServerWorld->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Spot, FRotator(0.0f, 180.0f, 0.0f)))
			{
				Enemy->GetHealth()->Reset(100000.0f);
				Target = Enemy;
			}
			Session.ClientPlayer->GetProfile()->EquipNext(EEISlot::Rune);
			EnterStep(2);
			return false;
		}
		case 2:
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			Test->TestEqual(TEXT("The server takes the guest's rune"), Session.ServerCopyOfClientHero->GetRunes()->GetRuneIndex(), 1);
			Test->TestEqual(TEXT("The guest knows the rune"), Session.ClientHero->GetRunes()->GetRuneIndex(), 1);
			EISendTestKey(Session.ClientPlayer, EKeys::E, IE_Pressed);
			EnterStep(3);
			return false;
		case 3:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::E, IE_Released);
			Test->TestTrue(TEXT("The guest sees the cooldown at once"), Session.ClientHero->GetRunes()->GetCooldownFraction() > 0.0f);
			EnterStep(4);
			return false;
		case 4:
			if (Now - StepAt < 0.4)
			{
				return false;
			}
			Test->TestEqual(TEXT("The server spawns the spell"), CountFields(ServerWorld), 1);
			Test->TestEqual(TEXT("The guest sees the spell"), CountFields(Session.ClientWorld), 1);
			Test->TestTrue(TEXT("The server counts the cooldown too"), Session.ServerCopyOfClientHero->GetRunes()->GetCooldownFraction() > 0.0f);
			Test->TestTrue(TEXT("The spell of the guest hurts on the server"), Target.IsValid() && Target->GetHealth()->GetHealth() < 100000.0f);
			EISendTestKey(Session.ClientPlayer, EKeys::E, IE_Pressed);
			EnterStep(5);
			return false;
		case 5:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::E, IE_Released);
			EnterStep(6);
			return false;
		case 6:
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			Test->TestEqual(TEXT("During the cooldown no second spell comes"), CountFields(ServerWorld), 1);
			// Another weapon: both sides have to agree on how it plays.
			Session.ClientPlayer->GetProfile()->EquipByName(TEXT("Aschenbrecher"));
			Session.ClientPlayer->GetProfile()->EquipByName(TEXT("Schuhe der Henkersgarde"));
			EnterStep(7);
			return false;
		case 7:
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			Test->TestEqual(TEXT("The guest holds the new weapon"), Session.ClientHero->GetSword()->GetValues().Light.Damage, 50.0f);
			Test->TestEqual(TEXT("The server knows the new weapon"), Session.ServerCopyOfClientHero->GetSword()->GetValues().Light.Damage, 50.0f);
			Test->TestEqual(TEXT("Guest and server agree on the speed"), Session.ClientHero->GetCharacterMovement()->MaxWalkSpeed, Session.ServerCopyOfClientHero->GetCharacterMovement()->MaxWalkSpeed);
			Test->TestTrue(TEXT("The shoes make the guest faster"), Session.ClientHero->GetCharacterMovement()->MaxWalkSpeed > Session.ClientHero->GetValues().RunSpeedMetersPerSecond * 100.0f);
			// Each player gets their own reward, and it stays on their own machine.
			if (const AEIPlayerController* ServerCopyOfGuest = Session.ServerCopyOfClientHero->GetController<AEIPlayerController>())
			{
				ServerCopyOfGuest->GetProfile()->GrantEnemyReward(0.0f, 1.0f);
				ServerCopyOfGuest->GetProfile()->EndRun(true);
			}
			EnterStep(8);
			return false;
		case 8:
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			{
				const UEIProfileComponent* Guest = Session.ClientPlayer->GetProfile();
				Test->TestEqual(TEXT("The guest gets the reward"), Guest->GetLoot().Num(), 1);
				Test->TestTrue(TEXT("The guest's save has the item"), Guest->GetLoot().Num() == 1 && Guest->GetSave().Items.Contains(Guest->GetTable().Items[Guest->GetLoot()[0].Item].Name));
				const AEIPlayerController* Host = Cast<AEIPlayerController>(ServerWorld->GetFirstPlayerController());
				Test->TestEqual(TEXT("The host gets nothing of it"), Host->GetProfile()->GetLoot().Num(), 0);
			}
			// A leader has to look like one on the guest's screen too.
			if (AEIEnemy* Leader = ServerWorld->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Session.ServerCopyOfClientHero->GetActorLocation() + FVector(0.0f, 1500.0f, 0.0f), FRotator::ZeroRotator))
			{
				Leader->GetHealth()->Reset(100000.0f);
				Leader->MakeLeader(2.0f, 1.0f, LeaderScale);
			}
			EnterStep(9);
			return false;
		case 9:
			if (Now - StepAt < 0.3)
			{
				return false;
			}
			{
				bool bSeen = false;
				for (TActorIterator<AEIEnemy> It(Session.ClientWorld); It; ++It)
				{
					if (It->IsLeader())
					{
						bSeen = true;
						Test->TestNearlyEqual(TEXT("The guest sees the leader bigger"), static_cast<float>(It->GetActorScale3D().X), It->GetValues().Scale * LeaderScale, 0.001f);
					}
				}
				Test->TestTrue(TEXT("The guest knows who the leader is"), bSeen);
			}
			return true;
		default:
			return true;
		}
	}

private:
	FAutomationTestBase* Test;
	double StartedAt;
	int32 Step = 0;
	double StepAt = 0.0;
	TWeakObjectPtr<AEIEnemy> Target;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITwoPlayerRuneTest,
	"Eisenseelen.Net.GuestRune",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITwoPlayerSprintTest,
	"Eisenseelen.Net.GuestSprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	FRequestPlaySessionParams MakeTwoPlayerSession()
	{
		// Rooted, because loading the map runs the garbage collector before the session starts.
		ULevelEditorPlaySettings* PlaySettings = NewObject<ULevelEditorPlaySettings>();
		PlaySettings->AddToRoot();
		PlaySettings->SetPlayNetMode(PIE_ListenServer);
		PlaySettings->SetPlayNumberOfClients(2);
		PlaySettings->SetRunUnderOneProcess(true);
		PlaySettings->bLaunchSeparateServer = false;

		FRequestPlaySessionParams Session;
		Session.SessionDestination = EPlaySessionDestinationType::InProcess;
		Session.EditorPlaySettings = PlaySettings;
		return Session;
	}
}

bool FEITwoPlayerSprintTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(ArenaMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(MakeTwoPlayerSession()));
	ADD_LATENT_AUTOMATION_COMMAND(FEITwoPlayerSprintCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITwoPlayerCombatTest,
	"Eisenseelen.Net.GuestCombat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEITwoPlayerCombatTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(ArenaMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(MakeTwoPlayerSession()));
	ADD_LATENT_AUTOMATION_COMMAND(FEITwoPlayerCombatCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITwoPlayerBlockTest,
	"Eisenseelen.Net.GuestBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEITwoPlayerBlockTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(ArenaMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(MakeTwoPlayerSession()));
	ADD_LATENT_AUTOMATION_COMMAND(FEITwoPlayerBlockCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITwoPlayerBossTest,
	"Eisenseelen.Net.GuestBoss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEITwoPlayerBossTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TEXT("/Game/Maps/L_ErsterLauf"))));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(MakeTwoPlayerSession()));
	ADD_LATENT_AUTOMATION_COMMAND(FEITwoPlayerBossCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITwoPlayerRunTest,
	"Eisenseelen.Net.GuestRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEITwoPlayerRunTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(TEXT("/Game/Maps/L_ErsterLauf"))));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(MakeTwoPlayerSession()));
	ADD_LATENT_AUTOMATION_COMMAND(FEITwoPlayerRunCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

bool FEITwoPlayerRuneTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(ArenaMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(MakeTwoPlayerSession()));
	ADD_LATENT_AUTOMATION_COMMAND(FEITwoPlayerRuneCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
