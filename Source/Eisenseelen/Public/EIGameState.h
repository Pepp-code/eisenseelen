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

/** Where the boss of a run stands. None means this run has no boss, and the fire burns any time. */
UENUM()
enum class EEIBossStage : uint8
{
	None,
	Sealed,
	Awake,
	Down
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

	/** The run is on, the boss has fallen and the fire does not burn yet. */
	bool CanLightSignal() const;

	EEIBossStage GetBossStage() const { return BossStage; }
	int32 GetBannbruchDone() const { return BannbruchDone; }
	int32 GetBannbruchTotal() const { return BannbruchTotal; }

	/** Whether the sky is red right now, because a task has called the enemies from all sides. */
	bool IsBloodMoon() const { return bBloodMoon; }

	/** Seconds of the note left after the blood moon rose. For the HUD. */
	float GetBloodMoonNote() const { return BloodMoonNoteRemaining; }

	/** Server only. */
	void SetRunPhase(EEIRunPhase NewPhase) { RunPhase = NewPhase; }
	void ResetRunClock();
	void StartRunClock();
	void StopRunClock();
	void SetSignalHold(float SecondsLeft, bool bHeld);
	void SetCastingOff(bool bNewCastingOff) { bCastingOff = bNewCastingOff; }
	void SetBossStage(EEIBossStage NewStage) { BossStage = NewStage; }
	void SetBannbruchProgress(int32 Done, int32 Total);
	void SetContracts(const TArray<FEIContract>& NewContracts);
	TArray<FEIContract>& EditContracts() { return Contracts; }

	void SetBloodMoon(bool bNewBloodMoon);

	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_BloodMoon();

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
	EEIBossStage BossStage = EEIBossStage::None;

	UPROPERTY(Replicated)
	int32 BannbruchDone = 0;

	UPROPERTY(Replicated)
	int32 BannbruchTotal = 0;

	UPROPERTY(Replicated)
	TArray<FEIContract> Contracts;

	UPROPERTY(ReplicatedUsing = OnRep_BloodMoon)
	bool bBloodMoon = false;

	float BloodMoonLook = 0.0f;
	float BloodMoonNoteRemaining = 0.0f;
};
