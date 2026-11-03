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
	Ritual,
	RitualDone,
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
	AEIGameState();

	bool IsHordeActive() const { return bHordeActive; }
	void SetHordeActive(bool bActive) { bHordeActive = bActive; }

	EEIRunPhase GetRunPhase() const { return RunPhase; }
	float GetRunSeconds() const;
	float GetBeaconHoldLeft() const { return BeaconHoldLeft; }
	bool IsBeaconHeld() const { return bBeaconHeld; }
	int32 GetAltarsBroken() const { return AltarsBroken; }
	int32 GetAltarsTotal() const { return AltarsTotal; }
	float GetAltarHoldLeft() const { return AltarHoldLeft; }
	bool IsAltarHeld() const { return bAltarHeld; }
	int32 GetCorruptionStage() const { return CorruptionStage; }
	bool IsBloodMoon() const { return bBloodMoon; }

	/** Seconds of the note left after the corruption rose. For the HUD. */
	float GetCorruptionNote() const { return CorruptionNoteRemaining; }

	/** Server only. */
	void SetRunPhase(EEIRunPhase NewPhase) { RunPhase = NewPhase; }
	void ResetRunClock();
	void StartRunClock();
	void StopRunClock();
	void SetBeaconHold(float SecondsLeft, bool bHeld);
	void SetAltarProgress(int32 Broken, int32 Total);
	void SetAltarHold(float SecondsLeft, bool bHeld);

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
	float BeaconHoldLeft = 0.0f;

	UPROPERTY(Replicated)
	bool bBeaconHeld = false;

	UPROPERTY(Replicated)
	int32 AltarsBroken = 0;

	UPROPERTY(Replicated)
	int32 AltarsTotal = 0;

	UPROPERTY(Replicated)
	float AltarHoldLeft = 0.0f;

	UPROPERTY(Replicated)
	bool bAltarHeld = false;

	UPROPERTY(ReplicatedUsing = OnRep_CorruptionStage)
	int32 CorruptionStage = 0;

	UPROPERTY(Replicated)
	float CorruptionShare = 0.0f;

	UPROPERTY(Replicated)
	bool bBloodMoon = false;

	float CorruptionLook = 0.0f;
	float CorruptionNoteRemaining = 0.0f;
};
