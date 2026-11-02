#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EIEnemy.h"
#include "Engine/DamageEvents.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHordeDirector.h"
#include "EISwordComponent.h"
#include "EITestScript.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace EITestScript;

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FVector OpenFloor(-1500.0f, 0.0f, 92.0f);

	AEIGameMode* GetGameMode(const FEIPlayContext& Context)
	{
		return Context.World->GetAuthGameMode<AEIGameMode>();
	}

	AEIEnemy* FindEnemy(const FEIPlayContext& Context)
	{
		TActorIterator<AEIEnemy> It(Context.World);
		return It ? *It : nullptr;
	}

	AEIEnemy* SpawnEnemyAhead(const FEIPlayContext& Context, float DistanceCm)
	{
		const FVector Location = Context.Hero->GetActorLocation() + Context.Hero->GetActorForwardVector() * DistanceCm;
		return Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Location, FRotator(0.0f, 180.0f, 0.0f));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIEnemyAttackTest,
	"Eisenseelen.Play.Enemy.Attack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIEnemyAttackTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		double TelegraphAt = 0.0;
		float LifeBefore = 0.0f;
	};
	const TSharedRef<FState> State = MakeShared<FState>();

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Teleport(Context, OpenFloor, 0.0f);
		State->LifeBefore = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Test->TestNotNull(TEXT("An enemy can spawn"), SpawnEnemyAhead(Context, 800.0f)); }));

	// It comes running.
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		const float Expected = Enemy->GetValues().SpeedMetersPerSecond * CentimetersPerMeter;
		Context.Test->AddInfo(FString::Printf(TEXT("The runner runs at %.0f cm/s, expected %.0f"), Enemy->GetVelocity().Size2D(), Expected));
		Context.Test->TestNearlyEqual(TEXT("The enemy runs at its speed"), static_cast<float>(Enemy->GetVelocity().Size2D()), Expected, Expected * 0.25f);
	}));

	// It warns first, and only hits after the windup.
	Steps.Add(WaitUntil(TEXT("the enemy winds up"), 5.0, [](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		return Enemy != nullptr && Enemy->IsTelegraphing();
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->TelegraphAt = Context.Now;
		Context.Test->TestEqual(TEXT("No damage before the windup ends"), Context.Hero->GetHealth()->GetHealth(), State->LifeBefore);
	}));
	Steps.Add(WaitUntil(TEXT("the hero takes damage"), 3.0, [State](const FEIPlayContext& Context)
	{
		return Context.Hero->GetHealth()->GetHealth() < State->LifeBefore;
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const double Windup = Context.Now - State->TelegraphAt;
		Context.Test->AddInfo(FString::Printf(TEXT("Warning to damage: %.2f s, the plan asks for at least 0.40"), Windup));
		Context.Test->TestTrue(TEXT("The warning comes at least 0.4 s before the damage"), Windup >= 0.4);
		const float Damage = State->LifeBefore - Context.Hero->GetHealth()->GetHealth();
		Context.Test->TestEqual(TEXT("The hit does the enemy's damage"), Damage, FindEnemy(Context)->GetValues().Damage);
	}));

	// Whoever moves away during the windup is not hit.
	Steps.Add(WaitUntil(TEXT("the enemy winds up again"), 5.0, [](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		return Enemy != nullptr && Enemy->IsTelegraphing();
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->LifeBefore = Context.Hero->GetHealth()->GetHealth();
		Teleport(Context, Context.Hero->GetActorLocation() - FVector(600.0f, 0.0f, 0.0f), 0.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the windup ends"), 3.0, [](const FEIPlayContext& Context) { return !FindEnemy(Context)->IsTelegraphing(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Moving away in time avoids the hit"), Context.Hero->GetHealth()->GetHealth(), State->LifeBefore);
	}));

	// Two light swings kill it, and it leaves the world.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		AEIEnemy* Enemy = FindEnemy(Context);
		Teleport(Context, Enemy->GetActorLocation() - FVector(180.0f, 0.0f, 0.0f), 0.0f);
	}));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("the enemy is gone"), 3.0, [](const FEIPlayContext& Context) { return FindEnemy(Context) == nullptr; }));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIHordeTest,
	"Eisenseelen.Play.Enemy.Horde",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIHordeTest::RunTest(const FString& Parameters)
{
	const TSharedRef<float> NearestSpawn = MakeShared<float>(TNumericLimits<float>::Max());
	const TSharedRef<TSet<const AActor*>> Seen = MakeShared<TSet<const AActor*>>();

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		Context.Test->TestEqual(TEXT("The horde starts switched off"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
	}));

	// H: the horde grows to its limit, spawning at a distance.
	AddTap(Steps, EKeys::H);
	Steps.Add(WaitUntil(TEXT("the horde reaches its limit"), 15.0, [NearestSpawn, Seen](const FEIPlayContext& Context)
	{
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			if (!Seen->Contains(*It))
			{
				Seen->Add(*It);
				*NearestSpawn = FMath::Min(*NearestSpawn, static_cast<float>(FVector::Dist2D(It->GetActorLocation(), Context.Hero->GetActorLocation())));
			}
		}
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		return Horde->CountEnemies() >= FMath::RoundToInt(Horde->GetValues().MaxAlive);
	}));
	Steps.Add(Do([NearestSpawn](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		const float MinCm = Horde->GetValues().MinDistanceMeters * CentimetersPerMeter;
		Context.Test->AddInfo(FString::Printf(TEXT("The nearest spawn was %.0f cm away, the minimum is %.0f cm"), *NearestSpawn, MinCm));
		Context.Test->TestTrue(TEXT("Enemies spawn at a distance"), *NearestSpawn >= MinCm * 0.9f);
		Context.Test->TestTrue(TEXT("The game state knows the horde is on"), Context.World->GetGameState<AEIGameState>()->IsHordeActive());
	}));

	// The limit holds.
	Steps.Add(Wait(1.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		Context.Test->TestTrue(TEXT("The horde stays at its limit"), Horde->CountEnemies() <= FMath::RoundToInt(Horde->GetValues().MaxAlive));
	}));

	// R: all gone, and the horde is off again.
	AddTap(Steps, EKeys::R);
	Steps.Add(Wait(0.3));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The restart removes all enemies"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		Context.Test->TestFalse(TEXT("The restart switches the horde off"), Context.World->GetGameState<AEIGameState>()->IsHordeActive());
	}));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIHeroDeathTest,
	"Eisenseelen.Play.Enemy.HeroDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIHeroDeathTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FVector> DeadAt = MakeShared<FVector>();
	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([DeadAt](const FEIPlayContext& Context)
	{
		Context.Hero->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
		Context.Test->TestTrue(TEXT("Too much damage kills the hero"), Context.Hero->IsDead());
		*DeadAt = Context.Hero->GetActorLocation();
	}));

	// A dead hero does not move.
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.5));
	Steps.Add(Release(EKeys::W));
	Steps.Add(Do([DeadAt](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("A dead hero does not move"), FVector::Dist2D(*DeadAt, Context.Hero->GetActorLocation()) < 5.0);
	}));

	// R brings a fresh one.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitUntil(TEXT("a fresh hero stands"), 3.0, [](const FEIPlayContext& Context) { return !Context.Hero->IsDead() && Context.Movement->IsMovingOnGround(); }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHealthComponent* Health = Context.Hero->GetHealth();
		Context.Test->TestEqual(TEXT("The fresh hero has full life"), Health->GetHealth(), Health->GetMaxHealth());
	}));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.4));
	Steps.Add(Release(EKeys::W));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The fresh hero moves"), Context.Hero->GetVelocity().Size2D() > 100.0);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
