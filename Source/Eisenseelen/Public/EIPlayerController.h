#pragma once

#include "CoreMinimal.h"
#include "EIGearMenu.h"
#include "EIHeroCharacter.h"
#include "GameFramework/PlayerController.h"
#include "EIPlayerController.generated.h"

class AEIForge;
class UEIProfileComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/** Holds all controls. They are built in code, so they can change without touching an asset. */
UCLASS()
class EISENSEELEN_API AEIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEIPlayerController();

	/** The class a new hero of this player starts as. It survives a restart. */
	EEIHeroClass GetSelectedClass() const { return SelectedClass; }

	UEIProfileComponent* GetProfile() const { return Profile; }

	/** The gear page. While it is open the hero stands still and the mouse is a pointer. */
	bool IsMenuOpen() const { return bMenuOpen; }
	FEIGearMenu& GetMenu() { return Menu; }
	void SetMenuOpen(bool bOpen);

	/** The overview of the contracts. The hero stays in the player's hands while it is open. */
	bool IsMapOpen() const { return bMapOpen; }

	/** The forge the own hero stands at, at the dock and off the barge. There the use key opens the forge page. */
	AEIForge* FindForgeInReach() const;

	/** For preview pictures: puts on an item, by its name. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewEquip(const FString& Name);

	/** For preview pictures: presses or releases a key through the real controls. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewKey(FName KeyName, bool bPressed);

	/** For preview pictures: the hero survives everything, so a long scene can be photographed to its end. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewInvulnerable();

	/** For preview pictures: the blood moon stands, or not. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewBloodMoon(bool bStands);

	/** For preview pictures: the run begins at once, without the way there. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewBeginRun();

	/** For preview pictures: the boss is free at once. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewSkipBannbruch();

	/** For preview pictures: this boss comes, by its place in Data/bosse.json. At the dock, before the run. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewChooseBoss(int32 BossIndex);

	/** For preview pictures: so many souls are in the kettle. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewKettleSouls(int32 Souls);

	/** For preview pictures: the boss summons or vanishes as soon as it can. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewHurryBoss();

	/** For preview pictures: the boss falls. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewDefeatBoss();

	/** For preview pictures: so many open contracts count as done. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewSkipContracts(int32 Count);

	/** For preview pictures: a group of that name appears some way in front of the hero. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewSpawnGroup(const FString& Name);

	/** For preview pictures: all guards of chests fall at once. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewDefeatGuards();

	/** For preview pictures: chooses the slot the gear page shows. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewMenuSlot(int32 Slot);

	/** For preview pictures: the open page shows the loot book, or the gear. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewLootBook(bool bShow);

	/** For preview pictures: the open page by its tab, 0 gear, 1 forge, 2 loot book. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewMenuPage(int32 Page);

	/** For preview pictures: buys an item in the forge, by its name. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewBuy(const FString& Name);

	/** For preview pictures: the profile has this much gold and experience, as if earned. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewPurse(int32 Gold, int32 Experience);

	/** X turns right, Y looks up. Takes the mouse factor from the hero's values. */
	void ApplyLookInput(const FVector2D& Input);

	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void SetupInputComponent() override;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted();
	void HandleJumpEnded();
	void HandleSprintStarted();
	void HandleSprintEnded();
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
	void HandleMenu();
	void HandleMap();

	UFUNCTION(Server, Reliable)
	void ServerInteract();

	UFUNCTION(Server, Reliable)
	void ServerToggleClass();

	UFUNCTION(Server, Reliable)
	void ServerToggleHorde();

	UFUNCTION(Server, Reliable)
	void ServerRestartRun();

	AEIHeroCharacter* GetLivingHero() const;

	/** The hero, if the keys and the mouse belong to the hero right now and not to the gear page. */
	AEIHeroCharacter* GetHeroForControls() const;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIProfileComponent> Profile;

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
	TObjectPtr<UInputAction> MenuAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MapAction;

	EEIHeroClass SelectedClass = EEIHeroClass::Bladewarden;
	FEIGearMenu Menu;
	bool bMenuOpen = false;
	bool bMapOpen = false;
};
