#pragma once

#include "CoreMinimal.h"
#include "EIEnemy.h"

/** How a boss comes into play. Each boss has its own way, and each way is a line in Data/bosse.json. */
enum class EEIBannbruchKind : uint8
{
	/** Stones to hold, each one drops a chain. */
	AnchorStones,
	/** A kettle that fills with every enemy that falls in its circle. The boss sleeps in its grave until it is full. */
	Kettle,
	/** Soul stones in guarded chests. Whoever sets them all into the hollow armor wakes the boss inside. */
	SoulStones
};

/** The task before a boss. */
struct EISENSEELEN_API FEIBannbruch
{
	EEIBannbruchKind Kind = EEIBannbruchKind::AnchorStones;
	FString Name;
	FString PartName;
	FString Hint;
	/** Stones, or souls for the kettle. */
	int32 Count = 3;
	float HoldSeconds = 20.0f;
	float ZoneMeters = 7.0f;
	/** From the boss to a stone, or from the grave to the kettle. */
	float DistanceMeters = 18.0f;
};

/** Some bosses call a group to their side and cannot be hurt while it lives. */
struct EISENSEELEN_API FEISummon
{
	bool bHas = false;
	FString GroupSize;
	float EverySeconds = 18.0f;
	float WindupSeconds = 1.5f;
	float MaxShieldSeconds = 20.0f;
	float DistanceMeters = 6.0f;
};

/** Some bosses vanish and come up somewhere else near their place. */
struct EISENSEELEN_API FEIVanish
{
	bool bHas = false;
	float EverySeconds = 14.0f;
	float WindupSeconds = 1.2f;
	float RangeMeters = 20.0f;
	float MinMeters = 10.0f;
};

/** The attack that hits everybody around a boss. */
struct EISENSEELEN_API FEIStomp
{
	bool bHas = true;
	float RadiusMeters = 9.0f;
	float FromMeters = 8.0f;
	float WindupSeconds = 1.4f;
	float Damage = 45.0f;
	float PauseSeconds = 7.0f;
};

/** Something a boss can drop, with the chance that it does. */
struct EISENSEELEN_API FEILootItem
{
	FString Name;
	float Chance = 0.0f;
};

/** One boss from Data/bosse.json. */
struct EISENSEELEN_API FEIBossValues
{
	FString Name;
	FString Entrance;
	FLinearColor Color = FLinearColor::Gray;
	FEIBannbruch Bannbruch;
	FEIEnemyValues Fight;
	FEIStomp Stomp;
	FEISummon Summon;
	FEIVanish Vanish;
	float GoldFrom = 0.0f;
	float GoldTo = 0.0f;
	TArray<FEILootItem> Loot;

	/** Every player rolls for themselves, so this is called once per player. */
	int32 RollGold() const;
	TArray<FString> RollLoot() const;
};

/** A chance said in words, from this share upwards. */
struct EISENSEELEN_API FEIChanceWord
{
	float From = 0.0f;
	FString Word;
};

/** The numbers of all bosses from Data/bosse.json. */
struct EISENSEELEN_API FEIBossTable
{
	float LeashMeters = 32.0f;
	float RecoverySharePerSecond = 0.05f;
	float LifePerExtraPlayer = 0.65f;
	float FireMinDistanceMeters = 100.0f;
	/** How many hunt letters hang at the dock. Never more than there are bosses. */
	int32 HuntLetters = 3;
	/** Highest share first. */
	TArray<FEIChanceWord> ChanceWords;
	TArray<FEIBossValues> Bosses;

	/** "häufig", "selten" and so on, as the file says. */
	FString DescribeChance(float Chance) const;

	/** OutTable only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEIBossTable& OutTable, FString& OutError);
	static bool LoadFromProjectFile(FEIBossTable& OutTable, FString& OutError);
};
