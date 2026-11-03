#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIRunDirector.generated.h"

class AEIAltar;
class AEIBeacon;
class AEIChest;
class AEIHeroCharacter;
class AEISkyBarge;
enum class EEIRunPhase : uint8;

/** The numbers of a run from Data/lauf.json. */
struct EISENSEELEN_API FEIRunValues
{
	float CampFactor = 1.0f;
	float CampRadiusMeters = 3.0f;
	float ChestsPerRun = 5.0f;
	float ChestReachMeters = 3.0f;
	float ChestZoneMeters = 10.0f;
	float ChestGuards = 8.0f;
	float ChestGuardDistanceMeters = 6.0f;
	float RitualAltars = 3.0f;
	float AltarHoldSeconds = 30.0f;
	float AltarZoneMeters = 8.0f;
	float BeaconHoldSeconds = 60.0f;
	float BeaconReachMeters = 4.0f;
	float BeaconZoneMeters = 15.0f;
	float BargeDistanceMeters = 12.0f;
	float BargeDepartMetersPerSecond = 6.0f;
	float CorruptionEverySeconds = 120.0f;
	float CorruptionMaxStage = 6.0f;
	float CorruptionThreatPerStage = 0.12f;
	float CorruptionStrengthPerStage = 0.1f;

	static bool LoadFromProjectFile(FEIRunValues& OutValues, FString& OutError);
};

/**
 * Leads a run from the jump off the barge to the flight home. Lives on the game mode, so only the server has it, and
 * only does anything on a map with a barge. What the players need to know goes through the game state.
 */
UCLASS()
class EISENSEELEN_API UEIRunDirector : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIRunDirector();

	void LoadValues();
	const FEIRunValues& GetValues() const { return Values; }
	void ApplyValues(const FEIRunValues& NewValues) { Values = NewValues; }

	/** Puts everything back to the start of a run. */
	void ResetRun();

	/** A hero pressed the use key. */
	void Interact(AEIHeroCharacter* Hero);

	AEISkyBarge* GetBarge() const { return Barge; }
	AEIAltar* GetActiveAltar() const;

	/** The clock raises it by itself. Directly only for preview pictures. */
	void SetCorruptionStage(int32 Stage);

	/** For preview pictures, which need not wait: the active altar needs no more time. */
	void SkipAltarHold() { AltarHoldLeft = 0.0f; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void HoldAltar(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void PrepareChests();
	void UpdateChests(const TArray<AEIHeroCharacter*>& LivingHeroes);
	int32 CountChestGuards(int32 HeroCount) const;
	bool OpenChest(AEIHeroCharacter* Hero);
	void IgniteBeacon(AEIBeacon* Beacon);
	void HoldBeacon(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void LeaveOnceAllAboard(const TArray<AEIHeroCharacter*>& LivingHeroes);
	void SetHorde(bool bActive);
	void UpdateCorruption();
	void UpdateThreatBudget(EEIRunPhase Phase, int32 HeroCount, float DeltaTime);
	void FillCamps(int32 HeroCount);
	float GetPlayerFactor(int32 HeroCount) const;

	UPROPERTY(Transient)
	TObjectPtr<AEISkyBarge> Barge;

	UPROPERTY(Transient)
	TObjectPtr<AEIBeacon> LitBeacon;

	/** The altars of this run in the order they wake up. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AEIAltar>> AltarOrder;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEIChest>> ChestsInUse;

	FEIRunValues Values;
	bool bPrepared = false;
	float HoldLeft = 0.0f;
	int32 AltarsBroken = 0;
	float AltarHoldLeft = 0.0f;
	float WaveClock = 0.0f;
};
