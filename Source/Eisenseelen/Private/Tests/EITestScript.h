#pragma once

#include "CoreMinimal.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EIPlayerController.h"
#include "EITestKeys.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

/** What a test step works with. Everything is valid while a step runs. */
struct FEIPlayContext
{
	FAutomationTestBase* Test = nullptr;
	UWorld* World = nullptr;
	AEIPlayerController* Player = nullptr;
	AEIHeroCharacter* Hero = nullptr;
	UEIHeroMovementComponent* Movement = nullptr;
	double Now = 0.0;
};

/** One step of a play test. It runs every frame until it returns true. */
using FEIPlayStep = TFunction<bool(const FEIPlayContext&)>;

/** Runs steps in order in the PIE world, once the hero stands on the ground. */
class FEIPlayScriptCommand : public IAutomationLatentCommand
{
public:
	FEIPlayScriptCommand(FAutomationTestBase* InTest, TArray<FEIPlayStep> InSteps)
		: Test(InTest), Steps(MoveTemp(InSteps)), StartedAt(FPlatformTime::Seconds())
	{
	}

	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - StartedAt > TimeoutSeconds)
		{
			Test->AddError(FString::Printf(TEXT("The play test timed out in step %d"), Index));
			return true;
		}
		FEIPlayContext Context;
		Context.Test = Test;
		Context.World = GEditor != nullptr ? GEditor->PlayWorld.Get() : nullptr;
		Context.Player = Context.World != nullptr ? Cast<AEIPlayerController>(Context.World->GetFirstPlayerController()) : nullptr;
		Context.Hero = Context.Player != nullptr ? Cast<AEIHeroCharacter>(Context.Player->GetPawn()) : nullptr;
		if (Context.Hero == nullptr || !Context.Hero->HasActorBegunPlay())
		{
			return false;
		}
		Context.Movement = Context.Hero->GetHeroMovement();
		Context.Now = Context.World->GetTimeSeconds();
		if (!bStood)
		{
			bStood = Context.Movement->IsMovingOnGround();
			return false;
		}
		if (Steps[Index](Context))
		{
			++Index;
		}
		return Index >= Steps.Num();
	}

private:
	static constexpr double TimeoutSeconds = 240.0;

	FAutomationTestBase* Test;
	TArray<FEIPlayStep> Steps;
	double StartedAt;
	int32 Index = 0;
	bool bStood = false;
};

namespace EITestScript
{
	inline const TCHAR* ArenaMap = TEXT("/Game/Maps/L_Testarena");
	inline const TCHAR* RunMap = TEXT("/Game/Maps/L_ErsterLauf");

	inline FEIPlayStep Wait(double Seconds)
	{
		return [Seconds, StartedAt = -1.0](const FEIPlayContext& Context) mutable
		{
			if (StartedAt < 0.0)
			{
				StartedAt = Context.Now;
			}
			return Context.Now - StartedAt >= Seconds;
		};
	}

	/** Runs Action once and goes on. */
	inline FEIPlayStep Do(TFunction<void(const FEIPlayContext&)> Action)
	{
		return [Action = MoveTemp(Action)](const FEIPlayContext& Context)
		{
			Action(Context);
			return true;
		};
	}

	/** Waits until Condition holds, for the time it may take. Running out of that time fails the test. */
	inline FEIPlayStep WaitUntil(const TCHAR* What, double MaxSeconds, TFunction<bool(const FEIPlayContext&)> Condition)
	{
		return [What, MaxSeconds, Condition = MoveTemp(Condition), StartedAt = -1.0](const FEIPlayContext& Context) mutable
		{
			if (StartedAt < 0.0)
			{
				StartedAt = Context.Now;
			}
			if (Condition(Context))
			{
				return true;
			}
			if (Context.Now - StartedAt > MaxSeconds)
			{
				Context.Test->AddError(FString::Printf(TEXT("Did not happen in time: %s"), What));
				return true;
			}
			return false;
		};
	}

	inline void Teleport(const FEIPlayContext& Context, const FVector& Location, float Yaw)
	{
		Context.Hero->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		Context.Movement->StopMovementImmediately();
		Context.Player->SetControlRotation(FRotator(0.0f, Yaw, 0.0f));
	}

	inline FEIPlayStep Press(const FKey& Key)
	{
		return Do([Key](const FEIPlayContext& Context) { EISendTestKey(Context.Player, Key, IE_Pressed); });
	}

	inline FEIPlayStep Release(const FKey& Key)
	{
		return Do([Key](const FEIPlayContext& Context) { EISendTestKey(Context.Player, Key, IE_Released); });
	}

	/** A short tap, with the frames Enhanced Input needs to see both edges. */
	inline void AddTap(TArray<FEIPlayStep>& Steps, const FKey& Key)
	{
		Steps.Add(Press(Key));
		Steps.Add(Wait(0.03));
		Steps.Add(Release(Key));
	}

	inline FEIPlayStep WaitForGround()
	{
		return WaitUntil(TEXT("the hero lands"), 6.0, [](const FEIPlayContext& Context) { return Context.Movement->IsMovingOnGround(); });
	}

	inline bool RunInMap(FAutomationTestBase* Test, const TCHAR* Map, TArray<FEIPlayStep> Steps)
	{
		if (!AutomationOpenMap(Map))
		{
			Test->AddError(FString::Printf(TEXT("Cannot open %s for PIE"), Map));
			return false;
		}
		ADD_LATENT_AUTOMATION_COMMAND(FEIPlayScriptCommand(Test, MoveTemp(Steps)));
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
		return true;
	}

	inline bool RunInArena(FAutomationTestBase* Test, TArray<FEIPlayStep> Steps)
	{
		return RunInMap(Test, ArenaMap, MoveTemp(Steps));
	}
}
