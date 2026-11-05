#pragma once

#include "CoreMinimal.h"
#include "EIEnemy.h"
#include "EIItems.generated.h"

/** All values that items, omens and attributes can raise. They add up on the hero. */
enum class EEIStat : uint8
{
	AttackPower,
	AttackSpeed,
	ChargeSpeed,
	SpellPower,
	FireDamage,
	IceDamage,
	LightningDamage,
	Cooldown,
	StatusDuration,
	ReactionDamage,
	Life,
	Armor,
	BlockStrength,
	Speed,
	Mana,
	ManaRegen,
	ManaCost
};

constexpr int32 EIStatCount = 17;

/** The places on a hero that can hold an item. */
enum class EEISlot : uint8
{
	Weapon,
	Rune,
	Helmet,
	Chest,
	Gloves,
	Boots,
	Crystal
};

constexpr int32 EISlotCount = 7;

/** The name of a value in the data files. */
EISENSEELEN_API const TCHAR* EIStatKey(EEIStat Stat);
EISENSEELEN_API bool EIStatFromKey(const FString& Key, EEIStat& OutStat);

/** Words for the player. */
EISENSEELEN_API const TCHAR* EIStatName(EEIStat Stat);
EISENSEELEN_API const TCHAR* EISlotName(EEISlot Slot);

/** The name of a slot in the data files and the save file. */
EISENSEELEN_API const TCHAR* EISlotKey(EEISlot Slot);

/** For example "+12 % Zaubermacht" or "+30 Leben". */
EISENSEELEN_API FString EIDescribeStat(EEIStat Stat, float Amount);

struct FEIStatLine
{
	EEIStat Stat = EEIStat::Life;
	float Amount = 0.0f;
};

/** How a kind of weapon plays, from the section grundformen of Data/waffen.json. */
struct EISENSEELEN_API FEIWeaponForm
{
	FString Key;
	FString Name;
	bool bStaff = false;

	float LightDamage = 35.0f;
	float LightSeconds = 0.45f;
	float HeavyDamage = 110.0f;
	float HeavySeconds = 0.55f;
	float ChargeSeconds = 0.6f;
	float StaggerSeconds = 1.0f;
	float LengthFactor = 1.0f;

	float BoltDamage = 40.0f;
	float BoltRadiusMeters = 1.5f;
	float BoltSpeedMetersPerSecond = 60.0f;
	float BoltMana = 10.0f;
	bool bBoltPierces = false;
};

/** One line of Data/items.json. */
struct EISENSEELEN_API FEIItem
{
	FString Name;
	EEISlot Slot = EEISlot::Helmet;
	bool bForBladewarden = true;
	bool bForArcanist = true;

	/** The art of armour it belongs to. Empty for everything else. */
	FString Set;
	TArray<FEIStatLine> Stats;

	/** Only for weapons. */
	int32 Form = INDEX_NONE;
	EEIElement Element = EEIElement::Fire;

	/** What it costs in the forge, and from which level the forge offers it. */
	int32 Price = 0;
	int32 FromLevel = 1;

	bool FitsClass(bool bArcanist) const { return bArcanist ? bForArcanist : bForBladewarden; }
};

/** An art of armour: 2 pieces give this bonus, 4 pieces give it twice. */
struct EISENSEELEN_API FEIArmorSet
{
	FString Name;
	TArray<FEIStatLine> PerTwoPieces;
};

/** What deeds bring, from the section belohnung of Data/items.json. */
struct EISENSEELEN_API FEIRewardValues
{
	float RadiusMeters = 40.0f;
};

struct EISENSEELEN_API FEIItemTable
{
	FEIRewardValues Rewards;
	float MaxLevel = 20.0f;
	float FirstLevelExperience = 100.0f;
	float LevelExperienceGrowth = 50.0f;
	TArray<FEIItem> Items;
	TArray<FEIWeaponForm> Forms;
	TArray<FEIArmorSet> Sets;
	TArray<FString> StartItems;
	float MaxSpeedBonus = 0.3f;
	float MaxArmor = 0.6f;
	float MaxCooldownCut = 0.5f;
	float MaxManaCostCut = 0.6f;
	float WeaponElementBonus = 0.1f;

	int32 FindItem(const FString& Name) const;
	const FEIArmorSet* FindSet(const FString& Name) const;
	const FEIWeaponForm* GetForm(const FEIItem& Item) const;

	/** All the experience it takes to be this level. Level 1 takes none. */
	int32 GetExperienceForLevel(int32 Level) const;
	int32 GetLevel(int32 Experience) const;

	/** What the forge starts to offer at exactly this level. */
	TArray<int32> GetUnlockedAt(int32 Level) const;

	/** What a new hero of this class has in this slot. INDEX_NONE if nothing. */
	int32 GetStartItem(EEISlot Slot, bool bArcanist) const;

	/** OutTable only changes when both texts are valid. */
	static bool LoadFromStrings(const FString& ItemsJson, const FString& WeaponsJson, FEIItemTable& OutTable, FString& OutError);
	static bool LoadFromProjectFiles(FEIItemTable& OutTable, FString& OutError);
};

USTRUCT()
struct FEIWornItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Item = INDEX_NONE;
};

/** What a hero of one class wears. It travels over the net by the position of the items in the table. */
USTRUCT()
struct FEIHeroSetup
{
	GENERATED_BODY()

	/** One entry per slot. */
	UPROPERTY()
	TArray<FEIWornItem> Slots;
};
