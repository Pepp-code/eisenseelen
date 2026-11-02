#pragma once

#include "CoreMinimal.h"
#include "EIHeroCharacter.h"
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
	/** The class a new hero of this player starts as. It survives a restart. */
	EEIHeroClass GetSelectedClass() const { return SelectedClass; }

	/** For preview pictures: presses or releases a key through the real controls. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewKey(FName KeyName, bool bPressed);

	/** X turns right, Y looks up. Takes the mouse factor from the hero's values. */
	void ApplyLookInput(const FVector2D& Input);

protected:
	virtual void SetupInputComponent() override;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpEnded();
	void HandleDash();
	void HandlePrimaryPressed();
	void HandlePrimaryReleased();
	void HandleBattleCry();
	void HandleHorde();
	void HandleRestart();
	void HandleSpell();
	void HandleClass();

	UFUNCTION(Server, Reliable)
	void ServerToggleClass();

	UFUNCTION(Server, Reliable)
	void ServerToggleHorde();

	UFUNCTION(Server, Reliable)
	void ServerRestartRun();

	AEIHeroCharacter* GetLivingHero() const;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PrimaryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> BattleCryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> HordeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RestartAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SpellAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ClassAction;

	EEIHeroClass SelectedClass = EEIHeroClass::Bladewarden;
};
