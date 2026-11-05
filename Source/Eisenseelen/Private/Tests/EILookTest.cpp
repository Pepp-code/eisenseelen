#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "EIAltar.h"
#include "EIAnchorStone.h"
#include "EIChest.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIEnemyCamp.h"
#include "EIForge.h"
#include "EIHealthComponent.h"
#include "EIHeroBodyComponent.h"
#include "EIHuntBoard.h"
#include "EILaunchPad.h"
#include "EILookComponent.h"
#include "EILooks.h"
#include "EISignalFire.h"
#include "EISkyBarge.h"
#include "EISoulKettle.h"
#include "EITestScript.h"
#include "EITrainingDummy.h"
#include "EIViewModelComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Guid.h"

using namespace EITestScript;

namespace
{
	const TCHAR* ConePath = TEXT("/Engine/BasicShapes/Cone.Cone");
	const TCHAR* SkeletalCubePath = TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube");
	const TCHAR* OverlayPath = TEXT("/Game/Materials/M_Ueberzug.M_Ueberzug");
	const FVector OpenFloor(-1500.0f, 0.0f, 92.0f);
	// Far from everything in the arena, so the places stand alone and nobody walks into them.
	const FVector PlacesStart(-1500.0f, 6000.0f, 0.0f);
	constexpr float PlaceSpacingCm = 1200.0f;

	void AddModel(FEILookTable& Table, const FString& Key, const TCHAR* Model, bool bKeepGlowParts = true)
	{
		FEILookEntry Entry;
		Entry.Model = Model;
		Entry.bKeepGlowParts = bKeepGlowParts;
		Table.Entries.Add(Key, MoveTemp(Entry));
	}

	UStaticMeshComponent* FindPart(const AActor* Actor, const TCHAR* Name)
	{
		TInlineComponentArray<UStaticMeshComponent*> Parts(Actor);
		for (UStaticMeshComponent* Part : Parts)
		{
			if (Part->GetFName() == Name)
			{
				return Part;
			}
		}
		return nullptr;
	}

	bool IsHidden(const AActor* Actor, const TCHAR* Name)
	{
		const UStaticMeshComponent* Part = FindPart(Actor, Name);
		return Part != nullptr && Part->bHiddenInGame;
	}

	bool ShowsMesh(const UEILookComponent* Look, const TCHAR* Path)
	{
		const UStaticMeshComponent* Static = Look != nullptr ? Cast<UStaticMeshComponent>(Look->GetModel()) : nullptr;
		return Static != nullptr && Static->GetStaticMesh() != nullptr && Static->GetStaticMesh()->GetPathName() == Path;
	}

