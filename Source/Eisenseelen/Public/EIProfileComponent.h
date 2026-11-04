#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIHeroCharacter.h"
#include "EIItems.h"
#include "EISaveGame.h"
#include "EIProfileComponent.generated.h"

class AEIPlayerController;

/** What a reward was given for. */
UENUM()
enum class EEIRewardSource : uint8
{
	Ritual,
	Leader,
	Shrine,
	Enemy
};

/** One thing found during a run. It is only safe once the hero has got away. */
struct FEILootLine
{
	int32 Item = INDEX_NONE;
	int32 Quality = 0;
	EEIRewardSource Source = EEIRewardSource::Ritual;

	/** What it came to, in words for the player. Empty until the run has ended. */
	FString Result;
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

	/** Owner only. In the test arena everything is free, and this says how good it is. */
	void SetArenaQuality(int32 Quality);

	/** Owner only. The level counts for both classes. In the test arena it is the highest. */
	int32 GetLevel() const;

	/** Owner only. Each class spends its points by itself, and a point can always move elsewhere. */
	int32 GetFreePoints() const;
	int32 GetAttributePoints(int32 Attribute) const;
	void ChangeAttribute(int32 Attribute, int32 Change);

	/** Owner only. What this run has brought so far. */
	const TArray<FEILootLine>& GetLoot() const { return Loot; }
	int32 GetRunExperience() const { return RunExperience; }

	/** Seconds of the note left after a reward, and what it says. For the HUD. */
	float GetRewardNote() const { return RewardNoteRemaining; }
	const FString& GetRewardText() const { return RewardText; }

	/** Server only. Dresses a hero of this player. */
	void ApplyTo(AEIHeroCharacter* Hero);

	/** Server only. The server chooses what this player gets, and prefers what the player does not have yet. */
	void GrantReward(EEIRewardSource Source);

	/** Server only. A fallen enemy brings a little experience, and very rarely an item. */
	void GrantEnemyReward(float Experience, float ItemChance);

	/** Server only. In the barge the loot is safe, without it it is gone. The experience stays either way. */
	void EndRun(bool bEscaped);

	/** Server only. A new run begins, and the data files are read again. */
	void StartFresh();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(Server, Reliable)
	void ServerSendSetups(const FEIHeroSetup& ForBladewarden, const FEIHeroSetup& ForArcanist, const TArray<FEIWornItem>& Owned);

	UFUNCTION(Client, Reliable)
	void ClientReceiveReward(int32 Item, int32 Quality, int32 Experience, EEIRewardSource Source);

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
	FEISavedClass& GetSavedClass(bool bArcanist);
	const FEISavedClass& GetSavedClass(bool bArcanist) const;
	FString AddToSave(int32 Item, int32 Quality);
	int32 RollItem(EEIRewardSource Source, int32 Quality);
	int32 ScaleExperience(float Experience) const;
	int32 GetCorruptionStage() const;

	FEIItemTable Table;
	FEISaveGame Save;
	bool bSent = false;
	bool bSentFree = false;
	TArray<FEILootLine> Loot;
	int32 RunExperience = 0;
	float RewardNoteRemaining = 0.0f;
	FString RewardText;

	// Only on the server: what the player told us, and what they got since.
	FEIHeroSetup ServerSetups[2];
	TArray<FEIWornItem> ServerOwned;
	bool bHasSetups = false;
};
