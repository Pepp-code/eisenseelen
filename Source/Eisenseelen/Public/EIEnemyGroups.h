#pragma once

#include "CoreMinimal.h"
#include "EIEnemy.h"

/** One group from Data/gruppen.json: who walks together, and in which stages of the corruption. */
struct EISENSEELEN_API FEIEnemyGroup
{
	FString Name;
	FString Size;
	int32 FromStage = 0;
	int32 ToStage = 0;
	int32 Counts[EIEnemyKindCount] = {};

	/** Everyone in the group, each kind multiplied and rounded. A factor of 0 leaves nobody, any other at least one of each. */
	TArray<EEIEnemyKind> ListMembers(float CountFactor) const;
};

/** What one stage of the corruption sends. A size that stays empty sends nobody. */
struct EISENSEELEN_API FEIGroupStage
{
	FString Camp;
	FString Reinforcement;
	float ReinforcementEverySeconds = 0.0f;
	FString Chest;
	FString AltarWave;
	FString FlareWave;
	float ThreatCap = 0.0f;
};

/** The table of enemy groups. All numbers are for one player. */
struct EISENSEELEN_API FEIEnemyGroups
{
	TArray<FEIEnemyGroup> Groups;
	TArray<FEIGroupStage> Stages;
	float AltarWaveEverySeconds = 12.0f;
	float FlareWaveEverySeconds = 15.0f;

	/** A random group of this size that fits this stage, or null if there is none. */
	const FEIEnemyGroup* Pick(const FString& Size, int32 Stage) const;

	/** A stage beyond the last one counts as the last one. */
	const FEIGroupStage& GetStage(int32 Stage) const;

	/** OutGroups only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEIEnemyGroups& OutGroups, FString& OutError);
	static bool LoadFromProjectFile(FEIEnemyGroups& OutGroups, FString& OutError);
};
