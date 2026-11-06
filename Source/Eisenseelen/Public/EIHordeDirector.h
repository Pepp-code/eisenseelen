#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIHordeDirector.generated.h"

class AEIEnemy;
class AEIHeroCharacter;
enum class EEIEnemyKind : uint8;
struct FEIEnemyGroup;

/** How many enemies come for how many players, from Data/gegner.json. */
struct EISENSEELEN_API FEIDirectorValues
{
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
 * Spawns enemies. During a run it does what the run director says and removes hunters left far behind. In the test arena
 * the horde keeps enemies after the hero by itself, up to a limit. Lives on the game mode, so only the server has it.
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

	/** During a run hunters far behind everyone make way, without any horde. */
	void SetRemovingLeftBehind(bool bActive) { bRemovingLeftBehind = bActive; }

	/** Spawns one bone runner out of sight around a random hero. Returns null if no place worked out this time. */
	AEIEnemy* SpawnOne();

	/** Spawns a group in a circle around a place, in plain sight. Places where it did not work out stay empty. */
	TArray<AEIEnemy*> SpawnGroup(const FEIEnemyGroup& Group, float CountFactor, const FVector& Center, float RadiusCm);

	TArray<AEIEnemy*> SpawnEliteAmbushAtLocation(const FVector& Location);

	/** Server only. One enemy of this kind at this place, if there is ground. */
	AEIEnemy* SpawnKind(EEIEnemyKind Kind, const FVector& Center);

	/** Spawns a group out of sight, on any side of a random hero. Empty if no place worked out this time. */
	TArray<AEIEnemy*> SpawnReinforcement(const FEIEnemyGroup& Group, float CountFactor);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool FindStandingPlace(const FVector& Ground, FVector& OutLocation) const;
	AEIEnemy* SpawnAt(const FVector& Location, EEIEnemyKind Kind);
	TArray<AEIHeroCharacter*> GetLivingHeroes() const;
	bool IsBadPlace(const FVector& Ground, const TArray<AEIHeroCharacter*>& Heroes) const;
	bool FindHiddenPlace(const TArray<AEIHeroCharacter*>& Heroes, float RadiusCm, FVector& OutGround) const;
	void RemoveEnemiesLeftBehind();

	FEIHordeValues Values;
	bool bHordeActive = false;
	bool bRemovingLeftBehind = false;
	float SpawnBudget = 0.0f;
	float LeftBehindCheckRemaining = 0.0f;
};
