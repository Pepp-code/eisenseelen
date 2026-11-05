#pragma once

#include "CoreMinimal.h"
#include "EIEnemy.h"

/** How a boss comes into play. Each boss has its own way, and each way is a line in Data/bosse.json. */
enum class EEIBannbruchKind : uint8
{
	AnchorStones
};

/** The task before a boss. */
struct EISENSEELEN_API FEIBannbruch
{
	EEIBannbruchKind Kind = EEIBannbruchKind::AnchorStones;
	FString Name;
	FString PartName;
	FString Hint;
	int32 Count = 3;
	float HoldSeconds = 20.0f;
	float ZoneMeters = 7.0f;
	float DistanceMeters = 18.0f;
};

/** The attack that hits everybody around a boss. */
struct EISENSEELEN_API FEIStomp
{
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
	float GoldFrom = 0.0f;
	float GoldTo = 0.0f;
	TArray<FEILootItem> Loot;

	/** Every player rolls for themselves, so this is called once per player. */
	int32 RollGold() const;
	TArray<FString> RollLoot() const;
};

/** The numbers of all bosses from Data/bosse.json. */
struct EISENSEELEN_API FEIBossTable
{
	float LeashMeters = 32.0f;
	float RecoverySharePerSecond = 0.05f;
	float LifePerExtraPlayer = 0.65f;
	float FireMinDistanceMeters = 100.0f;
	TArray<FEIBossValues> Bosses;

	/** OutTable only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEIBossTable& OutTable, FString& OutError);
	static bool LoadFromProjectFile(FEIBossTable& OutTable, FString& OutError);
};
