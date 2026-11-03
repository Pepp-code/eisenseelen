#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EICasterComponent.h"
#include "EIEnemy.h"
#include "EIHealthComponent.h"
#include "EIProjectile.h"
#include "EISwordComponent.h"
#include "EITestScript.h"
#include "EITrainingDummy.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace EITestScript;

namespace
{
	constexpr float HeroHeight = 92.0f;
	constexpr float EyeHeightAboveDummyCenter = 64.0f;
	constexpr float AimDistanceCm = 1000.0f;

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

	/** Stands ten metres before the middle dummy, aiming at its centre. */
	void AimAtMiddleDummy(const FEIPlayContext& Context)
	{
		const FVector Middle = SortedDummies(Context.World)[1]->GetActorLocation();
		Teleport(Context, FVector(Middle.X - AimDistanceCm, Middle.Y, HeroHeight), 0.0f);
		const float Pitch = -FMath::RadiansToDegrees(FMath::Atan2(EyeHeightAboveDummyCenter, AimDistanceCm));
		Context.Player->SetControlRotation(FRotator(Pitch, 0.0f, 0.0f));
		for (AEITrainingDummy* Dummy : SortedDummies(Context.World))
		{
			Dummy->ResetHits();
		}
	}

	int32 CountProjectiles(const FEIPlayContext& Context)
	{
		int32 Count = 0;
		for (TActorIterator<AEIProjectile> It(Context.World); It; ++It)
		{
			++Count;
		}
		return Count;
	}

	FEIPlayStep WaitForNoProjectiles()
	{
		return WaitUntil(TEXT("the projectile bursts"), 3.0, [](const FEIPlayContext& Context) { return CountProjectiles(Context) == 0; });
	}

	FEIPlayStep WaitForFullMana()
	{
		return WaitUntil(TEXT("the mana is back"), 8.0, [](const FEIPlayContext& Context)
		{
			const UEICasterComponent* Caster = Context.Hero->GetCaster();
			return Caster->GetMana() >= Caster->GetMaxMana();
		});
	}

