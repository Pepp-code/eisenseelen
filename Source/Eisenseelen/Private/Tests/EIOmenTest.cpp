#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EICasterComponent.h"
#include "EIGearComponent.h"
#include "EIHealthComponent.h"
#include "EIOmenComponent.h"
#include "EIProjectile.h"
#include "EISwordComponent.h"
#include "EITestScript.h"
#include "EITrainingDummy.h"
#include "EngineUtils.h"

using namespace EITestScript;

namespace
{
	constexpr float HeroHeight = 92.0f;
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float SpeedToleranceCm = 40.0f;
	const FVector OpenFloor(-500.0f, 0.0f, HeroHeight);
	constexpr float BackwardsYaw = 180.0f;

	const FEIOmen* FindOmen(const FEIPlayContext& Context, EEIStat Effect, int32* OutIndex = nullptr)
	{
		const TArray<FEIOmen>& Omens = Context.Hero->GetOmens()->GetTable().Omens;
		for (int32 Index = 0; Index < Omens.Num(); ++Index)
		{
			if (Omens[Index].Effect == Effect)
			{
				if (OutIndex != nullptr)
				{
					*OutIndex = Index;
				}
				return &Omens[Index];
			}
		}
		return nullptr;
	}

	void GiveOmen(const FEIPlayContext& Context, EEIStat Effect)
	{
		int32 Index = INDEX_NONE;
		FindOmen(Context, Effect, &Index);
		Context.Hero->GetOmens()->AddOmen(Index);
	}

	TArray<AEITrainingDummy*> SortedDummies(UWorld* World)
	{
		TArray<AEITrainingDummy*> Dummies;
		for (TActorIterator<AEITrainingDummy> It(World); It; ++It)
		{
			Dummies.Add(*It);
		}
		Dummies.Sort([](const AEITrainingDummy& A, const AEITrainingDummy& B) { return A.GetActorLocation().Y < B.GetActorLocation().Y; });
		return Dummies;
	}

	void StandBeforeDummies(const FEIPlayContext& Context, float DistanceCm)
	{
		const FVector Middle = SortedDummies(Context.World)[1]->GetActorLocation();
		Teleport(Context, FVector(Middle.X - DistanceCm, Middle.Y, HeroHeight), 0.0f);
		for (AEITrainingDummy* Dummy : SortedDummies(Context.World))
		{
			Dummy->ResetHits();
		}
	}

