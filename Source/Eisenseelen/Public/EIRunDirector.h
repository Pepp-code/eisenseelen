#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIContracts.h"
#include "EIEnemyGroups.h"
#include "EIRunDirector.generated.h"

class AEIChest;
class AEIEnemy;
class AEIEnemyCamp;
class AEIFlare;
class AEIHeroCharacter;
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
	float FlareHoldSeconds = 60.0f;
	float FlareZoneMeters = 15.0f;
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

	/** Which contracts a run offers. Tests may change it, and it counts from the next reset on. */
	FEIContractTable& GetContractTable() { return ContractTable; }

	/** Puts everything back to the start of a run, with new contracts. */
	void ResetRun();

	/** A hero pressed the use key. */
	void Interact(AEIHeroCharacter* Hero);

	/** A hero pressed the key for the flare. It works once enough contracts are done and the barge has room to land. */
	void FireFlare(AEIHeroCharacter* Hero);

	/** Whoever stood near when an enemy fell has earned something. */
	void NoteEnemyDied(const AEIEnemy& Enemy);

	AEISkyBarge* GetBarge() const { return Barge; }
	AEIFlare* GetFlare() const { return Flare; }

	/** The clock raises it by itself. Directly only for preview pictures. */
	void SetCorruptionStage(int32 Stage);

	/** For tests and preview pictures: so many open contracts count as done, as if somebody had just finished them. */
	void SkipContracts(int32 Count);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void PrepareContracts();
	void SpawnLeaders();
	void UpdateContracts(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void UpdateChest(AEIChest* Chest, const TArray<AEIHeroCharacter*>& LivingHeroes);
	void CompleteContract(int32 Index);
	bool OpenChest(AEIHeroCharacter* Hero);
	bool FindGround(const FVector& Location, FVector& OutGround) const;
	bool FindBargeSpot(const FVector& FlareGround, FVector& OutHover) const;
	void HoldFlare(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
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
	TObjectPtr<AEIFlare> Flare;

	FEIRunValues Values;
	FEIEnemyGroups Groups;
	FEIContractTable ContractTable;
	bool bPrepared = false;
	float HoldLeft = 0.0f;
	float WaveLeft = 0.0f;
	float ReinforcementLeft = 0.0f;
	int32 LastStage = 0;
};
