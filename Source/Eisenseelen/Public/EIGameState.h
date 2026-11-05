#pragma once

#include "CoreMinimal.h"
#include "EIContracts.h"
#include "GameFramework/GameStateBase.h"
#include "EIGameState.generated.h"

/** Where a run stands. None means this map has no run, like the test arena. */
UENUM()
enum class EEIRunPhase : uint8
{
	None,
	AtDock,
	Crossing,
	Exploring,
	SignalLit,
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
	AEIGameState();

	bool IsHordeActive() const { return bHordeActive; }
	void SetHordeActive(bool bActive) { bHordeActive = bActive; }

	EEIRunPhase GetRunPhase() const { return RunPhase; }
	float GetRunSeconds() const;
	float GetSignalHoldLeft() const { return SignalHoldLeft; }
	bool IsSignalHeld() const { return bSignalHeld; }

	/** Everybody is aboard, and the barge is about to leave the dock. */
	bool IsCastingOff() const { return bCastingOff; }

	const TArray<FEIContract>& GetContracts() const { return Contracts; }
	int32 CountContractsDone() const;

	/** Whether somebody stands in the zone of a contract right now. */
	bool IsContractHeld() const;

	/** The run is on, and the fire does not burn yet. */
	bool CanLightSignal() const;

	int32 GetCorruptionStage() const { return CorruptionStage; }
	bool IsBloodMoon() const { return bBloodMoon; }

	/** Seconds of the note left after the corruption rose. For the HUD. */
	float GetCorruptionNote() const { return CorruptionNoteRemaining; }

	/** Server only. */
	void SetRunPhase(EEIRunPhase NewPhase) { RunPhase = NewPhase; }
	void ResetRunClock();
	void StartRunClock();
	void StopRunClock();
	void SetSignalHold(float SecondsLeft, bool bHeld);
	void SetCastingOff(bool bNewCastingOff) { bCastingOff = bNewCastingOff; }
	void SetContracts(const TArray<FEIContract>& NewContracts);
	TArray<FEIContract>& EditContracts() { return Contracts; }

	/** Share is how far along the way to the worst the sky should be, from 0 to 1. */
	void SetCorruption(int32 Stage, float Share, bool bIsBloodMoon);

	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_CorruptionStage(int32 PreviousStage);

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
	float SignalHoldLeft = 0.0f;

	UPROPERTY(Replicated)
	bool bSignalHeld = false;

	UPROPERTY(Replicated)
	bool bCastingOff = false;

	UPROPERTY(Replicated)
	TArray<FEIContract> Contracts;

	UPROPERTY(ReplicatedUsing = OnRep_CorruptionStage)
	int32 CorruptionStage = 0;

	UPROPERTY(Replicated)
	float CorruptionShare = 0.0f;

	UPROPERTY(Replicated)
	bool bBloodMoon = false;

	float CorruptionLook = 0.0f;
	float CorruptionNoteRemaining = 0.0f;
};
