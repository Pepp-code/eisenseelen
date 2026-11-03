#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EIHeroMovementComponent.generated.h"

struct FEIHeroValues;

/**
 * The hero's movement: sprint, dodge and the slower pace of a block on top of the engine's character movement. All of
 * it runs inside the predicted move, so a client feels it at once and the server still decides.
 */
UCLASS()
class EISENSEELEN_API UEIHeroMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	void ApplyValues(const FEIHeroValues& Values);

	void SetWantsToSprint(bool bWants) { bWantsToSprint = bWants; }
	void SetWantsToBlock(bool bWants) { bWantsToBlock = bWants; }
	bool IsBlocking() const { return bWantsToBlock; }

	/** How long the block has been up. It only counts on the server, which decides about every hit. */
	float GetBlockHeldSeconds() const { return BlockHeldSeconds; }

	void RequestDodge() { bWantsToDodge = true; }
	bool IsDodgeReady() const { return DodgeCooldownRemaining <= 0.0f; }
	float GetDodgeCooldownRemaining() const { return DodgeCooldownRemaining; }
	float GetDodgeCooldownSeconds() const { return DodgeCooldownSeconds; }

	virtual float GetMaxSpeed() const override;
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

private:
	friend class FEIHeroSavedMove;

	void PerformDodge();
	void SteerTowardsInput(float DeltaTime);

	float SprintSpeedCm = 0.0f;
	float BlockSpeedCm = 0.0f;
	float DodgeSpeedCm = 0.0f;
	float DodgeHopCm = 0.0f;
	float DodgeCooldownSeconds = 0.0f;
	float AirTurnDegreesPerSecond = 0.0f;

	// The predicted state. FEIHeroSavedMove saves it, so a replayed move starts from the same values.
	bool bWantsToSprint = false;
	bool bWantsToBlock = false;
	bool bWantsToDodge = false;
	float DodgeCooldownRemaining = 0.0f;

	float BlockHeldSeconds = 0.0f;
};
