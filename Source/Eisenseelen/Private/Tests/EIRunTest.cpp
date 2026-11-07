#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AIController.h"
#include "Algo/Count.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "EIAltar.h"
#include "EIChest.h"
#include "EIEnemy.h"
#include "EIEnemyCamp.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIProfileComponent.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EIHordeDirector.h"
#include "EIRunDirector.h"
#include "EIRuneComponent.h"
#include "EISignalFire.h"
#include "EISkyBarge.h"
#include "EISoulKettle.h"
#include "EITestScript.h"
#include "EIWorldTools.h"
#include "Engine/DamageEvents.h"
#include "EIVisuals.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LandscapeProxy.h"
#include "EILaunchPad.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Tests/AutomationEditorCommon.h"

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

	AEIGameState* GetState(const FEIPlayContext& Context)
	{
		return Context.World->GetGameState<AEIGameState>();
	}

	AEISkyBarge* GetBarge(const FEIPlayContext& Context)
	{
		return GetGameMode(Context)->GetRun()->GetBarge();
	}

	/** The run begins at once, without the way there. It only works from the dock. */
	void StartRun(const FEIPlayContext& Context)
	{
		GetGameMode(Context)->GetRun()->BeginRunNow();
	}

	FEIPlayStep WaitForPhase(const TCHAR* What, double MaxSeconds, EEIRunPhase Phase)
	{
		return WaitUntil(What, MaxSeconds, [Phase](const FEIPlayContext& Context) { return GetState(Context)->GetRunPhase() == Phase; });
	}

	/** No task calls a group. The camps are another matter. */
	void SwitchOffReinforcements(const FEIPlayContext& Context)
	{
		FEIEnemyGroups& Groups = GetGameMode(Context)->GetRun()->GetGroups();
		for (FEIWaveRule* Wave : { &Groups.AltarWave, &Groups.StoneWave, &Groups.KettleWave, &Groups.FightWave, &Groups.PursuitWave, &Groups.FireWave })
		{
			Wave->Size.Empty();
		}
	}

	void DefeatAllEnemies(const FEIPlayContext& Context)
	{
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			It->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
		}
	}

	/** The run offers only contracts of this kind. It only works for a run that has not left the barge yet. */
	void OnlyContracts(const FEIPlayContext& Context, EEIContractKind Kind)
	{
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		Run->GetContractTable().Tasks.RemoveAll([Kind](const FEIContractTask& Task) { return Task.Kind != Kind; });
		Run->ResetRun();
	}

	/** The camps stay empty, no contract asks for a leader, and there is no boss, so the fire can burn at once. It only works for a run that has not left the barge yet. */
	void EmptyCamps(const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		FEIRunValues Values = Run->GetValues();
		Values.CampFactor = 0.0f;
		Run->ApplyValues(Values);
		Run->GetContractTable().Tasks.RemoveAll([](const FEIContractTask& Task) { return Task.Kind == EEIContractKind::Leader; });
		Run->GetBossTable().Bosses.Empty();
		Run->ResetRun();
	}

	/** No enemies at all, for a test that looks at something else. */
	void SwitchOffDirector(const FEIPlayContext& Context)
	{
		SwitchOffReinforcements(Context);
		EmptyCamps(Context);
	}

	/** Standing height on the terrain at this spot, for something the size of the hero. */
	FVector OnGround(const FEIPlayContext& Context, const FVector& Where)
	{
		constexpr float TunnelHeightCm = 400.0f;
		constexpr float SearchCm = 10000.0f;
		FHitResult Hit;
		const FCollisionObjectQueryParams StaticObjects(ECC_WorldStatic);
		if (!Context.World->LineTraceSingleByObjectType(Hit, FVector(Where.X, Where.Y, Where.Z + TunnelHeightCm), FVector(Where.X, Where.Y, -SearchCm), StaticObjects))
		{
			Context.World->LineTraceSingleByObjectType(Hit, FVector(Where.X, Where.Y, SearchCm), FVector(Where.X, Where.Y, -SearchCm), StaticObjects);
		}
		return FVector(Where.X, Where.Y, (Hit.bBlockingHit ? Hit.ImpactPoint.Z : 0.0) + HeroHeight);
	}

	/** Stands next to the fire of this run, far enough from it to look at it. */
	void StandAtFire(const FEIPlayContext& Context, float DistanceCm)
	{
		const FVector Location = GetGameMode(Context)->GetRun()->GetActiveFire()->GetActorLocation();
		Teleport(Context, FVector(Location.X + DistanceCm, Location.Y, HeroHeight), 180.0f);
	}

	/** A fresh dock at once. The R key itself opens the pause menu during a run, so the tests call the restart directly. */
	FEIPlayStep RestartNow()
	{
		return Do([](const FEIPlayContext& Context) { GetGameMode(Context)->RestartRun(); });
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
		FVector Dock = FVector::ZeroVector;
	};
	const TSharedRef<FState> State = MakeShared<FState>();

	const auto StandAtBarge = [](const FEIPlayContext& Context)
	{
		// At the railing, right beside the deck.
		const FVector Barge = GetBarge(Context)->GetActorLocation();
		Teleport(Context, FVector(Barge.X, Barge.Y - 500.0f, Barge.Z + HeroHeight), 90.0f);
	};

	TArray<FEIPlayStep> Steps;

	// The run starts on the dock, and the clock waits. Nothing of the map shows yet.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		FEIRunValues Short = GetGameMode(Context)->GetRun()->GetValues();
		Short.CastOffSeconds = 0.4f;
		Short.CrossingSeconds = 0.4f;
		Short.SignalHoldSeconds = ShortHoldSeconds;
		GetGameMode(Context)->GetRun()->ApplyValues(Short);
		SwitchOffDirector(Context);
		Context.Test->TestEqual(TEXT("The run starts at the dock"), GetState(Context)->GetRunPhase(), EEIRunPhase::AtDock);
		Context.Test->TestTrue(TEXT("The hero stands on the ground of the dock"), Context.Movement->IsMovingOnGround());
		Context.Test->TestFalse(TEXT("The hero is not aboard yet"), Context.Hero->IsAboard());
		Context.Test->TestTrue(TEXT("The dock lies high up, far away from the map"), Context.Hero->GetActorLocation().Size2D() > 100000.0);
		Context.Test->TestEqual(TEXT("The clock has not started"), GetState(Context)->GetRunSeconds(), 0.0f);
		State->Dock = Context.Hero->GetActorLocation();
	}));

	// At the dock the gear can still change, but only to what the player owns.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEIProfileComponent* Profile = Context.Player->GetProfile();
		Profile->EquipByName(TEXT("Donnerrune"));
		Context.Test->TestEqual(TEXT("What the player does not own stays off"), Context.Hero->GetRunes()->GetRuneIndex(), 0);
		Profile->GetSave().Items.Add(TEXT("Donnerrune"));
		Profile->EquipByName(TEXT("Donnerrune"));
	}));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("At the dock another rune can go on"), Context.Hero->GetRunes()->GetRuneIndex(), 1);
	}));

	// From the middle of the dock the use key does not reach the barge.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FVector Barge = GetBarge(Context)->GetActorLocation();
		Teleport(Context, FVector(Barge.X, Barge.Y - 1500.0f, Barge.Z + HeroHeight), 90.0f);
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([StandAtBarge](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("From afar nobody boards"), Context.Hero->IsAboard());
		StandAtBarge(Context);
	}));
	Steps.Add(Wait(0.1));

	// Right at the barge it takes the hero aboard, and with everybody aboard it casts off.
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.02));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The hero is aboard"), Context.Hero->IsAboard());
		Context.Test->TestTrue(TEXT("The hero rides the barge"), Context.Hero->GetAttachParentActor() == GetBarge(Context));
	}));

	// Then it goes across, in the dark, and there the team stands at the edge of the map.
	Steps.Add(WaitForPhase(TEXT("the barge casts off"), 5.0, EEIRunPhase::Crossing));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("On the way the hero stays aboard"), Context.Hero->IsAboard());
	}));
	Steps.Add(WaitForPhase(TEXT("the team arrives"), 5.0, EEIRunPhase::Exploring));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const FVector Location = Context.Hero->GetActorLocation();
		const float Half = GetGameMode(Context)->GetRun()->GetContractTable().MapMeters * CentimetersPerMeter / 2.0f;
		const float FromEdge = Half - static_cast<float>(FMath::Max(FMath::Abs(Location.X), FMath::Abs(Location.Y)));
		const float FromCenter = static_cast<float>(Location.Size2D());
		Context.Test->AddInfo(FString::Printf(TEXT("The hero arrives %.0f cm from the center, %.0f cm inside the border"), FromCenter, FromEdge));
		Context.Test->TestFalse(TEXT("The hero is off the barge"), Context.Hero->IsAboard());
		Context.Test->TestNull(TEXT("The hero rides nothing any more"), Context.Hero->GetAttachParentActor());
		Context.Test->TestTrue(TEXT("The arrival lies inside the map"), FromEdge > 0.0f);
		Context.Test->TestTrue(TEXT("The arrival lies away from the middle"), FromCenter >= 12000.0f);
		Context.Test->TestTrue(TEXT("The hero stands on the ground"), Context.Movement->IsMovingOnGround());
		Context.Test->TestTrue(TEXT("The hero can be hit again"), Context.Hero->CanBeDamaged());
		Context.Test->TestTrue(TEXT("The clock runs"), GetState(Context)->GetRunSeconds() > 0.0f);
		Context.Test->TestEqual(TEXT("A run has three side jobs"), GetState(Context)->GetContracts().Num(), 3);
		Context.Test->TestTrue(TEXT("The barge is on its way up"), GetBarge(Context)->GetActorLocation().Z > 400.0);
		Context.Hero->SetCanBeDamaged(false);
	}));

	// During the run the rune stays, and the gear page does not open.
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Player->GetProfile()->EquipByName(TEXT("Frostrune")); }));
	AddTap(Steps, EKeys::I);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("During a run the gear stays"), Context.Hero->GetRunes()->GetRuneIndex(), 1);
		Context.Test->TestFalse(TEXT("During a run the gear page stays shut"), Context.Player->IsMenuOpen());
	}));

	// The map has several places for the fire, and a run uses one.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		int32 All = 0;
		int32 InUse = 0;
		for (TActorIterator<AEISignalFire> It(Context.World); It; ++It)
		{
			++All;
			InUse += It->IsInUse() ? 1 : 0;
			Context.Test->TestEqual(TEXT("Only the fire in use can be seen"), It->IsHidden(), !It->IsInUse());
		}
		Context.Test->TestEqual(TEXT("A run uses one fire"), InUse, 1);
		Context.Test->TestTrue(TEXT("The map has several"), All > 1);
		StandAtFire(Context, 1500.0f);
	}));
	Steps.Add(Wait(0.1));

	// The use key does nothing away from the fire, and lights it right there.
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Away from the fire the use key does nothing"), GetState(Context)->GetRunPhase(), EEIRunPhase::Exploring);
		StandAtFire(Context, 200.0f);
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitForPhase(TEXT("the fire burns"), 1.0, EEIRunPhase::SignalLit));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The fire burns"), GetGameMode(Context)->GetRun()->GetActiveFire()->IsLit());
		StandAtFire(Context, 4000.0f);
	}));

	// Leaving the zone stops the wait for the barge.
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->HoldLeft = GetState(Context)->GetSignalHoldLeft(); }));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("Outside the zone nobody holds the fire"), GetState(Context)->IsSignalHeld());
		Context.Test->TestEqual(TEXT("Outside the zone the wait stands still"), GetState(Context)->GetSignalHoldLeft(), State->HoldLeft);
		StandAtFire(Context, 200.0f);
	}));

	// Back in the zone the barge arrives after the rest of the time.
	Steps.Add(WaitForPhase(TEXT("the barge arrives"), 5.0, EEIRunPhase::BargeWaiting));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FVector BargeLocation = GetBarge(Context)->GetActorLocation();
		const double Distance = FVector::Dist2D(BargeLocation, GetGameMode(Context)->GetRun()->GetActiveFire()->GetActorLocation());
		Context.Test->AddInfo(FString::Printf(TEXT("The barge hovers %.0f cm from the fire, %.0f cm high"), Distance, BargeLocation.Z));
		Context.Test->TestNearlyEqual(TEXT("The barge comes to the fire"), static_cast<float>(Distance), GetGameMode(Context)->GetRun()->GetValues().BargeDistanceMeters * CentimetersPerMeter, 10.0f);
		Context.Test->TestTrue(TEXT("The barge comes down"), BargeLocation.Z < 500.0);
	}));

	// The use key does not board from afar: on the other side of the fire.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FVector Fire = GetGameMode(Context)->GetRun()->GetActiveFire()->GetActorLocation();
		const FVector Mirrored = Fire * 2.0 - GetBarge(Context)->GetActorLocation();
		Teleport(Context, FVector(Mirrored.X, Mirrored.Y, HeroHeight), 0.0f);
	}));
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

	// Right there it ends the run, and the barge carries the hero away. Whatever the hero earned is safe now.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEIProfileComponent* Profile = Context.Player->GetProfile();
		Profile->GrantContract(0, 100, 10);
		Profile->GrantEnemyReward(10.0f);
	}));
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitForPhase(TEXT("the hero gets away"), 1.0, EEIRunPhase::Escaped));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		const FString Faction = Profile->GetContractTable().Factions[0].Name;
		Context.Test->TestEqual(TEXT("All the gold is safe"), Profile->GetSave().Gold, 100);
		Context.Test->TestEqual(TEXT("All the points are safe"), Profile->GetSave().FactionPoints.FindRef(Faction), 10);
		Context.Test->TestTrue(TEXT("The experience is safe"), Profile->GetSave().Experience >= 10);
		Context.Test->TestTrue(TEXT("The settlement counts it"), Profile->GetRunExperience() >= 10);
		Context.Test->TestTrue(TEXT("The hero stands on the barge"), Context.Hero->GetAttachParentActor() == GetBarge(Context));
		Context.Test->TestFalse(TEXT("The hero is safe on the barge"), Context.Hero->CanBeDamaged());
		State->Height = Context.Hero->GetActorLocation().Z;
		State->RunSeconds = GetState(Context)->GetRunSeconds();
	}));
	Steps.Add(Wait(5.0));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The barge rises"), Context.Hero->GetActorLocation().Z > State->Height + 100.0);
		Context.Test->TestEqual(TEXT("The clock stops"), GetState(Context)->GetRunSeconds(), State->RunSeconds);
	}));

	// R starts over, at the dock.
	Steps.Add(RestartNow());
	Steps.Add(WaitUntil(TEXT("a fresh run starts"), 3.0, [](const FEIPlayContext& Context)
	{
		return GetState(Context)->GetRunPhase() == EEIRunPhase::AtDock && Context.Movement->IsMovingOnGround();
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const AEISkyBarge* Barge = GetBarge(Context);
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		Context.Test->TestTrue(TEXT("The barge is back"), FVector::Dist(Barge->GetActorLocation(), Barge->GetStartLocation()) < 1.0);
		Context.Test->TestTrue(TEXT("The hero is back at the dock"), FVector::Dist(Context.Hero->GetActorLocation(), State->Dock) < 2000.0);
		Context.Test->TestFalse(TEXT("The hero is not aboard"), Context.Hero->IsAboard());
		Context.Test->TestFalse(TEXT("The hero is no longer attached"), Context.Hero->GetAttachParentActor() != nullptr);
		Context.Test->TestFalse(TEXT("The fire is out"), GetGameMode(Context)->GetRun()->GetActiveFire()->IsLit());
		Context.Test->TestEqual(TEXT("All enemies are gone"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		Context.Test->TestEqual(TEXT("The next run starts with the same rune"), Context.Hero->GetRunes()->GetRuneIndex(), 1);
		Context.Test->TestEqual(TEXT("The new run has earned nothing yet"), Profile->GetRunGold(), 0);
		Context.Test->TestEqual(TEXT("The gold of the last one stays"), Profile->GetSave().Gold, 100);
		Context.Test->TestEqual(TEXT("A new run has new side jobs"), GetState(Context)->CountContractsDone(), 0);
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIDockTest,
	"Eisenseelen.Play.Run.Dock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIDockTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;

	// A long wait on the deck, to get off again in time.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		FEIRunValues Long = Run->GetValues();
		Long.CastOffSeconds = 100.0f;
		Run->ApplyValues(Long);
		Run->ResetRun();
		const FVector Barge = GetBarge(Context)->GetActorLocation();
		Teleport(Context, FVector(Barge.X, Barge.Y - 500.0f, Barge.Z + HeroHeight), 90.0f);
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.3));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const AEISkyBarge* Barge = GetBarge(Context);
		Context.Test->TestTrue(TEXT("The hero is aboard"), Context.Hero->IsAboard());
		Context.Test->TestTrue(TEXT("With everybody aboard the barge casts off"), GetState(Context)->IsCastingOff());
		Context.Test->TestFalse(TEXT("Aboard the hero cannot be hit"), Context.Hero->CanBeDamaged());
		Context.Test->TestTrue(TEXT("The barge rises"), Barge->GetActorLocation().Z > Barge->GetStartLocation().Z);
	}));

	// The gear page works on the deck.
	AddTap(Steps, EKeys::I);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("On the deck the gear page opens"), Context.Player->IsMenuOpen());
		Context.Player->SetMenuOpen(false);
	}));

	// Stepping off in time brings the barge back.
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.15));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const AEISkyBarge* Barge = GetBarge(Context);
		Context.Test->TestFalse(TEXT("The hero is off the barge"), Context.Hero->IsAboard());
		Context.Test->TestNull(TEXT("The hero rides nothing any more"), Context.Hero->GetAttachParentActor());
		Context.Test->TestTrue(TEXT("The hero stands on the dock again"), Context.Movement->IsMovingOnGround());
		Context.Test->TestTrue(TEXT("The hero can be hit again"), Context.Hero->CanBeDamaged());
		Context.Test->TestFalse(TEXT("The barge stays"), GetState(Context)->IsCastingOff());
		Context.Test->TestTrue(TEXT("The barge is back at its place"), FVector::Dist(Barge->GetActorLocation(), Barge->GetStartLocation()) < 1.0);
		Context.Test->TestEqual(TEXT("Nothing has begun"), GetState(Context)->GetRunPhase(), EEIRunPhase::AtDock);
	}));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("Boarding works a second time"), Context.Hero->IsAboard());
	}));

	// R takes everything back.
	Steps.Add(RestartNow());
	Steps.Add(WaitUntil(TEXT("a fresh dock"), 3.0, [](const FEIPlayContext& Context) { return !Context.Hero->IsAboard() && !GetState(Context)->IsCastingOff(); }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const AEISkyBarge* Barge = GetBarge(Context);
		Context.Test->TestTrue(TEXT("The barge is back at its place"), FVector::Dist(Barge->GetActorLocation(), Barge->GetStartLocation()) < 1.0);
		Context.Test->TestEqual(TEXT("A new start at the dock"), GetState(Context)->GetRunPhase(), EEIRunPhase::AtDock);
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
		float Threat = 0.0f;
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
			Context.Test->TestTrue(TEXT("An enemy appears at a distance"), FVector::Dist2D(It->GetActorLocation(), Context.Hero->GetActorLocation()) >= MinDistance * 0.8f);
			const bool bInViewCone = FVector::DotProduct(Context.Hero->GetViewRotation().Vector(), (It->GetActorLocation() - Eye).GetSafeNormal()) >= InViewDot * 1.3f;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(EITestSight), false, Context.Hero);
			const bool bWallBetween = Context.World->LineTraceTestByObjectType(Eye, It->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Query);
			Context.Test->TestTrue(TEXT("An enemy appears out of sight"), !bInViewCone || bWallBetween);
		}
	};
	const auto Threat = [](const FEIPlayContext& Context) { return GetGameMode(Context)->GetHorde()->CountThreat(); };

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		EmptyCamps(Context);
		Context.Test->TestEqual(TEXT("At the dock nothing comes yet"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		StartRun(Context);
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));

	// Nothing comes by itself, however long one walks about.
	Steps.Add(WaitForPhase(TEXT("the run begins"), 2.0, EEIRunPhase::Exploring));
	Steps.Add(Wait(3.0));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Without a task nothing comes"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		Context.Test->TestFalse(TEXT("Without a task the blood moon does not rise"), GetState(Context)->IsBloodMoon());
	}));

	// Whoever holds an altar calls the blood moon: a small group comes at once, and more follow up to a limit.
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("a fresh run starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		EmptyCamps(Context);
		OnlyContracts(Context, EEIContractKind::Altar);
		FEIEnemyGroups& Groups = GetGameMode(Context)->GetRun()->GetGroups();
		Groups.AltarWave.Size = TEXT("klein");
		Groups.AltarWave.EverySeconds = 0.5f;
		Groups.ThreatCap = 8.0f;
		Groups.BloodMoonLingerSeconds = 0.5f;
		const FVector Altar = GetState(Context)->GetContracts()[0].Location;
		StartRun(Context);
		Teleport(Context, FVector(Altar.X + 300.0f, Altar.Y, HeroHeight), 180.0f);
	}));
	Steps.Add(WaitForPhase(TEXT("the run begins at the altar"), 2.0, EEIRunPhase::Exploring));
	Steps.Add(Wait(0.15));
	Steps.Add(Do([CheckNewEnemies, Threat](const FEIPlayContext& Context)
	{
		CheckNewEnemies(Context);
		Context.Test->TestTrue(TEXT("Holding the altar calls the blood moon"), GetState(Context)->IsBloodMoon());
		Context.Test->TestTrue(TEXT("At the altar a group comes at once"), Threat(Context) > 0.0f);
		Context.Test->TestTrue(TEXT("The first group is small"), Threat(Context) <= 4.0f);
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			Context.Test->TestFalse(TEXT("It hunts the hero"), It->IsWatching());
			Context.Test->TestNearlyEqual(TEXT("It is no stronger than its kind"), It->GetHealth()->GetMaxHealth(), It->GetValues().Life, 0.01f);
		}
	}));
	Steps.Add(WaitUntil(TEXT("more groups come up to the limit"), 15.0, [CheckNewEnemies, Threat](const FEIPlayContext& Context)
	{
		CheckNewEnemies(Context);
		return Threat(Context) >= 8.0f;
	}));
	Steps.Add(Do([State, Threat](const FEIPlayContext& Context)
	{
		State->Threat = Threat(Context);
		Context.Test->AddInfo(FString::Printf(TEXT("Altar: threat %.0f at a limit of 8, %d enemies checked"), State->Threat, State->Checked));
		Context.Test->TestTrue(TEXT("A group does not go far over the limit"), State->Threat < 12.0f);
	}));
	Steps.Add(Wait(0.7));
	Steps.Add(Do([State, Threat](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("At the limit nobody more comes"), Threat(Context), State->Threat);

		// Far away from everyone: such an enemy makes way.
		State->LeftBehind = Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), OnGround(Context, Context.Hero->GetActorLocation() + FVector(9000.0f, 0.0f, 0.0f)), FRotator::ZeroRotator);
		Context.Test->TestTrue(TEXT("An enemy can be far away"), State->LeftBehind.IsValid());
	}));
	Steps.Add(WaitUntil(TEXT("the enemy left behind is gone"), 3.0, [State](const FEIPlayContext& Context) { return !State->LeftBehind.IsValid(); }));

	// Away from the altar no group comes any more, and the sky clears soon after.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		DefeatAllEnemies(Context);
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context) { DefeatAllEnemies(Context); }));
	Steps.Add(Wait(1.0));
	Steps.Add(Do([Threat](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Away from the altar no group comes"), Threat(Context), 0.0f);
		Context.Test->TestFalse(TEXT("The sky clears again"), GetState(Context)->IsBloodMoon());
	}));

	// A fresh run starts without the blood moon.
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("another fresh run starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("A fresh run has no blood moon"), GetState(Context)->IsBloodMoon());
		Context.Test->TestEqual(TEXT("A fresh run has no enemies"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
	}));

	// At the fire too.
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("one more fresh run starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		EmptyCamps(Context);
		StartRun(Context);
		StandAtFire(Context, 200.0f);
	}));
	Steps.Add(WaitForPhase(TEXT("the run begins again"), 2.0, EEIRunPhase::Exploring));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([Threat](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Before the fire burns nobody comes"), Threat(Context), 0.0f);
	}));
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitForPhase(TEXT("the fire burns"), 1.0, EEIRunPhase::SignalLit));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([Threat](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("At the fire a wave comes"), Threat(Context) > 0.0f);
		Context.Test->TestTrue(TEXT("The fire calls the blood moon"), GetState(Context)->IsBloodMoon());
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEICampsTest,
	"Eisenseelen.Play.Run.Camps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEICampsTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		FVector Camp = FVector::ZeroVector;
		TWeakObjectPtr<AStaticMeshActor> Wall;
		int32 AllEnemies = 0;
	};
	const TSharedRef<FState> State = MakeShared<FState>();

	// The enemies of the camp the test looks at, wherever they went: none of the others are near it.
	const auto CampEnemies = [State](const FEIPlayContext& Context)
	{
		TArray<AEIEnemy*> Enemies;
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			if (FVector::Dist2D(It->GetActorLocation(), State->Camp) < 3000.0)
			{
				Enemies.Add(*It);
			}
		}
		return Enemies;
	};

	TArray<FEIPlayStep> Steps;
	Steps.Add(Wait(2.0));
	Steps.Add(WaitUntil(TEXT("the paths are built"), 30.0, [](const FEIPlayContext& Context)
	{
		const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Context.World);
		return Navigation != nullptr && Navigation->GetDefaultNavDataInstance() != nullptr && !Navigation->IsNavigationBeingBuilt(Context.World);
	}));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		SwitchOffReinforcements(Context);
		Context.Test->TestEqual(TEXT("At the dock the camps are still empty"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		StartRun(Context);
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));

	// When the run begins the camps are full, and from far away nobody notices the hero.
	Steps.Add(WaitForPhase(TEXT("the run begins"), 2.0, EEIRunPhase::Exploring));
	Steps.Add(Wait(1.0));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		int32 Expected = 0;
		double Farthest = -1.0;
		for (TActorIterator<AEIEnemyCamp> It(Context.World); It; ++It)
		{
			Expected += It->CountMembers();
			Context.Test->TestTrue(TEXT("Every camp has its group"), It->CountMembers() > 0);
			if (It->GetActorLocation().Size2D() > Farthest)
			{
				Farthest = It->GetActorLocation().Size2D();
				State->Camp = It->GetActorLocation();
			}
		}
		State->AllEnemies = Expected;
		Context.Test->AddInfo(FString::Printf(TEXT("%d enemies in the camps"), Expected));
		Context.Test->TestEqual(TEXT("All camps are full"), Horde->CountEnemies(), Expected);
		Context.Test->TestEqual(TEXT("From far away nobody comes"), Horde->CountThreat(), 0.0f);

		// In their sight range, but behind a wall.
		const FVector Outwards = State->Camp.GetSafeNormal2D();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Wall = Context.World->SpawnActor<AStaticMeshActor>(State->Camp + Outwards * 900.0f + FVector(0.0f, 0.0f, 400.0f), Outwards.Rotation(), Params);
		Wall->SetMobility(EComponentMobility::Movable);
		Wall->GetStaticMeshComponent()->SetStaticMesh(EIVisuals::GetCube());
		Wall->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
		Wall->SetActorScale3D(FVector(1.0f, 24.0f, 8.0f));
		State->Wall = Wall;
		const FVector Spot = State->Camp + Outwards * 1500.0f;
		Teleport(Context, FVector(Spot.X, Spot.Y, HeroHeight), 0.0f);
	}));
	Steps.Add(Wait(1.0));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Behind a wall nobody sees the hero"), GetGameMode(Context)->GetHorde()->CountThreat(), 0.0f);
		State->Wall->Destroy();
	}));

	// Without the wall they see the hero, and the whole camp comes along, but no other camp.
	Steps.Add(WaitUntil(TEXT("the camp notices the hero"), 1.0, [](const FEIPlayContext& Context) { return GetGameMode(Context)->GetHorde()->CountThreat() > 0.0f; }));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([State, CampEnemies](const FEIPlayContext& Context)
	{
		const TArray<AEIEnemy*> Enemies = CampEnemies(Context);
		if (Enemies.IsEmpty())
		{
			Context.Test->AddError(TEXT("The camp is empty"));
			return;
		}
		Context.Test->AddInfo(FString::Printf(TEXT("%d enemies notice the hero at 1500 cm, sight %.0f m"), Enemies.Num(), Enemies[0]->GetValues().SightMeters));
		for (const AEIEnemy* Enemy : Enemies)
		{
			Context.Test->TestFalse(TEXT("The whole camp comes"), Enemy->IsWatching());
		}
		float CampThreat = 0.0f;
		for (const AEIEnemy* Enemy : Enemies)
		{
			CampThreat += Enemy->GetValues().Threat;
		}
		Context.Test->TestEqual(TEXT("The other camps stay where they are"), GetGameMode(Context)->GetHorde()->CountThreat(), CampThreat);
		Context.Test->TestEqual(TEXT("All enemies are still there"), GetGameMode(Context)->GetHorde()->CountEnemies(), State->AllEnemies);

		// Whoever gets away far enough is left alone.
		for (AEIEnemy* Enemy : Enemies)
		{
			FEIEnemyValues Quick = Enemy->GetValues();
			Quick.StayAwakeSeconds = 0.0f;
			Enemy->ApplyValues(Quick);
		}
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the enemies give up"), 8.0, [](const FEIPlayContext& Context) { return GetGameMode(Context)->GetHorde()->CountThreat() <= 0.0f; }));
	Steps.Add(WaitUntil(TEXT("the enemies are back at their camp"), 15.0, [State](const FEIPlayContext& Context)
	{
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			// Their homes lie around the middle of the camp, and they stop a little before them.
			const double Distance = FVector::Dist2D(It->GetActorLocation(), State->Camp);
			if (Distance > 600.0 && Distance < 3000.0)
			{
				return false;
			}
		}
		return true;
	}));

	// A hit from afar wakes the one who is hit and those around it.
	Steps.Add(Do([CampEnemies](const FEIPlayContext& Context)
	{
		const TArray<AEIEnemy*> Enemies = CampEnemies(Context);
		if (Enemies.IsEmpty())
		{
			Context.Test->AddError(TEXT("The camp is gone"));
			return;
		}
		Context.Test->TestTrue(TEXT("They stand watch again"), Enemies[0]->IsWatching());
		Enemies[0]->TakeDamage(1.0f, FDamageEvent(), Context.Hero->GetController(), Context.Hero);
		for (const AEIEnemy* Enemy : Enemies)
		{
			Context.Test->TestFalse(TEXT("A hit wakes the camp"), Enemy->IsWatching());
		}
	}));

	// An empty camp stays empty: nothing comes back by itself.
	Steps.Add(Do([CampEnemies](const FEIPlayContext& Context)
	{
		for (AEIEnemy* Enemy : CampEnemies(Context))
		{
			Enemy->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
		}
	}));
	Steps.Add(Wait(0.4));
	Steps.Add(Do([CampEnemies](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The camp is empty now"), CampEnemies(Context).Num(), 0);
	}));
	Steps.Add(Wait(1.5));
	Steps.Add(Do([CampEnemies](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("An empty camp does not fill up again"), CampEnemies(Context).Num(), 0);
	}));

	// R clears the map, and the camps fill up with the next jump.
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("a fresh run starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A fresh run has empty camps"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
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
		Teleport(Context, FVector(Location.X + DistanceCm, Location.Y, Location.Z + HeroHeight), 180.0f);
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

	const TSharedRef<float> FirstGuardsThreat = MakeShared<float>(0.0f);

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([ChestsInUse](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		SwitchOffDirector(Context);
		OnlyContracts(Context, EEIContractKind::Chest);

		// The map has more chests than a run uses, and the others are gone.
		int32 All = 0;
		for (TActorIterator<AEIChest> It(Context.World); It; ++It)
		{
			++All;
			Context.Test->TestEqual(TEXT("Only chests in use can be seen"), It->IsHidden(), !It->IsInUse());
		}
		const int32 InUse = ChestsInUse(Context).Num();
		Context.Test->AddInfo(FString::Printf(TEXT("%d of %d chests in this run"), InUse, All));
		Context.Test->TestEqual(TEXT("Every contract has its chest"), InUse, GetState(Context)->GetContracts().Num());
		Context.Test->TestTrue(TEXT("The map has more chests than that"), All > InUse);
		StartRun(Context);
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
		Context.Test->TestEqual(TEXT("Far from a chest the use key finds nothing"), GetState(Context)->CountContractsDone(), 0);
		StandBeside(Context, Chest, 200.0f);
	}));
	Steps.Add(Wait(0.1));

	// In the circle the guards come, and the seal holds as long as one of them lives.
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([ChestsInUse, DefeatGuards, FirstGuardsThreat](const FEIPlayContext& Context)
	{
		const AEIChest* Chest = ChestsInUse(Context)[0];
		const FEIRunValues& Values = GetGameMode(Context)->GetRun()->GetValues();
		const int32 Expected = Chest->GetGuardsLeft();
		Context.Test->TestEqual(TEXT("Stepping into the circle calls the guards"), Chest->GetState(), EEIChestState::Guarded);
		Context.Test->TestTrue(TEXT("Guards come"), Expected > 0);
		Context.Test->TestEqual(TEXT("The guards are there"), GetGameMode(Context)->GetHorde()->CountEnemies(), Expected);
		*FirstGuardsThreat = GetGameMode(Context)->GetHorde()->CountThreat();
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			const float Distance = static_cast<float>(FVector::Dist2D(It->GetActorLocation(), Chest->GetActorLocation()));
			Context.Test->TestTrue(TEXT("They stand around the chest"), Distance < Values.ChestZoneMeters * CentimetersPerMeter);
		}
		Context.Test->TestEqual(TEXT("The seal holds: the contract stays open"), GetState(Context)->CountContractsDone(), 0);
		Context.Test->TestFalse(TEXT("The seal holds: the chest stays closed"), Chest->IsOpen());

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

	// Now the use key opens it and fulfils the contract, once, and nobody comes a second time.
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([ChestsInUse](const FEIPlayContext& Context)
	{
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		Context.Test->TestEqual(TEXT("The chest fulfils its contract"), GetState(Context)->CountContractsDone(), 1);
		Context.Test->TestEqual(TEXT("It pays the gold"), Profile->GetRunGold(), Profile->GetContractTable().GetGold());
		Context.Test->TestTrue(TEXT("The chest is open now"), ChestsInUse(Context)[0]->IsOpen());
		Context.Test->TestTrue(TEXT("The hero is told"), Profile->GetRewardNote() > 0.0f);
		Context.Test->TestEqual(TEXT("Nobody comes a second time"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
	}));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([ChestsInUse, StandBeside](const FEIPlayContext& Context)
	{
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		Context.Test->TestEqual(TEXT("An open chest counts only once"), GetState(Context)->CountContractsDone(), 1);
		Context.Test->TestEqual(TEXT("An open chest pays only once"), Profile->GetRunGold(), Profile->GetContractTable().GetGold());

		// The next chest has its own guards.
		StandBeside(Context, ChestsInUse(Context)[1], 200.0f);
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([ChestsInUse, FirstGuardsThreat](const FEIPlayContext& Context)
	{
		const float Later = GetGameMode(Context)->GetHorde()->CountThreat();
		Context.Test->AddInfo(FString::Printf(TEXT("The guards of the next chest have a threat of %.0f, those of the first %.0f"), Later, *FirstGuardsThreat));
		Context.Test->TestTrue(TEXT("The next chest has guards too"), Later > 0.0f);
		Context.Test->TestEqual(TEXT("The other chests stay sealed"), ChestsInUse(Context)[2]->GetState(), EEIChestState::Sealed);
	}));

	// R: the guards are gone, and the chests are closed and sealed again.
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("a fresh run starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([ChestsInUse](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The guards are gone"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		for (const AEIChest* Chest : ChestsInUse(Context))
		{
			Context.Test->TestEqual(TEXT("A fresh run has sealed chests"), Chest->GetState(), EEIChestState::Sealed);
			Context.Test->TestFalse(TEXT("A fresh run has closed chests"), Chest->IsOpen());
		}
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIContractsTest,
	"Eisenseelen.Play.Run.Contracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIContractsTest::RunTest(const FString& Parameters)
{
	const auto Leaders = [](const FEIPlayContext& Context)
	{
		TArray<AEIEnemy*> Found;
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			if (It->IsLeader())
			{
				Found.Add(*It);
			}
		}
		return Found;
	};
	const auto CountKind = [](const FEIPlayContext& Context, EEIContractKind Kind)
	{
		int32 Count = 0;
		for (const FEIContract& Contract : GetState(Context)->GetContracts())
		{
			Count += Contract.Kind == Kind ? 1 : 0;
		}
		return Count;
	};
	const auto StandAt = [](const FEIPlayContext& Context, const FVector& Location, float OffsetCm)
	{
		Teleport(Context, FVector(Location.X + OffsetCm, Location.Y, HeroHeight), 180.0f);
	};
	const TSharedRef<float> HoldLeft = MakeShared<float>(0.0f);

	TArray<FEIPlayStep> Steps;
	Steps.Add(WaitUntil(TEXT("the paths are built"), 30.0, [](const FEIPlayContext& Context)
	{
		const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Context.World);
		return Navigation != nullptr && Navigation->GetDefaultNavDataInstance() != nullptr && !Navigation->IsNavigationBeingBuilt(Context.World);
	}));

	// The list is there from the barge on: six jobs, each faction has its share, and each lies somewhere else.
	Steps.Add(Do([Leaders, CountKind](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		SwitchOffReinforcements(Context);
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		FEIRunValues NoCamps = Run->GetValues();
		NoCamps.CampFactor = 0.0f;
		Run->ApplyValues(NoCamps);

		const FEIContractTable& Table = Run->GetContractTable();
		const TArray<FEIContract>& Contracts = GetState(Context)->GetContracts();
		Context.Test->TestEqual(TEXT("A run offers three side jobs"), Contracts.Num(), 3);
		for (const FEIContractTask& Task : Table.Tasks)
		{
			Context.Test->TestTrue(TEXT("No task comes twice"), CountKind(Context, Task.Kind) <= 1);
		}
		for (int32 First = 0; First < Contracts.Num(); ++First)
		{
			for (int32 Second = First + 1; Second < Contracts.Num(); ++Second)
			{
				Context.Test->TestTrue(TEXT("No two contracts share a place"), FVector::Dist2D(Contracts[First].Location, Contracts[Second].Location) > 100.0);
			}
			const FEIContractTask* Task = Table.FindTask(Contracts[First].Kind);
			Context.Test->TestEqual(TEXT("A contract has its time to hold"), Contracts[First].HoldLeft, Task->HoldSeconds);
		}
		int32 Woken = 0;
		for (TActorIterator<AEIAltar> It(Context.World); It; ++It)
		{
			Woken += It->IsActive() ? 1 : 0;
		}
		Context.Test->TestEqual(TEXT("Only the altars of contracts are awake"), Woken, CountKind(Context, EEIContractKind::Altar) + CountKind(Context, EEIContractKind::Shrine));
		int32 ChestsInUse = 0;
		for (TActorIterator<AEIChest> It(Context.World); It; ++It)
		{
			ChestsInUse += It->IsInUse() ? 1 : 0;
		}
		Context.Test->TestEqual(TEXT("Only the chests of contracts are there"), ChestsInUse, CountKind(Context, EEIContractKind::Chest));
		Context.Test->TestEqual(TEXT("At the dock no leader stands yet"), Leaders(Context).Num(), 0);
		StartRun(Context);
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));

	// With the arrival the leaders stand in their camps.
	Steps.Add(WaitForPhase(TEXT("the run begins"), 3.0, EEIRunPhase::Exploring));
	Steps.Add(Do([Leaders, CountKind](const FEIPlayContext& Context)
	{
		const FEIRunValues& Values = GetGameMode(Context)->GetRun()->GetValues();
		const TArray<AEIEnemy*> Found = Leaders(Context);
		Context.Test->TestEqual(TEXT("Every leader contract has its leader"), Found.Num(), CountKind(Context, EEIContractKind::Leader));
		FEIEnemyValues Normal;
		FString Error;
		FEIEnemyValues::LoadFromProjectFile(Values.LeaderKind, Normal, Error);
		for (const AEIEnemy* Leader : Found)
		{
			Context.Test->TestEqual(TEXT("A leader has more life"), Leader->GetHealth()->GetMaxHealth(), Normal.Life * Values.LeaderLifeFactor);
			Context.Test->TestEqual(TEXT("A leader hits harder"), Leader->GetValues().Damage, Normal.Damage * Values.LeaderDamageFactor);
			Context.Test->TestEqual(TEXT("A leader is of the wanted kind"), Leader->GetKind(), Values.LeaderKind);
			Context.Test->TestTrue(TEXT("A leader waits at home"), Leader->HasHome());
			Context.Test->TestNearlyEqual(TEXT("A leader is bigger"), static_cast<float>(Leader->GetActorScale3D().X), Normal.Scale * Values.LeaderScaleFactor, 0.001f);
		}
	}));

	// A run of leaders: the fall counts for the contract, but only whoever stood near is paid.
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("a run of leaders starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		SwitchOffReinforcements(Context);
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		FEIRunValues NoCamps = Run->GetValues();
		NoCamps.CampFactor = 0.0f;
		Run->ApplyValues(NoCamps);
		OnlyContracts(Context, EEIContractKind::Leader);
		StartRun(Context);
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(WaitForPhase(TEXT("the run of leaders begins"), 3.0, EEIRunPhase::Exploring));
	Steps.Add(Do([Leaders](const FEIPlayContext& Context)
	{
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		const TArray<AEIEnemy*> Found = Leaders(Context);
		Context.Test->TestEqual(TEXT("Three side jobs, three leaders"), Found.Num(), 3);

		const FVector Far = Found[0]->GetActorLocation() + FVector(0.0f, Profile->GetTable().Rewards.RadiusMeters * CentimetersPerMeter + 1000.0f, 0.0f);
		Context.Hero->SetActorLocation(FVector(Far.X, Far.Y, 500.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Found[0]->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
		Context.Test->TestEqual(TEXT("The fall of a leader fulfils the contract"), GetState(Context)->CountContractsDone(), 1);
		Context.Test->TestEqual(TEXT("From afar it pays nothing"), Profile->GetRunGold(), 0);

		const float EnemyExperience = Found[1]->GetValues().Experience;
		const FVector Near = Found[1]->GetActorLocation() + FVector(500.0f, 0.0f, 300.0f);
		Context.Hero->SetActorLocation(Near, false, nullptr, ETeleportType::TeleportPhysics);
		const FEIContract* Contract = GetState(Context)->GetContracts().FindByPredicate([&Found](const FEIContract& Candidate) { return Candidate.Target == Found[1]; });
		Context.Test->TestNotNull(TEXT("The contract knows its leader"), Contract);
		const int32 Faction = Contract != nullptr ? Contract->Faction : 0;
		Found[1]->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
		const FEIContractTable& Table = Profile->GetContractTable();
		Context.Test->TestEqual(TEXT("Near it pays gold"), Profile->GetRunGold(), Table.GetGold());
		Context.Test->TestEqual(TEXT("Near it pays points of its faction"), Profile->GetRunPoints(Table.Factions[Faction].Name), Table.GetPoints());
		Context.Test->TestTrue(TEXT("The note names the faction"), Profile->GetRewardText().Contains(Table.Factions[Faction].Name));
		Context.Test->TestFalse(TEXT("Nothing is safe yet"), Profile->GetSave().Gold > 0);
		Context.Test->TestEqual(TEXT("Experience only comes from the enemy itself"), Profile->GetRunExperience(), FMath::RoundToInt32(EnemyExperience));
	}));

	// A run of shrines: the time runs in the zone and only there, and every shrine pays the same.
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("a run of shrines starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([StandAt](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		SwitchOffDirector(Context);
		OnlyContracts(Context, EEIContractKind::Shrine);
		const float Zone = GetGameMode(Context)->GetRun()->GetContractTable().Tasks[0].ZoneMeters;
		StartRun(Context);
		StandAt(Context, GetState(Context)->GetContracts()[0].Location, Zone * CentimetersPerMeter + 100.0f);
	}));
	Steps.Add(WaitForPhase(TEXT("the run of shrines begins"), 3.0, EEIRunPhase::Exploring));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([StandAt](const FEIPlayContext& Context)
	{
		const FEIContract& Contract = GetState(Context)->GetContracts()[0];
		const FEIContractTask& Task = GetGameMode(Context)->GetRun()->GetContractTable().Tasks[0];
		Context.Test->TestFalse(TEXT("Beside the zone nobody holds the shrine"), Contract.bHeld);
		Context.Test->TestEqual(TEXT("Beside the zone the time stands still"), Contract.HoldLeft, Task.HoldSeconds);
		StandAt(Context, Contract.Location, 200.0f);
	}));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([StandAt](const FEIPlayContext& Context)
	{
		const TArray<FEIContract>& Contracts = GetState(Context)->GetContracts();
		const FEIContractTask& Task = GetGameMode(Context)->GetRun()->GetContractTable().Tasks[0];
		Context.Test->TestTrue(TEXT("In the zone the hero holds it"), Contracts[0].bHeld);
		Context.Test->TestTrue(TEXT("Its time runs"), Contracts[0].HoldLeft < Task.HoldSeconds);
		Context.Test->TestEqual(TEXT("The other ones wait"), Contracts[1].HoldLeft, Task.HoldSeconds);
		StandAt(Context, FVector(-1500.0f, 0.0f, 0.0f), 0.0f);
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([HoldLeft](const FEIPlayContext& Context) { *HoldLeft = GetState(Context)->GetContracts()[0].HoldLeft; }));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([HoldLeft, StandAt](const FEIPlayContext& Context)
	{
		TArray<FEIContract>& Contracts = GetState(Context)->EditContracts();
		Context.Test->TestFalse(TEXT("Away from it nobody holds it"), Contracts[0].bHeld);
		Context.Test->TestEqual(TEXT("Away from it the time stands still"), Contracts[0].HoldLeft, *HoldLeft);

		// The rest of the time, shortened here.
		Contracts[0].HoldLeft = 0.2f;
		StandAt(Context, Contracts[0].Location, 200.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the shrine is done"), 2.0, [](const FEIPlayContext& Context) { return GetState(Context)->CountContractsDone() == 1; }));
	Steps.Add(Do([StandAt](const FEIPlayContext& Context)
	{
		TArray<FEIContract>& Contracts = GetState(Context)->EditContracts();
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		Context.Test->TestEqual(TEXT("The shrine pays"), Profile->GetRunGold(), Profile->GetContractTable().GetGold());
		Context.Test->TestEqual(TEXT("It gives no experience"), Profile->GetRunExperience(), 0);
		Context.Test->TestFalse(TEXT("A finished shrine is not held any more"), Contracts[0].bHeld);
		Context.Test->TestEqual(TEXT("It is broken now"), Cast<AEIAltar>(Contracts[0].Target)->GetState(), EEIAltarState::Broken);

		Contracts[1].HoldLeft = 0.1f;
		StandAt(Context, Contracts[1].Location, 200.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the next shrine is done"), 2.0, [](const FEIPlayContext& Context) { return GetState(Context)->CountContractsDone() == 2; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		const FEIContractTable& Table = Profile->GetContractTable();
		Context.Test->TestEqual(TEXT("The next shrine pays the same again"), Profile->GetRunGold(), Table.GetGold() * 2);
	}));

	// However often it is rolled: with a big distance the two contracts lie far apart.
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("a last run starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		FEIContractTable& Table = Run->GetContractTable();
		Table.ContractsPerRun = 2.0f;
		Table.MinDistanceMeters = 100.0f;
		Table.Tasks.RemoveAll([](const FEIContractTask& Task) { return Task.Kind != EEIContractKind::Altar; });
		for (int32 Roll = 0; Roll < 10; ++Roll)
		{
			Run->ResetRun();
			const TArray<FEIContract>& Contracts = GetState(Context)->GetContracts();
			Context.Test->TestEqual(TEXT("The number comes from the file"), Contracts.Num(), 2);
			Context.Test->TestTrue(TEXT("The contracts keep their distance"), FVector::Dist2D(Contracts[0].Location, Contracts[1].Location) >= Table.MinDistanceMeters * CentimetersPerMeter);
		}
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIPathfindingTest,
	"Eisenseelen.Play.Run.Pathfinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIPathfindingTest::RunTest(const FString& Parameters)
{
	// The first wall of the first place on the run map (at 95 m, 140 m): 10 m long, along X at y = 147 m.
	const FVector HeroSpot(9500.0f, 15300.0f, HeroHeight);
	const FVector EnemySpot(9500.0f, 14100.0f, HeroHeight);
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
	FEIRunTerrainTest,
	"Eisenseelen.Play.Run.Terrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIRunTerrainTest::RunTest(const FString& Parameters)
{
	// Data/gelaende.json keeps every place of the game level; around it the ground may rise and fall.
	constexpr float FlatToleranceCm = 30.0f;
	constexpr float LookAroundCm = 500.0f;
	// Altars and fires are solid, so a way ends in front of them.
	constexpr float CloseEnoughCm = 400.0f;

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		SwitchOffDirector(Context);
	}));
	Steps.Add(Wait(0.5));
	Steps.Add(WaitUntil(TEXT("the paths are built"), 60.0, [](const FEIPlayContext& Context)
	{
		const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Context.World);
		return Navigation != nullptr && Navigation->GetDefaultNavDataInstance() != nullptr && !Navigation->IsNavigationBeingBuilt(Context.World);
	}));
	Steps.Add(Do([CloseEnoughCm](const FEIPlayContext& Context)
	{
		TActorIterator<ALandscapeProxy> Landscape(Context.World);
		if (!Context.Test->TestTrue(TEXT("The run map has a landscape"), static_cast<bool>(Landscape)))
		{
			return;
		}
		const ALandscapeProxy* Ground = *Landscape;
		const auto GroundAt = [Ground](const FVector& Where, double& Height)
		{
			const TOptional<float> Found = Ground->GetHeightAtLocation(Where);
			Height = Found.Get(0.0f);
			return Found.IsSet();
		};
		double Lowest = TNumericLimits<double>::Max();
		double Highest = TNumericLimits<double>::Lowest();
		for (int32 X = -28000; X <= 28000; X += 2000)
		{
			for (int32 Y = -28000; Y <= 28000; Y += 2000)
			{
				double Height = 0.0;
				if (GroundAt(FVector(X, Y, 0.0), Height))
				{
					Lowest = FMath::Min(Lowest, Height);
					Highest = FMath::Max(Highest, Height);
				}
			}
		}
		Context.Test->AddInfo(FString::Printf(TEXT("The ground lies between %.1f m and %.1f m"), Lowest / CentimetersPerMeter, Highest / CentimetersPerMeter));
		Context.Test->TestTrue(TEXT("The sea lies below the island"), Lowest < -300.0);
		Context.Test->TestTrue(TEXT("The mountains rise above the island"), Highest > 1500.0);
		double SeaCornerHeight = 0.0;
		Context.Test->TestTrue(TEXT("The outer corner lies in the sea"), GroundAt(FVector(28000.0, 28000.0, 0.0), SeaCornerHeight) && SeaCornerHeight < -300.0);

		TArray<TPair<FString, FVector>> Places;
		const TArray<FEIArrivalPoint>& Arrivals = GetGameMode(Context)->GetRun()->GetArrivals();
		for (const FEIArrivalPoint& Arrival : Arrivals)
		{
			Places.Emplace(TEXT("arrival"), Arrival.Location);
		}
		for (TActorIterator<AEIAltar> It(Context.World); It; ++It) { Places.Emplace(TEXT("altar"), It->GetActorLocation()); }
		for (TActorIterator<AEISignalFire> It(Context.World); It; ++It) { Places.Emplace(TEXT("fire"), It->GetActorLocation()); }
		for (TActorIterator<AEIEnemyCamp> It(Context.World); It; ++It) { Places.Emplace(TEXT("camp"), It->GetActorLocation()); }
		for (TActorIterator<AEILaunchPad> It(Context.World); It; ++It) { Places.Emplace(TEXT("launch pad"), It->GetActorLocation()); }
		Context.Test->TestTrue(TEXT("The map has its places"), Places.Num() > 30);

		int32 NotFlat = 0;
		for (const TPair<FString, FVector>& Place : Places)
		{
			for (const FVector Offset : { FVector::ZeroVector, FVector(LookAroundCm, 0, 0), FVector(-LookAroundCm, 0, 0), FVector(0, LookAroundCm, 0), FVector(0, -LookAroundCm, 0) })
			{
				double Height = 0.0;
				if (!GroundAt(Place.Value + Offset, Height) || FMath::Abs(Height) > FlatToleranceCm)
				{
					Context.Test->AddError(FString::Printf(TEXT("The %s at %.0f, %.0f m is not on flat ground at height 0 (%.0f cm)"), *Place.Key, Place.Value.X / CentimetersPerMeter, Place.Value.Y / CentimetersPerMeter, Height));
					++NotFlat;
					break;
				}
			}
		}
		Context.Test->TestEqual(TEXT("Every place lies flat at height 0"), NotFlat, 0);

		// Everything a team has to reach can be reached on foot from where the barge sets it down, and no place stands in water.
		TArray<TPair<FString, FVector>> Goals = Places;
		for (TActorIterator<AEIChest> It(Context.World); It; ++It) { Goals.Emplace(TEXT("chest"), It->GetActorLocation()); }
		int32 InWater = 0;
		for (const TPair<FString, FVector>& Goal : Goals)
		{
			double Height = 0.0;
			if (!GroundAt(Goal.Value, Height) || Height < -10.0)
			{
				Context.Test->AddError(FString::Printf(TEXT("The %s at %.0f, %.0f m stands in water (%.0f cm)"), *Goal.Key, Goal.Value.X / CentimetersPerMeter, Goal.Value.Y / CentimetersPerMeter, Height));
				++InWater;
			}
		}
		Context.Test->TestEqual(TEXT("No gameplay place or chest stands in water"), InWater, 0);

		int32 BridgeDecks = 0;
		for (TActorIterator<AStaticMeshActor> It(Context.World); It; ++It)
		{
			const FVector Scale = It->GetActorScale3D();
			const UMaterialInterface* Mat = It->GetStaticMeshComponent()->GetMaterial(0);
			if (Mat != nullptr && Mat->GetName().Contains(TEXT("M_Holz")) && Scale.X >= 10.0)
			{
				++BridgeDecks;
				Goals.Emplace(TEXT("bridge"), It->GetActorLocation() + FVector(0.0, 0.0, Scale.Z * 50.0));
			}
		}
		Context.Test->TestEqual(TEXT("All 9 bridges and boardwalks stand on the island"), BridgeDecks, 9);

		UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Context.World);
		const FVector Start = Arrivals[0].Location;
		Goals.RemoveAt(0);
		int32 Unreachable = 0;
		for (const TPair<FString, FVector>& Goal : Goals)
		{
			const UNavigationPath* Path = Navigation->FindPathToLocationSynchronously(Context.World, Start, Goal.Value);
			const bool bArrives = Path != nullptr && Path->IsValid() && Path->PathPoints.Num() > 0 && FVector::Dist2D(Path->PathPoints.Last(), Goal.Value) < CloseEnoughCm;
			if (!bArrives)
			{
				Context.Test->AddError(FString::Printf(TEXT("No way on foot to the %s at %.0f, %.0f m"), *Goal.Key, Goal.Value.X / CentimetersPerMeter, Goal.Value.Y / CentimetersPerMeter));
				++Unreachable;
			}
		}
		Context.Test->AddInfo(FString::Printf(TEXT("%d goals (including %d bridges) checked for a way on foot"), Goals.Num(), BridgeDecks));
		Context.Test->TestEqual(TEXT("Every goal and bridge can be reached on foot"), Unreachable, 0);

		// The Y-shaped tunnel through Das Eisenjoch must be continuously walkable on foot and covered by a rock roof.
		const FVector TunnelNorthOutside(6400.0, 6600.0, 0.0);
		const FVector TunnelSouthOutside(-3800.0, 5800.0, 0.0);
		const FVector TunnelWestOutside(800.0, -1200.0, 0.0);
		const FVector TunnelHallCenter(1400.0, 5800.0, 0.0);
		const UNavigationPath* NorthToSouth = Navigation->FindPathToLocationSynchronously(Context.World, TunnelNorthOutside, TunnelSouthOutside);
		const bool bTunnelNS = NorthToSouth != nullptr && NorthToSouth->IsValid() && NorthToSouth->PathPoints.Num() > 0
			&& FVector::Dist2D(NorthToSouth->PathPoints.Last(), TunnelSouthOutside) < CloseEnoughCm
			&& NorthToSouth->GetPathLength() < 14000.0;
		Context.Test->TestTrue(TEXT("The tunnel is continuously walkable from north to south straight through the massif"), bTunnelNS);

		
		const UNavigationPath* WestToHall = Navigation->FindPathToLocationSynchronously(Context.World, TunnelWestOutside, TunnelHallCenter);
		if (WestToHall != nullptr)
		{
			Context.Test->AddInfo(FString::Printf(TEXT("WestToHall valid: %d, length: %.1f"), WestToHall->IsValid(), WestToHall->GetPathLength()));
		}
		
		const bool bTunnelWest = WestToHall != nullptr && WestToHall->IsValid() && WestToHall->PathPoints.Num() > 0
			&& FVector::Dist2D(WestToHall->PathPoints.Last(), TunnelHallCenter) < CloseEnoughCm
			&& WestToHall->GetPathLength() < 9000.0;
		Context.Test->TestTrue(TEXT("The west tunnel passage connects directly into the central cavern"), bTunnelWest);

		const FCollisionObjectQueryParams StaticObjects(ECC_WorldStatic);
		for (const FVector& Spot : { TunnelHallCenter, FVector(3600.0, 6150.0, 0.0), FVector(-700.0, 5800.0, 0.0), FVector(1100.0, 2500.0, 0.0) })
		{
			FHitResult RoofHit;
			const bool bHasRoof = Context.World->LineTraceSingleByObjectType(RoofHit, Spot + FVector(0.0, 0.0, 200.0), Spot + FVector(0.0, 0.0, 3000.0), StaticObjects);
			Context.Test->TestTrue(TEXT("The tunnel has a solid rock roof overhead"), bHasRoof && RoofHit.ImpactPoint.Z >= 550.0);
		}

		// The forest (pines, dead swamp trees, and bushes) must be packed into a few instanced mesh actors and leave all roads clear.
		int32 ForestActorCount = 0;
		int32 CollidingTreeInstances = 0;
		int32 NonCollidingFoliageInstances = 0;
		bool bBushesHaveCullDistance = false;
		for (TActorIterator<AEIInstancedMeshActor> It(Context.World); It; ++It)
		{
			++ForestActorCount;
			const UHierarchicalInstancedStaticMeshComponent* Comp = It->Instances;
			if (Comp == nullptr)
			{
				continue;
			}
			const int32 Count = Comp->GetInstanceCount();
			if (It->GetActorEnableCollision() && Comp->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				CollidingTreeInstances += Count;
			}
			else
			{
				NonCollidingFoliageInstances += Count;
				if (Comp->InstanceEndCullDistance > 0)
				{
					bBushesHaveCullDistance = true;
				}
			}
		}
		const double FrameMs = Context.World->GetDeltaSeconds() * 1000.0;
		Context.Test->AddInfo(FString::Printf(
			TEXT("Forest instancing: %d instanced actors holding %d colliding trunk/branch instances and %d non-colliding crown/bush instances (frame %.2f ms)"),
			ForestActorCount,
			CollidingTreeInstances,
			NonCollidingFoliageInstances,
			FrameMs));
		Context.Test->TestTrue(TEXT("The forest is packed into a small number of instanced mesh actors"), ForestActorCount >= 3 && ForestActorCount <= 8);
		Context.Test->TestTrue(TEXT("The island has hundreds of colliding tree instances"), CollidingTreeInstances >= 300);
		Context.Test->TestTrue(TEXT("The island has hundreds of non-colliding foliage instances"), NonCollidingFoliageInstances >= 400);
		Context.Test->TestTrue(TEXT("Bushes use a cull distance for performance"), bBushesHaveCullDistance);

		// Measure the island travel distances and times across Aschenwerder (North to South arrival) and along Arrival -> Side Job -> Boss -> Signal Fire.
		constexpr double WalkSpeedMetersPerSec = 5.5;
		constexpr double SprintSpeedMetersPerSec = 8.0;
		if (Arrivals.Num() >= 3)
		{
			const UNavigationPath* CrossPath = Navigation->FindPathToLocationSynchronously(Context.World, Arrivals[0].Location, Arrivals[2].Location);
			if (Context.Test->TestTrue(TEXT("A path crosses the island from north to south"), CrossPath != nullptr && CrossPath->IsValid()))
			{
				const double CrossMeters = CrossPath->GetPathLength() / CentimetersPerMeter;
				const double WalkMinutes = (CrossMeters / WalkSpeedMetersPerSec) / 60.0;
				const double SprintMinutes = (CrossMeters / SprintSpeedMetersPerSec) / 60.0;
				Context.Test->AddInfo(FString::Printf(TEXT("Island crossing (North -> South): %.0f m path (%.1f min walking, %.1f min sprinting)"), CrossMeters, WalkMinutes, SprintMinutes));
				Context.Test->TestTrue(TEXT("Crossing the island takes a substantial walk"), CrossMeters >= 380.0);
			}
		}
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		if (Arrivals.Num() >= 1 && Run->GetActiveFire() != nullptr)
		{
			const FVector BossSpot = Run->GetBossPlace();
			const FVector FireSpot = Run->GetActiveFire()->GetActorLocation();
			const UNavigationPath* ToBoss = Navigation->FindPathToLocationSynchronously(Context.World, Arrivals[0].Location, BossSpot);
			const UNavigationPath* ToFire = Navigation->FindPathToLocationSynchronously(Context.World, BossSpot, FireSpot);
			if (ToBoss != nullptr && ToBoss->IsValid() && ToFire != nullptr && ToFire->IsValid())
			{
				const double RouteMeters = (ToBoss->GetPathLength() + ToFire->GetPathLength()) / CentimetersPerMeter;
				Context.Test->AddInfo(FString::Printf(TEXT("Direct route Arrival -> Boss -> Fire: %.0f m (%.1f min walking)"), RouteMeters, (RouteMeters / WalkSpeedMetersPerSec) / 60.0));
			}
		}
		Teleport(Context, OnGround(Context, FVector(2000.0f, -14500.0f, 0.0f)), 0.0f);
	}));
	Steps.Add(Wait(0.15));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The hero stands on the wooden bridge over the stream"), Context.Movement->IsMovingOnGround() && Context.Hero->GetActorLocation().Z > HeroHeight);
		Context.Test->TestFalse(TEXT("On a bridge the hero does not wade in water"), Context.Movement->IsInShallowWater());
		Context.Test->TestNearlyEqual(TEXT("On a bridge the hero walks at full speed"), Context.Movement->GetMaxSpeed(), 550.0f, 1.0f);
		Teleport(Context, OnGround(Context, FVector(-12000.0f, 16500.0f, 0.0f)), 0.0f);
	}));
	Steps.Add(Wait(0.15));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("In the swamp pond the hero wades in shallow water"), Context.Movement->IsInShallowWater());
		Context.Test->TestNearlyEqual(TEXT("Shallow water slows the hero to 85 percent"), Context.Movement->GetMaxSpeed(), 550.0f * 0.85f, 1.0f);
		Teleport(Context, OnGround(Context, FVector(1400.0f, 5800.0f, 0.0f)), 0.0f);
	}));
	Steps.Add(Wait(0.15));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("Inside the tunnel cavern the hero stands on flat dry ground"), Context.Movement->IsMovingOnGround() && !Context.Movement->IsInShallowWater() && FMath::Abs(Context.Hero->GetActorLocation().Z - HeroHeight) < 35.0f);
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
		UEIProfileComponent* Profile = Context.Player->GetProfile();
		Profile->GrantContract(0, 100, 10);
		Profile->GrantEnemyReward(10.0f);
		Context.Hero->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
	}));
	Steps.Add(WaitForPhase(TEXT("the run is lost"), 1.0, EEIRunPhase::Failed));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		const FEIContractTable& Table = Profile->GetContractTable();
		Context.Test->TestEqual(TEXT("A share of the gold stays"), Profile->GetSave().Gold, Table.GetKeptOnFailure(100));
		Context.Test->TestEqual(TEXT("The file says 30 percent"), Table.GetKeptOnFailure(100), 30);
		Context.Test->TestEqual(TEXT("A share of the points stays"), Profile->GetSave().FactionPoints.FindRef(Table.Factions[0].Name), Table.GetKeptOnFailure(10));
		Context.Test->TestEqual(TEXT("The settlement can say so"), Profile->GetSecured(100), 30);
		Context.Test->TestTrue(TEXT("The experience stays"), Profile->GetSave().Experience > 0);
	}));
	Steps.Add(RestartNow());
	Steps.Add(WaitForPhase(TEXT("a fresh run starts"), 3.0, EEIRunPhase::AtDock));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("The fresh hero lives"), Context.Hero->IsDead());
		Context.Test->TestEqual(TEXT("What stayed is still there"), Context.Player->GetProfile()->GetSave().Gold, 30);
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEISignalFireCoopExtractionTest,
	"Eisenseelen.Play.Run.SignalFireCoopExtraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEISignalFireCoopExtractionTest::RunTest(const FString& Parameters)
{
	struct FTestState
	{
		float HoldAtStart = 0.0f;
		float HoldAfterSolo = 0.0f;
		float HoldAfterCoop = 0.0f;
		AEIHeroCharacter* SecondHero = nullptr;
	};
	const TSharedRef<FTestState> State = MakeShared<FTestState>();

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = GetGameMode(Context)->GetRun();
		FEIRunValues Fast = Run->GetValues();
		Fast.CampFactor = 0.0f;
		Fast.CastOffSeconds = 0.1f;
		Fast.CrossingSeconds = 0.1f;
		Fast.SignalHoldSeconds = 20.0f;
		Run->ApplyValues(Fast);
		SwitchOffDirector(Context);
		Context.Hero->SetCanBeDamaged(false);
		StartRun(Context);
		StandAtFire(Context, 200.0f);
	}));
	Steps.Add(WaitForPhase(TEXT("the run begins on island"), 2.0, EEIRunPhase::Exploring));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		GetState(Context)->SetBossStage(EEIBossStage::Down);
		Context.Test->TestNearlyEqual(TEXT("Signal Coop bonus is 35 percent"), GetGameMode(Context)->GetRun()->GetValues().SignalCoopSpeedBonus, 0.35f, 0.01f);
		Context.Test->TestNearlyEqual(TEXT("GameState has coop bonus"), GetState(Context)->GetSignalCoopBonus(), 0.35f, 0.01f);
		Context.Test->TestTrue(TEXT("Can light signal"), GetState(Context)->CanLightSignal());
	}));
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitForPhase(TEXT("signal fire is lit"), 1.0, EEIRunPhase::SignalLit));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIGameState* GameState = GetState(Context);
		Context.Test->TestTrue(TEXT("Fire is lit"), GameState->GetRunPhase() == EEIRunPhase::SignalLit);
		Context.Test->TestTrue(TEXT("Signal is held"), GameState->IsSignalHeld());
		Context.Test->TestEqual(TEXT("1 hero in extraction zone"), GameState->GetSignalHeldHeroes(), 1);
		State->HoldAtStart = GameState->GetSignalHoldLeft();
		Context.Test->TestNearlyEqual(TEXT("Progress begins near 0"), GameState->GetSignalHoldProgress(), 0.0f, 0.1f);
	}));
	Steps.Add(Wait(0.4));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIGameState* GameState = GetState(Context);
		State->HoldAfterSolo = GameState->GetSignalHoldLeft();
		const float SoloElapsed = State->HoldAtStart - State->HoldAfterSolo;
		Context.Test->TestTrue(TEXT("Solo hold reduced time"), SoloElapsed > 0.25f && SoloElapsed < 0.65f);

		AEISignalFire* Fire = GetGameMode(Context)->GetRun()->GetActiveFire();
		const FVector SecondPos = Fire->GetActorLocation() + FVector(-200.0f, 0.0f, HeroHeight);
		State->SecondHero = Context.World->SpawnActor<AEIHeroCharacter>(AEIHeroCharacter::StaticClass(), SecondPos, FRotator::ZeroRotator);
	}));
	Steps.Add(Wait(0.4));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIGameState* GameState = GetState(Context);
		State->HoldAfterCoop = GameState->GetSignalHoldLeft();
		Context.Test->TestEqual(TEXT("2 heroes holding zone"), GameState->GetSignalHeldHeroes(), 2);
		const float CoopElapsed = State->HoldAfterSolo - State->HoldAfterCoop;
		Context.Test->TestTrue(TEXT("Coop hold is faster than solo"), CoopElapsed > 0.45f);
		Context.Test->TestTrue(TEXT("Progress increased"), GameState->GetSignalHoldProgress() > 0.0f);

		StandAtFire(Context, 4000.0f);
		if (State->SecondHero)
		{
			State->SecondHero->SetActorLocation(Context.Hero->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIGameState* GameState = GetState(Context);
		Context.Test->TestFalse(TEXT("Nobody holds the fire outside zone"), GameState->IsSignalHeld());
		Context.Test->TestEqual(TEXT("0 heroes holding"), GameState->GetSignalHeldHeroes(), 0);
		Context.Test->TestNearlyEqual(TEXT("Timer pauses outside zone"), GameState->GetSignalHoldLeft(), State->HoldAfterCoop, 0.05f);

		if (State->SecondHero)
		{
			State->SecondHero->Destroy();
			State->SecondHero = nullptr;
		}
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEISoulCargoAndKettleOverchargeTest, "Eisenseelen.Run.SoulCargoAndKettleOvercharge", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEISoulCargoAndKettleOverchargeTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!World)
	{
		return false;
	}

	AEIGameMode* GameMode = World->SpawnActor<AEIGameMode>();
	UEIRunDirector* RunDir = GameMode ? GameMode->GetRun() : nullptr;
	TestNotNull(TEXT("RunDirector vorhanden"), RunDir);
	if (!RunDir)
	{
		World->DestroyWorld(false);
		return false;
	}

	AEISoulKettle* Kettle = World->SpawnActor<AEISoulKettle>(AEISoulKettle::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	AEIHeroCharacter* Hero = World->SpawnActor<AEIHeroCharacter>(AEIHeroCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);

	RunDir->StartHuntFromSkyBarge();
	UEIHeroMovementComponent* MoveComp = Cast<UEIHeroMovementComponent>(Hero->GetCharacterMovement());
	TestNotNull(TEXT("MovementComponent vorhanden"), MoveComp);
	if (!MoveComp)
	{
		World->DestroyWorld(false);
		return false;
	}
	MoveComp->SetMovementMode(MOVE_Walking);
	const float BaseSpeed = MoveComp->GetMaxSpeed();

	// 1. Seelenfracht aufheben -> Held muss verlangsamt sein
	const bool bPickedUp = Hero->PickUpSoulCargo(TEXT("eisen_seelenherz"));
	TestTrue(TEXT("Seelenfracht erfolgreich aufgehoben"), bPickedUp);
	TestTrue(TEXT("Held ist als Träger markiert"), Hero->IsCarryingSoulCargo());
	TestTrue(TEXT("Bewegungstempo ist während des Tragens reduziert"), MoveComp->GetMaxSpeed() < BaseSpeed);

	// 2. Seelenfracht vor dem Seelenkessel abgeben
	Hero->DropSoulCargo(false);
	TestFalse(TEXT("Held trägt nach Abgabe keine Fracht mehr"), Hero->IsCarryingSoulCargo());
	TestEqual(TEXT("Bewegungstempo ist wiederhergestellt"), MoveComp->GetMaxSpeed(), BaseSpeed);
	TestEqual(TEXT("Seelenkessel ist zu 100 % geladen"), Kettle->GetChargeProgressNormalized(), 1.0f);
	TestEqual(TEXT("RunDirector hat 2 Boss-Spuren erhalten"), RunDir->GetCurrentBossTracks(), 2);

	// 3. Seelenkessel überladen (Risk vs. Reward)
	const int32 OldTier = RunDir->GetCurrentEscalationTier();
	const bool bOvercharged = Kettle->TryOverchargeKettle(Hero);
	TestTrue(TEXT("Kessel-Überladung erfolgreich gestartet"), bOvercharged);
	TestEqual(TEXT("Eskalationsstufe ist um 1 gestiegen"), RunDir->GetCurrentEscalationTier(), OldTier + 1);

	World->DestroyWorld(false);
	return true;
}

#endif
