#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIHordeDirector.generated.h"

class AEIEnemy;

/** The numbers of the horde from Data/gegner.json. */
struct EISENSEELEN_API FEIHordeValues
{
	float MaxAlive = 20.0f;
	float SpawnsPerSecond = 3.0f;
	float MinDistanceMeters = 25.0f;
	float MaxDistanceMeters = 40.0f;
	float HeroLife = 150.0f;

	static bool LoadFromProjectFile(FEIHordeValues& OutValues, FString& OutError);
};

/** Keeps a number of enemies alive around the heroes. Lives on the game mode, so only the server has it. */
UCLASS()
class EISENSEELEN_API UEIHordeDirector : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIHordeDirector();

	void LoadValues();
	const FEIHordeValues& GetValues() const { return Values; }

	bool IsHordeActive() const { return bHordeActive; }
	void SetHordeActive(bool bActive);
	void RemoveAllEnemies();
	int32 CountEnemies() const;

	/** Spawns one enemy at a random place around a random hero. Returns null if that place does not work out. */
	AEIEnemy* SpawnOne();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FEIHordeValues Values;
	bool bHordeActive = false;
	float SpawnBudget = 0.0f;
};
