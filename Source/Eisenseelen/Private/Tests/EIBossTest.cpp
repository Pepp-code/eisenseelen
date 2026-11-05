#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EIAnchorStone.h"
#include "EIBoss.h"
#include "EIBosses.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHordeDirector.h"
#include "EIProfileComponent.h"
#include "EIRunDirector.h"
#include "EISignalFire.h"
#include "EISkyBarge.h"
#include "EITestScript.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"

using namespace EITestScript;

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float HeroHeight = 92.0f;
	constexpr float ShortHoldSeconds = 0.5f;

	UEIRunDirector* GetRun(const FEIPlayContext& Context)
	{
		return Context.World->GetAuthGameMode<AEIGameMode>()->GetRun();
	}

	const AEIGameState* GetState(const FEIPlayContext& Context)
	{
		return Context.World->GetGameState<AEIGameState>();
	}

	AEIBoss* GetBoss(const FEIPlayContext& Context)
	{
		return GetRun(Context)->GetBoss();
	}

	void StandBeside(const FEIPlayContext& Context, const AActor* Actor, float DistanceCm)
	{
		const FVector Location = Actor->GetActorLocation();
		Teleport(Context, FVector(Location.X + DistanceCm, Location.Y, HeroHeight), 180.0f);
	}

	/** A run with nothing but the boss, with short holds. It only works at the dock. */
	void PrepareBossRun(const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = GetRun(Context);
		FEIRunValues Values = Run->GetValues();
		Values.CampFactor = 0.0f;
		Values.SignalHoldSeconds = ShortHoldSeconds;
		Run->ApplyValues(Values);
		Run->GetContractTable().ContractsPerRun = 0.0f;
		// Nobody else comes until the boss is down, or a stray bone would count as its blow.
		Run->GetGroups().AltarWave.Size.Empty();
		Run->GetGroups().StoneWave.Size.Empty();
		Run->GetGroups().FightWave.Size.Empty();
		for (FEIBossValues& Boss : Run->GetBossTable().Bosses)
		{
			Boss.Bannbruch.HoldSeconds = ShortHoldSeconds;
		}
		Run->ResetRun();
	}

	FEIPlayStep WaitForPaths()
	{
		return WaitUntil(TEXT("the paths are built"), 30.0, [](const FEIPlayContext& Context)
		{
			const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Context.World);
			return Navigation != nullptr && Navigation->GetDefaultNavDataInstance() != nullptr && !Navigation->IsNavigationBeingBuilt(Context.World);
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIBossDataTest,
	"Eisenseelen.Data.Bosses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIBossDataTest::RunTest(const FString& Parameters)
{
	FString Error;

	// The file of the game.
	FEIBossTable Table;
	if (!FEIBossTable::LoadFromProjectFile(Table, Error))
	{
		AddError(Error);
		return true;
	}
	TestTrue(TEXT("There is a boss"), !Table.Bosses.IsEmpty());
	for (const FEIBossValues& Boss : Table.Bosses)
	{
		TestTrue(TEXT("A boss has a loot set"), !Boss.Loot.IsEmpty());
		TestTrue(TEXT("A boss has its Bannbruch"), Boss.Bannbruch.Count > 0);
		TestEqual(TEXT("A boss does not count as a threat for the groups"), Boss.Fight.Threat, 0.0f);
		// What the file promises is what the dice do.
		TMap<FString, int32> Counts;
		constexpr int32 Rolls = 3000;
		for (int32 Roll = 0; Roll < Rolls; ++Roll)
		{
			for (const FString& Name : Boss.RollLoot())
			{
				Counts.FindOrAdd(Name) += 1;
			}
		}
		for (const FEILootItem& Item : Boss.Loot)
		{
			const float Share = static_cast<float>(Counts.FindRef(Item.Name)) / Rolls;
			AddInfo(FString::Printf(TEXT("%s: %.1f %% of the rolls, %.1f %% in the file"), *Item.Name, Share * 100.0f, Item.Chance * 100.0f));
			TestTrue(TEXT("Rare things are rare"), Item.Chance < 1.0f);
			TestNearlyEqual(TEXT("A drop comes as often as the file says"), Share, Item.Chance, 0.05f);
		}
	}

	// A small table, to see that the rules are kept.
	const FString Fight = TEXT("{\"leben\": 100, \"bedrohung\": 0, \"tempo_m_pro_s\": 1, \"schaden\": 1, \"angriff_ab_abstand_m\": 1, \"ausholen_s\": 1, \"ausholen_folgen_s\": 0, \"trefferzone_m\": 1, \"trefferbreite_m\": 1, \"erholung_s\": 1, ")
		TEXT("\"taumeln_faktor_schwerer_hieb_0_bis_1\": 0, \"sicht_m\": 1, \"alarm_umkreis_m\": 0, \"aufgeben_m\": 1, \"wach_mindestens_s\": 1, \"groesse_faktor\": 2, \"drehtempo_grad_pro_s\": 1, ")
		TEXT("\"schild_schaden_faktor_0_bis_1\": 1, \"schild_winkel_grad\": 1, \"wurf_tempo_m_pro_s\": 0, \"erfahrung\": 7, \"item_chance_0_bis_1\": 0}");
	const FString Valid = FString(TEXT("{\"platz_leine_m\": 32, \"erholung_anteil_pro_s\": 0.1, \"leben_je_weiterem_spieler\": 0.5, \"kampf_welle_alle_s\": 20, \"verfolgung_welle_alle_s\": 12, \"signalfeuer_mindestabstand_m\": 100, ")
		TEXT("\"bosse\": [{\"name\": \"Probe\", \"auftritt\": \"Los!\", \"farbe\": [1, 0, 0], ")
		TEXT("\"bannbruch\": {\"art\": \"ankersteine\", \"name\": \"Steine\", \"teil\": \"Stein\", \"anleitung\": \"Halten\", \"anzahl\": 2, \"halten_s\": 5, \"zone_m\": 3, \"abstand_m\": 9}, \"werte\": ")) + Fight +
		TEXT(", \"stampfen\": {\"radius_m\": 4, \"ab_abstand_m\": 3, \"ankuendigung_s\": 1, \"schaden\": 2, \"pause_s\": 6}, ")
		TEXT("\"beute\": {\"gold_von\": 10, \"gold_bis\": 20, \"zutaten\": [{\"name\": \"Immer\", \"chance\": 1}, {\"name\": \"Nie\", \"chance\": 0}]}}]}");
	FEIBossTable Small;
	TestTrue(TEXT("A complete table loads"), FEIBossTable::LoadFromString(Valid, Small, Error));
	if (Small.Bosses.IsEmpty())
	{
		AddError(Error);
		return true;
	}
	const FEIBossValues& Boss = Small.Bosses[0];
	TestEqual(TEXT("The boss has its name"), Boss.Name, FString(TEXT("Probe")));
	TestEqual(TEXT("It has its numbers for the fight"), Boss.Fight.Experience, 7.0f);
	TestEqual(TEXT("It has its stones"), Boss.Bannbruch.Count, 2);
	TestEqual(TEXT("It has its stomp"), Boss.Stomp.RadiusMeters, 4.0f);
	TestEqual(TEXT("The leash counts"), Small.LeashMeters, 32.0f);

	// What it drops.
	for (int32 Roll = 0; Roll < 50; ++Roll)
	{
		const int32 Gold = Boss.RollGold();
		TestTrue(TEXT("The gold lies in its range"), Gold >= 10 && Gold <= 20);
		const TArray<FString> Loot = Boss.RollLoot();
		TestTrue(TEXT("What always drops, drops"), Loot.Contains(TEXT("Immer")));
		TestFalse(TEXT("What never drops, does not"), Loot.Contains(TEXT("Nie")));
	}

	// A chance really is a chance.
	FEIBossValues Even = Boss;
	Even.Loot[0].Chance = 0.5f;
	int32 Found = 0;
	for (int32 Roll = 0; Roll < 200; ++Roll)
	{
		Found += Even.RollLoot().Num();
	}
	TestTrue(TEXT("Half a chance is about half of the time"), Found > 50 && Found < 150);

	// What must not pass.
	FEIBossTable Untouched;
	TestFalse(TEXT("A Bannbruch nobody knows is refused"), FEIBossTable::LoadFromString(Valid.Replace(TEXT("\"art\": \"ankersteine\""), TEXT("\"art\": \"tanzen\"")), Untouched, Error));
	TestTrue(TEXT("The error names it"), Error.Contains(TEXT("tanzen")));
	TestFalse(TEXT("A chance above one is refused"), FEIBossTable::LoadFromString(Valid.Replace(TEXT("\"chance\": 0}"), TEXT("\"chance\": 3}")), Untouched, Error));
	TestFalse(TEXT("Gold has to go upwards"), FEIBossTable::LoadFromString(Valid.Replace(TEXT("\"gold_bis\": 20"), TEXT("\"gold_bis\": 5")), Untouched, Error));
	TestFalse(TEXT("A boss needs its loot"), FEIBossTable::LoadFromString(Valid.Replace(TEXT("\"beute\""), TEXT("\"schatz\"")), Untouched, Error));
	TestFalse(TEXT("Broken text is refused"), FEIBossTable::LoadFromString(TEXT("{ bosse"), Untouched, Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.Bosses.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIBossRunTest,
	"Eisenseelen.Play.Run.Boss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIBossRunTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		float HoldLeft = 0.0f;
		float LifeBefore = 0.0f;
		int32 ExperienceBefore = 0;
	};
	const TSharedRef<FState> State = MakeShared<FState>();

	TArray<FEIPlayStep> Steps;
	Steps.Add(WaitForPaths());

	// At the dock the boss is chosen, but nobody stands there yet.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		PrepareBossRun(Context);
		UEIRunDirector* Run = GetRun(Context);
		Context.Test->TestNull(TEXT("At the dock no boss stands yet"), GetBoss(Context));
		Context.Test->TestEqual(TEXT("It is sealed"), GetState(Context)->GetBossStage(), EEIBossStage::Sealed);
		Context.Test->TestEqual(TEXT("Its Bannbruch has three parts"), GetState(Context)->GetBannbruchTotal(), 3);
		const double FromBoss = FVector::Dist2D(Run->GetActiveFire()->GetActorLocation(), Run->GetBossPlace());
		Context.Test->AddInfo(FString::Printf(TEXT("The fire stands %.0f m from the boss"), FromBoss / CentimetersPerMeter));
		Context.Test->TestTrue(TEXT("The fire stands far from the boss"), FromBoss >= Run->GetBossTable().FireMinDistanceMeters * CentimetersPerMeter);
		Run->BeginRunNow();
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(Wait(0.2));

	// There it stands, bound, and nothing touches it.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = GetRun(Context);
		AEIBoss* Boss = GetBoss(Context);
		if (Boss == nullptr)
		{
			Context.Test->AddError(TEXT("The boss is missing"));
			return;
		}
		Context.Test->TestTrue(TEXT("The boss is bound"), Boss->IsBound());
		Context.Test->TestTrue(TEXT("It stands at its place"), FVector::Dist2D(Boss->GetActorLocation(), Run->GetBossPlace()) < 200.0);
		Context.Test->TestEqual(TEXT("It is big"), static_cast<float>(Boss->GetActorScale3D().X), Boss->GetBossValues()->Fight.Scale);
		const float Life = Boss->GetHealth()->GetHealth();
		Context.Test->TestEqual(TEXT("It has its life"), Life, Boss->GetBossValues()->Fight.Life);
		Boss->TakeDamage(1000.0f, FDamageEvent(), nullptr, Context.Hero);
		Boss->Freeze(5.0f);
		Boss->Stagger(5.0f);
		Context.Test->TestEqual(TEXT("A bound boss takes no damage"), Boss->GetHealth()->GetHealth(), Life);
		Context.Test->TestFalse(TEXT("A bound boss does not freeze"), Boss->IsFrozen());
		Context.Test->TestFalse(TEXT("A bound boss does not stagger"), Boss->IsStaggering());
		Context.Test->TestEqual(TEXT("It does not count among the enemies"), Context.World->GetAuthGameMode<AEIGameMode>()->GetHorde()->CountEnemies(), 0);

		const TArray<TObjectPtr<AEIAnchorStone>>& Anchors = Run->GetAnchors();
		Context.Test->TestEqual(TEXT("Three stones hold it"), Anchors.Num(), 3);
		const float Distance = Boss->GetBossValues()->Bannbruch.DistanceMeters * CentimetersPerMeter;
		for (const AEIAnchorStone* Anchor : Anchors)
		{
			Context.Test->TestNearlyEqual(TEXT("A stone stands around the boss"), static_cast<float>(FVector::Dist2D(Anchor->GetActorLocation(), Run->GetBossPlace())), Distance, 1.0f);
			Context.Test->TestFalse(TEXT("A stone is whole"), Anchor->IsBroken());
		}

		// The fire stays cold as long as the boss lives.
		Context.Test->TestFalse(TEXT("The fire cannot burn yet"), GetState(Context)->CanLightSignal());
		StandBeside(Context, Run->GetActiveFire(), 200.0f);
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = GetRun(Context);
		Context.Test->TestFalse(TEXT("Before the boss the fire stays cold"), Run->GetActiveFire()->IsLit());
		Context.Test->TestEqual(TEXT("And the run goes on"), GetState(Context)->GetRunPhase(), EEIRunPhase::Exploring);
		Context.Test->TestTrue(TEXT("The note says why"), Context.Player->GetProfile()->GetRewardText().Contains(TEXT("Boss")));
		StandBeside(Context, Run->GetAnchors()[0], 200.0f);
	}));

	// In the zone of a stone the time runs, and only there.
	Steps.Add(Wait(0.15));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const TArray<TObjectPtr<AEIAnchorStone>>& Anchors = GetRun(Context)->GetAnchors();
		Context.Test->TestTrue(TEXT("In the zone the hero holds the stone"), Anchors[0]->IsHeld());
		Context.Test->TestTrue(TEXT("Its time runs"), Anchors[0]->GetHoldLeft() < ShortHoldSeconds);
		Context.Test->TestEqual(TEXT("The others wait"), Anchors[1]->GetHoldLeft(), ShortHoldSeconds);
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->HoldLeft = GetRun(Context)->GetAnchors()[0]->GetHoldLeft(); }));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const AEIAnchorStone* Anchor = GetRun(Context)->GetAnchors()[0];
		Context.Test->TestFalse(TEXT("Away from it nobody holds it"), Anchor->IsHeld());
		Context.Test->TestEqual(TEXT("Away from it the time stands still"), Anchor->GetHoldLeft(), State->HoldLeft);
		StandBeside(Context, Anchor, 200.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the first stone breaks"), 3.0, [](const FEIPlayContext& Context) { return GetRun(Context)->GetAnchors()[0]->IsBroken(); }));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("One of three is done"), GetState(Context)->GetBannbruchDone(), 1);
		Context.Test->TestFalse(TEXT("A broken stone blocks nobody"), GetRun(Context)->GetAnchors()[0]->GetActorEnableCollision());
		Context.Test->TestTrue(TEXT("A whole stone stands in the way"), GetRun(Context)->GetAnchors()[1]->GetActorEnableCollision());
		Context.Test->TestTrue(TEXT("Two chains still hold it"), GetBoss(Context)->IsBound());
		StandBeside(Context, GetRun(Context)->GetAnchors()[1], 200.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the second stone breaks"), 3.0, [](const FEIPlayContext& Context) { return GetRun(Context)->GetAnchors()[1]->IsBroken(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->ExperienceBefore = Context.Player->GetProfile()->GetRunExperience();
		StandBeside(Context, GetRun(Context)->GetAnchors()[2], 200.0f);
	}));

	// With the last one it is free.
	Steps.Add(WaitUntil(TEXT("the boss breaks free"), 3.0, [](const FEIPlayContext& Context) { return !GetBoss(Context)->IsBound(); }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		AEIBoss* Boss = GetBoss(Context);
		Context.Test->TestEqual(TEXT("The fight is on"), GetState(Context)->GetBossStage(), EEIBossStage::Awake);
		Context.Test->TestEqual(TEXT("The screen says so"), Context.Player->GetProfile()->GetRewardText(), Boss->GetBossValues()->Entrance);
		Context.Test->TestFalse(TEXT("The fire still cannot burn"), GetState(Context)->CanLightSignal());
		Context.Test->TestFalse(TEXT("It has noticed the hero"), Boss->IsWatching());
		const float Life = Boss->GetHealth()->GetHealth();
		Context.Test->TestEqual(TEXT("Alone it has its plain life"), Life, Boss->GetBossValues()->Fight.Life);
		Boss->TakeDamage(100.0f, FDamageEvent(), nullptr, Context.Hero);
		Context.Test->TestEqual(TEXT("Now it can be hurt"), Boss->GetHealth()->GetHealth(), Life - 100.0f);
		Boss->Freeze(5.0f);
		Context.Test->TestFalse(TEXT("A boss does not freeze"), Boss->IsFrozen());
	}));

	// It stomps once somebody stands close by. Whoever is in the ring is hit.
	Steps.Add(WaitUntil(TEXT("the boss stomps"), 15.0, [](const FEIPlayContext& Context)
	{
		StandBeside(Context, GetBoss(Context), GetBoss(Context)->GetBossValues()->Stomp.FromMeters * CentimetersPerMeter - 100.0f);
		return GetBoss(Context)->IsStompMarked();
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("It shows what comes"), GetBoss(Context)->IsTelegraphing());
		Context.Hero->SetCanBeDamaged(true);
		State->LifeBefore = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(WaitUntil(TEXT("the stomp lands"), 5.0, [](const FEIPlayContext& Context) { return !GetBoss(Context)->IsStompMarked(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const float Lost = State->LifeBefore - Context.Hero->GetHealth()->GetHealth();
		Context.Test->AddInfo(FString::Printf(TEXT("The stomp takes %.0f life"), Lost));
		Context.Test->TestEqual(TEXT("The stomp hits"), Lost, GetBoss(Context)->GetBossValues()->Stomp.Damage);
		Context.Test->TestTrue(TEXT("The screen says what it was"), Context.Hero->GetLastHitNote().Contains(TEXT("Der Gefesselte: Stampfen")));
		Context.Hero->SetCanBeDamaged(false);

		// Its charge has its own test, and here it would only take it somewhere else.
		AEIBoss* Boss = GetBoss(Context);
		FEIEnemyValues NoCharge = Boss->GetValues();
		NoCharge.ChargeSpeedMetersPerSecond = 0.0f;
		Boss->ApplyValues(NoCharge);
	}));

	// Whoever has left the ring by then is not hit, close as it may be. On the way towards its place, or it would leave it.
	Steps.Add(WaitUntil(TEXT("the boss stomps again"), 15.0, [](const FEIPlayContext& Context)
	{
		const AEIBoss* Boss = GetBoss(Context);
		const FVector Spot = Boss->GetActorLocation();
		const FVector Towards = (GetRun(Context)->GetBossPlace() - Spot).GetSafeNormal2D();
		const FVector Way = Towards.IsNearlyZero() ? FVector::ForwardVector : Towards;
		const FVector Stand = Spot + Way * (Boss->GetBossValues()->Stomp.FromMeters * CentimetersPerMeter - 100.0f);
		Teleport(Context, FVector(Stand.X, Stand.Y, HeroHeight), 0.0f);
		return Boss->IsStompMarked();
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const AEIBoss* Boss = GetBoss(Context);
		Context.Test->TestEqual(TEXT("The ring is as wide as the file says"), Boss->GetGroundMark().LengthCm, Boss->GetBossValues()->Stomp.RadiusMeters * CentimetersPerMeter * 2.0f);
		Context.Hero->SetCanBeDamaged(true);
		State->LifeBefore = Context.Hero->GetHealth()->GetHealth();
		StandBeside(Context, Boss, Boss->GetBossValues()->Stomp.RadiusMeters * CentimetersPerMeter + 60.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the second stomp lands"), 5.0, [](const FEIPlayContext& Context) { return !GetBoss(Context)->IsStompMarked(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("Outside the ring nothing happens"), Context.Hero->GetHealth()->GetHealth() >= State->LifeBefore);
		Context.Hero->SetCanBeDamaged(false);

		// Far from its place it turns back, and gets its life back on the way.
		AEIBoss* Boss = GetBoss(Context);
		const float Leash = GetRun(Context)->GetBossTable().LeashMeters * CentimetersPerMeter;
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
		const FVector Place = GetRun(Context)->GetBossPlace();
		Boss->SetActorLocation(FVector(Place.X + Leash + 300.0f, Place.Y, Boss->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
	}));
	Steps.Add(WaitUntil(TEXT("the boss turns back"), 3.0, [](const FEIPlayContext& Context) { return GetBoss(Context)->IsWatching(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->LifeBefore = GetBoss(Context)->GetHealth()->GetHealth(); }));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const AEIBoss* Boss = GetBoss(Context);
		Context.Test->TestTrue(TEXT("On its way back it recovers"), Boss->GetHealth()->GetHealth() > State->LifeBefore);
		Context.Test->TestTrue(TEXT("It walks towards its place"), FVector::Dist2D(Boss->GetActorLocation(), GetRun(Context)->GetBossPlace()) <= GetRun(Context)->GetBossTable().LeashMeters * CentimetersPerMeter + 300.0);
	}));

	// It falls: gold and what the dice say, for whoever is there, and nothing of it is safe yet.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		TArray<FEILootItem>& Loot = GetRun(Context)->GetBossTable().Bosses[0].Loot;
		for (FEILootItem& Item : Loot)
		{
			Item.Chance = 0.0f;
		}
		Loot[0].Chance = 1.0f;
		AEIBoss* Boss = GetBoss(Context);
		Context.Hero->SetActorLocation(Boss->GetActorLocation() + FVector(800.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
		State->ExperienceBefore = Context.Player->GetProfile()->GetRunExperience();
		Boss->TakeDamage(100000000.0f, FDamageEvent(), nullptr, nullptr);

		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		const FEIBossValues& Values = GetRun(Context)->GetBossTable().Bosses[0];
		Context.Test->TestEqual(TEXT("The boss is down"), GetState(Context)->GetBossStage(), EEIBossStage::Down);
		Context.Test->TestTrue(TEXT("Now the fire can burn"), GetState(Context)->CanLightSignal());
		Context.Test->TestTrue(TEXT("The boss drops gold"), Profile->GetRunGold() >= FMath::RoundToInt32(Values.GoldFrom) && Profile->GetRunGold() <= FMath::RoundToInt32(Values.GoldTo));
		Context.Test->TestEqual(TEXT("It drops what the dice say"), Profile->GetRunIngredients().Num(), 1);
		Context.Test->TestEqual(TEXT("From its own set"), Profile->GetRunIngredients()[0], Values.Loot[0].Name);
		Context.Test->TestTrue(TEXT("The screen says so"), Profile->GetRewardText().Contains(Values.Name) && Profile->GetRewardText().Contains(Values.Loot[0].Name));
		Context.Test->TestEqual(TEXT("Nothing is safe yet"), Profile->GetSave().Ingredients.Num(), 0);
		Context.Test->TestEqual(TEXT("The boss brings its experience"), Profile->GetRunExperience() - State->ExperienceBefore, FMath::RoundToInt32(Values.Fight.Experience));
	}));

	// Now they come for the hero, wherever the hero runs.
	Steps.Add(WaitUntil(TEXT("the pursuit begins"), 3.0, [](const FEIPlayContext& Context)
	{
		return Context.World->GetAuthGameMode<AEIGameMode>()->GetHorde()->CountThreat() > 0.0f;
	}));

	// The fire, the barge, and home with it.
	Steps.Add(Do([](const FEIPlayContext& Context) { StandBeside(Context, GetRun(Context)->GetActiveFire(), 200.0f); }));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitUntil(TEXT("the barge is there"), 5.0, [](const FEIPlayContext& Context) { return GetState(Context)->GetRunPhase() == EEIRunPhase::BargeWaiting; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetActorLocation(GetRun(Context)->GetBarge()->GetActorLocation() + FVector(0.0f, 0.0f, -40.0f), false, nullptr, ETeleportType::TeleportPhysics);
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::F);
	Steps.Add(WaitUntil(TEXT("the hero gets away"), 2.0, [](const FEIPlayContext& Context) { return GetState(Context)->GetRunPhase() == EEIRunPhase::Escaped; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		const FString Name = GetRun(Context)->GetBossTable().Bosses[0].Loot[0].Name;
		Context.Test->TestEqual(TEXT("In the barge the ingredient is safe"), Profile->GetSave().Ingredients.FindRef(Name), 1);
		Context.Test->TestEqual(TEXT("In the barge the gold is safe"), Profile->GetSave().Gold, Profile->GetRunGold());
	}));

	// The next run has its boss anew, and this time it fails: the gold shrinks, and the ingredient is gone.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitUntil(TEXT("a fresh dock"), 3.0, [](const FEIPlayContext& Context) { return GetState(Context)->GetRunPhase() == EEIRunPhase::AtDock; }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A fresh boss is sealed again"), GetState(Context)->GetBossStage(), EEIBossStage::Sealed);
		Context.Test->TestEqual(TEXT("Its stones are gone"), GetRun(Context)->GetAnchors().Num(), 0);
		Context.Test->TestNull(TEXT("Nobody stands at the dock"), GetBoss(Context));
		PrepareBossRun(Context);
		TArray<FEILootItem>& Loot = GetRun(Context)->GetBossTable().Bosses[0].Loot;
		for (FEILootItem& Item : Loot)
		{
			Item.Chance = 1.0f;
		}
		GetRun(Context)->BeginRunNow();
		GetRun(Context)->SkipBannbruch();
		Context.Test->TestFalse(TEXT("Without the Bannbruch it is free at once"), GetBoss(Context)->IsBound());
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->ExperienceBefore = Context.Player->GetProfile()->GetSave().Gold;
		GetBoss(Context)->TakeDamage(100000000.0f, FDamageEvent(), nullptr, nullptr);
		Context.Test->TestEqual(TEXT("This boss drops all of its set"), Context.Player->GetProfile()->GetRunIngredients().Num(), GetRun(Context)->GetBossTable().Bosses[0].Loot.Num());
		Context.Hero->SetCanBeDamaged(true);
		Context.Hero->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
	}));
	Steps.Add(WaitUntil(TEXT("the run is lost"), 2.0, [](const FEIPlayContext& Context) { return GetState(Context)->GetRunPhase() == EEIRunPhase::Failed; }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const UEIProfileComponent* Profile = Context.Player->GetProfile();
		const FEIContractTable& Contracts = Profile->GetContractTable();
		const TArray<FEILootItem>& Loot = GetRun(Context)->GetBossTable().Bosses[0].Loot;
		Context.Test->TestEqual(TEXT("A share of the gold stays"), Profile->GetSave().Gold, State->ExperienceBefore + Contracts.GetKeptOnFailure(Profile->GetRunGold()));
		Context.Test->TestEqual(TEXT("The first ingredient is as many as before"), Profile->GetSave().Ingredients.FindRef(Loot[0].Name), 1);
		Context.Test->TestEqual(TEXT("All others are gone"), Profile->GetSave().Ingredients.Num(), 1);
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIBossFightWavesTest,
	"Eisenseelen.Play.Run.BossWaves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIBossFightWavesTest::RunTest(const FString& Parameters)
{
	const auto Threat = [](const FEIPlayContext& Context) { return Context.World->GetAuthGameMode<AEIGameMode>()->GetHorde()->CountThreat(); };

	TArray<FEIPlayStep> Steps;
	Steps.Add(WaitForPaths());
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		UEIRunDirector* Run = GetRun(Context);
		FEIRunValues Values = Run->GetValues();
		Values.CampFactor = 0.0f;
		Run->ApplyValues(Values);
		Run->GetContractTable().ContractsPerRun = 0.0f;
		for (FEIBossValues& Boss : Run->GetBossTable().Bosses)
		{
			Boss.Bannbruch.HoldSeconds = 100.0f;
		}
		Run->ResetRun();
		Run->BeginRunNow();
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([Threat](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("As long as nobody holds a stone nobody comes"), Threat(Context), 0.0f);
		StandBeside(Context, GetRun(Context)->GetAnchors()[0], 200.0f);
	}));
	Steps.Add(WaitUntil(TEXT("a wave comes to the stone"), 2.0, [Threat](const FEIPlayContext& Context) { return Threat(Context) > 0.0f; }));
	Steps.Add(Do([Threat](const FEIPlayContext& Context)
	{
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			if (!It->IsBoss())
			{
				It->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
			}
		}
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
	}));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([Threat](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Away from the stone nobody comes"), Threat(Context), 0.0f);
		GetRun(Context)->SkipBannbruch();
	}));

	// In the fight they come too.
	Steps.Add(WaitUntil(TEXT("a wave comes to the fight"), 2.0, [Threat](const FEIPlayContext& Context) { return Threat(Context) > 0.0f; }));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

#endif
