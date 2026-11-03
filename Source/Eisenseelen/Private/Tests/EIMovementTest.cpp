#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EILaunchPad.h"
#include "EITestScript.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace EITestScript;

namespace
{
	// Open floor behind the start, away from every obstacle.
	const FVector OpenFloor(-500.0f, 0.0f, 92.0f);
	constexpr float BackwardsYaw = 180.0f;
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float SpeedToleranceCm = 40.0f;

	constexpr float FastSpeedCm = 1150.0f;

	float SprintSpeedCm(const FEIPlayContext& Context)
	{
		return Context.Hero->GetValues().SprintSpeedMetersPerSecond * CentimetersPerMeter;
	}

	float RunSpeedCm(const FEIPlayContext& Context)
	{
		return Context.Hero->GetValues().RunSpeedMetersPerSecond * CentimetersPerMeter;
	}

	float HorizontalSpeed(const FEIPlayContext& Context)
	{
		return static_cast<float>(Context.Hero->GetVelocity().Size2D());
	}

	/** Faster than running, as after a jump pad, and in the air. */
	void FlingForwards(const FEIPlayContext& Context)
	{
		Context.Movement->SetMovementMode(MOVE_Falling);
		Context.Movement->Velocity = Context.Hero->GetActorForwardVector() * FastSpeedCm + FVector(0.0, 0.0, 300.0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEINoBunnyHopTest,
	"Eisenseelen.Play.Movement.NoBunnyHop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEINoBunnyHopTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;
	const auto FlungWhileRunning = [&Steps]()
	{
		Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, BackwardsYaw); }));
		Steps.Add(Wait(0.3));
		Steps.Add(Press(EKeys::W));
		Steps.Add(Wait(0.5));
		Steps.Add(Do(FlingForwards));
		Steps.Add(Wait(0.03));
	};
	const auto ExpectRunSpeedAtMost = [](const TCHAR* What)
	{
		return Do([What](const FEIPlayContext& Context)
		{
			Context.Test->AddInfo(FString::Printf(TEXT("%s: %.0f cm/s"), What, HorizontalSpeed(Context)));
			Context.Test->TestTrue(What, HorizontalSpeed(Context) <= RunSpeedCm(Context) + SpeedToleranceCm);
		});
	};

	// A jump at the moment of landing keeps nothing of the flight.
	FlungWhileRunning();
	Steps.Add(WaitForGround());
	Steps.Add(Press(EKeys::SpaceBar));
	Steps.Add(WaitUntil(TEXT("the jump after landing happens"), 0.3, [](const FEIPlayContext& Context) { return Context.Movement->IsFalling(); }));
	Steps.Add(ExpectRunSpeedAtMost(TEXT("A jump right after landing keeps no speed")));
	Steps.Add(Release(EKeys::SpaceBar));
	Steps.Add(WaitForGround());
	Steps.Add(Release(EKeys::W));

	// Neither does a jump during the flight, and there is only one of them.
	FlungWhileRunning();
	AddTap(Steps, EKeys::SpaceBar);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The jump in the flight happens"), Context.Hero->GetVelocity().Z > Context.Movement->JumpZVelocity * 0.7);
	}));
	Steps.Add(ExpectRunSpeedAtMost(TEXT("A jump during the flight keeps no speed")));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Movement->Velocity.Z = -100.0; }));
	AddTap(Steps, EKeys::SpaceBar);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("There is no second jump in the air"), Context.Hero->GetVelocity().Z < 0.0);
	}));
	Steps.Add(WaitForGround());
	Steps.Add(Release(EKeys::W));

	// Without a jump the ground brakes the flight away.
	FlungWhileRunning();
	Steps.Add(WaitForGround());
	Steps.Add(Wait(0.6));
	Steps.Add(ExpectRunSpeedAtMost(TEXT("Without a jump the ground brakes back to the run speed")));
	Steps.Add(Release(EKeys::W));

	// A press shortly before landing still counts as a jump, but carries nothing either.
	Steps.Add(Wait(0.3));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Teleport(Context, OpenFloor + FVector(0.0f, 0.0f, 10.0f), BackwardsYaw);
		Context.Movement->Velocity = FVector(-1000.0, 0.0, -100.0);
		Context.Movement->SetMovementMode(MOVE_Falling);
		Context.Hero->JumpCurrentCount = 1;
	}));
	AddTap(Steps, EKeys::SpaceBar);
	Steps.Add(WaitUntil(TEXT("the buffered jump happens"), 1.0, [](const FEIPlayContext& Context) { return Context.Hero->GetVelocity().Z > 100.0; }));
	Steps.Add(ExpectRunSpeedAtMost(TEXT("A press just before landing keeps no speed")));
	Steps.Add(WaitForGround());

	// Hopping over and over never gets faster than running.
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, BackwardsYaw); }));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.5));
	for (int32 Hop = 0; Hop < 3; ++Hop)
	{
		Steps.Add(Press(EKeys::SpaceBar));
		Steps.Add(WaitUntil(TEXT("the next hop happens"), 0.3, [](const FEIPlayContext& Context) { return Context.Movement->IsFalling(); }));
		Steps.Add(Release(EKeys::SpaceBar));
		Steps.Add(ExpectRunSpeedAtMost(TEXT("Hopping is no faster than running")));
		Steps.Add(WaitForGround());
	}
	Steps.Add(Release(EKeys::W));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEISprintTest,
	"Eisenseelen.Play.Movement.Sprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEISprintTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;

	// Holding shift runs faster, and only while it is held.
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, BackwardsYaw); }));
	Steps.Add(Wait(0.3));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestNearlyEqual(TEXT("W alone runs"), HorizontalSpeed(Context), RunSpeedCm(Context), SpeedToleranceCm);
	}));
	Steps.Add(Press(EKeys::LeftShift));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->AddInfo(FString::Printf(TEXT("Sprinting at %.0f cm/s"), HorizontalSpeed(Context)));
		Context.Test->TestNearlyEqual(TEXT("Shift sprints"), HorizontalSpeed(Context), SprintSpeedCm(Context), SpeedToleranceCm);
		Context.Test->TestTrue(TEXT("A sprint stays on the ground"), Context.Movement->IsMovingOnGround());
	}));

	// A jump while sprinting keeps the sprint speed and adds nothing.
	AddTap(Steps, EKeys::SpaceBar);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestNearlyEqual(TEXT("A jump keeps the sprint speed"), HorizontalSpeed(Context), SprintSpeedCm(Context), SpeedToleranceCm);
	}));
	Steps.Add(WaitForGround());
	Steps.Add(Release(EKeys::LeftShift));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestNearlyEqual(TEXT("Without shift the hero runs again"), HorizontalSpeed(Context), RunSpeedCm(Context), SpeedToleranceCm);
	}));
	Steps.Add(Release(EKeys::W));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIAirSteeringTest,
	"Eisenseelen.Play.Movement.AirSteering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIAirSteeringTest::RunTest(const FString& Parameters)
{
	constexpr double SteerSeconds = 0.25;
	constexpr float LookSidewaysYaw = 270.0f;
	const TSharedRef<double> SteerStartedAt = MakeShared<double>(0.0);

	// Flies fast along -X, high enough to stay in the air for the whole measurement.
	const auto FlyFast = [](const FEIPlayContext& Context)
	{
		Teleport(Context, OpenFloor + FVector(0.0f, 0.0f, 500.0f), BackwardsYaw);
		Context.Movement->SetMovementMode(MOVE_Falling);
		Context.Movement->Velocity = FVector(-FastSpeedCm, 0.0, 0.0);
	};

	TArray<FEIPlayStep> Steps;

	// The reported case: run, fly fast, then look to the side with W held.
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, BackwardsYaw); }));
	Steps.Add(Wait(0.3));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.5));
	Steps.Add(Do(FlingForwards));
	Steps.Add(Wait(0.03));
	Steps.Add(Do([SteerStartedAt](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The hero flies fast"), HorizontalSpeed(Context) > FastSpeedCm * 0.9f);
		Context.Player->SetControlRotation(FRotator(0.0f, LookSidewaysYaw, 0.0f));
		*SteerStartedAt = Context.Now;
	}));
	Steps.Add(Wait(SteerSeconds));
	Steps.Add(Do([SteerStartedAt](const FEIPlayContext& Context)
	{
		const FVector Velocity = Context.Hero->GetVelocity();
		const double Seconds = Context.Now - *SteerStartedAt;
		const double Turned = FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Velocity.Y), FMath::Abs(Velocity.X)));
		const double Expected = FMath::Min(90.0, Context.Hero->GetValues().AirTurnDegreesPerSecond * Seconds);
		Context.Test->AddInfo(FString::Printf(TEXT("In %.2f s the flight turned %.0f degrees at %.0f cm/s, expected about %.0f"), Seconds, Turned, HorizontalSpeed(Context), Expected));
		Context.Test->TestTrue(TEXT("The flight direction follows the input"), Turned >= Expected * 0.8);
		Context.Test->TestTrue(TEXT("Steering costs no speed"), HorizontalSpeed(Context) > FastSpeedCm * 0.95f);
	}));
	Steps.Add(Release(EKeys::W));
	Steps.Add(WaitForGround());

	// Input against the flight brakes instead of turning around.
	Steps.Add(Do(FlyFast));
	Steps.Add(Press(EKeys::S));
	Steps.Add(Wait(SteerSeconds));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FVector Velocity = Context.Hero->GetVelocity();
		Context.Test->AddInfo(FString::Printf(TEXT("Holding S in the air: %.0f cm/s after 0.25 s"), HorizontalSpeed(Context)));
		Context.Test->TestTrue(TEXT("Input against the flight slows it"), HorizontalSpeed(Context) < FastSpeedCm * 0.92f);
		Context.Test->TestTrue(TEXT("Input against the flight does not turn it"), FMath::Abs(Velocity.Y) < 50.0 && Velocity.X < 0.0);
	}));
	Steps.Add(Release(EKeys::S));
	Steps.Add(WaitForGround());

	// Without input the flight stays as it is, wherever the player looks.
	Steps.Add(Do(FlyFast));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Player->SetControlRotation(FRotator(0.0f, LookSidewaysYaw, 0.0f)); }));
	Steps.Add(Wait(SteerSeconds));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FVector Velocity = Context.Hero->GetVelocity();
		Context.Test->TestTrue(TEXT("Without input the flight keeps its direction"), FMath::Abs(Velocity.Y) < 5.0);
		Context.Test->TestNearlyEqual(TEXT("Without input the flight keeps its speed"), HorizontalSpeed(Context), FastSpeedCm, 5.0f);
	}));
	Steps.Add(WaitForGround());
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEILaunchPadTest,
	"Eisenseelen.Play.Movement.LaunchPad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEILaunchPadTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		FVector PadLocation = FVector::ZeroVector;
		float ExpectedCm = 0.0f;
		double PeakZ = 0.0;
	};
	const TSharedRef<FState> State = MakeShared<FState>();

	const auto StepOnPad = [State](const FEIPlayContext& Context)
	{
		Teleport(Context, State->PadLocation + FVector(0.0f, 0.0f, 92.0f), 0.0f);
		State->PeakZ = 0.0;
	};

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		TActorIterator<AEILaunchPad> Pad(Context.World);
		if (!Pad)
		{
			Context.Test->AddError(TEXT("The test arena has no launch pad"));
			return;
		}
		State->PadLocation = Pad->GetActorLocation();
		State->ExpectedCm = Pad->GetLaunchHeightMeters() * CentimetersPerMeter;
	}));

	// Standing on the pad: straight up to the height from the file.
	Steps.Add(Do(StepOnPad));
	Steps.Add(Wait(0.1));
	// Falling back onto the pad would throw the hero up again, so this measures the first rise only.
	Steps.Add(WaitUntil(TEXT("the launch reaches its top"), 4.0, [State](const FEIPlayContext& Context)
	{
		State->PeakZ = FMath::Max(State->PeakZ, Context.Hero->GetActorLocation().Z);
		return Context.Hero->GetVelocity().Z < 0.0 && State->PeakZ > 100.0;
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const float Height = static_cast<float>(State->PeakZ) - 90.0f;
		Context.Test->AddInfo(FString::Printf(TEXT("The launch pad throws %.0f cm high, expected %.0f"), Height, State->ExpectedCm));
		Context.Test->TestNearlyEqual(TEXT("The pad reaches its height"), Height, State->ExpectedCm, State->ExpectedCm * 0.1f);
	}));

	// Holding W in the air reaches the tower behind the pad.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Teleport(Context, FVector(State->PadLocation.X - 500.0f, State->PadLocation.Y, 92.0f), 0.0f);
	}));
	Steps.Add(Wait(0.2));
	Steps.Add(Do(StepOnPad));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.3));
	Steps.Add(WaitForGround());
	Steps.Add(Release(EKeys::W));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const double FeetHeight = Context.Hero->GetActorLocation().Z - 90.0;
		Context.Test->AddInfo(FString::Printf(TEXT("After the pad with W held the hero stands %.0f cm high"), FeetHeight));
		Context.Test->TestTrue(TEXT("From the pad the hero reaches the tower"), FeetHeight > 400.0);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
