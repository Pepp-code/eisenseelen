#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EICasterComponent.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EIProjectile.h"
#include "EITrainingDummy.h"
#include "EngineUtils.h"
#include "EIPlayerController.h"
#include "EITestKeys.h"
#include "Editor.h"
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
 * Plays as the guest of a real listen server: the guest sprints and dodges, the prediction must show it at once, and
 * afterwards guest and server must agree on where the hero stands.
 */
class FEITwoPlayerDodgeCommand : public IAutomationLatentCommand
{
public:
	explicit FEITwoPlayerDodgeCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

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
			EISendTestKey(Session.ClientPlayer, EKeys::V, IE_Pressed);
			EnterStep(4);
			return false;
		case 4:
			if (Now - StepAt < 0.08)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::V, IE_Released);
			GuestSpeed = static_cast<float>(Session.ClientHero->GetVelocity().Size2D());
			Test->AddInfo(FString::Printf(TEXT("The guest dodges at %.0f cm/s"), GuestSpeed));
			Test->TestTrue(TEXT("The guest feels the dodge at once"), GuestSpeed > 1000.0f);
			// Steering in the air is part of the predicted move, so it must not make guest and server drift apart.
			Session.ClientPlayer->SetControlRotation(FRotator(0.0f, 230.0f, 0.0f));
			EnterStep(5);
			return false;
		case 5:
			if (Now - StepAt < 0.1)
			{
				return false;
			}
			Test->AddInfo(FString::Printf(TEXT("The server sees the dodge at %.0f cm/s"), Session.ServerCopyOfClientHero->GetVelocity().Size2D()));
			Test->TestFalse(TEXT("The server counts down the cooldown too"), Session.ServerCopyOfClientHero->GetHeroMovement()->IsDodgeReady());
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
		Test->TestTrue(TEXT("Sprint and dodge really moved the guest"), Moved > 500.0f);
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
 * The guest fights: a sword swing and a staff bolt must count on the server and show up on the guest's machine, and
 * the server has the last word on the guest's mana.
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
			// Only the server's mana runs out, so the guest still fires and the server has to refuse.
			Session.ServerCopyOfClientHero->GetCaster()->SpendMana(Session.ServerCopyOfClientHero->GetCaster()->GetMaxMana());
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Pressed);
			EnterStep(9);
			return false;
		case 9:
			if (Now - StepAt < 0.05)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::LeftMouseButton, IE_Released);
			EnterStep(10);
			return false;
		case 10:
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITwoPlayerDodgeTest,
	"Eisenseelen.Net.GuestDodge",
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

bool FEITwoPlayerDodgeTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(ArenaMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(MakeTwoPlayerSession()));
	ADD_LATENT_AUTOMATION_COMMAND(FEITwoPlayerDodgeCommand(this));
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

#endif
