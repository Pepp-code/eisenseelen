#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EICasterComponent.h"
#include "EIEnemy.h"
#include "EIGearComponent.h"
#include "EIHealthComponent.h"
#include "EIProfileComponent.h"
#include "EIRuneComponent.h"
#include "EISwordComponent.h"
#include "EIRuneField.h"
#include "EITestScript.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace EITestScript;

namespace
{
	const FVector OpenFloor(-1500.0f, 0.0f, 92.0f);
	constexpr float TargetDistanceCm = 400.0f;
	constexpr float TargetSpeedMetersPerSecond = 0.2f;

	/** A tough target that hardly moves and does not strike back. */
	AEIEnemy* SpawnTarget(const FEIPlayContext& Context)
	{
		const FVector Location = Context.Hero->GetActorLocation() + Context.Hero->GetActorForwardVector() * TargetDistanceCm;
		AEIEnemy* Enemy = Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Location, FRotator(0.0f, 180.0f, 0.0f));
		FEIEnemyValues Tough = Enemy->GetValues();
		Tough.SpeedMetersPerSecond = TargetSpeedMetersPerSecond;
		Tough.AttackFromMeters = 0.1f;
		Enemy->ApplyValues(Tough);
		Enemy->GetHealth()->Reset(100000.0f);
		return Enemy;
	}

	int32 CountFields(const FEIPlayContext& Context)
	{
		int32 Count = 0;
		for (TActorIterator<AEIRuneField> It(Context.World); It; ++It)
		{
			++Count;
		}
		return Count;
	}

	void LookAtFloorBeforeHero(const FEIPlayContext& Context)
	{
		const float Pitch = -FMath::RadiansToDegrees(FMath::Atan2(156.0f, TargetDistanceCm));
		Context.Player->SetControlRotation(FRotator(Pitch, 0.0f, 0.0f));
	}

	FEIPlayStep NextRune()
	{
		return Do([](const FEIPlayContext& Context) { Context.Player->GetProfile()->EquipNext(EEISlot::Rune); });
	}

	const FEIRune& CurrentRune(const FEIPlayContext& Context)
	{
		return *Context.Hero->GetRunes()->GetRune();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIRuneTableTest,
	"Eisenseelen.Data.Runes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIRuneTableTest::RunTest(const FString& Parameters)
{
	FEIRuneTable Table;
	FString Error;
	if (!FEIRuneTable::LoadFromProjectFile(Table, Error))
	{
		AddError(Error);
	}
	TestEqual(TEXT("There are three runes"), Table.Runes.Num(), 3);

	const FString Valid = TEXT("{\"reaktion_fenster_s\": 2, \"dampfexplosion_schaden\": 50, \"dampfexplosion_radius_m\": 4, \"dampfwolke_s\": 2, \"splitterbruch_schaden\": 90, \"splitterbruch_radius_m\": 3, ")
		TEXT("\"funkenflug_schaden\": 20, \"funkenflug_radius_m\": 5, \"funkenflug_ziele\": 2, \"funkenflug_brennen_pro_s\": 4, \"funkenflug_brennen_s\": 1, \"runen\": [")
		TEXT("{\"name\": \"Proberune\", \"zauber\": \"Probe\", \"form\": \"wand\", \"element\": \"feuer\", \"schaden\": 5, \"abklingzeit_s\": 3, \"groesse_m\": 9,")
		TEXT("\"reichweite_m\": 8, \"dauer_s\": 4, \"takt_s\": 0.5, \"gefrieren_s\": 1, \"verlangsamen_0_bis_1\": 0.25, \"brennen_pro_s\": 6, \"zustand_s\": 2}]}");
	FEIRuneTable Small;
	TestTrue(TEXT("A complete line loads"), FEIRuneTable::LoadFromString(Valid, Small, Error));
	TestEqual(TEXT("The name counts"), Small.Runes[0].Name, FString(TEXT("Proberune")));
	TestTrue(TEXT("The form counts"), Small.Runes[0].bWall);
	TestEqual(TEXT("The element counts"), Small.Runes[0].Element, EEIElement::Fire);
	TestEqual(TEXT("The size counts"), Small.Runes[0].SizeMeters, 9.0f);
	TestEqual(TEXT("The freeze counts"), Small.Runes[0].FreezeSeconds, 1.0f);
	TestEqual(TEXT("The reaction counts"), Small.SteamDamage, 50.0f);
	TestEqual(TEXT("The shatter counts"), Small.ShatterDamage, 90.0f);
	TestEqual(TEXT("The sparks count"), Small.SparkDamage, 20.0f);

	FEIRuneTable Untouched;
	TestFalse(TEXT("An unknown form is refused"), FEIRuneTable::LoadFromString(Valid.Replace(TEXT("\"wand\""), TEXT("\"stern\"")), Untouched, Error));
	TestTrue(TEXT("The error names the rune"), Error.Contains(TEXT("Proberune")));
	TestFalse(TEXT("An unknown element is refused"), FEIRuneTable::LoadFromString(Valid.Replace(TEXT("\"feuer\""), TEXT("\"wasser\"")), Untouched, Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.Runes.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIRuneSpellsTest,
	"Eisenseelen.Play.Runes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIRuneSpellsTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TWeakObjectPtr<AEIEnemy> Target;
		float Life = 0.0f;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	const auto Lost = [State]() { return State->Life - State->Target->GetHealth()->GetHealth(); };

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		Teleport(Context, OpenFloor, 0.0f);
	}));
	Steps.Add(Wait(0.2));

	// The first rune: a burst around the hero that slows.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->Target = SpawnTarget(Context);
		State->Life = State->Target->GetHealth()->GetHealth();
		Context.Test->TestEqual(TEXT("The first rune is equipped"), CurrentRune(Context).Name, FString(TEXT("Frostrune")));
		Context.Test->TestEqual(TEXT("It is ready"), Context.Hero->GetRunes()->GetCooldownFraction(), 0.0f);
	}));
	AddTap(Steps, EKeys::E);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State, Lost](const FEIPlayContext& Context)
	{
		const FEIRune& Rune = CurrentRune(Context);
		Context.Test->TestNearlyEqual(TEXT("The spell on E hits"), Lost(), Rune.Damage, 0.01f);
		Context.Test->TestTrue(TEXT("It slows"), State->Target->IsSlowed());
		Context.Test->TestTrue(TEXT("It freezes"), State->Target->IsFrozen());
		const float SlowSpeed = TargetSpeedMetersPerSecond * 100.0f * (1.0f - Rune.SlowShare);
		Context.Test->TestNearlyEqual(TEXT("A slowed enemy walks slower"), State->Target->GetCharacterMovement()->MaxWalkSpeed, SlowSpeed, 0.01f);
		Context.Test->TestTrue(TEXT("Now it has to cool down"), Context.Hero->GetRunes()->GetCooldownFraction() > 0.0f);
		State->Life = State->Target->GetHealth()->GetHealth();
	}));

	// During the cooldown nothing happens.
	AddTap(Steps, EKeys::E);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State, Lost](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("During the cooldown E does nothing"), Lost(), 0.0f);
		const float Pitch = -FMath::RadiansToDegrees(FMath::Atan2(64.0f, TargetDistanceCm));
		Context.Player->SetControlRotation(FRotator(Pitch, 0.0f, 0.0f));
	}));

	// A fireball on the frost: the steam explodes on top of it.
	AddTap(Steps, EKeys::Q);
	Steps.Add(Wait(0.5));
	Steps.Add(Do([State, Lost](const FEIPlayContext& Context)
	{
		const float Expected = Context.Hero->GetCaster()->GetValues().Fireball.Damage * Context.Hero->GetGear()->GetSpellFactor() + Context.Hero->GetRunes()->GetTable().SteamDamage;
		Context.Test->AddInfo(FString::Printf(TEXT("A fireball on frost: %.0f damage, expected %.0f at 400 cm"), Lost(), Expected));
		Context.Test->TestNearlyEqual(TEXT("Fire on ice brings the steam explosion"), Lost(), Expected, 0.01f);

		// The other way round it works too, once, and only while the first element still holds.
		State->Life = State->Target->GetHealth()->GetHealth();
		UEIRuneComponent* Runes = Context.Hero->GetRunes();
		Runes->HitWithElement(State->Target.Get(), EEIElement::Fire);
		Context.Test->TestEqual(TEXT("One element alone brings nothing"), Lost(), 0.0f);
		Runes->HitWithElement(State->Target.Get(), EEIElement::Ice);
		Context.Test->TestNearlyEqual(TEXT("Ice on fire brings it too"), Lost(), Runes->GetTable().SteamDamage, 0.01f);
		State->Life = State->Target->GetHealth()->GetHealth();
		Runes->HitWithElement(State->Target.Get(), EEIElement::Fire);
	}));
	Steps.Add(Wait(2.2));
	Steps.Add(Do([State, Lost](const FEIPlayContext& Context)
	{
		Context.Hero->GetRunes()->HitWithElement(State->Target.Get(), EEIElement::Ice);
		Context.Test->TestEqual(TEXT("After the time window nothing reacts"), Lost(), 0.0f);
		LookAtFloorBeforeHero(Context);
		// Short, so that the test need not wait for it.
		Context.Hero->GetRunes()->GetTable().Runes[1].DurationSeconds = 1.3f;
		Context.Hero->GetRunes()->GetTable().Runes[2].DurationSeconds = 0.8f;
	}));

	// The next rune, with another spell, and it is ready at once. A field on the floor that hits again and again.
	Steps.Add(NextRune());
	Steps.Add(Wait(0.05));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The next rune is on"), CurrentRune(Context).Name, FString(TEXT("Sturmrune")));
		Context.Test->TestEqual(TEXT("A new rune is ready at once"), Context.Hero->GetRunes()->GetCooldownFraction(), 0.0f);
		State->Life = State->Target->GetHealth()->GetHealth();
	}));
	AddTap(Steps, EKeys::E);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The field stands"), CountFields(Context), 1);
		TActorIterator<AEIRuneField> Field(Context.World);
		const double Distance = FVector::Dist2D(Field->GetActorLocation(), Context.Hero->GetActorLocation());
		Context.Test->AddInfo(FString::Printf(TEXT("The field lands %.0f cm from the hero, where the hero looks"), Distance));
		Context.Test->TestTrue(TEXT("The field lands where the hero looks"), FMath::Abs(Distance - TargetDistanceCm) < 100.0);
	}));
	Steps.Add(Wait(0.9));
	Steps.Add(Do([Lost](const FEIPlayContext& Context)
	{
		const FEIRune& Rune = CurrentRune(Context);
		Context.Test->AddInfo(FString::Printf(TEXT("The field did %.0f damage in 1 s, one hit is %.0f"), Lost(), Rune.Damage));
		Context.Test->TestTrue(TEXT("The field hits again and again"), Lost() >= Rune.Damage * 2.0f && Lost() <= Rune.Damage * 4.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the field is over"), 2.0, [](const FEIPlayContext& Context) { return CountFields(Context) == 0; }));

	// The third rune: a wall in the way, and whoever touches it keeps burning.
	Steps.Add(NextRune());
	Steps.Add(Wait(0.05));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The third rune is on"), CurrentRune(Context).Name, FString(TEXT("Flammenrune")));
		State->Life = State->Target->GetHealth()->GetHealth();
	}));
	AddTap(Steps, EKeys::E);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State, Lost](const FEIPlayContext& Context)
	{
		TActorIterator<AEIRuneField> Field(Context.World);
		Context.Test->TestTrue(TEXT("The third spell is a wall"), Field && Field->IsWall());
		Context.Test->TestTrue(TEXT("The wall hits"), Lost() > 0.0f);
		Context.Test->TestTrue(TEXT("The wall sets on fire"), State->Target->IsBurning());
	}));
	Steps.Add(WaitUntil(TEXT("the wall is gone"), 2.0, [](const FEIPlayContext& Context) { return CountFields(Context) == 0; }));
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->Life = State->Target->GetHealth()->GetHealth(); }));
	Steps.Add(Wait(0.7));
	Steps.Add(Do([State, Lost](const FEIPlayContext& Context)
	{
		Context.Test->AddInfo(FString::Printf(TEXT("Burning without the wall: %.0f damage in 0.7 s"), Lost()));
		Context.Test->TestTrue(TEXT("It keeps burning without the wall"), Lost() > 0.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the fire goes out"), 4.0, [State](const FEIPlayContext& Context) { return !State->Target->IsBurning(); }));

	// The choice goes round, and the choice survives a restart.
	Steps.Add(NextRune());
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("After the last rune comes the first"), Context.Hero->GetRunes()->GetRuneIndex(), 0);
	}));
	Steps.Add(NextRune());
	Steps.Add(Wait(0.05));
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitUntil(TEXT("a fresh hero stands"), 3.0, [](const FEIPlayContext& Context) { return Context.Movement->IsMovingOnGround() && Context.Hero->GetRunes()->GetRune() != nullptr; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A new hero keeps the rune"), Context.Hero->GetRunes()->GetRuneIndex(), 1);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIReactionsTest,
	"Eisenseelen.Play.Reactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIReactionsTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TWeakObjectPtr<AEIEnemy> Target;
		TWeakObjectPtr<AEIEnemy> Neighbour;
		float TargetLife = 0.0f;
		float NeighbourLife = 0.0f;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	const auto Remember = [State]()
	{
		State->TargetLife = State->Target->GetHealth()->GetHealth();
		State->NeighbourLife = State->Neighbour->GetHealth()->GetHealth();
	};
	const auto TargetLost = [State]() { return State->TargetLife - State->Target->GetHealth()->GetHealth(); };
	const auto NeighbourLost = [State]() { return State->NeighbourLife - State->Neighbour->GetHealth()->GetHealth(); };

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		Teleport(Context, OpenFloor, 0.0f);
	}));
	Steps.Add(Wait(0.2));

	// Lightning and fire: sparks jump over and set the neighbours on fire.
	Steps.Add(Do([State, Remember, TargetLost, NeighbourLost](const FEIPlayContext& Context)
	{
		AEIEnemy* Target = SpawnTarget(Context);
		AEIEnemy* Neighbour = SpawnTarget(Context);
		Target->SetActorLocation(Context.Hero->GetActorLocation() + FVector(200.0f, 0.0f, 0.0f));
		Neighbour->SetActorLocation(Context.Hero->GetActorLocation() + FVector(200.0f, 200.0f, 0.0f));
		State->Target = Target;
		State->Neighbour = Neighbour;
		Remember();
		UEIRuneComponent* Runes = Context.Hero->GetRunes();
		const FEIRuneTable& Table = Runes->GetTable();
		Runes->HitWithElement(Target, EEIElement::Lightning);
		Context.Test->TestEqual(TEXT("Lightning alone sets off nothing"), TargetLost(), 0.0f);
		Runes->HitWithElement(Target, EEIElement::Fire);
		Context.Test->TestEqual(TEXT("Sparks hurt the target"), TargetLost(), Table.SparkDamage);
		Context.Test->TestEqual(TEXT("Sparks jump over"), NeighbourLost(), Table.SparkDamage);
		Context.Test->TestTrue(TEXT("Sparks set the target on fire"), Target->IsBurning());
		Context.Test->TestTrue(TEXT("Sparks set the neighbour on fire"), Neighbour->IsBurning());

		// Lightning on the frozen: it breaks, and the splinters reach the neighbour.
		Remember();
		Target->Freeze(5.0f);
		Context.Test->TestTrue(TEXT("It is frozen"), Target->IsFrozen());
		Runes->HitWithElement(Target, EEIElement::Lightning);
		Context.Test->TestEqual(TEXT("Lightning breaks the frozen"), TargetLost(), Table.ShatterDamage);
		Context.Test->TestEqual(TEXT("The splinters hit the neighbour"), NeighbourLost(), Table.ShatterDamage);
		Context.Test->TestFalse(TEXT("Whoever broke is no longer frozen"), Target->IsFrozen());
		Remember();
	}));
	Steps.Add(WaitUntil(TEXT("the fire goes out"), 5.0, [State](const FEIPlayContext& Context) { return !State->Target->IsBurning(); }));

	// The way of the blade: E freezes, the heavy hit breaks.
	Steps.Add(Do([Remember](const FEIPlayContext& Context)
	{
		Remember();
		Context.Test->TestEqual(TEXT("The frost rune is equipped"), CurrentRune(Context).Name, FString(TEXT("Frostrune")));
	}));
	AddTap(Steps, EKeys::E);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([State, Remember](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The frost freezes the target"), State->Target->IsFrozen());
		Remember();
	}));
	Steps.Add(Press(EKeys::LeftMouseButton));
	Steps.Add(Wait(0.7));
	Steps.Add(Release(EKeys::LeftMouseButton));
	Steps.Add(WaitUntil(TEXT("the heavy swing is over"), 3.0, [](const FEIPlayContext& Context) { return !Context.Hero->GetSword()->IsSwinging(); }));
	Steps.Add(Do([State, TargetLost, NeighbourLost](const FEIPlayContext& Context)
	{
		const float Expected = Context.Hero->GetSword()->GetValues().Heavy.Damage + Context.Hero->GetRunes()->GetTable().ShatterDamage;
		Context.Test->AddInfo(FString::Printf(TEXT("A heavy hit on the frozen at 200 cm: %.0f damage, expected %.0f"), TargetLost(), Expected));
		Context.Test->TestNearlyEqual(TEXT("A heavy hit breaks the frozen"), TargetLost(), Expected, 0.01f);
		Context.Test->TestFalse(TEXT("It is thawed now"), State->Target->IsFrozen());
		State->Target->Destroy();
		State->Neighbour->Destroy();
	}));
	Steps.Add(Wait(0.1));

	// Whoever is frozen does not strike, and the frost melts by itself.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIEnemy* Runner = Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Context.Hero->GetActorLocation() + FVector(130.0f, 0.0f, 0.0f), FRotator(0.0f, 180.0f, 0.0f));
		Runner->GetHealth()->Reset(100000.0f);
		Runner->Freeze(1.0f);
		State->Target = Runner;
	}));
	Steps.Add(Wait(0.7));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("It is still frozen"), State->Target->IsFrozen());
		Context.Test->TestFalse(TEXT("Whoever is frozen does not strike"), State->Target->IsTelegraphing());
	}));
	Steps.Add(WaitUntil(TEXT("it thaws and winds up"), 2.0, [State](const FEIPlayContext& Context) { return !State->Target->IsFrozen() && State->Target->IsTelegraphing(); }));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
