#pragma once

#include "CoreMinimal.h"
#include "EIHeroValues.h"
#include "EIPoses.h"
#include "GameFramework/Character.h"
#include "EIHeroCharacter.generated.h"

class UCameraComponent;
class UEIHealthComponent;
class UEIHeroMovementComponent;
class UEISwordComponent;
class UEIViewModelComponent;

/** The first-person hero. There is no model yet: a capsule and a camera. */
UCLASS()
class EISENSEELEN_API AEIHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	explicit AEIHeroCharacter(const FObjectInitializer& ObjectInitializer);

	void ApplyValues(const FEIHeroValues& NewValues);
	const FEIHeroValues& GetValues() const { return Values; }
	UEIHeroMovementComponent* GetHeroMovement() const;

	UEIHealthComponent* GetHealth() const { return Health; }
	bool IsDead() const;

	/** Seconds of the red flash left after taking a hit. For the HUD. */
	float GetHurtFlash() const { return HurtFlashRemaining; }
	UEISwordComponent* GetSword() const { return Sword; }
	const FEIPoseLibrary& GetPoses() const { return Poses; }

	/** Null on everyone but the owner, and until the owner knows it is local. */
	UEIViewModelComponent* GetViewModel() const { return bViewModelBuilt ? ViewModel : nullptr; }

	void AddCameraKick(float Degrees);

	/** Owner only. The server plays it for everyone. */
	void PressBattleCry();

	/** A press shortly before landing counts as a jump on landing, so it does not get lost to bad luck. */
	void PressJump();

	/** Called by the movement component once per dash, not again when a move is replayed. */
	void OnDashPerformed();

	virtual void Landed(const FHitResult& Hit) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode = 0) override;

protected:
	virtual void BeginPlay() override;

	/** The one jump stays unused after a dash or a fall, it is only spent by jumping. */
	virtual bool CanJumpWhileFalling() const override;

private:
	UFUNCTION(Server, Reliable)
	void ServerBattleCry();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBattleCry();

	UFUNCTION(Client, Unreliable)
	void ClientHurt();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDied();

	void HandleDied();

	void UpdateCameraKick(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIViewModelComponent> ViewModel;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIHealthComponent> Health;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEISwordComponent> Sword;

	FEIHeroValues Values;
	FEIPoseLibrary Poses;
	bool bViewModelBuilt = false;
	float CameraKickRemaining = 0.0f;
	float CameraKickDegrees = 0.0f;
	double NextBattleCryAt = 0.0;
	float HurtFlashRemaining = 0.0f;
	double BufferedJumpUntil = -1.0;
	bool bJumpOnGround = false;
};
