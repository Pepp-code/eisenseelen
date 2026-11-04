#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIHeroCharacter.h"
#include "EIItems.h"
#include "EISaveGame.h"
#include "EIProfileComponent.generated.h"

class AEIPlayerController;

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

	/** Server only. Dresses a hero of this player. */
	void ApplyTo(AEIHeroCharacter* Hero);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(Server, Reliable)
	void ServerSendSetups(const FEIHeroSetup& ForBladewarden, const FEIHeroSetup& ForArcanist);

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

	FEIItemTable Table;
	FEISaveGame Save;
	bool bSent = false;
	bool bSentFree = false;

	// Only on the server: what the player told us.
	FEIHeroSetup ServerSetups[2];
	bool bHasSetups = false;
};
