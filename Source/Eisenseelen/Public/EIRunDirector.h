#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIBosses.h"
#include "EIContracts.h"
#include "EIEnemyGroups.h"
#include "EIRunDirector.generated.h"

class AEIAnchorStone;
class AEIBoss;
class AEIChest;
class AEIEnemy;
class AEIEnemyCamp;
class AEIHeroCharacter;
class AEISignalFire;
class AEISkyBarge;
enum class EEIRunPhase : uint8;

/** The numbers of a run from Data/lauf.json. */
struct EISENSEELEN_API FEIRunValues
{
	float CampFactor = 1.0f;
	float CampRadiusMeters = 3.0f;
	float CampRefillFromMeters = 40.0f;
	float ChestReachMeters = 3.0f;
	float ChestZoneMeters = 10.0f;
	float ChestGuardDistanceMeters = 6.0f;
	float CastOffSeconds = 3.0f;
	float CrossingSeconds = 4.0f;
	float ArrivalSpacingMeters = 2.5f;
	float ArrivalDepartMetersPerSecond = 14.0f;
	float SignalHoldSeconds = 60.0f;
	float SignalReachMeters = 4.0f;
	float SignalZoneMeters = 15.0f;
	float BargeDistanceMeters = 12.0f;
	float BargeDepartMetersPerSecond = 6.0f;
	EEIEnemyKind LeaderKind = EEIEnemyKind::ShieldBearer;
	float LeaderLifeFactor = 2.5f;
	float LeaderDamageFactor = 1.5f;
	float LeaderScaleFactor = 1.25f;
	float CorruptionEverySeconds = 120.0f;
	float CorruptionMaxStage = 6.0f;
	float CorruptionStrengthPerStage = 0.1f;

	static bool LoadFromProjectFile(FEIRunValues& OutValues, FString& OutError);
};

/** A place at the edge of the map where the barge sets the team down, from Data/laufkarte.json. */
struct EISENSEELEN_API FEIArrivalPoint
{
	FVector Location = FVector::ZeroVector;
	float YawDegrees = 0.0f;
};

/**
 * Leads a run from the dock to the flight home. Lives on the game mode, so only the server has it, and only does anything
 * on a map with a barge. What the players need to know goes through the game state.
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

	/** Which contracts a run offers. Tests may change it, and it counts from the next reset on. */
	FEIContractTable& GetContractTable() { return ContractTable; }

	const TArray<FEIArrivalPoint>& GetArrivals() const { return Arrivals; }

	/** Which bosses there are. Tests may change it, and it counts from the next reset on. With an empty table a run has no boss. */
	FEIBossTable& GetBossTable() { return BossTable; }
	AEIBoss* GetBoss() const { return Boss; }
	const FVector& GetBossPlace() const { return BossPlace; }
	const TArray<TObjectPtr<AEIAnchorStone>>& GetAnchors() const { return Anchors; }

	/** Puts everything back to the dock, with new contracts. */
	void ResetRun();

	/** A hero pressed the use key. */
	void Interact(AEIHeroCharacter* Hero);

	/** Whoever stood near when an enemy fell has earned something. */
	void NoteEnemyDied(const AEIEnemy& Enemy);

	AEISkyBarge* GetBarge() const { return Barge; }
	AEISignalFire* GetActiveFire() const { return ActiveFire; }

	/** The crossing is over: the barge sets everybody who lives down at the edge of the map. Tests and preview pictures call it directly, to skip the way there. */
	void BeginRunNow();

	/** The clock raises it by itself. Directly only for preview pictures. */
	void SetCorruptionStage(int32 Stage);

	/** For tests and preview pictures: the Bannbruch is done at once, as if somebody had just finished it. */
	void SkipBannbruch();

	/** For tests and preview pictures: so many open contracts count as done, as if somebody had just finished them. */
	void SkipContracts(int32 Count);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void PrepareBoss();
	void PrepareContracts();
	void PrepareSignalFire();
	void SpawnLeaders();
	void SpawnBoss(int32 HeroCount);
	void UpdateBannbruch(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void BreakBossFree(int32 HeroCount);
	void BossFell();
	bool IsAnchorHeld() const;
	void UpdateDock(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void UpdateContracts(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void UpdateChest(AEIChest* Chest, const TArray<AEIHeroCharacter*>& LivingHeroes);
	void CompleteContract(int32 Index);
	bool OpenChest(AEIHeroCharacter* Hero);
	bool FindGround(const FVector& Location, FVector& OutGround) const;
	void LightSignal();
	void HoldSignal(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void LeaveOnceAllAboard(const TArray<AEIHeroCharacter*>& LivingHeroes);
	void UpdateCorruption();
	void UpdateEnemyGroups(EEIRunPhase Phase, const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void FillCamp(AEIEnemyCamp* Camp, const FString& Size, int32 HeroCount);
	void RefillEmptyCamps(const TArray<AEIHeroCharacter*>& LivingHeroes, const FString& Size);
	bool SendGroup(const FString& Size, int32 HeroCount);
	float GetPlayerFactor(int32 HeroCount) const;
	void EndRunForAll(bool bEscaped);

	UPROPERTY(Transient)
	TObjectPtr<AEISkyBarge> Barge;

	UPROPERTY(Transient)
	TObjectPtr<AEISignalFire> ActiveFire;

	UPROPERTY(Transient)
	TObjectPtr<AEIBoss> Boss;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEIAnchorStone>> Anchors;

	FEIBossTable BossTable;
	TArray<FVector> BossPlaces;
	int32 BossIndex = INDEX_NONE;
	FVector BossPlace = FVector::ZeroVector;
	FEIRunValues Values;
	FEIEnemyGroups Groups;
	FEIContractTable ContractTable;
	TArray<FEIArrivalPoint> Arrivals;
	bool bPrepared = false;
	float CastOffLeft = 0.0f;
	float CrossingLeft = 0.0f;
	float HoldLeft = 0.0f;
	float WaveLeft = 0.0f;
	float ReinforcementLeft = 0.0f;
	int32 LastStage = 0;
};
