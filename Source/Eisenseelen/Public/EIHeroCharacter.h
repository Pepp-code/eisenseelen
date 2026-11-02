#pragma once

#include "CoreMinimal.h"
#include "EIHeroValues.h"
#include "GameFramework/Character.h"
#include "EIHeroCharacter.generated.h"

class UCameraComponent;
class UEIHeroMovementComponent;

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

	/** A press shortly before landing counts as a jump on landing, so it does not get lost to bad luck. */
	void PressJump();

	/** Called by the movement component once per dash, not again when a move is replayed. */
	void OnDashPerformed();

	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode = 0) override;

protected:
	virtual void BeginPlay() override;

	/** The one jump stays unused after a dash or a fall, it is only spent by jumping. */
	virtual bool CanJumpWhileFalling() const override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	FEIHeroValues Values;
	double BufferedJumpUntil = -1.0;
	bool bJumpOnGround = false;
};