	/** The names the file must know, read from the files that name them. */
	TArray<FString> ReadNames(FAutomationTestBase* Test, const TCHAR* FileName, const TCHAR* Section, const TCHAR* Prefix, const TCHAR* SkipSlot = nullptr)
	{
		TArray<FString> Names;
		FString Error;
		const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(FileName, Error);
		if (!Test->TestTrue(FString::Printf(TEXT("Data/%s loads: %s"), FileName, *Error), Root.IsValid()))
		{
			return Names;
		}
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (Root->TryGetArrayField(Section, List))
		{
			for (const TSharedPtr<FJsonValue>& Value : *List)
			{
				const TSharedPtr<FJsonObject>* Item = nullptr;
				FString Name;
				FString Slot;
				if (Value->TryGetObject(Item) && (*Item)->TryGetStringField(TEXT("name"), Name)
					&& !(SkipSlot != nullptr && (*Item)->TryGetStringField(TEXT("platz"), Slot) && Slot == SkipSlot))
				{
					Names.Add(Prefix + Name);
				}
			}
		}
		else if (Root->TryGetObjectField(Section, Object))
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Object)->Values)
			{
				if (!Pair.Key.StartsWith(TEXT("_")))
				{
					Names.Add(Prefix + Pair.Key);
				}
			}
		}
		Test->TestTrue(FString::Printf(TEXT("Data/%s names something in %s"), FileName, Section), !Names.IsEmpty());
		return Names;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEILookTableTest,
	"Eisenseelen.Data.Looks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEILookTableTest::RunTest(const FString& Parameters)
{
	// Paths as the editor copies them all end up as full object paths.
	TestEqual(TEXT("A package path gets its object name"), FEILookTable::NormalizeObjectPath(TEXT("/Game/Kunst/SK_Laeufer")), FString(TEXT("/Game/Kunst/SK_Laeufer.SK_Laeufer")));
	TestEqual(TEXT("Copy Reference works"), FEILookTable::NormalizeObjectPath(TEXT("SkeletalMesh'/Game/Kunst/SK_Laeufer.SK_Laeufer'")), FString(TEXT("/Game/Kunst/SK_Laeufer.SK_Laeufer")));
	TestEqual(TEXT("A blueprint path gets its class"), FEILookTable::NormalizeClassPath(TEXT("/Game/Kunst/ABP_Laeufer")), FString(TEXT("/Game/Kunst/ABP_Laeufer.ABP_Laeufer_C")));
	TestTrue(TEXT("A Windows path is refused"), FEILookTable::NormalizeObjectPath(TEXT("C:\\Kunst\\Laeufer.fbx")).IsEmpty());
	TestTrue(TEXT("A folder is refused"), FEILookTable::NormalizeObjectPath(TEXT("/Game/Kunst/")).IsEmpty());

	FEILookTable Table;
	FString Error;
	const bool bLoaded = FEILookTable::LoadFromString(TEXT(R"json({ "aussehen": {
		"_hinweis": "wird uebersprungen",
		"gegner.knochenlaeufer": {
			"modell": "/Game/Kunst/SK_Laeufer", "skalierung": 2, "versatz_cm": [1, 2, 3], "drehung_grad": [10, 20, -90],
			"anim_blueprint": "/Game/Kunst/ABP_Laeufer", "animationen": { "laufen": "/Game/Kunst/A_Laufen", "tod": "" },
			"sockel": "hand_r", "leuchtteile_behalten": false
		},
		"ort.truhe": { "modell": "" },
		"boss.Der Gefesselte": {}
	} })json"), Table, Error);
	TestTrue(*FString::Printf(TEXT("A full entry loads: %s"), *Error), bLoaded);
	const FEILookEntry* Runner = Table.Find(TEXT("gegner.knochenlaeufer"));
	if (TestNotNull(TEXT("The runner is in the table"), Runner))
	{
		TestEqual(TEXT("Model"), Runner->Model, FString(TEXT("/Game/Kunst/SK_Laeufer.SK_Laeufer")));
		TestEqual(TEXT("One number scales all three ways"), Runner->Scale, FVector(2.0));
		TestEqual(TEXT("Offset"), Runner->OffsetCm, FVector(1.0, 2.0, 3.0));
		TestEqual(TEXT("The angles are X, Y, Z as in the editor"), Runner->Rotation, FRotator(20.0, -90.0, 10.0));
		TestEqual(TEXT("Anim blueprint"), Runner->AnimBlueprint, FString(TEXT("/Game/Kunst/ABP_Laeufer.ABP_Laeufer_C")));
		TestEqual(TEXT("An empty animation is left out"), Runner->Animations.Num(), 1);
		TestEqual(TEXT("Socket"), Runner->Socket, FName(TEXT("hand_r")));
		TestFalse(TEXT("Glow parts go"), Runner->bKeepGlowParts);
	}
	TestFalse(TEXT("An empty model keeps the blocks"), Table.Find(TEXT("ort.truhe")) != nullptr && Table.Find(TEXT("ort.truhe"))->HasModel());
	TestNull(TEXT("Notes are no entries"), Table.Find(TEXT("_hinweis")));
	FString Key;
	TestTrue(TEXT("The first key with a model wins"), Table.FindFirstModel({TEXT("ort.truhe"), TEXT("gegner.knochenlaeufer")}, &Key) == Runner && Key == TEXT("gegner.knochenlaeufer"));

	const TArray<TPair<const TCHAR*, const TCHAR*>> Broken = {
		{TEXT("an unknown enemy"), TEXT(R"json({ "aussehen": { "gegner.drache": { "modell": "" } } })json")},
		{TEXT("an unknown place"), TEXT(R"json({ "aussehen": { "ort.turm": { "modell": "" } } })json")},
		{TEXT("an unknown prefix"), TEXT(R"json({ "aussehen": { "waffe.Frostklinge": { "modell": "" } } })json")},
		{TEXT("an unknown field"), TEXT(R"json({ "aussehen": { "ort.truhe": { "model": "" } } })json")},
		{TEXT("an unknown moment"), TEXT(R"json({ "aussehen": { "ort.truhe": { "animationen": { "tanzen": "/Game/A" } } } })json")},
		{TEXT("a path outside the project"), TEXT(R"json({ "aussehen": { "ort.truhe": { "modell": "D:/Kunst/Truhe.fbx" } } })json")},
		{TEXT("a scale of zero"), TEXT(R"json({ "aussehen": { "ort.truhe": { "skalierung": 0 } } })json")},
		{TEXT("two numbers for an offset"), TEXT(R"json({ "aussehen": { "ort.truhe": { "versatz_cm": [1, 2] } } })json")},
		{TEXT("a missing section"), TEXT(R"json({ "looks": {} })json")}};
	for (const TPair<const TCHAR*, const TCHAR*>& Case : Broken)
	{
		FEILookTable Kept = Table;
		FString CaseError;
		TestFalse(*FString::Printf(TEXT("The file refuses %s"), Case.Key), FEILookTable::LoadFromString(Case.Value, Kept, CaseError));
		TestFalse(*FString::Printf(TEXT("It says why it refuses %s"), Case.Key), CaseError.IsEmpty());
		TestEqual(*FString::Printf(TEXT("The table stays as it was after %s"), Case.Key), Kept.Entries.Num(), Table.Entries.Num());
	}

	TestEqual(TEXT("Running falls back to walking"), FEILookTable::GetFallback(TEXT("rennen")), FName(TEXT("laufen")));
	TestEqual(TEXT("Walking falls back to standing"), FEILookTable::GetFallback(TEXT("laufen")), FName(TEXT("stehen")));
	TestEqual(TEXT("A heavy blow falls back to a blow"), FEILookTable::GetFallback(TEXT("schwerer_hieb")), FName(TEXT("hieb")));
	TestTrue(TEXT("Standing is the end of the line"), FEILookTable::GetFallback(TEXT("stehen")).IsNone());
	for (const FName& Event : FEILookTable::GetEvents())
	{
		for (FName Current = FEILookTable::GetFallback(Event); !Current.IsNone(); Current = FEILookTable::GetFallback(Current))
		{
			TestTrue(*FString::Printf(TEXT("%s falls back to a known moment"), *Event.ToString()), FEILookTable::GetEvents().Contains(Current));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEILookFileTest,
	"Eisenseelen.Data.LookFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEILookFileTest::RunTest(const FString& Parameters)
{
	FEILookTable Table;
	FString Error;
	if (!TestTrue(*FString::Printf(TEXT("Data/aussehen.json loads: %s"), *Error), FEILookTable::LoadFromProjectFile(Table, Error)))
	{
		return false;
	}

	// Every thing in the game has a line, so whoever adds a model only fills in a path.
	TArray<FString> Required;
	for (const FString& ClassName : FEILookTable::GetClassNames())
	{
		Required.Add(TEXT("held.") + ClassName);
		Required.Add(TEXT("hand.") + ClassName);
	}
	for (int32 Index = 0; Index < EIEnemyKindCount; ++Index)
	{
		Required.Add(FString(TEXT("gegner.")) + EIEnemyKindKey(static_cast<EEIEnemyKind>(Index)));
	}
	for (const FString& Place : FEILookTable::GetPlaceNames())
	{
		Required.Add(TEXT("ort.") + Place);
	}
	TArray<FString> Named;
	Named.Append(ReadNames(this, TEXT("bosse.json"), TEXT("bosse"), TEXT("boss.")));
	// Runes show as their spell, not as a thing in the hand.
	Named.Append(ReadNames(this, TEXT("items.json"), TEXT("items"), TEXT("item."), TEXT("rune")));
	Named.Append(ReadNames(this, TEXT("waffen.json"), TEXT("grundformen"), TEXT("form.")));
	Required.Append(Named);
	for (const FString& Key : Required)
	{
		TestTrue(*FString::Printf(TEXT("Data/aussehen.json has a line for %s"), *Key), Table.Entries.Contains(Key));
	}

	for (const TPair<FString, FEILookEntry>& Pair : Table.Entries)
	{
		const FString& Key = Pair.Key;
		const FEILookEntry& Entry = Pair.Value;
		if (Key.StartsWith(TEXT("boss.")) || Key.StartsWith(TEXT("item.")) || Key.StartsWith(TEXT("form.")))
		{
			TestTrue(*FString::Printf(TEXT("%s names something that is in the game"), *Key), Named.ContainsByPredicate([&Key](const FString& Name) { return Name.Equals(Key, ESearchCase::CaseSensitive); }));
		}
		if (Entry.HasModel())
		{
			const UObject* Model = LoadObject<UObject>(nullptr, *Entry.Model);
			TestTrue(*FString::Printf(TEXT("%s: %s is a model"), *Key, *Entry.Model), Cast<UStaticMesh>(Model) != nullptr || Cast<USkeletalMesh>(Model) != nullptr);
		}
		if (!Entry.AnimBlueprint.IsEmpty())
		{
			TestNotNull(*FString::Printf(TEXT("%s: %s is an animation blueprint"), *Key, *Entry.AnimBlueprint), LoadClass<UAnimInstance>(nullptr, *Entry.AnimBlueprint));
		}
		for (const TPair<FName, FString>& Animation : Entry.Animations)
		{
			TestNotNull(*FString::Printf(TEXT("%s: %s is an animation"), *Key, *Animation.Value), LoadObject<UAnimSequenceBase>(nullptr, *Animation.Value));
		}
	}

	const UMaterialInterface* Overlay = LoadObject<UMaterialInterface>(nullptr, OverlayPath);
	if (TestNotNull(TEXT("M_Ueberzug exists"), Overlay))
	{
		float Strength = -1.0f;
		FLinearColor Color;
		TestTrue(TEXT("M_Ueberzug has Staerke"), Overlay->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Staerke")), Strength));
		TestTrue(TEXT("M_Ueberzug has Farbe"), Overlay->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Farbe")), Color));
		TestEqual(TEXT("M_Ueberzug is off by default"), Strength, 0.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEILookFiguresTest,
	"Eisenseelen.Play.Looks.Figures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEILookFiguresTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		FEILookTable Models;
		FEILookTable Empty;
		TWeakObjectPtr<AEIEnemy> Runner;
		TWeakObjectPtr<AEIEnemy> Thrower;
		TWeakObjectPtr<AEITrainingDummy> Dummy;
		EEIHeroClass StartClass = EEIHeroClass::Bladewarden;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	// A unique name, so the missing model is reported in this run even if an earlier run reported another.
	const FString Unique = FGuid::NewGuid().ToString();
	const FString Missing = FString::Printf(TEXT("/Game/Kunst/Fehlt_%s.Fehlt_%s"), *Unique, *Unique);
	AddExpectedErrorPlain(TEXT("/Game/Kunst/Fehlt_"), EAutomationExpectedErrorFlags::Contains, 0);

	AddModel(State->Models, TEXT("gegner.knochenlaeufer"), SkeletalCubePath);
	AddModel(State->Models, TEXT("gegner.knochenwerfer"), *Missing);
	AddModel(State->Models, TEXT("ort.trainingspuppe"), ConePath);
	for (const FString& ClassName : FEILookTable::GetClassNames())
	{
		AddModel(State->Models, TEXT("held.") + ClassName, SkeletalCubePath, false);
		AddModel(State->Models, TEXT("hand.") + ClassName, ConePath, false);
	}
	for (const TCHAR* Form : {TEXT("langschwert"), TEXT("zweihaender"), TEXT("kurzschwert"), TEXT("stab"), TEXT("zepter")})
	{
		AddModel(State->Models, FString(TEXT("form.")) + Form, ConePath, false);
	}

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, 0.0f); }));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		// The table only counts while things are built, so it is set back right away.
		FEILookTable::SetTestOverride(&State->Models);
		const FVector Ahead = Context.Hero->GetActorLocation() + FVector(600.0f, 0.0f, 0.0f);
		for (const bool bThrower : {false, true})
		{
			const FTransform Spawn(FRotator(0.0f, 180.0f, 0.0f), Ahead + FVector(0.0f, bThrower ? 300.0f : -300.0f, 0.0f));
			AEIEnemy* Enemy = Context.World->SpawnActorDeferred<AEIEnemy>(AEIEnemy::StaticClass(), Spawn);
			Enemy->SetKind(bThrower ? EEIEnemyKind::Thrower : EEIEnemyKind::Runner);
			Enemy->FinishSpawning(Spawn);
			FEIEnemyValues Still = Enemy->GetValues();
			Still.SpeedMetersPerSecond = 0.0f;
			Still.AttackFromMeters = 0.1f;
			Enemy->ApplyValues(Still);
			(bThrower ? State->Thrower : State->Runner) = Enemy;
		}
		State->Dummy = Context.World->SpawnActor<AEITrainingDummy>(AEITrainingDummy::StaticClass(), Ahead + FVector(400.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		State->StartClass = Context.Hero->GetHeroClass();
		Context.Hero->SetHeroClass(State->StartClass == EEIHeroClass::Arcanist ? EEIHeroClass::Bladewarden : EEIHeroClass::Arcanist);
		FEILookTable::SetTestOverride(nullptr);
	}));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		FAutomationTestBase* Test = Context.Test;
		AEIEnemy* Runner = State->Runner.Get();
		if (Test->TestNotNull(TEXT("The runner is there"), Runner))
		{
			UEILookComponent* Look = Runner->GetLook();
			Test->TestNotNull(TEXT("The runner shows its model with a skeleton"), Look->GetSkeletalModel());
			Test->TestEqual(TEXT("It comes from its own line"), Look->GetModelKey(), FString(TEXT("gegner.knochenlaeufer")));
			Test->TestTrue(TEXT("Its body block is hidden"), IsHidden(Runner, TEXT("Body")));
			Test->TestTrue(TEXT("Its head block is hidden"), IsHidden(Runner, TEXT("Head")));
			Test->TestTrue(TEXT("It is still hit by its capsule"), Runner->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
			if (UMeshComponent* Model = Look->GetModel())
			{
				Test->TestEqual(TEXT("The model blocks nothing"), Model->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
				Look->SetStatus(FLinearColor::Red, 1.0f);
				Test->TestNotNull(TEXT("A warning lays a shimmer over the model"), Model->GetOverlayMaterial());
				Look->SetStatus(FLinearColor::Red, 0.0f);
				Test->TestNull(TEXT("Without a warning the shimmer goes"), Model->GetOverlayMaterial());
			}
			Test->TestFalse(TEXT("Without a death animation there is no body left behind"), Look->LeaveCorpse(1.0f));
		}

		AEIEnemy* Thrower = State->Thrower.Get();
		if (Test->TestNotNull(TEXT("The thrower is there"), Thrower))
		{
			Test->TestFalse(TEXT("A missing model shows nothing"), Thrower->GetLook()->HasModel());
			Test->TestFalse(TEXT("A missing model keeps the blocks"), IsHidden(Thrower, TEXT("Body")));
		}

		AEITrainingDummy* Dummy = State->Dummy.Get();
		if (Test->TestNotNull(TEXT("The dummy is there"), Dummy))
		{
			UEILookComponent* Look = Dummy->FindComponentByClass<UEILookComponent>();
			Test->TestTrue(TEXT("The dummy shows a model without a skeleton"), ShowsMesh(Look, ConePath));
			Test->TestTrue(TEXT("Its body block is hidden"), IsHidden(Dummy, TEXT("Body")));
		}

		UEIHeroBodyComponent* Body = Context.Hero->GetBody();
		if (Test->TestNotNull(TEXT("The hero has a body for the others"), Body))
		{
			const USkeletalMeshComponent* Model = Body->GetLook()->GetSkeletalModel();
			if (Test->TestNotNull(TEXT("The body shows the class model"), Model))
			{
				Test->TestTrue(TEXT("The own body is hidden from the own camera"), Model->bOwnerNoSee);
				Test->TestTrue(TEXT("The body is visible to the others"), Model->IsVisible());
			}
			Test->TestTrue(TEXT("The body holds the weapon model"), ShowsMesh(Body->GetWeaponLook(), ConePath));
		}
		UEIViewModelComponent* View = Context.Hero->GetViewModel();
		if (Test->TestNotNull(TEXT("The hero has a hand in front of the camera"), View))
		{
			Test->TestTrue(TEXT("The hand shows its model"), ShowsMesh(View->GetHandLook(), ConePath));
			Test->TestTrue(TEXT("The hand holds the weapon model"), ShowsMesh(View->GetWeaponLook(), ConePath));
			const UMeshComponent* Hand = View->GetHandLook()->GetModel();
			Test->TestTrue(TEXT("Only the own camera sees the hand"), Hand != nullptr && Hand->bOnlyOwnerSee);
		}

		// The model is only seen, the enemy still dies as before. It is gone a moment later, so it is checked right away.
		if (Runner != nullptr)
		{
			Runner->TakeDamage(Runner->GetHealth()->GetHealth() * 10.0f, FDamageEvent(), nullptr, Context.Hero);
			Test->TestTrue(TEXT("The runner dies"), Runner->IsDead());
			Test->TestFalse(TEXT("The killed runner's model is gone"), Runner->GetLook()->IsVisible());
		}
		FEILookTable::SetTestOverride(&State->Empty);
		Context.Hero->SetHeroClass(State->StartClass);
		FEILookTable::SetTestOverride(nullptr);
	}));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("Back in the old class the blocks are back"), Context.Hero->GetBody()->GetLook()->HasModel());
		Context.Test->TestFalse(TEXT("The hand is a block again"), Context.Hero->GetViewModel()->GetHandLook()->HasModel());
	}));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEILookPlacesTest,
	"Eisenseelen.Play.Looks.Places",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEILookPlacesTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		FEILookTable Models;
		TArray<TWeakObjectPtr<AActor>> Places;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	for (const FString& Place : FEILookTable::GetPlaceNames())
	{
		AddModel(State->Models, TEXT("ort.") + Place, ConePath);
	}

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const TArray<UClass*> Classes = {
			AEIAltar::StaticClass(), AEIAnchorStone::StaticClass(), AEIChest::StaticClass(), AEIEnemyCamp::StaticClass(),
			AEIForge::StaticClass(), AEIHuntBoard::StaticClass(), AEILaunchPad::StaticClass(), AEISignalFire::StaticClass(),
			AEISkyBarge::StaticClass(), AEISoulKettle::StaticClass(), AEITrainingDummy::StaticClass()};
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FEILookTable::SetTestOverride(&State->Models);
		for (int32 Index = 0; Index < Classes.Num(); ++Index)
		{
			const FVector Location = PlacesStart + FVector(0.0f, Index * PlaceSpacingCm, 0.0f);
			State->Places.Add(Context.World->SpawnActor<AActor>(Classes[Index], Location, FRotator::ZeroRotator, Parameters));
		}
		FEILookTable::SetTestOverride(nullptr);
	}));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		for (const TWeakObjectPtr<AActor>& Weak : State->Places)
		{
			const AActor* Place = Weak.Get();
			if (!Context.Test->TestNotNull(TEXT("The place is there"), Place))
			{
				continue;
			}
			TInlineComponentArray<UEILookComponent*> Looks(Place);
			Context.Test->TestFalse(*FString::Printf(TEXT("%s can carry a model"), *Place->GetClass()->GetName()), Looks.IsEmpty());
			for (const UEILookComponent* Look : Looks)
			{
				Context.Test->TestTrue(*FString::Printf(TEXT("%s shows the model of %s"), *Place->GetClass()->GetName(), *Look->GetModelKey()), ShowsMesh(Look, ConePath));
			}
		}
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
