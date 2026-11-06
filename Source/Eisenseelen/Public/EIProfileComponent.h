#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIBosses.h"
#include "EIContracts.h"
#include "EIEngravings.h"
#include "EIHeroCharacter.h"
#include "EIItems.h"
#include "EISaveGame.h"
#include "EIProfileComponent.generated.h"

class AEIPlayerController;

/** How the forge offers an item to this player. */
enum class EEIOffer : uint8
{
	Owned,
	Locked,
	TooPoor,
	Buyable
};

/** How the engravings page offers an engraving to this player. */
enum class EEIEngravingState : uint8
{
	Active,
	Owned,
	Buyable,
	TooPoor,
	Locked
};

/**
 * The player behind the heroes, who stays when a hero falls. On the owner's side it holds the save file and everything
 * the player chooses, and tells the server what to put on each new hero. The server only uses what it was told.
 */
UCLASS()
class EISENSEELEN_API UEIProfileComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIProfileComponent();

	/** Owner only. */
	const FEIItemTable& GetTable() const { return Table; }
	const FEIContractTable& GetContractTable() const { return ContractTable; }

	/** For the loot book: which boss can drop what. */
	const FEIBossTable& GetBossTable() const { return BossTable; }

	/** Owner only. Whether a boss has ever dropped this for the player. */
	bool HasFound(const FString& Ingredient) const { return Save.FoundIngredients.Contains(Ingredient); }
	FEISaveGame& GetSave() { return Save; }
	const FEISaveGame& GetSave() const { return Save; }

	/** Whether items are free right now, as in the test arena. */
	bool IsFreeChoice() const;

	/** Before and after a run, but not during one, because a hero flies out with what they have on. */
	bool CanChangeGear() const;

	/** The class the player's hero has right now. */
	EEIHeroClass GetCurrentClass() const;

	/** What the current class could put into this slot. */
	TArray<FEIWornItem> ListChoices(EEISlot Slot) const;
	FEIWornItem GetEquipped(EEISlot Slot) const;

	/** Owner only. INDEX_NONE empties a slot, except that a hero always keeps a weapon. */
	void Equip(EEISlot Slot, int32 Item);
	void EquipByName(const FString& Name);

	/** Owner only. The next choice in the slot. */
	void EquipNext(EEISlot Slot);

	/** Owner only. The level counts for both classes. In the test arena it is the highest. */
	int32 GetLevel() const;

	/** Owner only. In the test arena everything counts as owned. */
	bool Owns(int32 Item) const;

	/** Owner only. The level opens items in the forge, and gold buys them there. */
	bool IsUnlocked(int32 Item) const;
	EEIOffer GetOffer(int32 Item) const;
	bool Buy(int32 Item);
	bool BuyByName(const FString& Name);

	/** Owner only. All the forge has for this slot and the current class, owned or not. */
	TArray<int32> ListOffers(EEISlot Slot) const;

	/** Owner only. The next level that opens something for the current class, and what. 0 if there is none. */
	int32 GetNextUnlockLevel(TArray<int32>& OutItems) const;

	/** In words: what the forge opens on reaching these levels. */
	FString DescribeUnlocks(int32 FirstLevel, int32 LastLevel) const;

	const FEIEngravingTable& GetEngravingTable() const { return EngravingTable; }

	/** Owner only. The weapon the current class has in hand. */
	int32 GetWornWeapon() const { return GetEquipped(EEISlot::Weapon).Item; }

	/** The engravings of this weapon's kind. Null if that kind has none yet. */
	const FEIEngravingForm* GetEngravingForm(int32 Weapon) const;

	/** The faction whose points pay for the engravings of this weapon. */
	const FString& GetEngravingFaction(int32 Weapon) const;

	/** Owner only. A tier opens once something in the tier before it is bought. In the test arena everything counts as bought. */
	EEIEngravingState GetEngravingState(int32 Weapon, int32 Tier, int32 Choice) const;

	/** Owner only. Gold and the faction's points buy an engraving for this one weapon, and it is used at once. */
	bool BuyEngraving(int32 Weapon, int32 Tier, int32 Choice);

	/** Owner only. Makes a bought engraving the one of its tier, or takes it off again if it already is. */
	bool ChooseEngraving(int32 Weapon, int32 Tier, int32 Choice);

	/** Owner only. For the weapon in hand. */
	bool BuyEngravingByName(const FString& Name);
	bool ChooseEngravingByName(const FString& Name);

	/** Owner only. What this run has brought so far. */
	int32 GetRunExperience() const { return RunExperience; }
	int32 GetRunGold() const { return RunGold; }
	int32 GetRunPoints(const FString& Faction) const { return RunPoints.FindRef(Faction); }
	const TArray<FString>& GetRunIngredients() const { return RunIngredients; }
	bool WasLastRunEscaped() const { return bLastRunEscaped; }

	/** Owner only. What stays of an amount this run has earned, once the run is over. */
	int32 GetSecured(int32 Amount) const;

	/** Seconds of the note left after a reward, and what it says. For the HUD. */
	float GetRewardNote() const { return RewardNoteRemaining; }
	const FString& GetRewardText() const { return RewardText; }

	/** Server only. Dresses a hero of this player. */
	void ApplyTo(AEIHeroCharacter* Hero);

	/** Server only. A side job was finished with this player near. */
	void GrantContract(int32 Faction, int32 Gold, int32 Points);

	/** Server only. A boss has fallen, and this is what it dropped for this player. */
	void GrantBossLoot(const FString& BossName, int32 Gold, const TArray<FString>& Ingredients);

	/** Tells the player something in the middle of the screen. */
	UFUNCTION(Client, Reliable)
	void ClientShowNote(const FString& Text);

	/** Server only. A fallen enemy brings a little experience. */
	void GrantEnemyReward(float Experience);

	/** Server only. In the barge all of it is safe. Without the barge a share of the gold and points stays, and the ingredients are gone. The experience stays either way. */
	void EndRun(bool bEscaped);

	/** Server only. A new run begins, and the data files are read again. */
	void StartFresh();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(Server, Reliable)
	void ServerSendSetups(const FEIHeroSetup& ForBladewarden, const FEIHeroSetup& ForArcanist);

	UFUNCTION(Client, Reliable)
	void ClientReceiveReward(int32 Experience);

	UFUNCTION(Client, Reliable)
	void ClientReceiveContract(int32 Faction, int32 Gold, int32 Points);

	UFUNCTION(Client, Reliable)
	void ClientReceiveBossLoot(const FString& BossName, int32 Gold, const TArray<FString>& Ingredients);

	UFUNCTION(Client, Reliable)
	void ClientEndRun(bool bEscaped);

	UFUNCTION(Client, Reliable)
	void ClientStartFresh();

	AEIPlayerController* GetPlayer() const;
	bool IsOwnerSide() const;
	void LoadTable();
	void LoadSave();
	void WriteSave() const;
	void SendSetups();
	FEIHeroSetup BuildSetup(bool bArcanist) const;
	FEIHeroSetup MakeStartSetup(bool bArcanist) const;
	void ActivateEngraving(int32 Weapon, int32 Tier, int32 Choice);
	FEISavedClass& GetSavedClass(bool bArcanist);
	const FEISavedClass& GetSavedClass(bool bArcanist) const;

	FEIItemTable Table;
	FEIContractTable ContractTable;
	FEIBossTable BossTable;
	FEIEngravingTable EngravingTable;
	FEISaveGame Save;
	bool bSent = false;
	bool bSentFree = false;
	int32 RunExperience = 0;
	int32 RunGold = 0;
	TMap<FString, int32> RunPoints;
	TArray<FString> RunIngredients;
	bool bLastRunEscaped = true;
	float RewardNoteRemaining = 0.0f;
	FString RewardText;

	// Only on the server: what the player told us to put on.
	FEIHeroSetup ServerSetups[2];
	bool bHasSetups = false;
};
