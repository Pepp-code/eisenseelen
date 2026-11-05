#pragma once

#include "CoreMinimal.h"
#include "EIContracts.generated.h"

/** What a contract asks for. */
UENUM()
enum class EEIContractKind : uint8
{
	Leader,
	Shrine,
	Chest,
	Altar
};

/** One line of the section aufgaben of Data/nebenauftraege.json. */
struct EISENSEELEN_API FEIContractTask
{
	EEIContractKind Kind = EEIContractKind::Leader;
	FString Name;
	FString ShortName;
	float HoldSeconds = 0.0f;
	float ZoneMeters = 0.0f;
};

/** Who gives contracts and pays in their own points. A new faction is a new line in the file. */
struct EISENSEELEN_API FEIFaction
{
	FString Name;
	FLinearColor Color = FLinearColor::White;
};

/** A small job on the side, which pays in gold and in the points of its faction. Every player sees the list from the arrival on. */
USTRUCT()
struct EISENSEELEN_API FEIContract
{
	GENERATED_BODY()

	UPROPERTY()
	EEIContractKind Kind = EEIContractKind::Leader;

	UPROPERTY()
	int32 Faction = 0;

	/** Where to go. It follows a leader around. */
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	bool bDone = false;

	/** Seconds still to hold, for the kinds that are held. */
	UPROPERTY()
	float HoldLeft = 0.0f;

	UPROPERTY()
	bool bHeld = false;

	/** The altar, chest, camp or leader behind it. Only the server needs to know. */
	UPROPERTY(NotReplicated)
	TObjectPtr<AActor> Target;
};

/** The numbers of the side jobs from Data/nebenauftraege.json. */
struct EISENSEELEN_API FEIContractTable
{
	float ContractsPerRun = 3.0f;
	float MinDistanceMeters = 40.0f;
	float Gold = 100.0f;
	float Points = 25.0f;
	float CorruptionBonusPerStage = 0.1f;
	float KeptOnFailure = 0.3f;

	/** How long a side of the run map is, for the overview. From Data/laufkarte.json. */
	float MapMeters = 300.0f;

	TArray<FEIFaction> Factions;
	TArray<FEIContractTask> Tasks;

	const FEIContractTask* FindTask(EEIContractKind Kind) const;
	const FEIFaction* FindFaction(int32 Faction) const;

	/** What one contract pays at a stage of the corruption. */
	int32 GetGold(int32 Stage) const;
	int32 GetPoints(int32 Stage) const;

	/** What is left of an amount after a failed run. */
	int32 GetKeptOnFailure(int32 Amount) const;

	/** The jobs of a run, sorted by faction and without a place yet. Tasks take turns, so the same one only comes twice once all others have been there. Factions are pure chance. */
	TArray<FEIContract> RollContracts() const;

	/** OutTable only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEIContractTable& OutTable, FString& OutError);
	static bool LoadFromProjectFile(FEIContractTable& OutTable, FString& OutError);
};
