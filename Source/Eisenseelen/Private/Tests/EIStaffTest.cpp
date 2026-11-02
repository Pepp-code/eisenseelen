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

	// A tap fires one bolt, and the bolt does its damage at the target.
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("a bolt flies"), 1.0, [](const FEIPlayContext& Context) { return CountProjectiles(Context) == 1; }));

	// During the cooldown a second tap fires nothing.
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The staff waits for its cooldown"), CountProjectiles(Context), 1);
	}));
	Steps.Add(WaitForNoProjectiles());
	Steps.Add(Do([LifeBefore](const FEIPlayContext& Context)
	{
		const AEITrainingDummy* Middle = SortedDummies(Context.World)[1];
		const float Expected = Context.Hero->GetCaster()->GetValues().Bolt.Damage;
		Context.Test->AddInfo(FString::Printf(TEXT("A bolt from 1000 cm did %.0f damage, expected %.0f"), Middle->GetLastDamage(), Expected));
		Context.Test->TestEqual(TEXT("A bolt does its damage"), Middle->GetLastDamage(), Expected);
		Context.Test->TestEqual(TEXT("Fire does not hurt heroes"), Context.Hero->GetHealth()->GetHealth(), *LifeBefore);
	}));
	Steps.Add(Wait(0.6));

	// A hold fires a fireball, whose wide burst reaches all dummies. Then it cools down.
	Steps.Add(Do([](const FEIPlayContext& Context) { AimAtMiddleDummy(Context); }));
	Steps.Add(Press(EKeys::LeftMouseButton));
	Steps.Add(WaitUntil(TEXT("the staff charges"), 2.0, [](const FEIPlayContext& Context) { return Context.Hero->GetCaster()->GetChargeFraction() >= 1.0f; }));
	Steps.Add(Release(EKeys::LeftMouseButton));
	Steps.Add(WaitUntil(TEXT("a fireball flies"), 1.0, [](const FEIPlayContext& Context) { return CountProjectiles(Context) == 1; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Cooldown = Context.Hero->GetCaster()->GetFireballCooldownSeconds();
		Context.Test->AddInfo(FString::Printf(TEXT("The arcanist's fireball cools down in %.1f s"), Cooldown));
		Context.Test->TestTrue(TEXT("The fireball starts its cooldown"), Context.Hero->GetCaster()->GetFireballCooldownFraction() > 0.5f);
		Context.Test->TestTrue(TEXT("The arcanist casts faster"), Cooldown < Context.Hero->GetCaster()->GetValues().Fireball.CooldownSeconds);
	}));
	Steps.Add(WaitForNoProjectiles());
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Expected = Context.Hero->GetCaster()->GetValues().Fireball.Damage;
		for (const AEITrainingDummy* Dummy : SortedDummies(Context.World))
		{
			Context.Test->TestEqual(TEXT("The fireball's burst reaches every dummy"), Dummy->GetLastDamage(), Expected);
		}
	}));

	// Q during the cooldown does nothing.
	AddTap(Steps, EKeys::Q);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Q waits for the cooldown"), CountProjectiles(Context), 0);
	}));

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
		Context.Test->TestEqual(TEXT("The blade warden casts at the normal pace"), Context.Hero->GetCaster()->GetFireballCooldownSeconds(), Context.Hero->GetCaster()->GetValues().Fireball.CooldownSeconds);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
