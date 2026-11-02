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

	float DashBonusCm(const FEIPlayContext& Context)
	{
		return Context.Hero->GetValues().DashSpeedBonusMetersPerSecond * CentimetersPerMeter;
	}

	float RunSpeedCm(const FEIPlayContext& Context)
	{
		return Context.Hero->GetValues().RunSpeedMetersPerSecond * CentimetersPerMeter;
	}

	float HorizontalSpeed(const FEIPlayContext& Context)
	{
		return static_cast<float>(Context.Hero->GetVelocity().Size2D());
	}

	FEIPlayStep WaitForDashReady()
	{
		return WaitUntil(TEXT("the dash is ready"), 6.0, [](const FEIPlayContext& Context) { return Context.Movement->IsDashReady(); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIDashTest,
	"Eisenseelen.Play.Movement.Dash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIDashTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;

	// From a standstill: speed bonus, small hop, cooldown.
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, BackwardsYaw); }));
	Steps.Add(Wait(0.3));
	AddTap(Steps, EKeys::LeftShift);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->AddInfo(FString::Printf(TEXT("Dash from a standstill: %.0f cm/s"), HorizontalSpeed(Context)));
		Context.Test->TestNearlyEqual(TEXT("A dash from a standstill gives the bonus"), HorizontalSpeed(Context), DashBonusCm(Context), SpeedToleranceCm);
		Context.Test->TestTrue(TEXT("The dash is a small hop"), Context.Movement->IsFalling());
		Context.Test->TestTrue(TEXT("The dash goes forward without input"), FVector::DotProduct(Context.Hero->GetVelocity(), Context.Hero->GetActorForwardVector()) > 0.0);
		Context.Test->TestFalse(TEXT("The dash starts its cooldown"), Context.Movement->IsDashReady());
	}));

	// A second dash during the cooldown does nothing.
	AddTap(Steps, EKeys::LeftShift);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("No second dash during the cooldown"), HorizontalSpeed(Context) < DashBonusCm(Context) + SpeedToleranceCm);
	}));

	// The dash leaves the jump unused, and the air jump is only there once.
	AddTap(Steps, EKeys::SpaceBar);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("A jump still works in the air after a dash"), Context.Hero->GetVelocity().Z > Context.Movement->JumpZVelocity * 0.7);
		Context.Hero->GetCharacterMovement()->Velocity.Z = -100.0;
	}));
	AddTap(Steps, EKeys::SpaceBar);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("There is no second jump in the air"), Context.Hero->GetVelocity().Z < 0.0);
	}));
	Steps.Add(WaitForGround());
	Steps.Add(WaitForDashReady());

	// Running: the bonus comes on top of the speed.
	Steps.Add(Wait(0.4));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.6));
	AddTap(Steps, EKeys::LeftShift);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Expected = RunSpeedCm(Context) + DashBonusCm(Context);
		Context.Test->AddInfo(FString::Printf(TEXT("Dash while running: %.0f cm/s, expected %.0f"), HorizontalSpeed(Context), Expected));
		Context.Test->TestNearlyEqual(TEXT("A dash while running adds the bonus"), HorizontalSpeed(Context), Expected, SpeedToleranceCm);
	}));
	Steps.Add(Release(EKeys::W));
	Steps.Add(WaitForGround());
	Steps.Add(WaitForDashReady());

	// The dash follows the input, here to the right.
	Steps.Add(Wait(0.4));
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, 0.0f); }));
	Steps.Add(Wait(0.2));
	Steps.Add(Press(EKeys::D));
	Steps.Add(Wait(0.05));
	AddTap(Steps, EKeys::LeftShift);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FVector Velocity = Context.Hero->GetVelocity();
		Context.Test->TestTrue(TEXT("The dash follows the input"), Velocity.Y > 400.0 && FMath::Abs(Velocity.X) < 100.0);
	}));
	Steps.Add(Release(EKeys::D));
	Steps.Add(WaitForGround());
	Steps.Add(WaitForDashReady());

	// A dash also works in the air.
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, BackwardsYaw); }));
	Steps.Add(Wait(0.3));
	AddTap(Steps, EKeys::SpaceBar);
	Steps.Add(Wait(0.05));
	AddTap(Steps, EKeys::LeftShift);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestNearlyEqual(TEXT("A dash works in the air"), HorizontalSpeed(Context), DashBonusCm(Context), SpeedToleranceCm);
	}));
	Steps.Add(WaitForGround());
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEISpeedChainTest,
	"Eisenseelen.Play.Movement.SpeedChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEISpeedChainTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;
	const auto DashWhileRunning = [&Steps]()
	{
		Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, BackwardsYaw); }));
		Steps.Add(Wait(0.3));
		Steps.Add(Press(EKeys::W));
		Steps.Add(Wait(0.5));
		AddTap(Steps, EKeys::LeftShift);
		Steps.Add(Wait(0.03));
		Steps.Add(WaitForGround());
	};

	// A jump right after landing keeps the speed of the dash.
	DashWhileRunning();
	Steps.Add(Press(EKeys::SpaceBar));
	Steps.Add(WaitUntil(TEXT("the chain jump happens"), 0.3, [](const FEIPlayContext& Context) { return Context.Movement->IsFalling(); }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Expected = RunSpeedCm(Context) + DashBonusCm(Context);
		Context.Test->AddInfo(FString::Printf(TEXT("Jump right after landing: %.0f cm/s, dash gave %.0f"), HorizontalSpeed(Context), Expected));
		Context.Test->TestTrue(TEXT("A jump in the landing window keeps the speed"), HorizontalSpeed(Context) > Expected * 0.9f);
	}));
	Steps.Add(Release(EKeys::SpaceBar));
	Steps.Add(WaitForGround());
	Steps.Add(Wait(0.6));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->AddInfo(FString::Printf(TEXT("Without a jump the ground brakes to %.0f cm/s"), HorizontalSpeed(Context)));
		Context.Test->TestTrue(TEXT("Without a jump the ground brakes back to the run speed"), HorizontalSpeed(Context) < RunSpeedCm(Context) + SpeedToleranceCm);
	}));
	Steps.Add(Release(EKeys::W));
	Steps.Add(WaitUntil(TEXT("the dash is ready"), 6.0, [](const FEIPlayContext& Context) { return Context.Movement->IsDashReady(); }));

	// A press shortly before landing counts too.
	DashWhileRunning();
	Steps.Add(Release(EKeys::W));
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
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("A press just before landing also keeps the speed"), HorizontalSpeed(Context) > 800.0f);
	}));
	Steps.Add(WaitForGround());
	Steps.Add(Wait(0.3));

	// The speed has a ceiling.
	Steps.Add(WaitUntil(TEXT("the dash is ready"), 6.0, [](const FEIPlayContext& Context) { return Context.Movement->IsDashReady(); }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Teleport(Context, OpenFloor + FVector(0.0f, 0.0f, 100.0f), BackwardsYaw);
		Context.Movement->SetMovementMode(MOVE_Falling);
		const float NearlyCeiling = Context.Hero->GetValues().MaxSpeedMetersPerSecond * CentimetersPerMeter - 50.0f;
		Context.Movement->Velocity = Context.Hero->GetActorForwardVector() * NearlyCeiling;
	}));
	AddTap(Steps, EKeys::LeftShift);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Ceiling = Context.Hero->GetValues().MaxSpeedMetersPerSecond * CentimetersPerMeter;
		Context.Test->AddInfo(FString::Printf(TEXT("Dash near the ceiling: %.0f cm/s, ceiling %.0f"), HorizontalSpeed(Context), Ceiling));
		Context.Test->TestNearlyEqual(TEXT("The speed stops at the ceiling"), HorizontalSpeed(Context), Ceiling, 5.0f);
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
