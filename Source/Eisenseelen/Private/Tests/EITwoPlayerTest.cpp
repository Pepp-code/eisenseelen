#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
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
 * Plays as the guest of a real listen server: the guest dashes, the prediction must show it at once, and afterwards
 * guest and server must agree on where the hero stands.
 */
class FEITwoPlayerDashCommand : public IAutomationLatentCommand
{
public:
	explicit FEITwoPlayerDashCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

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
			if (Now - StepAt < 0.08)
			{
				return false;
			}
			EISendTestKey(Session.ClientPlayer, EKeys::LeftShift, IE_Released);
			GuestSpeed = static_cast<float>(Session.ClientHero->GetVelocity().Size2D());
			Test->AddInfo(FString::Printf(TEXT("The guest dashes at %.0f cm/s"), GuestSpeed));
			Test->TestTrue(TEXT("The guest feels the dash at once"), GuestSpeed > 1000.0f);
			EnterStep(4);
			return false;
		case 4:
			if (Now - StepAt < 0.1)
			{
				return false;
			}
			Test->AddInfo(FString::Printf(TEXT("The server sees the dash at %.0f cm/s"), Session.ServerCopyOfClientHero->GetVelocity().Size2D()));
			Test->TestFalse(TEXT("The server counts down the cooldown too"), Session.ServerCopyOfClientHero->GetHeroMovement()->IsDashReady());
			EISendTestKey(Session.ClientPlayer, EKeys::W, IE_Released);
			EnterStep(5);
			return false;
		case 5:
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
		Test->TestTrue(TEXT("The dash really moved the guest"), Moved > 500.0f);
		Test->TestTrue(TEXT("Guest and server agree on the position"), Drift < MaxDriftCm);
	}

	FAutomationTestBase* Test;
	double StartedAt;
	int32 Step = 0;
	double StepAt = 0.0;
	FVector StartLocation = FVector::ZeroVector;
	float GuestSpeed = 0.0f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITwoPlayerDashTest,
	"Eisenseelen.Net.GuestDash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEITwoPlayerDashTest::RunTest(const FString& Parameters)
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

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(ArenaMap)));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Session));
	ADD_LATENT_AUTOMATION_COMMAND(FEITwoPlayerDashCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
