#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "EIGameState.generated.h"

/** Where a run stands. None means this map has no run, like the test arena. */
UENUM()
enum class EEIRunPhase : uint8
{
	None,
	OnBarge,
	Exploring,
	BeaconLit,
	BargeWaiting,
	Escaped,
	Failed
};

/** What every player needs to know about the session. */
UCLASS()
class EISENSEELEN_API AEIGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	bool IsHordeActive() const { return bHordeActive; }
	void SetHordeActive(bool bActive) { bHordeActive = bActive; }

	EEIRunPhase GetRunPhase() const { return RunPhase; }
	float GetRunSeconds() const;
	float GetBeaconHoldLeft() const { return BeaconHoldLeft; }
	bool IsBeaconHeld() const { return bBeaconHeld; }

	/** Server only. */
	void SetRunPhase(EEIRunPhase NewPhase) { RunPhase = NewPhase; }
	void ResetRunClock();
	void StartRunClock();
	void StopRunClock();
	void SetBeaconHold(float SecondsLeft, bool bHeld);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	bool bHordeActive = false;

	UPROPERTY(Replicated)
	EEIRunPhase RunPhase = EEIRunPhase::None;

	UPROPERTY(Replicated)
	double RunStartedAt = -1.0;

	/** Negative while the clock runs. */
	UPROPERTY(Replicated)
	float RunEndedAfter = 0.0f;

	UPROPERTY(Replicated)
	float BeaconHoldLeft = 0.0f;

	UPROPERTY(Replicated)
	bool bBeaconHeld = false;
};
