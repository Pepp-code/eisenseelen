#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EIHeroMovementComponent.generated.h"

struct FEIHeroValues;

/**
 * The hero's movement: sprint, the slower pace of a block and the dive from the barge on top of the engine's
 * character movement. All of it runs inside the predicted move, so a client feels it at once and the server still decides.
 */
UCLASS()
class EISENSEELEN_API UEIHeroMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	/** SpeedFactor raises running, sprinting and the pace of a block, and nothing else. */
	void ApplyValues(const FEIHeroValues& Values, float SpeedFactor = 1.0f);

	void SetWantsToSprint(bool bWants) { bWantsToSprint = bWants; }
	void SetWantsToBlock(bool bWants) { bWantsToBlock = bWants; }
	bool IsBlocking() const { return bWantsToBlock; }

	/** How long the block has been up. It only counts on the server, which decides about every hit. */
	float GetBlockHeldSeconds() const { return BlockHeldSeconds; }

	virtual float GetMaxSpeed() const override;
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

private:
	friend class FEIHeroSavedMove;

	void SteerTowardsInput(float DeltaTime);

	float SprintSpeedCm = 0.0f;
	float BlockSpeedCm = 0.0f;
	float AirTurnDegreesPerSecond = 0.0f;

	// The predicted state. FEIHeroSavedMove saves it, so a replayed move starts from the same values.
	bool bWantsToSprint = false;
	bool bWantsToBlock = false;

	float BlockHeldSeconds = 0.0f;
};
