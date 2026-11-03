#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIRunDirector.generated.h"

class AEIBeacon;
class AEIHeroCharacter;
class AEISkyBarge;

/** The numbers of a run from Data/lauf.json. */
struct EISENSEELEN_API FEIRunValues
{
	float BeaconHoldSeconds = 60.0f;
	float BeaconReachMeters = 4.0f;
	float BeaconZoneMeters = 15.0f;
	float BargeDistanceMeters = 12.0f;
	float BargeDepartMetersPerSecond = 6.0f;

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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void IgniteBeacon(AEIBeacon* Beacon);
	void HoldBeacon(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void LeaveOnceAllAboard(const TArray<AEIHeroCharacter*>& LivingHeroes);
	void SetHorde(bool bActive);

	UPROPERTY(Transient)
	TObjectPtr<AEISkyBarge> Barge;

	UPROPERTY(Transient)
	TObjectPtr<AEIBeacon> LitBeacon;

	FEIRunValues Values;
	bool bPrepared = false;
	float HoldLeft = 0.0f;
};
