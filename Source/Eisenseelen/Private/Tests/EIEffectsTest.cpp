#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EIEffects.h"
#include "EIEnemy.h"
#include "EIHealthComponent.h"
#include "EILooks.h"
#include "EIProjectile.h"
#include "EIRuneComponent.h"
#include "EIRuneField.h"
#include "EITestScript.h"
#include "EngineUtils.h"

using namespace EITestScript;

namespace
{
	const FVector EffectFloor(-1500.0f, 0.0f, 92.0f);
	const TCHAR* NiagaraPath = TEXT("/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst");
	const TCHAR* MissingPath = TEXT("/Game/Kunst/Effekte/Fehlt_Effekt.Fehlt_Effekt");
	const TCHAR* MeshPath = TEXT("/Engine/BasicShapes/Cone.Cone");

	/** A tough enemy that stands still and does not strike back. */
	AEIEnemy* SpawnStill(const FEIPlayContext& Context, const FVector& Offset)
	{
		AEIEnemy* Enemy = Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Context.Hero->GetActorLocation() + Offset, FRotator(0.0f, 180.0f, 0.0f));
		FEIEnemyValues Still = Enemy->GetValues();
		Still.SpeedMetersPerSecond = 0.0f;
		Still.AttackFromMeters = 0.1f;
		Enemy->ApplyValues(Still);
		Enemy->GetHealth()->Reset(100000.0f);
		return Enemy;
	}

	/** The running effect of this name on the actor, if there is one. */
	AEIEffect* FindOn(const FEIPlayContext& Context, const AActor* Actor, FName Name)
	{
		for (TActorIterator<AEIEffect> It(Context.World); It; ++It)
		{
			if (It->GetAttachParentActor() == Actor && It->GetEffectName() == Name && !It->IsStopped())
			{
				return *It;
			}
		}
		return nullptr;
	}

	void AddModel(FEILookTable& Table, const FString& Key, const TCHAR* Model)
	{
		FEILookEntry Entry;
		Entry.Model = Model;
		Entry.bKeepGlowParts = false;
		Table.Entries.Add(Key, Entry);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIEffectsTest,
	"Eisenseelen.Play.Effects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIEffectsTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TArray<TWeakObjectPtr<AEIEffect>> All;
		TWeakObjectPtr<AEIEnemy> Target;
		TWeakObjectPtr<AEIEnemy> Neighbour;
		TWeakObjectPtr<AEIEffect> WithModel;
		TWeakObjectPtr<AEIEffect> Missing;
		TWeakObjectPtr<AEIEffect> NoEffect;
		FEILookTable Models;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	AddExpectedErrorPlain(TEXT("/Game/Kunst/Effekte/Fehlt_Effekt"), EAutomationExpectedErrorFlags::Contains, 0);
	AddExpectedErrorPlain(TEXT("ist kein Niagara-Effekt"), EAutomationExpectedErrorFlags::Contains, 0);
	AddModel(State->Models, TEXT("effekt.splitterbruch"), NiagaraPath);
	AddModel(State->Models, TEXT("effekt.zustand_brennend"), MissingPath);
	AddModel(State->Models, TEXT("effekt.truhe_siegel"), MeshPath);

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		Teleport(Context, EffectFloor, 0.0f);
	}));
	Steps.Add(Wait(0.2));

	// Every effect plays with its glowing placeholder, as long as there is no art for it.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIEffect::ResetPlayCounts();
		FEIEffectParams Params;
		Params.Location = Context.Hero->GetActorLocation() + FVector(400.0f, 0.0f, 0.0f);
		Params.Target = Params.Location + FVector(0.0f, 300.0f, 200.0f);
		Params.RadiusCm = 150.0f;
		Params.AngleDegrees = 60.0f;
		Params.Color = FLinearColor(1.0f, 0.5f, 0.1f);
		for (const FName& Name : AEIEffect::GetNames())
		{
			AEIEffect* Effect = AEIEffect::Play(Context.World, Name, Params);
			if (Context.Test->TestNotNull(*FString::Printf(TEXT("%s plays"), *Name.ToString()), Effect))
			{
				Context.Test->TestFalse(*FString::Printf(TEXT("%s has no art yet"), *Name.ToString()), Effect->UsesModel());
				State->All.Add(Effect);
			}
			Context.Test->TestEqual(*FString::Printf(TEXT("%s is counted"), *Name.ToString()), AEIEffect::GetPlayCount(Name), 1);
		}
		Context.Test->TestNull(TEXT("An unknown effect does not play"), AEIEffect::Play(Context.World, TEXT("regenbogen"), Params));
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		for (const TWeakObjectPtr<AEIEffect>& Effect : State->All)
		{
			if (Context.Test->TestTrue(TEXT("An effect is still there after a moment"), Effect.IsValid()))
			{
				Context.Test->TestTrue(*FString::Printf(TEXT("%s shows something"), *Effect->GetEffectName().ToString()), Effect->CountShownParts() > 0);
				if (Effect->IsLasting())
				{
					Effect->Stop();
				}
			}
		}
	}));
	Steps.Add(WaitUntil(TEXT("every effect is gone again"), 10.0, [State](const FEIPlayContext& Context)
	{
		return !State->All.ContainsByPredicate([](const TWeakObjectPtr<AEIEffect>& Effect) { return Effect.IsValid(); });
	}));

	// The reactions show.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIEffect::ResetPlayCounts();
		AEIEnemy* Target = SpawnStill(Context, FVector(300.0f, 0.0f, 0.0f));
		AEIEnemy* Neighbour = SpawnStill(Context, FVector(300.0f, 200.0f, 0.0f));
		State->Target = Target;
		State->Neighbour = Neighbour;
		UEIRuneComponent* Runes = Context.Hero->GetRunes();
		Runes->HitWithElement(Target, EEIElement::Lightning);
		Runes->HitWithElement(Target, EEIElement::Fire);
		Context.Test->TestTrue(TEXT("Sparks fly to the target and its neighbour"), AEIEffect::GetPlayCount(TEXT("funkenflug")) >= 2);
		Target->Freeze(5.0f);
		Runes->HitWithElement(Target, EEIElement::Lightning);
		Context.Test->TestEqual(TEXT("The frozen shatters"), AEIEffect::GetPlayCount(TEXT("splitterbruch")), 1);
		Runes->HitWithElement(Target, EEIElement::Fire);
		Runes->HitWithElement(Target, EEIElement::Ice);
		Context.Test->TestEqual(TEXT("Steam bursts"), AEIEffect::GetPlayCount(TEXT("dampfexplosion")), 1);
		Target->Freeze(0.5f);
		Runes->HitWithElement(Neighbour, EEIElement::Ice);
	}));
	Steps.Add(Wait(0.15));

	// The states show on the body and go with them.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		if (!State->Target.IsValid() || !State->Neighbour.IsValid())
		{
			Context.Test->AddError(TEXT("The enemies are gone"));
			return;
		}
		Context.Test->TestTrue(TEXT("The target burns"), State->Target->IsBurning());
		Context.Test->TestNotNull(TEXT("Its fire shows on it"), FindOn(Context, State->Target.Get(), TEXT("zustand_brennend")));
		Context.Test->TestNotNull(TEXT("Its frost shows on it"), FindOn(Context, State->Target.Get(), TEXT("zustand_gefroren")));
		Context.Test->TestNotNull(TEXT("The ice mark shows on the neighbour"), FindOn(Context, State->Neighbour.Get(), TEXT("mal_eis")));
		Context.Test->TestNull(TEXT("No other mark shows on it"), FindOn(Context, State->Neighbour.Get(), TEXT("mal_feuer")));
	}));
	Steps.Add(WaitUntil(TEXT("the states and their effects end"), 10.0, [State](const FEIPlayContext& Context)
	{
		return State->Target.IsValid() && State->Neighbour.IsValid()
			&& !State->Target->IsBurning() && !State->Target->IsFrozen() && !State->Neighbour->IsBurning()
			&& FindOn(Context, State->Target.Get(), TEXT("zustand_brennend")) == nullptr
			&& FindOn(Context, State->Target.Get(), TEXT("zustand_gefroren")) == nullptr
			&& FindOn(Context, State->Neighbour.Get(), TEXT("mal_eis")) == nullptr;
	}));

	// The rune of the hero lays its field, and the field shows its element.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		AEIEffect::ResetPlayCounts();
		Context.Test->TestNotNull(TEXT("A rune is equipped"), Context.Hero->GetRunes()->GetRune());
	}));
	AddTap(Steps, EKeys::E);
	Steps.Add(WaitUntil(TEXT("the rune field shows"), 1.0, [](const FEIPlayContext& Context)
	{
		for (TActorIterator<AEIRuneField> It(Context.World); It; ++It)
		{
			if (FindOn(Context, *It, It->GetElement() == EEIElement::Ice ? FName(TEXT("frostnova")) : FName(TEXT("flammenwand"))) != nullptr
				|| FindOn(Context, *It, TEXT("gewitterfeld")) != nullptr || FindOn(Context, *It, TEXT("windhose")) != nullptr)
			{
				return true;
			}
		}
		return false;
	}));

	// A fireball trails fire and bursts on whom it hits.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIEffect::ResetPlayCounts();
		const FVector From = Context.Hero->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f);
		const FVector To = State->Target->GetActorLocation();
		const FTransform Spawn((To - From).Rotation(), From);
		AEIProjectile* Ball = Context.World->SpawnActorDeferred<AEIProjectile>(AEIProjectile::StaticClass(), Spawn, Context.Hero, Context.Hero, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		FEIProjectileSetup Setup;
		Setup.Damage = 1.0f;
		Setup.SpeedCm = 3000.0f;
		Setup.bBig = true;
		Ball->Configure(Setup);
		Ball->FinishSpawning(Spawn);
		Context.Test->TestEqual(TEXT("The fireball trails fire"), AEIEffect::GetPlayCount(TEXT("feuerball")), 1);
	}));
	Steps.Add(WaitUntil(TEXT("the fireball bursts"), 2.0, [](const FEIPlayContext& Context) { return AEIEffect::GetPlayCount(TEXT("feuerball_explosion")) == 1; }));

	// A burning enemy that goes takes its fire along.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->Neighbour->ApplyBurn(1.0f, 5.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the neighbour burns"), 1.0, [State](const FEIPlayContext& Context) { return FindOn(Context, State->Neighbour.Get(), TEXT("zustand_brennend")) != nullptr; }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->All.Reset();
		for (TActorIterator<AEIEffect> It(Context.World); It; ++It)
		{
			if (It->GetAttachParentActor() == State->Neighbour.Get())
			{
				State->All.Add(*It);
			}
		}
		State->Neighbour->Destroy();
		State->Target->Destroy();
	}));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		for (const TWeakObjectPtr<AEIEffect>& Effect : State->All)
		{
			Context.Test->TestTrue(TEXT("The effects of a gone enemy stop"), !Effect.IsValid() || Effect->IsStopped());
		}
	}));

	// Art from Data/aussehen.json plays instead, and a wrong path keeps the placeholder.
	// The art is loaded first, so that loading does not stall the frames in which the effects are looked at.
	Steps.Add(Do([](const FEIPlayContext& Context) { LoadObject<UObject>(nullptr, NiagaraPath); }));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIEffect::ResetPlayCounts();
		FEIEffectParams Params;
		Params.Location = Context.Hero->GetActorLocation() + FVector(400.0f, 0.0f, 0.0f);
		Params.Target = Params.Location + FVector(0.0f, 300.0f, 0.0f);
		FEILookTable::SetTestOverride(&State->Models);
		State->WithModel = AEIEffect::Play(Context.World, TEXT("splitterbruch"), Params);
		// A lasting one, because loading the art may stall a frame longer than a short burst lasts.
		State->Missing = AEIEffect::Play(Context.World, TEXT("zustand_brennend"), Params);
		State->NoEffect = AEIEffect::Play(Context.World, TEXT("truhe_siegel"), Params);
		FEILookTable::SetTestOverride(nullptr);
	}));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		if (Context.Test->TestTrue(TEXT("The effect with art plays"), State->WithModel.IsValid()))
		{
			Context.Test->TestTrue(TEXT("It uses the Niagara effect"), State->WithModel->UsesModel());
			Context.Test->TestTrue(TEXT("It hides the placeholder"), State->WithModel->HidesPlaceholder());
			Context.Test->TestEqual(TEXT("No glowing parts show beside the art"), State->WithModel->CountShownParts(), 0);
		}
		if (Context.Test->TestTrue(TEXT("The effect with a missing path plays"), State->Missing.IsValid()))
		{
			Context.Test->TestFalse(TEXT("A missing path keeps the placeholder"), State->Missing->UsesModel());
			Context.Test->TestTrue(TEXT("The placeholder shows"), State->Missing->CountShownParts() > 0);
			State->Missing->Stop();
		}
		if (Context.Test->TestTrue(TEXT("The effect with a mesh path plays"), State->NoEffect.IsValid()))
		{
			Context.Test->TestFalse(TEXT("A mesh is no effect"), State->NoEffect->UsesModel());
		}
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