	float HorizontalSpeed(const FEIPlayContext& Context)
	{
		return static_cast<float>(Context.Hero->GetVelocity().Size2D());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIOmenTableTest,
	"Eisenseelen.Data.Omens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIOmenTableTest::RunTest(const FString& Parameters)
{
	FEIOmenTable Table;
	FString Error;
	if (!FEIOmenTable::LoadFromProjectFile(Table, Error))
	{
		AddError(Error);
	}
	TestEqual(TEXT("There are five omens"), Table.Omens.Num(), 5);

	const FString Valid = TEXT("{\"omen\": [{\"name\": \"Omen der Probe\", \"wirkung\": \"tempo\", \"wert\": 0.2, \"klasse\": \"arkanist\"}]}");
	FEIOmenTable Loaded;
	TestTrue(TEXT("A complete line loads"), FEIOmenTable::LoadFromString(Valid, Loaded, Error));
	TestEqual(TEXT("The name counts"), Loaded.Omens[0].Name, FString(TEXT("Omen der Probe")));
	TestEqual(TEXT("The value counts"), Loaded.Omens[0].Amount, 0.2f);
	TestFalse(TEXT("The class counts"), Loaded.Omens[0].bForBladewarden);
	TestEqual(TEXT("The words come from the effect"), Loaded.Omens[0].Describe(), FString(TEXT("+20 % Tempo")));

	FEIOmenTable Untouched;
	const FString BadEffect = Valid.Replace(TEXT("tempo"), TEXT("gibt es nicht"));
	TestFalse(TEXT("An unknown effect is refused"), FEIOmenTable::LoadFromString(BadEffect, Untouched, Error));
	TestTrue(TEXT("The error names the omen"), Error.Contains(TEXT("Omen der Probe")));
	const FString BadClass = Valid.Replace(TEXT("arkanist"), TEXT("niemand"));
	TestFalse(TEXT("An unknown class is refused"), FEIOmenTable::LoadFromString(BadClass, Untouched, Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.Omens.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIOmenEffectsTest,
	"Eisenseelen.Play.Omens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIOmenEffectsTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;

	// Sword: the first target takes the full damage of a light swing, and the omen raises it.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		GiveOmen(Context, EEIStat::AttackPower);
		StandBeforeDummies(Context, 180.0f);
	}));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("the swing ends"), 3.0, [](const FEIPlayContext& Context)
	{
		return SortedDummies(Context.World)[1]->GetHitCount() > 0 && !Context.Hero->GetSword()->IsSwinging();
	}));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const TArray<AEITrainingDummy*> Dummies = SortedDummies(Context.World);
		const float Highest = FMath::Max3(Dummies[0]->GetLastDamage(), Dummies[1]->GetLastDamage(), Dummies[2]->GetLastDamage());
		const float Expected = Context.Hero->GetSword()->GetValues().Light.Damage * (1.0f + FindOmen(Context, EEIStat::AttackPower)->Amount);
		Context.Test->AddInfo(FString::Printf(TEXT("With the omen a swing at 180 cm does %.1f damage, expected %.1f"), Highest, Expected));
		Context.Test->TestNearlyEqual(TEXT("The omen raises the sword's damage"), Highest, Expected, 0.01f);
	}));

	// Life: more of it at once, and more room for it.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHealthComponent* Health = Context.Hero->GetHealth();
		const float Before = Health->GetMaxHealth();
		GiveOmen(Context, EEIStat::Life);
		const float Bonus = FindOmen(Context, EEIStat::Life)->Amount;
		Context.Test->TestEqual(TEXT("The omen raises the maximum life"), Health->GetMaxHealth(), Before + Bonus);
		Context.Test->TestEqual(TEXT("The omen fills up what it adds"), Health->GetHealth(), Before + Bonus);
	}));

	// Speed: running and sprinting both get faster.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		GiveOmen(Context, EEIStat::Speed);
		Teleport(Context, OpenFloor, BackwardsYaw);
	}));
	Steps.Add(Wait(0.3));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Factor = Context.Hero->GetOmens()->GetFactor(EEIStat::Speed);
		const float Expected = Context.Hero->GetValues().RunSpeedMetersPerSecond * CentimetersPerMeter * Factor;
		Context.Test->AddInfo(FString::Printf(TEXT("With the omen W runs at %.0f cm/s, expected %.0f"), HorizontalSpeed(Context), Expected));
		Context.Test->TestNearlyEqual(TEXT("The omen raises the run speed"), HorizontalSpeed(Context), Expected, SpeedToleranceCm);
	}));
	Steps.Add(Press(EKeys::LeftShift));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Factor = Context.Hero->GetOmens()->GetFactor(EEIStat::Speed);
		const float Expected = Context.Hero->GetValues().SprintSpeedMetersPerSecond * CentimetersPerMeter * Factor;
		Context.Test->TestNearlyEqual(TEXT("The omen raises the sprint speed"), HorizontalSpeed(Context), Expected, SpeedToleranceCm);
	}));
	Steps.Add(Release(EKeys::LeftShift));
	Steps.Add(Release(EKeys::W));

	// On to the arcanist: more mana comes back, at the new rate only from now on.
	AddTap(Steps, EKeys::C);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float LifeBonus = FindOmen(Context, EEIStat::Life)->Amount;
		Context.Test->TestEqual(TEXT("The class keeps the life from the omen"), Context.Hero->GetHealth()->GetMaxHealth(), Context.Hero->GetClassValues().Life + LifeBonus);
		UEICasterComponent* Caster = Context.Hero->GetCaster();
		Caster->SpendMana(Caster->GetMaxMana());
		Context.Test->TestTrue(TEXT("The omen takes no mana away"), Caster->GetMana() < 1.0f);
		GiveOmen(Context, EEIStat::ManaRegen);
		Context.Test->TestTrue(TEXT("The omen gives no mana at once"), Caster->GetMana() < 1.0f);
	}));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Factor = Context.Hero->GetOmens()->GetFactor(EEIStat::ManaRegen);
		const float Expected = Context.Hero->GetClassValues().ManaPerSecond * Factor * 0.5f;
		const float Mana = Context.Hero->GetCaster()->GetMana();
		Context.Test->AddInfo(FString::Printf(TEXT("With the omen 0.5 s bring %.1f mana, expected about %.1f at 1000 cm"), Mana, Expected));
		Context.Test->TestTrue(TEXT("The omen brings the mana back faster"), Mana >= Expected * 0.9f && Mana <= Expected * 1.4f);
	}));
	Steps.Add(WaitUntil(TEXT("the mana is back"), 8.0, [](const FEIPlayContext& Context)
	{
		return Context.Hero->GetCaster()->GetMana() >= Context.Hero->GetCaster()->GetMaxMana();
	}));

	// Fire: a bolt does more damage.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		GiveOmen(Context, EEIStat::FireDamage);
		StandBeforeDummies(Context, 1000.0f);
		const float Pitch = -FMath::RadiansToDegrees(FMath::Atan2(64.0f, 1000.0f));
		Context.Player->SetControlRotation(FRotator(Pitch, 0.0f, 0.0f));
	}));
	Steps.Add(Wait(0.3));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("the bolt bursts"), 2.0, [](const FEIPlayContext& Context) { return SortedDummies(Context.World)[1]->GetHitCount() > 0; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		// All bonuses add up: this omen, the staff's own for its element, and the omen of the blade from the start of this test.
		const float Bonus = FindOmen(Context, EEIStat::FireDamage)->Amount + Context.Hero->GetGear()->GetTable().WeaponElementBonus + FindOmen(Context, EEIStat::AttackPower)->Amount;
		const float Expected = Context.Hero->GetCaster()->GetValues().Bolt.Damage * (1.0f + Bonus);
		Context.Test->TestNearlyEqual(TEXT("The omen raises the fire damage"), SortedDummies(Context.World)[1]->GetLastDamage(), Expected, 0.01f);
	}));

	// A restart ends them all.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitUntil(TEXT("a fresh hero stands"), 3.0, [](const FEIPlayContext& Context)
	{
		return Context.Hero->GetOmens()->CountAll() == 0 && Context.Movement->IsMovingOnGround();
	}));

	// As an arcanist: first all four that fit, none twice and none for the sword, and only then a second one.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEIOmenComponent* Omens = Context.Hero->GetOmens();
		Context.Test->TestEqual(TEXT("The class survives the restart"), Context.Hero->GetHeroClass(), EEIHeroClass::Arcanist);
		for (int32 Number = 0; Number < 4; ++Number)
		{
			Omens->GrantRandomOmen();
		}
		int32 SwordIndex = INDEX_NONE;
		FindOmen(Context, EEIStat::AttackPower, &SwordIndex);
		Context.Test->TestEqual(TEXT("An omen for the sword does not come to the arcanist"), Omens->CountOf(SwordIndex), 0);
		for (int32 Index = 0; Index < Omens->GetTable().Omens.Num(); ++Index)
		{
			Context.Test->TestTrue(TEXT("Nothing comes twice before everything came once"), Omens->CountOf(Index) <= 1);
		}
		Omens->GrantRandomOmen();
		Context.Test->TestEqual(TEXT("After that they come again"), Omens->CountAll(), 5);
		Context.Test->TestEqual(TEXT("But still none for the sword"), Omens->CountOf(SwordIndex), 0);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
