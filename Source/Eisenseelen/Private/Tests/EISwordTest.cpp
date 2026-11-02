#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EIHealthComponent.h"
#include "EISwordComponent.h"
#include "EITestScript.h"
#include "EITrainingDummy.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace EITestScript;

namespace
{
	constexpr float HeroHeight = 92.0f;

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

	void ResetDummies(const FEIPlayContext& Context)
	{
		for (AEITrainingDummy* Dummy : SortedDummies(Context.World))
		{
			Dummy->ResetHits();
		}
	}

	/** Stands in front of the middle dummy, looking at it. */
	void StandBeforeDummies(const FEIPlayContext& Context, float DistanceCm)
	{
		const TArray<AEITrainingDummy*> Dummies = SortedDummies(Context.World);
		if (Dummies.Num() != 3)
		{
			Context.Test->AddError(TEXT("The test arena should have three training dummies"));
			return;
		}
		const FVector Middle = Dummies[1]->GetActorLocation();
		Teleport(Context, FVector(Middle.X - DistanceCm, Middle.Y, HeroHeight), 0.0f);
		ResetDummies(Context);
	}

	FEIPlayStep WaitForSwingEnd()
	{
		return WaitUntil(TEXT("the swing ends"), 3.0, [](const FEIPlayContext& Context) { return !Context.Hero->GetSword()->IsSwinging(); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEISwordLightTest,
	"Eisenseelen.Play.Sword.Light",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEISwordLightTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;

	// A light swing hits all three dummies in its arc, each next one for less.
	Steps.Add(Do([](const FEIPlayContext& Context) { StandBeforeDummies(Context, 180.0f); }));
	Steps.Add(Wait(0.3));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("A tap starts a swing"), Context.Hero->GetSword()->IsSwinging());
		Context.Test->TestEqual(TEXT("The first swing of a chain"), Context.Hero->GetSword()->GetSwingKind(), EEISwingKind::Light1);
	}));
	Steps.Add(WaitForSwingEnd());
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FEISwingValues& Light = Context.Hero->GetSword()->GetValues().Light;
		float Total = 0.0f;
		for (AEITrainingDummy* Dummy : SortedDummies(Context.World))
		{
			Context.Test->AddInfo(FString::Printf(TEXT("Dummy at %.0f cm took %.1f in %d hits"), Dummy->GetActorLocation().Y, Dummy->GetLastDamage(), Dummy->GetHitCount()));
			Context.Test->TestEqual(TEXT("Each dummy is hit exactly once per swing"), Dummy->GetHitCount(), 1);
			Total += Dummy->GetLastDamage();
		}
		const float Expected = Light.Damage * (3.0f - 3.0f * Light.TargetFalloff);
		Context.Test->TestNearlyEqual(TEXT("Each next target takes less"), Total, Expected, 0.1f);
		Context.Test->TestEqual(TEXT("The first target takes the full damage"), FMath::Max3(SortedDummies(Context.World)[0]->GetLastDamage(), SortedDummies(Context.World)[1]->GetLastDamage(), SortedDummies(Context.World)[2]->GetLastDamage()), Light.Damage);
	}));

	// Three fast taps run through the chain, and a press during a swing is not lost.
	Steps.Add(Wait(0.7));
	Steps.Add(Do([](const FEIPlayContext& Context) { StandBeforeDummies(Context, 180.0f); }));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("the second chain swing starts"), 1.0, [](const FEIPlayContext& Context) { return Context.Hero->GetSword()->GetSwingKind() == EEISwingKind::Light2; }));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("the third chain swing starts"), 1.0, [](const FEIPlayContext& Context) { return Context.Hero->GetSword()->GetSwingKind() == EEISwingKind::Light3; }));
	Steps.Add(WaitForSwingEnd());
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const int32 MiddleHits = SortedDummies(Context.World)[1]->GetHitCount();
		Context.Test->AddInfo(FString::Printf(TEXT("A chain of three hit the middle dummy %d times at 180 cm"), MiddleHits));
		Context.Test->TestEqual(TEXT("All three chain swings hit the dummy straight ahead"), MiddleHits, 3);
	}));

	// After a pause the chain starts over.
	Steps.Add(Wait(0.7));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.03));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("After a pause the chain starts over"), Context.Hero->GetSword()->GetSwingKind(), EEISwingKind::Light1);
	}));
	Steps.Add(WaitForSwingEnd());

	// Out of reach nothing happens.
	Steps.Add(Do([](const FEIPlayContext& Context) { StandBeforeDummies(Context, 500.0f); }));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.05));
	Steps.Add(WaitForSwingEnd());
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Out of reach nothing is hit"), SortedDummies(Context.World)[1]->GetHitCount(), 0);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEISwordHeavyTest,
	"Eisenseelen.Play.Sword.Heavy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEISwordHeavyTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;

	// Held long enough: a heavy swing with its own damage.
	Steps.Add(Do([](const FEIPlayContext& Context) { StandBeforeDummies(Context, 200.0f); }));
	Steps.Add(Wait(0.3));
	Steps.Add(Press(EKeys::LeftMouseButton));
	Steps.Add(WaitUntil(TEXT("the charge fills up"), 2.0, [](const FEIPlayContext& Context) { return Context.Hero->GetSword()->GetChargeFraction() >= 1.0f; }));
	Steps.Add(Release(EKeys::LeftMouseButton));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A full hold gives a heavy swing"), Context.Hero->GetSword()->GetSwingKind(), EEISwingKind::Heavy);
	}));
	Steps.Add(WaitForSwingEnd());
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		AEITrainingDummy* Middle = SortedDummies(Context.World)[1];
		Context.Test->AddInfo(FString::Printf(TEXT("A heavy swing at 200 cm: %.0f damage"), Middle->GetLastDamage()));
		Context.Test->TestEqual(TEXT("A heavy swing does its damage"), Middle->GetLastDamage(), Context.Hero->GetSword()->GetValues().Heavy.Damage);
	}));

	// Released too early: still a light swing.
	Steps.Add(Do([](const FEIPlayContext& Context) { ResetDummies(Context); }));
	Steps.Add(Press(EKeys::LeftMouseButton));
	Steps.Add(Wait(0.2));
	Steps.Add(Release(EKeys::LeftMouseButton));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("A short hold stays a light swing"), Context.Hero->GetSword()->GetSwingKind() != EEISwingKind::Heavy);
	}));
	Steps.Add(WaitForSwingEnd());
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIHeroDamageTest,
	"Eisenseelen.Play.HeroLife",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIHeroDamageTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		UEIHealthComponent* Health = Context.Hero->GetHealth();
		Context.Test->TestTrue(TEXT("The hero starts with full life"), Health->GetHealth() > 0.0f && Health->GetHealth() == Health->GetMaxHealth());
		const float Before = Health->GetHealth();
		Context.Hero->TakeDamage(10.0f, FDamageEvent(), nullptr, nullptr);
		Context.Test->TestEqual(TEXT("Damage lowers the life"), Health->GetHealth(), Before - 10.0f);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
