#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EICasterComponent.h"
#include "EIEnemy.h"
#include "EIGameMode.h"
#include "EIGearComponent.h"
#include "EIGearMenu.h"
#include "EIHealthComponent.h"
#include "EIItems.h"
#include "EIProfileComponent.h"
#include "EIProjectile.h"
#include "EIRunDirector.h"
#include "EIRuneComponent.h"
#include "EISaveGame.h"
#include "EISwordComponent.h"
#include "EITestScript.h"
#include "EITrainingDummy.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

using namespace EITestScript;

namespace
{
	constexpr float HeroHeight = 92.0f;
	const FVector OpenFloor(-1500.0f, 0.0f, HeroHeight);
	constexpr float BigLife = 100000.0f;

	const TCHAR* SmallItems = TEXT(R"json({
		"qualitaeten": [{ "name": "Einfach", "faktor": 1 }, { "name": "Doppelt", "faktor": 2 }],
		"tempo_bonus_max": 0.3, "ruestung_max": 0.6, "abklingzeit_abzug_max": 0.5, "manakosten_abzug_max": 0.6, "waffe_element_bonus": 0.2,
		"start_items": ["Probeschwert", "Probestab"],
		"belohnung": { "ritual_erfahrung": 11, "anfuehrer_erfahrung": 12, "schrein_erfahrung": 13, "doppelt_erfahrung": 14, "umkreis_m": 15, "verderbnis_erfahrung_je_stufe": 0.5, "geschmiedet_ab_stufe": 2 },
		"arten": [{ "name": "Probeart", "je_zwei_teile": { "leben": 10 } }],
		"items": [
			{ "name": "Probeschwert", "platz": "waffe", "grundform": "schwert", "element": "eis" },
			{ "name": "Probestab", "platz": "waffe", "grundform": "stock", "element": "blitz" },
			{ "name": "Probehelm", "platz": "helm", "art": "Probeart", "klasse": "arkanist", "werte": { "tempo": -0.03, "leben": 5 } }
		]
	})json");

	const TCHAR* SmallWeapons = TEXT(R"json({ "grundformen": {
		"schwert": { "name": "Schwert", "klasse": "klingenwacht", "leichter_hieb_schaden": 1, "leichter_hieb_s": 1, "schwerer_hieb_schaden": 2, "schwerer_hieb_s": 2, "aufladen_s": 1, "taumeln_s": 1, "laenge_faktor": 2 },
		"stock": { "name": "Stock", "klasse": "arkanist", "schuss_schaden": 3, "schuss_radius_m": 1, "schuss_tempo_m_pro_s": 10, "schuss_mana": 4, "durchschlag_0_oder_1": 1 }
	} })json");

	UEIProfileComponent* Profile(const FEIPlayContext& Context)
	{
		return Context.Player->GetProfile();
	}

	UEIGearComponent* Gear(const FEIPlayContext& Context)
	{
		return Context.Hero->GetGear();
	}

	AEIEnemy* SpawnStandingTarget(const FEIPlayContext& Context, const FVector& Offset)
	{
		AEIEnemy* Enemy = Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Context.Hero->GetActorLocation() + Offset, FRotator(0.0f, 180.0f, 0.0f));
		FEIEnemyValues Still = Enemy->GetValues();
		Still.SpeedMetersPerSecond = 0.01f;
		Still.AttackFromMeters = 0.1f;
		Enemy->ApplyValues(Still);
		Enemy->GetHealth()->Reset(BigLife);
		return Enemy;
	}

	float LostLife(const TWeakObjectPtr<AEIEnemy>& Enemy)
	{
		return BigLife - Enemy->GetHealth()->GetHealth();
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

	float HighestDummyDamage(const FEIPlayContext& Context)
	{
		const TArray<AEITrainingDummy*> Dummies = SortedDummies(Context.World);
		return FMath::Max3(Dummies[0]->GetLastDamage(), Dummies[1]->GetLastDamage(), Dummies[2]->GetLastDamage());
	}

	/** One light swing at the dummies, until it is over. */
	void AddSwingAtDummies(TArray<FEIPlayStep>& Steps)
	{
		Steps.Add(Do([](const FEIPlayContext& Context) { StandBeforeDummies(Context, 180.0f); }));
		AddTap(Steps, EKeys::LeftMouseButton);
		Steps.Add(WaitUntil(TEXT("the swing ends"), 3.0, [](const FEIPlayContext& Context)
		{
			return SortedDummies(Context.World)[1]->GetHitCount() > 0 && !Context.Hero->GetSword()->IsSwinging();
		}));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIItemTableTest,
	"Eisenseelen.Data.Items",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIItemTableTest::RunTest(const FString& Parameters)
{
	FEIItemTable Table;
	FString Error;
	if (!FEIItemTable::LoadFromProjectFiles(Table, Error))
	{
		AddError(Error);
	}
	TestEqual(TEXT("There are three qualities"), Table.QualityFactors.Num(), 3);
	TestEqual(TEXT("There are five arts of armour"), Table.Sets.Num(), 5);
	for (const FEIArmorSet& Set : Table.Sets)
	{
		int32 Pieces[EISlotCount] = {};
		for (const FEIItem& Item : Table.Items)
		{
			Pieces[static_cast<int32>(Item.Slot)] += Item.Set == Set.Name ? 1 : 0;
		}
		for (const EEISlot Slot : { EEISlot::Helmet, EEISlot::Chest, EEISlot::Gloves, EEISlot::Boots })
		{
			TestEqual(*FString::Printf(TEXT("%s has one piece for %s"), *Set.Name, EISlotName(Slot)), Pieces[static_cast<int32>(Slot)], 1);
		}
	}
	TestEqual(TEXT("A forged item counts for more"), Table.GetQualityFactor(1), 1.25f);
	TestEqual(TEXT("Each class starts with a weapon of its own"), Table.Items[Table.GetStartItem(EEISlot::Weapon, true)].Name, FString(TEXT("Feuerstab")));
	TestEqual(TEXT("The blade warden starts with the flame sword"), Table.Items[Table.GetStartItem(EEISlot::Weapon, false)].Name, FString(TEXT("Flammenschwert")));
	for (const FEIItem& Item : Table.Items)
	{
		TestTrue(*FString::Printf(TEXT("%s can be worn by someone"), *Item.Name), Item.bForBladewarden || Item.bForArcanist);
	}

	TestEqual(TEXT("Absolute values have no percent"), EIDescribeStat(EEIStat::Life, 30.0f), FString(TEXT("+30 Leben")));
	TestEqual(TEXT("Shares are percent, and they can be negative"), EIDescribeStat(EEIStat::Speed, -0.03f), FString(TEXT("-3 % Tempo")));
	TestEqual(TEXT("Half a percent stays"), EIDescribeStat(EEIStat::Speed, 0.005f), FString(TEXT("+0.5 % Tempo")));

	FEIItemTable Small;
	TestTrue(TEXT("A complete table loads"), FEIItemTable::LoadFromStrings(SmallItems, SmallWeapons, Small, Error));
	TestEqual(TEXT("All lines are there"), Small.Items.Num(), 3);
	TestEqual(TEXT("The rewards count"), Small.Rewards.ShrineExperience, 13.0f);
	const FEIItem& Sword = Small.Items[Small.FindItem(TEXT("Probeschwert"))];
	TestTrue(TEXT("A sword is for the blade warden"), Sword.bForBladewarden && !Sword.bForArcanist);
	TestEqual(TEXT("A weapon gives a bonus for its element"), Sword.Stats[0].Amount, 0.2f);
	TestTrue(TEXT("That bonus is for its own element"), Sword.Stats[0].Stat == EEIStat::IceDamage);
	const FEIItem& Staff = Small.Items[Small.FindItem(TEXT("Probestab"))];
	TestTrue(TEXT("A staff is for the arcanist"), !Staff.bForBladewarden && Staff.bForArcanist);
	TestTrue(TEXT("The form of the weapon counts"), Small.GetForm(Staff)->bBoltPierces);
	const FEIItem& Helmet = Small.Items[Small.FindItem(TEXT("Probehelm"))];
	TestFalse(TEXT("The class of an item counts"), Helmet.bForBladewarden);
	TestEqual(TEXT("All values of an item count"), Helmet.Stats.Num(), 2);
	TestEqual(TEXT("The set counts"), Helmet.Set, FString(TEXT("Probeart")));

	const auto Refuses = [this](const TCHAR* What, const TCHAR* Old, const TCHAR* New, const TCHAR* Named)
	{
		FEIItemTable Untouched;
		FString Why;
		const FString Changed = FString(SmallItems).Replace(Old, New);
		TestFalse(What, FEIItemTable::LoadFromStrings(Changed, SmallWeapons, Untouched, Why));
		TestTrue(*FString::Printf(TEXT("The error names the place: %s"), *Why), Why.Contains(Named));
		TestEqual(TEXT("A refused file changes nothing"), Untouched.Items.Num(), 0);
	};
	Refuses(TEXT("An unknown value is refused"), TEXT("\"leben\": 5"), TEXT("\"glueck\": 5"), TEXT("Probehelm"));
	Refuses(TEXT("An unknown slot is refused"), TEXT("\"platz\": \"helm\""), TEXT("\"platz\": \"nase\""), TEXT("Probehelm"));
	Refuses(TEXT("An unknown form is refused"), TEXT("\"grundform\": \"schwert\""), TEXT("\"grundform\": \"loeffel\""), TEXT("Probeschwert"));
	Refuses(TEXT("An unknown element is refused"), TEXT("\"element\": \"eis\""), TEXT("\"element\": \"erde\""), TEXT("Probeschwert"));
	Refuses(TEXT("An unknown set is refused"), TEXT("\"art\": \"Probeart\""), TEXT("\"art\": \"Keine\""), TEXT("Probehelm"));
	Refuses(TEXT("An unknown class is refused"), TEXT("\"klasse\": \"arkanist\""), TEXT("\"klasse\": \"niemand\""), TEXT("Probehelm"));
	Refuses(TEXT("The same name twice is refused"), TEXT("\"name\": \"Probehelm\""), TEXT("\"name\": \"Probestab\""), TEXT("Probestab"));
	Refuses(TEXT("An unknown start item is refused"), TEXT("[\"Probeschwert\", \"Probestab\"]"), TEXT("[\"Gibtsnicht\"]"), TEXT("Gibtsnicht"));
	Refuses(TEXT("A class without a start weapon is refused"), TEXT("[\"Probeschwert\", \"Probestab\"]"), TEXT("[\"Probeschwert\"]"), TEXT("Start-Waffe"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEISaveGameTest,
	"Eisenseelen.Data.SaveGame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEISaveGameTest::RunTest(const FString& Parameters)
{
	FEIItemTable Table;
	FString Error;
	if (!FEIItemTable::LoadFromProjectFiles(Table, Error))
	{
		AddError(Error);
	}
	TestFalse(TEXT("Tests keep their hands off the player's file"), FEISaveGame::UsesDisk());

	const int32 WeaponSlot = static_cast<int32>(EEISlot::Weapon);
	const int32 RuneSlot = static_cast<int32>(EEISlot::Rune);
	FEISaveGame Save = FEISaveGame::MakeNew(Table);
	TestTrue(TEXT("A new player owns the start items"), Save.Items.Contains(TEXT("Frostrune")));
	TestFalse(TEXT("A new player owns nothing else"), Save.Items.Contains(TEXT("Sturmrune")));
	TestEqual(TEXT("A new blade warden holds the sword"), Save.Classes[0].Slots[WeaponSlot], FString(TEXT("Flammenschwert")));
	TestEqual(TEXT("A new arcanist holds the staff"), Save.Classes[1].Slots[WeaponSlot], FString(TEXT("Feuerstab")));
	TestEqual(TEXT("Both have the first rune on"), Save.Classes[1].Slots[RuneSlot], FString(TEXT("Frostrune")));

	Save.Experience = 340;
	Save.Items.Add(TEXT("Sturmrune"), 2);
	Save.Classes[0].Slots[RuneSlot] = TEXT("Sturmrune");
	Save.Classes[1].Slots[RuneSlot].Reset();
	Save.ArenaClasses[1].Slots[WeaponSlot] = TEXT("Donnerzepter");
	Save.ArenaQuality = 1;

	FEISaveGame Loaded = FEISaveGame::MakeNew(Table);
	TestTrue(TEXT("What was written can be read"), Loaded.FromJson(Save.ToJson(), Error));
	TestEqual(TEXT("The experience stays"), Loaded.Experience, 340);
	TestEqual(TEXT("The quality stays"), Loaded.Items.FindRef(TEXT("Sturmrune")), 2);
	TestEqual(TEXT("What is on stays on"), Loaded.Classes[0].Slots[RuneSlot], FString(TEXT("Sturmrune")));
	TestTrue(TEXT("An empty slot stays empty"), Loaded.Classes[1].Slots[RuneSlot].IsEmpty());
	TestEqual(TEXT("The test arena keeps its own things"), Loaded.ArenaClasses[1].Slots[WeaponSlot], FString(TEXT("Donnerzepter")));
	TestEqual(TEXT("The arena keeps its quality"), Loaded.ArenaQuality, 1);
	TestEqual(TEXT("A real run is not touched by that"), Loaded.Classes[1].Slots[WeaponSlot], FString(TEXT("Feuerstab")));

	FEISaveGame Untouched = FEISaveGame::MakeNew(Table);
	TestFalse(TEXT("A broken file is refused"), Untouched.FromJson(TEXT("{ das ist kein Spielstand"), Error));
	TestFalse(TEXT("A file without items is refused"), Untouched.FromJson(TEXT("{ \"version\": 1 }"), Error));
	TestTrue(TEXT("A refused file changes nothing"), Untouched.Items.Contains(TEXT("Frostrune")));

	const FString Path = FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("SpielstandProbe.json"));
	TestTrue(TEXT("The file gets written"), Save.SaveToFile(Path));
	FString Json;
	TestTrue(TEXT("The file is there"), FFileHelper::LoadFileToString(Json, *Path));
	FEISaveGame FromFile = FEISaveGame::MakeNew(Table);
	TestTrue(TEXT("The file can be read"), FromFile.FromJson(Json, Error));
	TestEqual(TEXT("The file holds the same"), FromFile.Items.FindRef(TEXT("Sturmrune")), 2);
	TestFalse(TEXT("No half file stays behind"), IFileManager::Get().FileExists(*(Path + TEXT(".neu"))));
	IFileManager::Get().Delete(*Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIGearTest,
	"Eisenseelen.Play.Gear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIGearTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		float BaseSpeed = 0.0f;
		TWeakObjectPtr<AEIEnemy> First;
		TWeakObjectPtr<AEIEnemy> Second;
		TWeakObjectPtr<AEIEnemy> Third;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	TArray<FEIPlayStep> Steps;

	// A new hero wears the start items, and the flame sword gives its element a bonus.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The hero holds the start weapon"), Gear(Context)->GetWorn(EEISlot::Weapon)->Name, FString(TEXT("Flammenschwert")));
		Context.Test->TestEqual(TEXT("The hero has the start rune"), Gear(Context)->GetWorn(EEISlot::Rune)->Name, FString(TEXT("Frostrune")));
		Context.Test->TestEqual(TEXT("The E key knows it"), Context.Hero->GetRunes()->GetRune()->Name, FString(TEXT("Frostrune")));
		Context.Test->TestNearlyEqual(TEXT("The weapon raises its element"), Gear(Context)->GetStat(EEIStat::FireDamage), Gear(Context)->GetTable().WeaponElementBonus);
		Context.Test->TestEqual(TEXT("Its spell is the fireball"), Context.Hero->GetCaster()->GetSpell().Name, FString(TEXT("Feuerball")));
		Context.Test->TestNull(TEXT("The other slots are empty"), Gear(Context)->GetWorn(EEISlot::Chest));
		State->BaseSpeed = Context.Hero->GetCharacterMovement()->MaxWalkSpeed;
	}));

	// Another form plays differently: slower and harder, and the higher quality counts on top.
	Steps.Add(Do([](const FEIPlayContext& Context) { Profile(Context)->EquipByName(TEXT("Aschenbrecher")); }));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const FEISwordValues& Values = Context.Hero->GetSword()->GetValues();
		Context.Test->TestEqual(TEXT("The two-hander hits harder"), Values.Light.Damage, 50.0f);
		Context.Test->TestEqual(TEXT("The two-hander is slower"), Values.Light.DurationSeconds, 0.65f);
		Context.Test->TestEqual(TEXT("The two-hander staggers longer"), Values.HeavyStaggerSeconds, 1.6f);
	}));
	AddSwingAtDummies(Steps);
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->AddInfo(FString::Printf(TEXT("A swing of the two-hander at 180 cm: %.1f damage"), HighestDummyDamage(Context)));
		Context.Test->TestNearlyEqual(TEXT("The swing does the form's damage"), HighestDummyDamage(Context), 50.0f, 0.01f);
		Profile(Context)->SetArenaQuality(2);
	}));
	Steps.Add(Wait(0.05));
	AddSwingAtDummies(Steps);
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestNearlyEqual(TEXT("The quality raises the damage"), HighestDummyDamage(Context), 75.0f, 0.01f);
		Context.Test->TestNearlyEqual(TEXT("The quality raises the element too"), Gear(Context)->GetStat(EEIStat::FireDamage), Gear(Context)->GetTable().WeaponElementBonus * 1.5f);
		Profile(Context)->SetArenaQuality(0);
	}));

	// Armour: life and the share of a hit it swallows, at the price of some speed.
	Steps.Add(Do([](const FEIPlayContext& Context) { Profile(Context)->EquipByName(TEXT("Brustpanzer der Eisenwacht")); }));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const UEIHealthComponent* Health = Context.Hero->GetHealth();
		Context.Test->TestEqual(TEXT("The armour gives life"), Health->GetMaxHealth(), Context.Hero->GetClassValues().Life + 30.0f);
		Context.Test->TestEqual(TEXT("A hero at full life stays at full life"), Health->GetHealth(), Health->GetMaxHealth());
		Context.Test->TestNearlyEqual(TEXT("The armour swallows a share"), Gear(Context)->GetArmorShare(), 0.06f);
		Context.Test->TestNearlyEqual(TEXT("The armour is heavy"), Context.Hero->GetCharacterMovement()->MaxWalkSpeed, State->BaseSpeed * 0.97f, 0.1f);
		const float Before = Health->GetHealth();
		Context.Hero->TakeDamage(100.0f, FDamageEvent(), nullptr, nullptr);
		Context.Test->TestNearlyEqual(TEXT("A hit takes less"), Before - Health->GetHealth(), 94.0f, 0.01f);
		Context.Hero->GetHealth()->Heal(100.0f);
		Profile(Context)->EquipByName(TEXT("Helm der Eisenwacht"));
	}));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		// Chest 30 + helmet 15 + two pieces of one art 15.
		Context.Test->TestEqual(TEXT("Two pieces of one art give their bonus"), Context.Hero->GetHealth()->GetMaxHealth(), Context.Hero->GetClassValues().Life + 60.0f);
		Profile(Context)->EquipByName(TEXT("Handschuhe der Eisenwacht"));
		Profile(Context)->EquipByName(TEXT("Schuhe der Eisenwacht"));
	}));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		// All pieces 60 + four pieces of one art 30.
		Context.Test->TestEqual(TEXT("Four pieces give the bonus twice"), Context.Hero->GetHealth()->GetMaxHealth(), Context.Hero->GetClassValues().Life + 90.0f);
		const float Full = Context.Hero->GetSword()->GetValues().Block.DamageReduction;
		Context.Test->TestNearlyEqual(TEXT("The gloves raise the block"), Gear(Context)->GetStat(EEIStat::BlockStrength), 0.1f);
		Context.Test->TestTrue(TEXT("The block still lets something through"), Full + Gear(Context)->GetStat(EEIStat::BlockStrength) < 1.0f);

		// What cannot be: no weapon at all, or a weapon of the other class.
		Profile(Context)->Equip(EEISlot::Weapon, INDEX_NONE);
		Context.Test->TestNotNull(TEXT("A hero always keeps a weapon"), Gear(Context)->GetWorn(EEISlot::Weapon));
		Profile(Context)->EquipByName(TEXT("Feuerstab"));
		Context.Test->TestEqual(TEXT("A staff does not fit a blade warden"), Gear(Context)->GetWorn(EEISlot::Weapon)->Name, FString(TEXT("Aschenbrecher")));

		// An empty rune slot leaves the E key without a spell.
		Profile(Context)->Equip(EEISlot::Rune, INDEX_NONE);
	}));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestNull(TEXT("Without a rune there is no spell on E"), Context.Hero->GetRunes()->GetRune());
		Context.Hero->SetCanBeDamaged(false);
		Teleport(Context, OpenFloor, 0.0f);
		Profile(Context)->EquipByName(TEXT("Frostrune"));
		Profile(Context)->EquipByName(TEXT("Frostklinge"));
	}));
	Steps.Add(Wait(0.2));

	// The ice lance flies through all in its way. Whoever it slows twice freezes.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("An ice weapon has the ice spell"), Context.Hero->GetCaster()->GetSpell().Name, FString(TEXT("Eislanze")));
		Context.Player->SetControlRotation(FRotator(-5.0f, 0.0f, 0.0f));
		State->First = SpawnStandingTarget(Context, FVector(600.0f, 0.0f, 0.0f));
		State->Second = SpawnStandingTarget(Context, FVector(1000.0f, 0.0f, 0.0f));
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::Q);
	Steps.Add(WaitUntil(TEXT("the lance reaches the second target"), 2.0, [State](const FEIPlayContext& Context) { return LostLife(State->Second) > 0.0f; }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const float Expected = Context.Hero->GetCaster()->GetValues().IceLance.Damage * Gear(Context)->GetSpellFactor();
		Context.Test->AddInfo(FString::Printf(TEXT("The lance: %.0f and %.0f damage at 600 and 1000 cm, expected %.0f"), LostLife(State->First), LostLife(State->Second), Expected));
		Context.Test->TestNearlyEqual(TEXT("The first target takes the lance"), LostLife(State->First), Expected, 0.01f);
		Context.Test->TestNearlyEqual(TEXT("The lance goes through to the second"), LostLife(State->Second), Expected, 0.01f);
		Context.Test->TestTrue(TEXT("The lance slows"), State->First->IsSlowed());
		Context.Test->TestFalse(TEXT("One lance does not freeze yet"), State->First->IsFrozen());
	}));
	Steps.Add(WaitUntil(TEXT("Q cools down"), 8.0, [](const FEIPlayContext& Context) { return Context.Hero->GetCaster()->GetSpellCooldownFraction() <= 0.0f; }));
	AddTap(Steps, EKeys::Q);
	Steps.Add(WaitUntil(TEXT("the second lance hits"), 2.0, [State](const FEIPlayContext& Context) { return State->First->IsFrozen(); }));

	// The chain lightning jumps from target to target, and each jump weakens it.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->First->Destroy();
		State->Second->Destroy();
		Profile(Context)->EquipByName(TEXT("Sturmklinge"));
	}));
	Steps.Add(WaitUntil(TEXT("Q cools down"), 8.0, [](const FEIPlayContext& Context) { return Context.Hero->GetCaster()->GetSpellCooldownFraction() <= 0.0f; }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A lightning weapon has the lightning spell"), Context.Hero->GetCaster()->GetSpell().Name, FString(TEXT("Kettenblitz")));
		Context.Player->SetControlRotation(FRotator(-5.0f, 0.0f, 0.0f));
		State->First = SpawnStandingTarget(Context, FVector(600.0f, 0.0f, 0.0f));
		State->Second = SpawnStandingTarget(Context, FVector(600.0f, 400.0f, 0.0f));
		State->Third = SpawnStandingTarget(Context, FVector(600.0f, 5000.0f, 0.0f));
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::Q);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const FEISpellValues& Spell = Context.Hero->GetCaster()->GetValues().ChainLightning;
		const float Expected = Spell.Damage * Gear(Context)->GetSpellFactor();
		Context.Test->AddInfo(FString::Printf(TEXT("The chain at 600 cm: %.0f, then %.0f after a jump"), LostLife(State->First), LostLife(State->Second)));
		Context.Test->TestNearlyEqual(TEXT("The first target takes it all"), LostLife(State->First), Expected, 0.01f);
		Context.Test->TestNearlyEqual(TEXT("The jump costs strength"), LostLife(State->Second), Expected * (1.0f - Spell.JumpFalloff), 0.01f);
		Context.Test->TestEqual(TEXT("Far away nothing arrives"), LostLife(State->Third), 0.0f);
		State->First->Destroy();
		State->Second->Destroy();
		State->Third->Destroy();
	}));
	Steps.Add(Wait(0.1));

	// The other class has its own things on, and none of the blade warden's armour.
	AddTap(Steps, EKeys::C);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The arcanist holds the staff"), Gear(Context)->GetWorn(EEISlot::Weapon)->Name, FString(TEXT("Feuerstab")));
		Context.Test->TestNull(TEXT("The arcanist does not wear the other's armour"), Gear(Context)->GetWorn(EEISlot::Chest));
		Context.Test->TestEqual(TEXT("So the arcanist has the class's life"), Context.Hero->GetHealth()->GetMaxHealth(), Context.Hero->GetClassValues().Life);
		Profile(Context)->EquipByName(TEXT("Aschenzepter"));
	}));
	Steps.Add(Wait(0.05));

	// The bolt of a scepter goes through all in its way.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const UEICasterComponent* Caster = Context.Hero->GetCaster();
		Context.Test->TestEqual(TEXT("The scepter hits harder"), Caster->GetValues().Bolt.Damage, 70.0f);
		Context.Test->TestEqual(TEXT("The scepter costs more"), Caster->GetValues().Bolt.ManaCost, 20.0f);
		Context.Player->SetControlRotation(FRotator(-5.0f, 0.0f, 0.0f));
		State->First = SpawnStandingTarget(Context, FVector(600.0f, 0.0f, 0.0f));
		State->Second = SpawnStandingTarget(Context, FVector(1000.0f, 0.0f, 0.0f));
	}));
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("the bolt reaches the second target"), 2.0, [State](const FEIPlayContext& Context) { return LostLife(State->Second) > 0.0f; }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const float Expected = 70.0f * Gear(Context)->GetShotFactor();
		Context.Test->TestNearlyEqual(TEXT("The first target takes the bolt"), LostLife(State->First), Expected, 0.01f);
		Context.Test->TestNearlyEqual(TEXT("The bolt goes through to the second"), LostLife(State->Second), Expected, 0.01f);
	}));

	// What the arcanist's armour does: more mana, less cost, more damage from two pieces of an art.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEICasterComponent* Caster = Context.Hero->GetCaster();
		const float ManaBefore = Caster->GetMaxMana();
		Context.Test->TestNearlyEqual(TEXT("A scepter shot costs its mana at first"), Caster->GetValues().Bolt.ManaCost, 20.0f);
		Profile(Context)->EquipByName(TEXT("Helm des Quellw\u00E4chters"));
		Profile(Context)->EquipByName(TEXT("Handschuhe des Quellw\u00E4chters"));
		Context.Test->TestEqual(TEXT("The helmet gives mana"), Caster->GetMaxMana(), ManaBefore + 25.0f);
		Context.Test->TestNearlyEqual(TEXT("The gloves make it cheaper"), Gear(Context)->GetManaCostFactor(), 0.85f);
		Context.Test->TestNearlyEqual(TEXT("Two pieces raise the attack"), Gear(Context)->GetStat(EEIStat::AttackPower), 0.1f);
		Context.Test->TestNearlyEqual(TEXT("The shots follow"), Gear(Context)->GetShotFactor(), 1.2f);
		Profile(Context)->EquipByName(TEXT("Helm der Glutschmiede"));
		Profile(Context)->EquipByName(TEXT("Handschuhe der Glutschmiede"));
		Context.Test->TestNearlyEqual(TEXT("An art for both classes fits the arcanist"), Gear(Context)->GetStat(EEIStat::StatusDuration), 0.25f);
		Context.Test->TestNearlyEqual(TEXT("It makes conditions last longer"), Gear(Context)->GetStatusFactor(), 1.25f);
		Profile(Context)->EquipByName(TEXT("Handschuhe der Henkersgarde"));
		Context.Test->TestEqual(TEXT("An art of the other class stays off"), Gear(Context)->GetWorn(EEISlot::Gloves)->Name, FString(TEXT("Handschuhe der Glutschmiede")));
	}));

	// What the blade warden's art of attack does.
	AddTap(Steps, EKeys::C);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Charge = Context.Hero->GetSword()->GetValues().ChargeSeconds;
		Profile(Context)->EquipByName(TEXT("Helm der Henkersgarde"));
		Profile(Context)->EquipByName(TEXT("Handschuhe der Henkersgarde"));
		Context.Test->TestNearlyEqual(TEXT("Two pieces charge faster"), Context.Hero->GetSword()->GetValues().ChargeSeconds, Charge / 1.1f);
	}));
	AddTap(Steps, EKeys::C);
	Steps.Add(Wait(0.1));

	// A new hero wears the same.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitUntil(TEXT("a fresh hero stands"), 3.0, [](const FEIPlayContext& Context) { return Context.Movement->IsMovingOnGround() && Context.Hero->GetGear()->GetWorn(EEISlot::Weapon) != nullptr; }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A new hero keeps the weapon"), Gear(Context)->GetWorn(EEISlot::Weapon)->Name, FString(TEXT("Aschenzepter")));
	}));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEILootTest,
	"Eisenseelen.Play.Run.Loot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEILootTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		FString Item;
		int32 Experience = 0;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	TArray<FEIPlayStep> Steps;
	const auto Remember = [State](const FEIPlayContext& Context)
	{
		State->Experience = Profile(Context)->GetSave().Experience;
		const TArray<FEILootLine>& Loot = Profile(Context)->GetLoot();
		State->Item = Loot.IsEmpty() ? FString() : Profile(Context)->GetTable().Items[Loot.Last().Item].Name;
	};

	// A reward lands in the bag at once. The experience is safe, the item is not yet.
	Steps.Add(Do([Remember](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A new player has no experience"), Profile(Context)->GetSave().Experience, 0);
		Profile(Context)->GrantReward(EEIRewardSource::Ritual);
		Remember(Context);
		const FEIRewardValues& Rewards = Profile(Context)->GetTable().Rewards;
		const FEIItemTable& Table = Profile(Context)->GetTable();
		Context.Test->TestEqual(TEXT("The ritual brings one item"), Profile(Context)->GetLoot().Num(), 1);
		const FEIItem& Item = Table.Items[Profile(Context)->GetLoot()[0].Item];
		Context.Test->TestTrue(TEXT("The ritual brings a weapon or a rune"), Item.Slot == EEISlot::Weapon || Item.Slot == EEISlot::Rune);
		Context.Test->TestTrue(TEXT("It fits the class"), Item.FitsClass(false));
		Context.Test->TestFalse(TEXT("It is not safe yet"), Profile(Context)->GetSave().Items.Contains(Item.Name));
		Context.Test->TestEqual(TEXT("The experience is there at once"), Profile(Context)->GetSave().Experience, FMath::RoundToInt32(Rewards.RitualExperience));
		Context.Test->TestTrue(TEXT("The screen says so"), Profile(Context)->GetRewardNote() > 0.0f);
		Context.Test->TestTrue(TEXT("The note names the item"), Profile(Context)->GetRewardText().Contains(Item.Name));

		// The next one is something else, as long as there is something else.
		Profile(Context)->GrantReward(EEIRewardSource::Ritual);
		Context.Test->TestNotEqual(TEXT("The second reward is another item"), Profile(Context)->GetLoot()[1].Item, Profile(Context)->GetLoot()[0].Item);

		// Without the barge the loot is gone.
		Profile(Context)->EndRun(false);
		Context.Test->TestFalse(TEXT("A failed run loses the item"), Profile(Context)->GetSave().Items.Contains(Item.Name));
		Context.Test->TestEqual(TEXT("The loot can still be read"), Profile(Context)->GetLoot()[0].Result, FString(TEXT("verloren")));
		Context.Test->TestEqual(TEXT("A failed run keeps the experience"), Profile(Context)->GetSave().Experience, FMath::RoundToInt32(Rewards.RitualExperience * 2.0f));
		Profile(Context)->StartFresh();
		Context.Test->TestEqual(TEXT("A new run has an empty bag"), Profile(Context)->GetLoot().Num(), 0);
		Context.Test->TestEqual(TEXT("A new run counts from nothing"), Profile(Context)->GetRunExperience(), 0);

		// In the barge it is safe.
		Profile(Context)->GrantReward(EEIRewardSource::Ritual);
		Remember(Context);
		Profile(Context)->EndRun(true);
		Context.Test->TestTrue(TEXT("Getting away secures the item"), Profile(Context)->GetSave().Items.Contains(Profile(Context)->GetTable().Items[Profile(Context)->GetLoot()[0].Item].Name));
		Context.Test->TestTrue(TEXT("The settlement says it is new"), Profile(Context)->GetLoot()[0].Result.StartsWith(TEXT("neu")));
		Profile(Context)->StartFresh();

		// Once all is owned, a find raises the quality.
		FEISaveGame& Save = Profile(Context)->GetSave();
		for (const FEIItem& Entry : Table.Items)
		{
			Save.Items.Add(Entry.Name, 0);
		}
		Profile(Context)->EquipByName(TEXT("Frostrune"));
		Profile(Context)->GrantReward(EEIRewardSource::Ritual);
		Profile(Context)->EndRun(true);
		Context.Test->TestTrue(TEXT("A second find raises the quality"), Profile(Context)->GetLoot()[0].Result.Contains(Table.GetQualityName(1)));
		Context.Test->TestEqual(TEXT("It counts in the save"), Save.Items.FindRef(Table.Items[Profile(Context)->GetLoot()[0].Item].Name), 1);
		Profile(Context)->StartFresh();

		// And when all of it is at its best, a find is worth experience.
		for (const FEIItem& Entry : Table.Items)
		{
			Save.Items.Add(Entry.Name, Table.QualityFactors.Num() - 1);
		}
		Profile(Context)->EquipByName(TEXT("Frostrune"));
		const int32 Before = Save.Experience;
		Profile(Context)->GrantReward(EEIRewardSource::Ritual);
		Profile(Context)->EndRun(true);
		Context.Test->TestEqual(TEXT("A find of the best is worth experience"), Save.Experience, Before + FMath::RoundToInt32(Rewards.RitualExperience + Rewards.DuplicateExperience));
		Profile(Context)->StartFresh();
		Save.Items.Reset();
		for (const FString& Name : Table.StartItems)
		{
			Save.Items.Add(Name, 0);
		}
		Profile(Context)->EquipByName(TEXT("Frostrune"));
	}));
	Steps.Add(Wait(0.05));

	// A late stage of the corruption pays more, and what is new comes forged.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		AEIGameMode* GameMode = Context.World->GetAuthGameMode<AEIGameMode>();
		const FEIRewardValues& Rewards = Profile(Context)->GetTable().Rewards;
		const int32 Stage = FMath::RoundToInt32(Rewards.ForgedFromStage);
		GameMode->GetRun()->SetCorruptionStage(Stage);
		const int32 Before = Profile(Context)->GetSave().Experience;
		Profile(Context)->GrantReward(EEIRewardSource::Ritual);
		const int32 Expected = FMath::RoundToInt32(Rewards.RitualExperience * (1.0f + Rewards.ExperiencePerStage * Stage));
		Context.Test->TestEqual(TEXT("More corruption brings more experience"), Profile(Context)->GetSave().Experience - Before, Expected);
		Context.Test->TestEqual(TEXT("From that stage on a new item comes forged"), Profile(Context)->GetLoot()[0].Quality, 1);
		Profile(Context)->EndRun(true);
		Context.Test->TestEqual(TEXT("It is forged in the save"), Profile(Context)->GetSave().Items.FindRef(Profile(Context)->GetTable().Items[Profile(Context)->GetLoot()[0].Item].Name), 1);
		GameMode->GetRun()->SetCorruptionStage(0);
		Profile(Context)->StartFresh();
	}));

	// The arcanist gets what an arcanist can use.
	AddTap(Steps, EKeys::C);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		for (int32 Count = 0; Count < 4; ++Count)
		{
			Profile(Context)->GrantReward(EEIRewardSource::Ritual);
		}
		for (const FEILootLine& Line : Profile(Context)->GetLoot())
		{
			Context.Test->TestTrue(TEXT("It fits the arcanist"), Profile(Context)->GetTable().Items[Line.Item].FitsClass(true));
		}
	}));
	return RunInMap(this, RunMap, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIGearMenuTest,
	"Eisenseelen.Play.GearMenu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIGearMenuTest::RunTest(const FString& Parameters)
{
	constexpr float ScreenWidth = 1600.0f;
	constexpr float ScreenHeight = 900.0f;
	const auto Page = [](const FEIPlayContext& Context) -> FEIGearMenu&
	{
		FEIGearMenu& Menu = Context.Player->GetMenu();
		Menu.Build(*Profile(Context), *Context.Hero, ScreenWidth, ScreenHeight, -1.0f, -1.0f);
		return Menu;
	};
	const auto ClickOn = [Page](const FEIPlayContext& Context, const TCHAR* Text)
	{
		FEIGearMenu& Menu = Page(Context);
		const FEIMenuElement* Button = Menu.FindButton(Text);
		Context.Test->TestNotNull(*FString::Printf(TEXT("The page has a button for %s"), Text), Button);
		if (Button != nullptr)
		{
			Menu.Click(Button->X + Button->Width / 2.0f, Button->Y + Button->Height / 2.0f);
		}
	};

	TArray<FEIPlayStep> Steps;

	// I opens the page, and then the hero stands still.
	AddTap(Steps, EKeys::I);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([Page](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("I opens the page"), Context.Player->IsMenuOpen());
		Context.Test->TestTrue(TEXT("The mouse is a pointer now"), Context.Player->bShowMouseCursor);
		FEIGearMenu& Menu = Page(Context);
		Context.Test->TestTrue(TEXT("The page shows the class"), Menu.Shows(TEXT("Klingenwacht")));
		Context.Test->TestTrue(TEXT("The page shows all slots"), Menu.Shows(TEXT("Waffe:")) && Menu.Shows(TEXT("Rune:")) && Menu.Shows(TEXT("Helm:")) && Menu.Shows(TEXT("Brustpanzer:")) && Menu.Shows(TEXT("Handschuhe:")) && Menu.Shows(TEXT("Schuhe:")) && Menu.Shows(TEXT("Herzkristall:")));
		Context.Test->TestTrue(TEXT("It shows what is on"), Menu.Shows(TEXT("Waffe: Flammenschwert (Rostig)")));
		Context.Test->TestTrue(TEXT("It shows the sum of the values"), Menu.Shows(TEXT("+10 % Feuerschaden")));
		Context.Test->TestTrue(TEXT("It shows the spell on Q"), Menu.Shows(TEXT("Zauber auf Q: Feuerball")));
		Context.Test->TestNotNull(TEXT("The arena has all weapons of the class"), Menu.FindButton(TEXT("Donnerbrecher")));
		Context.Test->TestNull(TEXT("But none of the other class"), Menu.FindButton(TEXT("Feuerstab")));
		for (const FEIMenuElement& Element : Menu.GetElements())
		{
			Context.Test->TestTrue(*FString::Printf(TEXT("%s fits on the screen"), *Element.Text), Element.X + Element.Width <= ScreenWidth && Element.Y + Element.Height <= ScreenHeight);
		}
	}));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("With the page open the hero does not walk"), Context.Hero->GetVelocity().Size2D() < 10.0);
	}));
	Steps.Add(Release(EKeys::W));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([ClickOn, Page](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("With the page open the hero does not swing"), Context.Hero->GetSword()->IsSwinging());

		// A click puts it on.
		ClickOn(Context, TEXT("Donnerbrecher"));
		Context.Test->TestEqual(TEXT("A click on an item puts it on"), Gear(Context)->GetWorn(EEISlot::Weapon)->Name, FString(TEXT("Donnerbrecher")));
		Context.Test->TestTrue(TEXT("The page shows it"), Page(Context).Shows(TEXT("Waffe: Donnerbrecher")));
		Context.Test->TestTrue(TEXT("The page shows its spell"), Page(Context).Shows(TEXT("Zauber auf Q: Kettenblitz")));

		// Hovering compares it to what is on.
		FEIGearMenu& Menu = Page(Context);
		const FEIMenuElement* Other = Menu.FindButton(TEXT("Frostklinge"));
		const float OverX = Other->X + 5.0f;
		const float OverY = Other->Y + 5.0f;
		Menu.Build(*Profile(Context), *Context.Hero, ScreenWidth, ScreenHeight, OverX, OverY);
		Context.Test->TestTrue(TEXT("The page compares what the mouse is on"), Menu.Shows(TEXT("Frostklinge statt Donnerbrecher")));
		Context.Test->TestTrue(TEXT("It says what is gained"), Menu.Shows(TEXT("+10 % Eisschaden")));
		Context.Test->TestTrue(TEXT("It says what is lost"), Menu.Shows(TEXT("-10 % Blitzschaden")));

		// Another slot, and something that can come off again.
		ClickOn(Context, TEXT("Brustpanzer:"));
		Context.Test->TestEqual(TEXT("A click on a slot chooses it"), Context.Player->GetMenu().GetSelectedSlot(), EEISlot::Chest);
		ClickOn(Context, TEXT("Brustpanzer der Eisenwacht"));
		Context.Test->TestEqual(TEXT("The armour is on"), Gear(Context)->GetWorn(EEISlot::Chest)->Name, FString(TEXT("Brustpanzer der Eisenwacht")));
		Context.Test->TestTrue(TEXT("The sum of the values follows"), Page(Context).Shows(TEXT("+30 Leben")));
		ClickOn(Context, TEXT("Ablegen"));
		Context.Test->TestNull(TEXT("It can come off"), Gear(Context)->GetWorn(EEISlot::Chest));

		// The weapon cannot come off.
		ClickOn(Context, TEXT("Waffe:"));
		Context.Test->TestNull(TEXT("A weapon has no button to take it off"), Page(Context).FindButton(TEXT("Ablegen")));

		// In the arena the quality is free too.
		ClickOn(Context, TEXT("Qualit\u00E4t:"));
		Context.Test->TestEqual(TEXT("The button raises the quality"), Gear(Context)->GetWornQuality(EEISlot::Weapon), 1);
		ClickOn(Context, TEXT("Qualit\u00E4t:"));
		ClickOn(Context, TEXT("Qualit\u00E4t:"));
		Context.Test->TestEqual(TEXT("After the best it starts over"), Gear(Context)->GetWornQuality(EEISlot::Weapon), 0);
	}));

	// I closes it, and the hero is free again.
	AddTap(Steps, EKeys::I);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("I closes the page"), Context.Player->IsMenuOpen());
		Context.Test->TestFalse(TEXT("The mouse looks around again"), Context.Player->bShowMouseCursor);
	}));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("Now the hero swings again"), Context.Hero->GetSword()->IsSwinging());
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
