#pragma once

#include "CoreMinimal.h"
#include "EIEnemy.h"

/** One group from Data/gruppen.json: who walks together. */
struct EISENSEELEN_API FEIEnemyGroup
{
	FString Name;
	FString Size;
	int32 Counts[EIEnemyKindCount] = {};

	/** Everyone in the group, each kind multiplied and rounded. A factor of 0 leaves nobody, any other at least one of each. */
	TArray<EEIEnemyKind> ListMembers(float CountFactor) const;
};

/** What comes under the blood moon at one kind of task: groups of this size, one after the other. A size that stays empty sends nobody. */
struct EISENSEELEN_API FEIWaveRule
{
	FString Size;
	float EverySeconds = 10.0f;
};

/** The table of enemy groups, and what comes when. All numbers are for one player. */
struct EISENSEELEN_API FEIEnemyGroups
{
	TArray<FEIEnemyGroup> Groups;
	FString ChestGuards;
	float ThreatCap = 16.0f;
	float BloodMoonLingerSeconds = 8.0f;
	FEIWaveRule AltarWave;
	FEIWaveRule StoneWave;
	FEIWaveRule FightWave;
	FEIWaveRule PursuitWave;
	FEIWaveRule FireWave;

	/** A random group of this size, or null if there is none. */
	const FEIEnemyGroup* Pick(const FString& Size) const;

	/** OutGroups only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEIEnemyGroups& OutGroups, FString& OutError);
	static bool LoadFromProjectFile(FEIEnemyGroups& OutGroups, FString& OutError);
};
