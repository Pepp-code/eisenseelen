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

	/** The rune a new hero of this player has equipped. It survives a restart too. */
	int32 GetSelectedRune() const { return SelectedRune; }

	/** For preview pictures: presses or releases a key through the real controls. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewKey(FName KeyName, bool bPressed);

	/** For preview pictures: the hero survives everything, so a long scene can be photographed to its end. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewInvulnerable();

	/** For preview pictures: jumps to a stage of the corruption. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewCorruption(int32 Stage);

	/** For preview pictures: the active altar breaks as soon as a hero stands in its zone. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewBreakAltar();

	/** For preview pictures: a group of that name appears some way in front of the hero. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewSpawnGroup(const FString& Name);

	/** For preview pictures: all guards of chests fall at once. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewDefeatGuards();

	/** X turns right, Y looks up. Takes the mouse factor from the hero's values. */
	void ApplyLookInput(const FVector2D& Input);

protected:
	virtual void SetupInputComponent() override;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpEnded();
	void HandleSprintStarted();
	void HandleSprintEnded();
	void HandleDodge();
	void HandlePrimaryPressed();
	void HandlePrimaryReleased();
	void HandleSecondaryPressed();
	void HandleSecondaryReleased();
	void HandleInteract();
	void HandleHorde();
	void HandleRestart();
	void HandleSpell();
	void HandleRuneSpell();
	void HandleClass();
	void HandleNextRune();

	UFUNCTION(Server, Reliable)
	void ServerInteract();

	UFUNCTION(Server, Reliable)
	void ServerToggleClass();

	UFUNCTION(Server, Reliable)
	void ServerNextRune();

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
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PrimaryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SecondaryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> HordeAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RestartAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SpellAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RuneSpellAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ClassAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> NextRuneAction;

	EEIHeroClass SelectedClass = EEIHeroClass::Bladewarden;
	int32 SelectedRune = 0;
};
