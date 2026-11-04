#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIEnemyGroups.h"
#include "EIRunDirector.generated.h"

class AEIAltar;
class AEIBeacon;
class AEIChest;
class AEIEnemy;
class AEIEnemyCamp;
class AEIHeroCharacter;
class AEISkyBarge;
enum class EEIRewardSource : uint8;
enum class EEIRunPhase : uint8;

/** The numbers of a run from Data/lauf.json. */
struct EISENSEELEN_API FEIRunValues
{
	float CampFactor = 1.0f;
	float CampRadiusMeters = 3.0f;
	float CampRefillFromMeters = 40.0f;
	float ChestsPerRun = 5.0f;
	float ChestReachMeters = 3.0f;
	float ChestZoneMeters = 10.0f;
	float ChestGuardDistanceMeters = 6.0f;
	float RitualAltars = 3.0f;
	float AltarHoldSeconds = 30.0f;
	float AltarZoneMeters = 8.0f;
	float BeaconHoldSeconds = 60.0f;
	float BeaconReachMeters = 4.0f;
	float BeaconZoneMeters = 15.0f;
	float BargeDistanceMeters = 12.0f;
	float BargeDepartMetersPerSecond = 6.0f;
	float LeadersPerRun = 2.0f;
	EEIEnemyKind LeaderKind = EEIEnemyKind::ShieldBearer;
	float LeaderLifeFactor = 2.5f;
	float LeaderDamageFactor = 1.5f;
	float LeaderScaleFactor = 1.25f;
	float ShrinesPerRun = 2.0f;
	float ShrineHoldSeconds = 15.0f;
	float ShrineZoneMeters = 6.0f;
	float CorruptionEverySeconds = 120.0f;
	float CorruptionMaxStage = 6.0f;
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

	/** Who comes when. Tests may change it. */
	FEIEnemyGroups& GetGroups() { return Groups; }

	/** Puts everything back to the start of a run. */
	void ResetRun();

	/** A hero pressed the use key. */
	void Interact(AEIHeroCharacter* Hero);

	/** Whoever stood near when a leader fell has earned the reward. */
	void NoteEnemyDied(const AEIEnemy& Enemy);

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
	bool OpenChest(AEIHeroCharacter* Hero);
	void IgniteBeacon(AEIBeacon* Beacon);
	void HoldBeacon(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void LeaveOnceAllAboard(const TArray<AEIHeroCharacter*>& LivingHeroes);
	void UpdateCorruption();
	void UpdateEnemyGroups(EEIRunPhase Phase, const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void FillCamp(AEIEnemyCamp* Camp, const FString& Size, int32 HeroCount);
	void RefillEmptyCamps(const TArray<AEIHeroCharacter*>& LivingHeroes, const FString& Size);
	bool SendGroup(const FString& Size, int32 HeroCount);
	float GetPlayerFactor(int32 HeroCount) const;
	void EndRunForAll(bool bEscaped);
	void PrepareLeaders();
	void UpdateShrines(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	bool IsShrineHeld() const;
	void GrantNear(const FVector& Location, EEIRewardSource Source);

	UPROPERTY(Transient)
	TObjectPtr<AEISkyBarge> Barge;

	UPROPERTY(Transient)
	TObjectPtr<AEIBeacon> LitBeacon;

	/** The altars of this run in the order they wake up. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AEIAltar>> AltarOrder;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEIChest>> ChestsInUse;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEIAltar>> Shrines;

	FEIRunValues Values;
	FEIEnemyGroups Groups;
	bool bPrepared = false;
	float HoldLeft = 0.0f;
	int32 AltarsBroken = 0;
	float AltarHoldLeft = 0.0f;
	float WaveLeft = 0.0f;
	float ReinforcementLeft = 0.0f;
	int32 LastStage = 0;
};
