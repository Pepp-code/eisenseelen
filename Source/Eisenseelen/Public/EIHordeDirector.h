#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIHordeDirector.generated.h"

class AEIEnemy;

/** How much threat may be after the heroes during a run, from Data/gegner.json. */
struct EISENSEELEN_API FEIDirectorValues
{
	float ThreatPerStage = 2.0f;
	float AltarThreat = 5.0f;
	float BeaconThreat = 7.0f;
	float PeakThreat = 22.0f;
	float WaveEverySeconds = 12.0f;
	float WaveBurstSeconds = 3.0f;
	float QuietSpawnShare = 0.35f;
	float PlayerShare[4] = { 0.45f, 0.7f, 0.85f, 1.0f };
};

/** The numbers of the horde from Data/gegner.json. */
struct EISENSEELEN_API FEIHordeValues
{
	float MaxAlive = 20.0f;
	float SpawnsPerSecond = 3.0f;
	float MinDistanceMeters = 25.0f;
	float MaxDistanceMeters = 40.0f;
	float TooFarMeters = 80.0f;
	FEIDirectorValues Director;

	static bool LoadFromProjectFile(FEIHordeValues& OutValues, FString& OutError);
};

/**
 * Keeps enemies after the heroes, out of their sight, up to a budget of threat. They come out of the camps nearby, or out
 * of nowhere on a map without camps. In the test arena the budget is fixed, during a run the run director sets it. Lives
 * on the game mode, so only the server has it.
 */
UCLASS()
class EISENSEELEN_API UEIHordeDirector : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIHordeDirector();

	void LoadValues();
	const FEIHordeValues& GetValues() const { return Values; }
	void ApplyValues(const FEIHordeValues& NewValues) { Values = NewValues; }

	bool IsHordeActive() const { return bHordeActive; }
	void SetHordeActive(bool bActive);
	void RemoveAllEnemies();
	int32 CountEnemies() const;
	float CountThreat() const;

	/** How much threat may live at once, and which share of the normal spawn rate brings it in. */
	void SetThreatBudget(float Threat, float SpawnRateShare);
	float GetThreatBudget() const { return ThreatBudget; }

	/** How strong the enemies are that spawn from now on. */
	void SetEnemyStrength(float Factor) { EnemyStrength = Factor; }

	/** Spawns one enemy at a random place around a random hero. Returns null if no place worked out this time. */
	AEIEnemy* SpawnOne();

	/** Spawns guards in a circle around a place, in plain sight. Places where it did not work out stay empty. */
	TArray<AEIEnemy*> SpawnGuards(const FVector& Center, float DistanceCm, int32 Count);

	/** Spawns enemies around a place who stand watch there until they notice a hero. */
	void SpawnWatchers(const FVector& Center, float DistanceCm, int32 Count);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool FindStandingPlace(const FVector& Ground, FVector& OutLocation) const;
	AEIEnemy* SpawnAt(const FVector& Location);
	void RemoveEnemiesLeftBehind();

	FEIHordeValues Values;
	bool bHordeActive = false;
	float SpawnBudget = 0.0f;
	float ThreatBudget = 0.0f;
	float SpawnShare = 1.0f;
	float EnemyStrength = 1.0f;
	float LeftBehindCheckRemaining = 0.0f;
};
