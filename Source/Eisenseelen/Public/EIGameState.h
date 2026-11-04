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
	OnBarge,
	Contracts,
	FlareLit,
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
	float GetFlareHoldLeft() const { return FlareHoldLeft; }
	bool IsFlareHeld() const { return bFlareHeld; }

	const TArray<FEIContract>& GetContracts() const { return Contracts; }
	int32 CountContractsDone() const;
	int32 GetContractsToFlare() const { return ContractsToFlare; }

	/** Whether somebody stands in the zone of a contract right now. */
	bool IsContractHeld() const;

	/** Enough contracts are done, and the flare has not been fired yet. */
	bool CanFireFlare() const;

	int32 GetCorruptionStage() const { return CorruptionStage; }
	bool IsBloodMoon() const { return bBloodMoon; }

	/** Seconds of the note left after the corruption rose. For the HUD. */
	float GetCorruptionNote() const { return CorruptionNoteRemaining; }

	/** Server only. */
	void SetRunPhase(EEIRunPhase NewPhase) { RunPhase = NewPhase; }
	void ResetRunClock();
	void StartRunClock();
	void StopRunClock();
	void SetFlareHold(float SecondsLeft, bool bHeld);
	void SetContracts(const TArray<FEIContract>& NewContracts, int32 ToFlare);
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
	float FlareHoldLeft = 0.0f;

	UPROPERTY(Replicated)
	bool bFlareHeld = false;

	UPROPERTY(Replicated)
	TArray<FEIContract> Contracts;

	UPROPERTY(Replicated)
	int32 ContractsToFlare = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CorruptionStage)
	int32 CorruptionStage = 0;

	UPROPERTY(Replicated)
	float CorruptionShare = 0.0f;

	UPROPERTY(Replicated)
	bool bBloodMoon = false;

	float CorruptionLook = 0.0f;
	float CorruptionNoteRemaining = 0.0f;
};
