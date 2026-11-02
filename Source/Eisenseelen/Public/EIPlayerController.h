#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EIPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/** Holds all controls. They are built in code, so they can change without touching an asset. */
UCLASS()
class EISENSEELEN_API AEIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** X turns right, Y looks up. Takes the mouse factor from the hero's values. */
	void ApplyLookInput(const FVector2D& Input);

protected:
	virtual void SetupInputComponent() override;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpEnded();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;
};