	void AddChargedRelease(TArray<FEIPlayStep>& Steps)
	{
		Steps.Add(Press(EKeys::RightMouseButton));
		Steps.Add(WaitUntil(TEXT("the staff charges"), 2.0, [](const FEIPlayContext& Context) { return Context.Hero->GetCaster()->GetChargeFraction() >= 1.0f; }));
		Steps.Add(Release(EKeys::RightMouseButton));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIStaffTest,
	"Eisenseelen.Play.Staff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIStaffTest::RunTest(const FString& Parameters)
{
	const TSharedRef<float> LifeBefore = MakeShared<float>(0.0f);
	TArray<FEIPlayStep> Steps;

	// C switches to the arcanist, with the arcanist's life.
	AddTap(Steps, EKeys::C);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([LifeBefore](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("C switches the class"), Context.Hero->GetHeroClass(), EEIHeroClass::Arcanist);
		Context.Test->TestEqual(TEXT("The arcanist has the arcanist's life"), Context.Hero->GetHealth()->GetMaxHealth(), Context.Hero->GetClassValues().Life);
		AimAtMiddleDummy(Context);
		*LifeBefore = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(Wait(0.3));

	// A tap fires one bolt and costs its mana.
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("a bolt flies"), 1.0, [](const FEIPlayContext& Context) { return CountProjectiles(Context) == 1; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEICasterComponent* Caster = Context.Hero->GetCaster();
		const float Spent = Caster->GetMaxMana() - Caster->GetMana();
		Context.Test->AddInfo(FString::Printf(TEXT("A bolt costs %.1f of %.0f mana, %.0f minus what came back"), Spent, Caster->GetMaxMana(), Caster->GetValues().Bolt.ManaCost));
		Context.Test->TestTrue(TEXT("A bolt costs mana"), Spent > Caster->GetValues().Bolt.ManaCost * 0.5f);
	}));

	// The next tap fires at once, and each bolt does its damage at the target.
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("both bolts burst"), 2.0, [](const FEIPlayContext& Context) { return SortedDummies(Context.World)[1]->GetHitCount() == 2; }));
	Steps.Add(Do([LifeBefore](const FEIPlayContext& Context)
	{
		const AEITrainingDummy* Middle = SortedDummies(Context.World)[1];
		const float Expected = Context.Hero->GetCaster()->GetValues().Bolt.Damage;
		Context.Test->AddInfo(FString::Printf(TEXT("Two quick bolts from 1000 cm: %d hits for %.0f damage each, expected %.0f"), Middle->GetHitCount(), Middle->GetLastDamage(), Expected));
		Context.Test->TestEqual(TEXT("A bolt does its damage"), Middle->GetLastDamage(), Expected);
		Context.Test->TestEqual(TEXT("Fire does not hurt heroes"), Context.Hero->GetHealth()->GetHealth(), *LifeBefore);
	}));
	Steps.Add(WaitForNoProjectiles());
	Steps.Add(WaitForFullMana());

	// Holding the attack key charges nothing, so a shot never turns into something else.
	Steps.Add(Do([](const FEIPlayContext& Context) { AimAtMiddleDummy(Context); }));
	Steps.Add(Press(EKeys::LeftMouseButton));
	Steps.Add(Wait(0.6));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Holding the attack key charges nothing"), Context.Hero->GetCaster()->GetChargeFraction(), 0.0f);
	}));
	Steps.Add(Release(EKeys::LeftMouseButton));
	Steps.Add(Wait(0.05));
	Steps.Add(WaitForNoProjectiles());
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const AEITrainingDummy* Middle = SortedDummies(Context.World)[1];
		Context.Test->TestEqual(TEXT("A long press is still one bolt"), Middle->GetHitCount(), 1);
		Context.Test->TestEqual(TEXT("A long press does a bolt's damage"), Middle->GetLastDamage(), Context.Hero->GetCaster()->GetValues().Bolt.Damage);
	}));

	// Letting go of the second key too early fires nothing.
	Steps.Add(Press(EKeys::RightMouseButton));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("The arcanist does not block"), Context.Movement->IsBlocking());
	}));
	Steps.Add(Release(EKeys::RightMouseButton));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("An early release fires nothing"), CountProjectiles(Context), 0);
	}));
	Steps.Add(WaitForFullMana());

	// The reported case: fully charged fireballs right after each other. The wide burst reaches all dummies.
	Steps.Add(Do([](const FEIPlayContext& Context) { AimAtMiddleDummy(Context); }));
	AddChargedRelease(Steps);
	Steps.Add(WaitUntil(TEXT("a fireball flies"), 1.0, [](const FEIPlayContext& Context) { return CountProjectiles(Context) == 1; }));
	AddChargedRelease(Steps);
	Steps.Add(WaitUntil(TEXT("both fireballs burst"), 3.0, [](const FEIPlayContext& Context) { return SortedDummies(Context.World)[1]->GetHitCount() == 2; }));
	Steps.Add(WaitForNoProjectiles());
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Expected = Context.Hero->GetCaster()->GetValues().Fireball.Damage;
		for (const AEITrainingDummy* Dummy : SortedDummies(Context.World))
		{
			Context.Test->TestEqual(TEXT("Both fireballs reach every dummy"), Dummy->GetHitCount(), 2);
			Context.Test->TestEqual(TEXT("A fireball does its damage"), Dummy->GetLastDamage(), Expected);
		}
		Context.Test->TestEqual(TEXT("Charging starts no cooldown"), Context.Hero->GetCaster()->GetSpellCooldownFraction(), 0.0f);
	}));

	// Without enough mana nothing fires, and the HUD gets told so.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEICasterComponent* Caster = Context.Hero->GetCaster();
		Caster->SpendMana(Caster->GetMana());
	}));
	AddChargedRelease(Steps);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEICasterComponent* Caster = Context.Hero->GetCaster();
		Context.Test->TestEqual(TEXT("No fireball without mana"), CountProjectiles(Context), 0);
		Context.Test->TestTrue(TEXT("The failed fireball shows"), Caster->GetManaDeniedFlash() > 0.0f);
		Caster->SpendMana(Caster->GetMana());
	}));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("No bolt without mana"), CountProjectiles(Context), 0);
	}));

	// Then the mana comes back by itself.
	Steps.Add(WaitForFullMana());

	// Q needs no charge and no mana, but it has a cooldown.
	AddTap(Steps, EKeys::Q);
	Steps.Add(WaitUntil(TEXT("Q casts"), 1.0, [](const FEIPlayContext& Context) { return CountProjectiles(Context) == 1; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEICasterComponent* Caster = Context.Hero->GetCaster();
		Context.Test->AddInfo(FString::Printf(TEXT("The arcanist's Q cools down in %.1f s"), Caster->GetSpellCooldownSeconds()));
		Context.Test->TestTrue(TEXT("Q starts its cooldown"), Caster->GetSpellCooldownFraction() > 0.5f);
		Context.Test->TestTrue(TEXT("The arcanist casts faster"), Caster->GetSpellCooldownSeconds() < Caster->GetValues().SpellCooldownSeconds);
		Context.Test->TestEqual(TEXT("Q costs no mana"), Caster->GetMana(), Caster->GetMaxMana());
	}));
	AddTap(Steps, EKeys::Q);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Q waits for its cooldown"), CountProjectiles(Context), 1);
	}));
	Steps.Add(WaitForNoProjectiles());
	Steps.Add(WaitUntil(TEXT("Q cools down"), 8.0, [](const FEIPlayContext& Context) { return Context.Hero->GetCaster()->GetSpellCooldownFraction() <= 0.0f; }));

	// So it also works with an empty mana bar.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEICasterComponent* Caster = Context.Hero->GetCaster();
		Caster->SpendMana(Caster->GetMana());
	}));
	AddTap(Steps, EKeys::Q);
	Steps.Add(WaitUntil(TEXT("Q casts without mana"), 1.0, [](const FEIPlayContext& Context) { return CountProjectiles(Context) == 1; }));
	Steps.Add(WaitForNoProjectiles());
	Steps.Add(WaitForFullMana());

	// One bolt kills a runner.
	Steps.Add(Wait(0.6));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Teleport(Context, FVector(-1500.0f, 0.0f, HeroHeight), 0.0f);
		Context.Player->SetControlRotation(FRotator(-3.0f, 0.0f, 0.0f));
		Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), FVector(-500.0f, 0.0f, HeroHeight), FRotator(0.0f, 180.0f, 0.0f));
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("the bolt kills the runner"), 3.0, [](const FEIPlayContext& Context)
	{
		TActorIterator<AEIEnemy> Enemy(Context.World);
		return !Enemy;
	}));

	// The switch back: the class stays over a restart, and the blade warden swings instead of firing.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitUntil(TEXT("the fresh hero exists"), 3.0, [](const FEIPlayContext& Context) { return Context.Hero->GetHealth()->GetHealth() > 0.0f && Context.Movement->IsMovingOnGround(); }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The class survives a restart"), Context.Hero->GetHeroClass(), EEIHeroClass::Arcanist);
		Context.Test->TestEqual(TEXT("A fresh arcanist has full mana"), Context.Hero->GetCaster()->GetMana(), Context.Hero->GetCaster()->GetMaxMana());
	}));
	AddTap(Steps, EKeys::C);
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("C switches back"), Context.Hero->GetHeroClass(), EEIHeroClass::Bladewarden);
		Context.Test->TestTrue(TEXT("The blade warden swings"), Context.Hero->GetSword()->IsSwinging());
		Context.Test->TestEqual(TEXT("The blade warden fires nothing with the attack key"), CountProjectiles(Context), 0);
	}));
	Steps.Add(Wait(0.6));

	// But the spell key works for both classes.
	AddTap(Steps, EKeys::Q);
	Steps.Add(WaitUntil(TEXT("the blade warden casts"), 1.0, [](const FEIPlayContext& Context) { return CountProjectiles(Context) == 1; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEICasterComponent* Caster = Context.Hero->GetCaster();
		Context.Test->TestEqual(TEXT("The blade warden casts at the normal pace"), Caster->GetSpellCooldownSeconds(), Caster->GetValues().SpellCooldownSeconds);
		Context.Test->TestEqual(TEXT("The blade warden has no mana to pay"), Caster->GetMaxMana(), 0.0f);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
