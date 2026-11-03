#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AIController.h"
#include "EIBeacon.h"
#include "EIEnemy.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHordeDirector.h"
#include "EIRunDirector.h"
#include "EISkyBarge.h"
#include "EITestScript.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

using namespace EITestScript;

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float HeroHeight = 92.0f;
	constexpr float SpeedToleranceCm = 40.0f;

	AEIGameMode* GetGameMode(const FEIPlayContext& Context)
	{
		return Context.World->GetAuthGameMode<AEIGameMode>();
	}

	const AEIGameState* GetState(const FEIPlayContext& Context)
	{
		return Context.World->GetGameState<AEIGameState>();
	}

	AEISkyBarge* GetBarge(const FEIPlayContext& Context)
	{
		return GetGameMode(Context)->GetRun()->GetBarge();
	}

	TArray<AEIBeacon*> GetBeacons(const FEIPlayContext& Context)
	{
		TArray<AEIBeacon*> Beacons;
		for (TActorIterator<AEIBeacon> It(Context.World); It; ++It)
		{
			Beacons.Add(*It);
		}
		return Beacons;
	}

	FEIPlayStep WaitForPhase(const TCHAR* What, double MaxSeconds, EEIRunPhase Phase)
	{
		return WaitUntil(What, MaxSeconds, [Phase](const FEIPlayContext& Context) { return GetState(Context)->GetRunPhase() == Phase; });
	}

	/** Stands next to the beacon, far enough from the fire to look at it. */
	void StandAtBeacon(const FEIPlayContext& Context, const AEIBeacon* Beacon, float DistanceCm)
	{
		const FVector Location = Beacon->GetActorLocation();
		Teleport(Context, FVector(Location.X + DistanceCm, Location.Y, HeroHeight), 180.0f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIRunCompleteTest,
	"Eisenseelen.Play.Run.Complete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIRunCompleteTest::RunTest(const FString& Parameters)
{
	constexpr float ShortHoldSeconds = 1.5f;
	struct FState
	{
		float HoldLeft = 0.0f;
		float RunSeconds = 0.0f;
		double Height = 0.0;
	};
	const TSharedRef<FState> State = MakeShared<FState>();

	TArray<FEIPlayStep> Steps;

	// The run starts high up on the barge, and the clock waits for the jump.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The run starts on the barge"), GetState(Context)->GetRunPhase(), EEIRunPhase::OnBarge);
		Context.Test->TestTrue(TEXT("The barge flies high"), Context.Hero->GetActorLocation().Z > GetBarge(Context)->GetDeckHeight());
		Context.Test->TestTrue(TEXT("The hero stands on the deck"), Context.Movement->IsMovingOnGround());
	}));

	// Running off the deck begins the run, and the fall is a flight.
	Steps.Add(Press(EKeys::W));
	Steps.Add(WaitForPhase(TEXT("the hero leaves the barge"), 6.0, EEIRunPhase::Exploring));
	Steps.Add(Wait(1.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FVector Velocity = Context.Hero->GetVelocity();
		const float DiveSpeed = Context.Hero->GetValues().DiveSpeedMetersPerSecond * CentimetersPerMeter;
		const float FallSpeed = Context.Hero->GetValues().DiveFallSpeedMetersPerSecond * CentimetersPerMeter;
		Context.Test->AddInfo(FString::Printf(TEXT("In the dive: %.0f cm/s across, %.0f cm/s down at %.0f cm height"), Velocity.Size2D(), -Velocity.Z, Context.Hero->GetActorLocation().Z));
		Context.Test->TestTrue(TEXT("The dive is high up"), Context.Movement->IsDiving());
		Context.Test->TestTrue(TEXT("The dive steers fast"), Velocity.Size2D() > DiveSpeed * 0.7f);
		Context.Test->TestTrue(TEXT("The dive limits the fall"), -Velocity.Z <= FallSpeed + SpeedToleranceCm);
	}));
	Steps.Add(WaitUntil(TEXT("the hero lands"), 15.0, [](const FEIPlayContext& Context) { return Context.Movement->IsMovingOnGround(); }));
	Steps.Add(Release(EKeys::W));
	Steps.Add(Wait(0.6));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHealthComponent* Health = Context.Hero->GetHealth();
		Context.Test->TestEqual(TEXT("The fall does no damage"), Health->GetHealth(), Health->GetMaxHealth());
		Context.Test->TestTrue(TEXT("The ground stops the hero"), Context.Hero->GetVelocity().Size2D() < 50.0);
		Context.Test->TestTrue(TEXT("The clock runs"), GetState(Context)->GetRunSeconds() > 2.0f);
		Context.Hero->SetCanBeDamaged(false);

		FEIRunValues Short = GetGameMode(Context)->GetRun()->GetValues();
		Short.BeaconHoldSeconds = ShortHoldSeconds;
		GetGameMode(Context)->GetRun()->ApplyValues(Short);
	}));

	// The use key does nothing far from a beacon.
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Far from a beacon the use key does nothing"), GetState(Context)->GetRunPhase(), EEIRunPhase::Exploring);
		StandAtBeacon(Context, GetBeacons(Context)[0], 200.0f);
	}));
	Steps.Add(Wait(0.1));

	// At a beacon it lights the fire, calls the barge and brings the last wave.
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitForPhase(TEXT("the beacon burns"), 1.0, EEIRunPhase::BeaconLit));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const TArray<AEIBeacon*> Beacons = GetBeacons(Context);
		Context.Test->TestEqual(TEXT("There are two beacons on the map"), Beacons.Num(), 2);
		Context.Test->TestTrue(TEXT("The beacon burns"), Beacons[0]->IsLit());
		Context.Test->TestFalse(TEXT("The other one does not"), Beacons[1]->IsLit());
		Context.Test->TestTrue(TEXT("The last wave starts"), GetGameMode(Context)->GetHorde()->IsHordeActive());
	}));

	// Leaving the zone stops the wait for the barge.
	Steps.Add(Do([](const FEIPlayContext& Context) { StandAtBeacon(Context, GetBeacons(Context)[0], 4000.0f); }));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->HoldLeft = GetState(Context)->GetBeaconHoldLeft(); }));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("Outside the zone nobody holds the fire"), GetState(Context)->IsBeaconHeld());
		Context.Test->TestEqual(TEXT("Outside the zone the wait stands still"), GetState(Context)->GetBeaconHoldLeft(), State->HoldLeft);
		StandAtBeacon(Context, GetBeacons(Context)[0], 200.0f);
	}));

	// Back in the zone the barge arrives after the rest of the time.
	Steps.Add(WaitForPhase(TEXT("the barge arrives"), 5.0, EEIRunPhase::BargeWaiting));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FVector BargeLocation = GetBarge(Context)->GetActorLocation();
		const double Distance = FVector::Dist2D(BargeLocation, GetBeacons(Context)[0]->GetActorLocation());
		Context.Test->AddInfo(FString::Printf(TEXT("The barge hovers %.0f cm from the beacon, %.0f cm high"), Distance, BargeLocation.Z));
		Context.Test->TestNearlyEqual(TEXT("The barge comes to the beacon"), static_cast<float>(Distance), GetGameMode(Context)->GetRun()->GetValues().BargeDistanceMeters * CentimetersPerMeter, 10.0f);
		Context.Test->TestTrue(TEXT("The barge comes down"), BargeLocation.Z < 500.0);
	}));

	// The use key does not board from afar.
	Steps.Add(Do([](const FEIPlayContext& Context) { StandAtBeacon(Context, GetBeacons(Context)[0], -1200.0f); }));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("From afar the use key does not board"), GetState(Context)->GetRunPhase(), EEIRunPhase::BargeWaiting);
		Context.Hero->SetActorLocation(GetBarge(Context)->GetActorLocation() + FVector(0.0f, 0.0f, -40.0f), false, nullptr, ETeleportType::TeleportPhysics);
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Standing at the barge is not boarding yet"), GetState(Context)->GetRunPhase(), EEIRunPhase::BargeWaiting);
	}));

	// Right there it ends the run, and the barge carries the hero away.
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitForPhase(TEXT("the hero gets away"), 1.0, EEIRunPhase::Escaped));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The hero stands on the barge"), Context.Hero->GetAttachParentActor() == GetBarge(Context));
		Context.Test->TestFalse(TEXT("The hero is safe on the barge"), Context.Hero->CanBeDamaged());
		Context.Test->TestFalse(TEXT("The wave ends"), GetGameMode(Context)->GetHorde()->IsHordeActive());
		State->Height = Context.Hero->GetActorLocation().Z;
		State->RunSeconds = GetState(Context)->GetRunSeconds();
	}));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The barge rises"), Context.Hero->GetActorLocation().Z > State->Height + 100.0);
		Context.Test->TestEqual(TEXT("The clock stops"), GetState(Context)->GetRunSeconds(), State->RunSeconds);
	}));

	// R starts over, up on the barge.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitUntil(TEXT("a fresh run starts"), 3.0, [](const FEIPlayContext& Context)
	{
		return GetState(Context)->GetRunPhase() == EEIRunPhase::OnBarge && Context.Movement->IsMovingOnGround();
	}));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const AEISkyBarge* Barge = GetBarge(Context);
		Context.Test->TestTrue(TEXT("The barge is back"), FVector::Dist(Barge->GetActorLocation(), Barge->GetStartLocation()) < 1.0);
		Context.Test->TestTrue(TEXT("The hero is back on the deck"), Context.Hero->GetActorLocation().Z > Barge->GetDeckHeight());
		Context.Test->TestFalse(TEXT("The hero is no longer attached"), Context.Hero->GetAttachParentActor() != nullptr);
		for (const AEIBeacon* Beacon : GetBeacons(Context))
		{
			Context.Test->TestFalse(TEXT("The fires are out"), Beacon->IsLit());
		}
		Context.Test->TestEqual(TEXT("All enemies are gone"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIPathfindingTest,
	"Eisenseelen.Play.Run.Pathfinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIPathfindingTest::RunTest(const FString& Parameters)
{
	// The northern wall of the first place on the run map: 10 m long, along X at y = 117 m.
	const FVector HeroSpot(0.0f, 12300.0f, HeroHeight);
	const FVector EnemySpot(0.0f, 11100.0f, HeroHeight);
	const TSharedRef<double> StartedAt = MakeShared<double>(0.0);

	const auto FindEnemy = [](const FEIPlayContext& Context) -> AEIEnemy*
	{
		TActorIterator<AEIEnemy> It(Context.World);
		return It ? *It : nullptr;
	};

	TArray<FEIPlayStep> Steps;
	Steps.Add(WaitUntil(TEXT("the paths are built"), 30.0, [](const FEIPlayContext& Context)
	{
		const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Context.World);
		return Navigation != nullptr && Navigation->GetDefaultNavDataInstance() != nullptr && !Navigation->IsNavigationBeingBuilt(Context.World);
	}));
	Steps.Add(Do([HeroSpot, EnemySpot, StartedAt](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		Teleport(Context, HeroSpot, 270.0f);
		FHitResult Wall;
		const bool bBlocked = Context.World->LineTraceSingleByObjectType(Wall, EnemySpot, HeroSpot, FCollisionObjectQueryParams(ECC_WorldStatic));
		Context.Test->TestTrue(TEXT("A wall stands between enemy and hero"), bBlocked);
		Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), EnemySpot, FRotator::ZeroRotator);
		*StartedAt = Context.Now;
	}));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([FindEnemy](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		const AAIController* AI = Enemy->GetController<AAIController>();
		Context.Test->TestEqual(TEXT("The enemy follows a path"), AI->GetMoveStatus(), EPathFollowingStatus::Moving);
		const FNavPathSharedPtr Path = AI->GetPathFollowingComponent()->GetPath();
		Context.Test->TestTrue(TEXT("The path has a real course"), Path.IsValid() && Path->GetPathPoints().Num() > 2);
		if (Path.IsValid())
		{
			Context.Test->AddInfo(FString::Printf(TEXT("The path around the wall has %d points, %.0f cm long"), Path->GetPathPoints().Num(), Path->GetLength()));
		}
	}));
	Steps.Add(WaitUntil(TEXT("the enemy winds up beside the hero"), 8.0, [FindEnemy](const FEIPlayContext& Context) { return FindEnemy(Context)->IsTelegraphing(); }));
	Steps.Add(Do([StartedAt, FindEnemy](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		Context.Test->AddInfo(FString::Printf(TEXT("Around the wall in %.1f s, %.0f cm from the hero"), Context.Now - *StartedAt, FVector::Dist(Enemy->GetActorLocation(), Context.Hero->GetActorLocation())));
		const AAIController* AI = Enemy->GetController<AAIController>();
		Context.Test->TestNotEqual(TEXT("The enemy stands still to strike"), AI->GetMoveStatus(), EPathFollowingStatus::Moving);
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIRunFailedTest,
	"Eisenseelen.Play.Run.Failed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIRunFailedTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
	}));
	Steps.Add(WaitForPhase(TEXT("the run is lost"), 1.0, EEIRunPhase::Failed));
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitForPhase(TEXT("a fresh run starts"), 3.0, EEIRunPhase::OnBarge));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("The fresh hero lives"), Context.Hero->IsDead());
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

#endif
