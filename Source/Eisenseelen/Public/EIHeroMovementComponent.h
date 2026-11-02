#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EIHeroMovementComponent.generated.h"

struct FEIHeroValues;

/**
 * The hero's movement: dash and speed chain on top of the engine's character movement. All of it runs inside the
 * predicted move, so a client feels it at once and the server still decides.
 */
UCLASS()
class EISENSEELEN_API UEIHeroMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	void ApplyValues(const FEIHeroValues& Values);

	void RequestDash() { bWantsToDash = true; }
	bool IsDashReady() const { return DashCooldownRemaining <= 0.0f; }
	float GetDashCooldownRemaining() const { return DashCooldownRemaining; }
	float GetDashCooldownSeconds() const { return DashCooldownSeconds; }

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

protected:
	virtual void ProcessLanded(const FHitResult& Hit, float RemainingTime, int32 Iterations) override;

private:
	friend class FEIHeroSavedMove;

	void PerformDash();

	float DashSpeedBonusCm = 0.0f;
	float DashHopCm = 0.0f;
	float DashCooldownSeconds = 0.0f;
	float SpeedChainWindowSeconds = 0.0f;
	float MaxSpeedCm = 0.0f;

	// The predicted state. FEIHeroSavedMove saves it, so a replayed move starts from the same values.
	bool bWantsToDash = false;
	float DashCooldownRemaining = 0.0f;
	float SpeedChainWindowRemaining = 0.0f;
};
