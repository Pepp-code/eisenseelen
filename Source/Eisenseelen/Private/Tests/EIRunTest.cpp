#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AIController.h"
#include "EIAltar.h"
#include "EIBeacon.h"
#include "EIChest.h"
#include "EIEnemy.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHordeDirector.h"
#include "EIOmenComponent.h"
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

	/** No enemies at all, for a test that looks at something else. */
	void SwitchOffDirector(const FEIPlayContext& Context)
	{
		UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		FEIHordeValues Values = Horde->GetValues();
		Values.Director.FullThreat = 0.0f;
		Horde->ApplyValues(Values);
	}

	/** No ritual, for a test that looks at what comes after it. It only counts for the run that starts next. */
	void SkipRitual(const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		FEIRunValues Values = Run->GetValues();
		Values.RitualAltars = 0.0f;
		Run->ApplyValues(Values);
		Run->ResetRun();
	}

	float ExpectedBudget(const FEIPlayContext& Context, float PhaseShare)
	{
		const FEIDirectorValues& Director = GetGameMode(Context)->GetHorde()->GetValues().Director;
		const float Corruption = 1.0f + GetGameMode(Context)->GetRun()->GetValues().CorruptionThreatPerStage * GetState(Context)->GetCorruptionStage();
		return FMath::Min(Director.FullThreat * Director.PlayerShare[0] * PhaseShare * Corruption, Director.PeakThreat);
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
		SwitchOffDirector(Context);
		SkipRitual(Context);
		FEIHeroValues WithDodge = Context.Hero->GetValues();
		WithDodge.DodgeSwitch = 1.0f;
		Context.Hero->ApplyValues(WithDodge);
		Context.Test->TestEqual(TEXT("The run starts on the barge"), GetState(Context)->GetRunPhase(), EEIRunPhase::OnBarge);
		Context.Test->TestTrue(TEXT("The barge flies high"), Context.Hero->GetActorLocation().Z > GetBarge(Context)->GetDeckHeight());
		Context.Test->TestTrue(TEXT("The hero stands on the deck"), Context.Movement->IsMovingOnGround());
	}));

	// Running off the deck begins the run, and the fall is a flight.
	Steps.Add(Press(EKeys::W));
	Steps.Add(WaitForPhase(TEXT("the hero leaves the barge"), 6.0, EEIRunPhase::RitualDone));
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

	// Neither dodging nor jumping stops the dive.
	AddTap(Steps, EKeys::V);
	AddTap(Steps, EKeys::SpaceBar);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float FallSpeed = Context.Hero->GetValues().DiveFallSpeedMetersPerSecond * CentimetersPerMeter;
		Context.Test->TestTrue(TEXT("The dive goes on"), Context.Movement->IsDiving());
		Context.Test->TestTrue(TEXT("No dodge in the dive"), Context.Movement->IsDodgeReady());
		Context.Test->TestNearlyEqual(TEXT("Nothing stops the fall in the dive"), static_cast<float>(-Context.Hero->GetVelocity().Z), FallSpeed, SpeedToleranceCm);
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
		Context.Test->TestEqual(TEXT("Far from a beacon the use key does nothing"), GetState(Context)->GetRunPhase(), EEIRunPhase::RitualDone);
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
	FEIRunDirectorTest,
	"Eisenseelen.Play.Run.Director",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIRunDirectorTest::RunTest(const FString& Parameters)
{
	constexpr float InViewDot = 0.35f;
	struct FState
	{
		TSet<const AEIEnemy*> Seen;
		int32 Checked = 0;
		TWeakObjectPtr<AEIEnemy> LeftBehind;
	};
	const TSharedRef<FState> State = MakeShared<FState>();

	// Every enemy is checked at the moment it appears: far enough away and out of sight.
	const auto CheckNewEnemies = [State](const FEIPlayContext& Context)
	{
		const float MinDistance = GetGameMode(Context)->GetHorde()->GetValues().MinDistanceMeters * CentimetersPerMeter;
		const FVector Eye = Context.Hero->GetPawnViewLocation();
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			if (State->Seen.Contains(*It))
			{
				continue;
			}
			State->Seen.Add(*It);
			++State->Checked;
			Context.Test->TestTrue(TEXT("An enemy appears at a distance"), FVector::Dist2D(It->GetActorLocation(), Context.Hero->GetActorLocation()) >= MinDistance * 0.9f);
			const bool bInViewCone = FVector::DotProduct(Context.Hero->GetViewRotation().Vector(), (It->GetActorLocation() - Eye).GetSafeNormal()) >= InViewDot * 1.1f;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(EITestSight), false, Context.Hero);
			const bool bWallBetween = Context.World->LineTraceTestByObjectType(Eye, It->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Query);
			Context.Test->TestTrue(TEXT("An enemy appears out of sight"), !bInViewCone || bWallBetween);
		}
	};
	const auto TooMuchThreat = [](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		return Horde->CountThreat() > Horde->GetThreatBudget() + 1.0f;
	};

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		SkipRitual(Context);
		Context.Test->TestFalse(TEXT("On the barge nothing comes yet"), GetGameMode(Context)->GetHorde()->IsHordeActive());
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));

	// The quiet phase: a share of the budget, and it fills up.
	Steps.Add(WaitForPhase(TEXT("the run begins"), 2.0, EEIRunPhase::RitualDone));
	Steps.Add(WaitUntil(TEXT("the enemies of the quiet phase are there"), 20.0, [CheckNewEnemies, TooMuchThreat](const FEIPlayContext& Context)
	{
		CheckNewEnemies(Context);
		Context.Test->TestFalse(TEXT("The budget holds"), TooMuchThreat(Context));
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		return Horde->CountThreat() >= Horde->GetThreatBudget();
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		const float Expected = ExpectedBudget(Context, Horde->GetValues().Director.ExploringShare);
		Context.Test->AddInfo(FString::Printf(TEXT("Quiet phase: budget %.1f, threat %.0f, %d enemies checked at 1500 cm from the middle"), Horde->GetThreatBudget(), Horde->CountThreat(), State->Checked));
		Context.Test->TestNearlyEqual(TEXT("The quiet phase has its share of the budget"), Horde->GetThreatBudget(), Expected, 0.01f);

		// Far away from everyone: such an enemy makes way for one nearby.
		State->LeftBehind = Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Context.Hero->GetActorLocation() + FVector(9000.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		Context.Test->TestTrue(TEXT("An enemy can be far away"), State->LeftBehind.IsValid());
	}));
	Steps.Add(WaitUntil(TEXT("the enemy left behind is gone"), 3.0, [State](const FEIPlayContext& Context) { return !State->LeftBehind.IsValid(); }));

	// The corruption rises with the clock and makes more of everything.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		FEIRunValues Fast = GetGameMode(Context)->GetRun()->GetValues();
		Fast.CorruptionEverySeconds = GetState(Context)->GetRunSeconds() / 2.5f;
		GetGameMode(Context)->GetRun()->ApplyValues(Fast);
	}));
	Steps.Add(WaitUntil(TEXT("the corruption rises"), 2.0, [](const FEIPlayContext& Context) { return GetState(Context)->GetCorruptionStage() >= 2; }));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		const float Expected = ExpectedBudget(Context, Horde->GetValues().Director.ExploringShare);
		Context.Test->AddInfo(FString::Printf(TEXT("Corruption %d: budget %.1f"), GetState(Context)->GetCorruptionStage(), Horde->GetThreatBudget()));
		Context.Test->TestNearlyEqual(TEXT("The corruption raises the budget"), Horde->GetThreatBudget(), Expected, 0.01f);
		Context.Test->TestFalse(TEXT("This is no blood moon yet"), GetState(Context)->IsBloodMoon());

		FEIRunValues Fast = GetGameMode(Context)->GetRun()->GetValues();
		Fast.CorruptionEverySeconds = 0.1f;
		GetGameMode(Context)->GetRun()->ApplyValues(Fast);
	}));

	// At the end comes the blood moon: no quiet phase any more, and the stage stops there.
	Steps.Add(WaitUntil(TEXT("the blood moon rises"), 2.0, [](const FEIPlayContext& Context) { return GetState(Context)->IsBloodMoon(); }));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		const float MaxStage = GetGameMode(Context)->GetRun()->GetValues().CorruptionMaxStage;
		Context.Test->AddInfo(FString::Printf(TEXT("Blood moon: budget %.1f"), Horde->GetThreatBudget()));
		Context.Test->TestEqual(TEXT("The stage stops at the blood moon"), GetState(Context)->GetCorruptionStage(), FMath::RoundToInt(MaxStage));
		Context.Test->TestNearlyEqual(TEXT("The blood moon knows no quiet phase"), Horde->GetThreatBudget(), ExpectedBudget(Context, Horde->GetValues().Director.BeaconShare), 0.01f);
		Context.Test->TestTrue(TEXT("The budget stops at its peak"), Horde->GetThreatBudget() <= Horde->GetValues().Director.PeakThreat);
	}));

	// R takes it all back, and the next run has no corruption yet.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitForPhase(TEXT("a fresh run starts"), 3.0, EEIRunPhase::OnBarge));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A fresh run has no corruption"), GetState(Context)->GetCorruptionStage(), 0);
		Context.Test->TestFalse(TEXT("A fresh run has no blood moon"), GetState(Context)->IsBloodMoon());
		Context.Test->TestFalse(TEXT("On the barge nothing comes again"), GetGameMode(Context)->GetHorde()->IsHordeActive());
		Context.Hero->SetCanBeDamaged(false);
		SkipRitual(Context);
		StandAtBeacon(Context, GetBeacons(Context)[0], 200.0f);
	}));

	// At the beacon the last wave comes with the full budget.
	Steps.Add(WaitForPhase(TEXT("the run begins again"), 2.0, EEIRunPhase::RitualDone));
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitForPhase(TEXT("the beacon burns"), 1.0, EEIRunPhase::BeaconLit));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		Context.Test->AddInfo(FString::Printf(TEXT("At the beacon: budget %.1f"), Horde->GetThreatBudget()));
		Context.Test->TestNearlyEqual(TEXT("The beacon brings the full budget"), Horde->GetThreatBudget(), ExpectedBudget(Context, Horde->GetValues().Director.BeaconShare), 0.01f);
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIChestTest,
	"Eisenseelen.Play.Run.Chests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIChestTest::RunTest(const FString& Parameters)
{
	const auto ChestsInUse = [](const FEIPlayContext& Context)
	{
		TArray<AEIChest*> Chests;
		for (TActorIterator<AEIChest> It(Context.World); It; ++It)
		{
			if (It->IsInUse())
			{
				Chests.Add(*It);
			}
		}
		return Chests;
	};
	const auto StandBeside = [](const FEIPlayContext& Context, const AEIChest* Chest, float DistanceCm)
	{
		const FVector Location = Chest->GetActorLocation();
		Teleport(Context, FVector(Location.X + DistanceCm, Location.Y, HeroHeight), 180.0f);
	};
	const auto DefeatGuards = [](const FEIPlayContext& Context, int32 HowMany)
	{
		for (TActorIterator<AEIEnemy> It(Context.World); It && HowMany > 0; ++It)
		{
			if (It->IsGuard() && !It->IsDead())
			{
				It->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
				--HowMany;
			}
		}
	};

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([ChestsInUse](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		SwitchOffDirector(Context);

		// The map has more chests than a run uses, and the others are gone.
		int32 All = 0;
		for (TActorIterator<AEIChest> It(Context.World); It; ++It)
		{
			++All;
			Context.Test->TestEqual(TEXT("Only chests in use can be seen"), It->IsHidden(), !It->IsInUse());
		}
		const int32 InUse = ChestsInUse(Context).Num();
		Context.Test->AddInfo(FString::Printf(TEXT("%d of %d chests in this run"), InUse, All));
		Context.Test->TestEqual(TEXT("A run uses its number of chests"), InUse, FMath::RoundToInt(GetGameMode(Context)->GetRun()->GetValues().ChestsPerRun));
		Context.Test->TestTrue(TEXT("The map has more chests than that"), All > InUse);
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(WaitForPhase(TEXT("the run begins"), 2.0, EEIRunPhase::Exploring));

	// Outside the circle nothing happens.
	Steps.Add(Do([ChestsInUse, StandBeside](const FEIPlayContext& Context)
	{
		StandBeside(Context, ChestsInUse(Context)[0], GetGameMode(Context)->GetRun()->GetValues().ChestZoneMeters * CentimetersPerMeter + 200.0f);
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([ChestsInUse, StandBeside](const FEIPlayContext& Context)
	{
		const AEIChest* Chest = ChestsInUse(Context)[0];
		Context.Test->TestEqual(TEXT("Outside the circle the chest stays sealed"), Chest->GetState(), EEIChestState::Sealed);
		Context.Test->TestEqual(TEXT("Outside the circle nobody comes"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		Context.Test->TestEqual(TEXT("Far from a chest the use key finds nothing"), Context.Hero->GetOmens()->CountAll(), 0);
		StandBeside(Context, Chest, 200.0f);
	}));
	Steps.Add(Wait(0.1));

	// In the circle the guards come, and the seal holds as long as one of them lives.
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([ChestsInUse, DefeatGuards](const FEIPlayContext& Context)
	{
		const AEIChest* Chest = ChestsInUse(Context)[0];
		const FEIRunValues& Values = GetGameMode(Context)->GetRun()->GetValues();
		const int32 Expected = FMath::RoundToInt(Values.ChestGuards);
		Context.Test->TestEqual(TEXT("Stepping into the circle calls the guards"), Chest->GetState(), EEIChestState::Guarded);
		Context.Test->TestEqual(TEXT("All guards come"), Chest->GetGuardsLeft(), Expected);
		Context.Test->TestEqual(TEXT("The guards are there"), GetGameMode(Context)->GetHorde()->CountEnemies(), Expected);
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			const float Distance = static_cast<float>(FVector::Dist2D(It->GetActorLocation(), Chest->GetActorLocation()));
			Context.Test->TestTrue(TEXT("They stand around the chest"), Distance < Values.ChestZoneMeters * CentimetersPerMeter);
		}
		Context.Test->TestEqual(TEXT("The seal holds: no omen yet"), Context.Hero->GetOmens()->CountAll(), 0);
		Context.Test->TestFalse(TEXT("The seal holds: the chest stays closed"), Chest->IsOpenFor(Context.Hero));

		// All but one fall, and the last one is far from the hero, but it does not count as left behind.
		DefeatGuards(Context, Expected - 1);
		UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		FEIHordeValues HordeValues = Horde->GetValues();
		HordeValues.TooFarMeters = 1.0f;
		Horde->ApplyValues(HordeValues);
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(Wait(1.3));
	Steps.Add(Do([ChestsInUse, DefeatGuards](const FEIPlayContext& Context)
	{
		const AEIChest* Chest = ChestsInUse(Context)[0];
		Context.Test->TestEqual(TEXT("A fallen guard counts"), Chest->GetGuardsLeft(), 1);
		Context.Test->TestEqual(TEXT("Running away does not break the seal"), Chest->GetState(), EEIChestState::Guarded);
		DefeatGuards(Context, 1);
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([ChestsInUse, StandBeside](const FEIPlayContext& Context)
	{
		const AEIChest* Chest = ChestsInUse(Context)[0];
		Context.Test->TestEqual(TEXT("With the last guard the seal breaks"), Chest->GetState(), EEIChestState::Free);
		Context.Test->TestEqual(TEXT("No guards are left"), Chest->GetGuardsLeft(), 0);
		StandBeside(Context, Chest, 200.0f);
	}));
	Steps.Add(Wait(0.1));

	// Now the use key opens it and gives an omen, once, and nobody comes a second time.
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([ChestsInUse](const FEIPlayContext& Context)
	{
		const UEIOmenComponent* Omens = Context.Hero->GetOmens();
		Context.Test->TestEqual(TEXT("A chest gives an omen"), Omens->CountAll(), 1);
		Context.Test->TestTrue(TEXT("The chest is open now"), ChestsInUse(Context)[0]->IsOpenFor(Context.Hero));
		Context.Test->TestTrue(TEXT("The hero is told"), Omens->GetFoundNote() > 0.0f);
		Context.Test->TestEqual(TEXT("Nobody comes a second time"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
	}));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([ChestsInUse, StandBeside](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("An open chest gives nothing more"), Context.Hero->GetOmens()->CountAll(), 1);

		// Later in the run a chest has more guards.
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		Run->SetCorruptionStage(6);
		StandBeside(Context, ChestsInUse(Context)[1], 200.0f);
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([ChestsInUse](const FEIPlayContext& Context)
	{
		const FEIRunValues& Values = GetGameMode(Context)->GetRun()->GetValues();
		const int32 Expected = FMath::RoundToInt(Values.ChestGuards * (1.0f + Values.CorruptionThreatPerStage * 6.0f));
		Context.Test->AddInfo(FString::Printf(TEXT("At the blood moon a chest has %d guards, at the start %.0f"), ChestsInUse(Context)[1]->GetGuardsLeft(), Values.ChestGuards));
		Context.Test->TestEqual(TEXT("The corruption brings more guards"), ChestsInUse(Context)[1]->GetGuardsLeft(), Expected);
		Context.Test->TestEqual(TEXT("The other chests stay sealed"), ChestsInUse(Context)[2]->GetState(), EEIChestState::Sealed);
	}));

	// R: the omens end with the run, the guards are gone, and the chests are closed and sealed again.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitForPhase(TEXT("a fresh run starts"), 3.0, EEIRunPhase::OnBarge));
	Steps.Add(Do([ChestsInUse](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The omens end with the run"), Context.Hero->GetOmens()->CountAll(), 0);
		Context.Test->TestEqual(TEXT("The guards are gone"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		for (const AEIChest* Chest : ChestsInUse(Context))
		{
			Context.Test->TestEqual(TEXT("A fresh run has sealed chests"), Chest->GetState(), EEIChestState::Sealed);
			Context.Test->TestFalse(TEXT("A fresh run has closed chests"), Chest->IsOpenFor(Context.Hero));
		}
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIRitualTest,
	"Eisenseelen.Play.Run.Ritual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIRitualTest::RunTest(const FString& Parameters)
{
	constexpr float ShortHoldSeconds = 0.5f;
	const TSharedRef<TWeakObjectPtr<AEIAltar>> FirstAltar = MakeShared<TWeakObjectPtr<AEIAltar>>();

	const auto StandAtActiveAltar = [](const FEIPlayContext& Context)
	{
		const FVector Location = GetGameMode(Context)->GetRun()->GetActiveAltar()->GetActorLocation();
		Teleport(Context, FVector(Location.X + 300.0f, Location.Y, HeroHeight), 180.0f);
	};

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([FirstAltar](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		SwitchOffDirector(Context);
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		FEIRunValues Short = Run->GetValues();
		Short.AltarHoldSeconds = ShortHoldSeconds;
		Run->ApplyValues(Short);
		Run->ResetRun();

		// One altar burns, and the marker shows it already from the barge.
		int32 Active = 0;
		int32 Altars = 0;
		for (TActorIterator<AEIAltar> It(Context.World); It; ++It)
		{
			++Altars;
			Active += It->IsActive() ? 1 : 0;
		}
		Context.Test->TestEqual(TEXT("Every place has an altar"), Altars, 8);
		Context.Test->TestEqual(TEXT("One altar burns"), Active, 1);
		Context.Test->TestEqual(TEXT("The ritual has three altars"), GetState(Context)->GetAltarsTotal(), 3);
		*FirstAltar = Run->GetActiveAltar();

		// Away from the barge and from the altar.
		const FVector AltarLocation = Run->GetActiveAltar()->GetActorLocation();
		Teleport(Context, AltarLocation + FVector(2000.0f, 0.0f, HeroHeight), 180.0f);
	}));
	Steps.Add(WaitForPhase(TEXT("the run begins"), 2.0, EEIRunPhase::Exploring));

	// The beacons stay cold until the ritual is broken.
	Steps.Add(Do([](const FEIPlayContext& Context) { StandAtBeacon(Context, GetBeacons(Context)[0], 200.0f); }));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([StandAtActiveAltar](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Before the ritual is broken the beacon stays cold"), GetState(Context)->GetRunPhase(), EEIRunPhase::Exploring);
		StandAtActiveAltar(Context);
	}));

	// In the zone of the altar the time runs, however many enemies stand in it.
	Steps.Add(WaitForPhase(TEXT("the ritual begins"), 1.0, EEIRunPhase::Ritual));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		const float Expected = ExpectedBudget(Context, Horde->GetValues().Director.RitualShare);
		Context.Test->TestNearlyEqual(TEXT("The ritual brings more enemies"), GetGameMode(Context)->GetHorde()->GetThreatBudget(), Expected, 0.01f);
		const FVector AltarLocation = GetGameMode(Context)->GetRun()->GetActiveAltar()->GetActorLocation();
		for (int32 Index = 0; Index < 6; ++Index)
		{
			Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), AltarLocation + FVector(-300.0f, -300.0f + Index * 120.0f, HeroHeight), FRotator::ZeroRotator);
		}
	}));

	Steps.Add(WaitUntil(TEXT("the first altar breaks with enemies in the zone"), 2.0, [](const FEIPlayContext& Context) { return GetState(Context)->GetAltarsBroken() == 1; }));
	Steps.Add(Do([FirstAltar, StandAtActiveAltar](const FEIPlayContext& Context)
	{
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			It->Destroy();
		}
		const AEIAltar* NextAltar = GetGameMode(Context)->GetRun()->GetActiveAltar();
		Context.Test->TestEqual(TEXT("The altar is broken"), FirstAltar->Get()->GetState(), EEIAltarState::Broken);
		Context.Test->TestTrue(TEXT("The next altar is another one"), NextAltar != FirstAltar->Get() && NextAltar->IsActive());
		StandAtActiveAltar(Context);
	}));
	Steps.Add(WaitUntil(TEXT("the second altar breaks"), 3.0, [](const FEIPlayContext& Context) { return GetState(Context)->GetAltarsBroken() == 2; }));
	Steps.Add(Do(StandAtActiveAltar));

	// After the third the ritual is over, and now a beacon can burn.
	Steps.Add(WaitForPhase(TEXT("the ritual is broken"), 3.0, EEIRunPhase::RitualDone));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestNull(TEXT("No altar burns any more"), GetGameMode(Context)->GetRun()->GetActiveAltar());
		StandAtBeacon(Context, GetBeacons(Context)[0], 200.0f);
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitForPhase(TEXT("the beacon burns"), 1.0, EEIRunPhase::BeaconLit));

	// R wakes the ritual up again.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitForPhase(TEXT("a fresh run starts"), 3.0, EEIRunPhase::OnBarge));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A fresh ritual has no broken altars"), GetState(Context)->GetAltarsBroken(), 0);
		Context.Test->TestNotNull(TEXT("A fresh ritual has an altar"), GetGameMode(Context)->GetRun()->GetActiveAltar());
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
		SwitchOffDirector(Context);
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
