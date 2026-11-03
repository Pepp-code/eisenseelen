#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EIEnemy.h"
#include "EIHealthComponent.h"
#include "EITestScript.h"
#include "Engine/DamageEvents.h"

using namespace EITestScript;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEILifeRecoveryTest,
	"Eisenseelen.Play.LifeRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEILifeRecoveryTest::RunTest(const FString& Parameters)
{
	constexpr float WaitSeconds = 1.0f;
	constexpr float SharePerSecond = 0.25f;
	constexpr float Hit = 60.0f;
	const TSharedRef<float> Life = MakeShared<float>(0.0f);

	TArray<FEIPlayStep> Steps;

	// A hit: for a while nothing comes back.
	Steps.Add(Do([Life](const FEIPlayContext& Context)
	{
		FEIHeroValues Short = Context.Hero->GetValues();
		Short.LifeBackAfterSeconds = WaitSeconds;
		Short.LifeBackSharePerSecond = SharePerSecond;
		Context.Hero->ApplyValues(Short);
		Context.Hero->TakeDamage(Hit, FDamageEvent(), nullptr, nullptr);
		*Life = Context.Hero->GetHealth()->GetHealth();
		Context.Test->TestEqual(TEXT("The hit counts"), *Life, Context.Hero->GetHealth()->GetMaxHealth() - Hit);
	}));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([Life](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Right after a hit no life comes back"), Context.Hero->GetHealth()->GetHealth(), *Life);
		Context.Test->TestFalse(TEXT("The hero does not recover yet"), Context.Hero->GetHealth()->IsRecovering());
	}));

	// After the wait it comes back at its rate.
	Steps.Add(WaitUntil(TEXT("the recovery starts"), 1.5, [](const FEIPlayContext& Context) { return Context.Hero->GetHealth()->IsRecovering(); }));
	Steps.Add(Do([Life](const FEIPlayContext& Context) { *Life = Context.Hero->GetHealth()->GetHealth(); }));
	Steps.Add(Wait(0.4));
	Steps.Add(Do([Life](const FEIPlayContext& Context)
	{
		const UEIHealthComponent* Health = Context.Hero->GetHealth();
		const float PerSecond = (Health->GetHealth() - *Life) / 0.4f;
		const float Expected = Health->GetMaxHealth() * SharePerSecond;
		Context.Test->AddInfo(FString::Printf(TEXT("%.1f life per second, expected %.1f"), PerSecond, Expected));
		Context.Test->TestTrue(TEXT("Life comes back at its rate"), PerSecond > Expected * 0.7f && PerSecond < Expected * 1.3f);

		// A new hit stops it at once.
		Context.Hero->TakeDamage(Hit, FDamageEvent(), nullptr, nullptr);
		*Life = Health->GetHealth();
	}));
	Steps.Add(Wait(0.5));

	// Half way through the wait the hero hits something, and that counts as fighting too.
	Steps.Add(Do([Life](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A new hit stops the recovery"), Context.Hero->GetHealth()->GetHealth(), *Life);
		const FVector Far = Context.Hero->GetActorLocation() + FVector(3000.0f, 0.0f, 0.0f);
		if (AEIEnemy* Enemy = Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Far, FRotator::ZeroRotator))
		{
			Enemy->TakeDamage(1.0f, FDamageEvent(), Context.Hero->GetController(), Context.Hero);
			Enemy->Destroy();
		}
	}));
	Steps.Add(Wait(0.7));
	Steps.Add(Do([Life](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Dealing damage holds up the recovery"), Context.Hero->GetHealth()->GetHealth(), *Life);
	}));

	// It ends at full life.
	Steps.Add(WaitUntil(TEXT("life is full again"), 6.0, [](const FEIPlayContext& Context)
	{
		const UEIHealthComponent* Health = Context.Hero->GetHealth();
		return Health->GetHealth() >= Health->GetMaxHealth();
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHealthComponent* Health = Context.Hero->GetHealth();
		Context.Test->TestEqual(TEXT("Life stops at its maximum"), Health->GetHealth(), Health->GetMaxHealth());
		Context.Test->TestFalse(TEXT("At full life nothing recovers"), Health->IsRecovering());
		Context.Hero->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
	}));

	// The dead stay dead.
	Steps.Add(Wait(1.3));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The dead stay dead"), Context.Hero->IsDead());
		Context.Test->TestEqual(TEXT("The dead get no life back"), Context.Hero->GetHealth()->GetHealth(), 0.0f);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
