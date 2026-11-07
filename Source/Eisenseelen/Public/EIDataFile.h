#pragma once

#include "CoreMinimal.h"
#include "EIDataFile.generated.h"

class FJsonObject;

USTRUCT(BlueprintType)
struct EISENSEELEN_API FEIWorldMutatorDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Id = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float IntervalSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RuneFieldScaleFactor = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LightFactor = 1.0f;
};

USTRUCT(BlueprintType)
struct EISENSEELEN_API FEISoulCargoDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Id = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CarrierSpeedFactor = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThrowDamage = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ThrowRadius = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KettleChargeBonus = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SoulValue = 85;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ThrowSoundId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ExplosionSoundId;
};

USTRUCT(BlueprintType)
struct EISENSEELEN_API FEIIslandEventsDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CursedChestChance = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CursedChestDurationSeconds = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CursedChestRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KettleOverchargeWindowSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float KettleOverchargeRewardMultiplier = 2.0f;
};

/**
 * The tuning files in Data/. They are read when play starts, so a changed value only needs another press of Play.
 * Errors are in German, because the developer reads them on screen after editing a file by hand.
 */
struct EISENSEELEN_API FEIDataFile
{
	static TSharedPtr<FJsonObject> Load(const FString& FileName, FString& OutError);
	static TSharedPtr<FJsonObject> Parse(const FString& Json, FString& OutError);
	static bool ReadNumber(const FJsonObject& Object, const TCHAR* Key, double Min, double Max, float& OutNumber, FString& OutError);

	/** Editing a file by hand must not stop the game: says what is wrong, and the game plays on with default values. */
	static void ReportError(const FString& FileName, const FString& Error);

	static const FEIWorldMutatorDef* FindWorldMutator(FName MutatorId);
	static const FEISoulCargoDef* FindSoulCargoDef(FName CargoId);
	static const FEIIslandEventsDef& GetIslandEventsDef();
	static const TArray<FEIWorldMutatorDef>& GetWorldMutators();

	static void ReloadData();

private:
	static void EnsureLoaded();
	static bool bDataLoaded;
	static TArray<FEIWorldMutatorDef> CachedMutators;
	static TArray<FEISoulCargoDef> CachedSoulCargo;
	static FEIIslandEventsDef CachedIslandEvents;
};
