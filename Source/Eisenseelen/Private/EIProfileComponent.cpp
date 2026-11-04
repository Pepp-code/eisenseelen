#include "EIProfileComponent.h"

#include "EIDataFile.h"
#include "EIGameState.h"
#include "EIGearComponent.h"
#include "EIPlayerController.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

namespace
{
	bool AllowsGearChange(EEIRunPhase Phase)
	{
		return Phase == EEIRunPhase::None || Phase == EEIRunPhase::OnBarge || Phase == EEIRunPhase::Escaped || Phase == EEIRunPhase::Failed;
	}
}

UEIProfileComponent::UEIProfileComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

AEIPlayerController* UEIProfileComponent::GetPlayer() const
{
	return CastChecked<AEIPlayerController>(GetOwner());
}

bool UEIProfileComponent::IsOwnerSide() const
{
	return GetPlayer()->IsLocalController();
}

void UEIProfileComponent::BeginPlay()
{
	Super::BeginPlay();
	LoadTable();
	if (IsOwnerSide())
	{
		LoadSave();
	}
}

void UEIProfileComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsOwnerSide())
	{
		WriteSave();
	}
	Super::EndPlay(EndPlayReason);
}

void UEIProfileComponent::LoadTable()
{
	FString Error;
	if (!FEIItemTable::LoadFromProjectFiles(Table, Error))
	{
		FEIDataFile::ReportError(TEXT("items.json"), Error);
	}
}

void UEIProfileComponent::LoadSave()
{
	Save = FEISaveGame::MakeNew(Table);
	if (!FEISaveGame::UsesDisk())
	{
		return;
	}
	FString Json;
	const FString Path = FEISaveGame::GetPath();
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		return;
	}
	FString Error;
	if (!Save.FromJson(Json, Error))
	{
		// A broken file must not be lost when the new one gets written.
		IFileManager::Get().Copy(*(Path + TEXT(".kaputt")), *Path);
		FEIDataFile::ReportError(TEXT("Spielstand.json"), Error);
	}
}

void UEIProfileComponent::WriteSave() const
{
	if (FEISaveGame::UsesDisk() && !Table.Items.IsEmpty())
	{
		Save.SaveToFile(FEISaveGame::GetPath());
	}
}

void UEIProfileComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsOwnerSide() || GetWorld()->GetGameState<AEIGameState>() == nullptr)
	{
		return;
	}
	// A guest learns only after a moment what kind of map this is.
	const bool bFree = IsFreeChoice();
	if (!bSent || bFree != bSentFree)
	{
		bSent = true;
		bSentFree = bFree;
		SendSetups();
	}
}

bool UEIProfileComponent::IsFreeChoice() const
{
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	return State == nullptr || State->GetRunPhase() == EEIRunPhase::None;
}

bool UEIProfileComponent::CanChangeGear() const
{
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	return State == nullptr || AllowsGearChange(State->GetRunPhase());
}

EEIHeroClass UEIProfileComponent::GetCurrentClass() const
{
	const AEIHeroCharacter* Hero = GetPlayer()->GetPawn<AEIHeroCharacter>();
	return Hero != nullptr ? Hero->GetHeroClass() : GetPlayer()->GetSelectedClass();
}

FEISavedClass& UEIProfileComponent::GetSavedClass(bool bArcanist)
{
	return IsFreeChoice() ? Save.ArenaClasses[bArcanist ? 1 : 0] : Save.Classes[bArcanist ? 1 : 0];
}

const FEISavedClass& UEIProfileComponent::GetSavedClass(bool bArcanist) const
{
	return IsFreeChoice() ? Save.ArenaClasses[bArcanist ? 1 : 0] : Save.Classes[bArcanist ? 1 : 0];
}

TArray<FEIWornItem> UEIProfileComponent::ListChoices(EEISlot Slot) const
{
	const bool bFree = IsFreeChoice();
	const bool bArcanist = GetCurrentClass() == EEIHeroClass::Arcanist;
	TArray<FEIWornItem> Choices;
	for (int32 Index = 0; Index < Table.Items.Num(); ++Index)
	{
		const FEIItem& Item = Table.Items[Index];
		const int32* Quality = Save.Items.Find(Item.Name);
		if (Item.Slot == Slot && Item.FitsClass(bArcanist) && (bFree || Quality != nullptr))
		{
			FEIWornItem Choice;
			Choice.Item = Index;
			Choice.Quality = bFree ? Save.ArenaQuality : *Quality;
			Choices.Add(Choice);
		}
	}
	return Choices;
}

FEIWornItem UEIProfileComponent::GetEquipped(EEISlot Slot) const
{
	return BuildSetup(GetCurrentClass() == EEIHeroClass::Arcanist).Slots[static_cast<int32>(Slot)];
}

void UEIProfileComponent::Equip(EEISlot Slot, int32 Item)
{
	if (!CanChangeGear() || (Item == INDEX_NONE && Slot == EEISlot::Weapon))
	{
		return;
	}
	const FEIItem* Chosen = Table.Items.IsValidIndex(Item) ? &Table.Items[Item] : nullptr;
	const bool bArcanist = GetCurrentClass() == EEIHeroClass::Arcanist;
	if (Chosen != nullptr && (Chosen->Slot != Slot || !Chosen->FitsClass(bArcanist) || (!IsFreeChoice() && !Save.Items.Contains(Chosen->Name))))
	{
		return;
	}
	GetSavedClass(bArcanist).Slots[static_cast<int32>(Slot)] = Chosen != nullptr ? Chosen->Name : FString();
	WriteSave();
	SendSetups();
}

void UEIProfileComponent::EquipByName(const FString& Name)
{
	const int32 Item = Table.FindItem(Name);
	if (Table.Items.IsValidIndex(Item))
	{
		Equip(Table.Items[Item].Slot, Item);
	}
}

void UEIProfileComponent::EquipNext(EEISlot Slot)
{
	const TArray<FEIWornItem> Choices = ListChoices(Slot);
	if (Choices.IsEmpty())
	{
		return;
	}
	const int32 Current = GetEquipped(Slot).Item;
	const int32 Place = Choices.IndexOfByPredicate([Current](const FEIWornItem& Choice) { return Choice.Item == Current; });
	Equip(Slot, Choices[(Place + 1) % Choices.Num()].Item);
}

void UEIProfileComponent::SetArenaQuality(int32 Quality)
{
	Save.ArenaQuality = FMath::Clamp(Quality, 0, FMath::Max(Table.QualityFactors.Num() - 1, 0));
	WriteSave();
	SendSetups();
}

FEIHeroSetup UEIProfileComponent::MakeStartSetup(bool bArcanist) const
{
	FEIHeroSetup Setup;
	Setup.Slots.SetNum(EISlotCount);
	for (int32 Slot = 0; Slot < EISlotCount; ++Slot)
	{
		Setup.Slots[Slot].Item = Table.GetStartItem(static_cast<EEISlot>(Slot), bArcanist);
	}
	return Setup;
}

FEIHeroSetup UEIProfileComponent::BuildSetup(bool bArcanist) const
{
	const bool bFree = IsFreeChoice();
	const FEISavedClass& Saved = GetSavedClass(bArcanist);
	FEIHeroSetup Setup = MakeStartSetup(bArcanist);
	for (int32 Slot = 0; Slot < EISlotCount; ++Slot)
	{
		const int32 Index = Table.FindItem(Saved.Slots[Slot]);
		const int32* Quality = Table.Items.IsValidIndex(Index) ? Save.Items.Find(Table.Items[Index].Name) : nullptr;
		const bool bFits = Table.Items.IsValidIndex(Index) && static_cast<int32>(Table.Items[Index].Slot) == Slot && Table.Items[Index].FitsClass(bArcanist) && (bFree || Quality != nullptr);
		// The data files may have changed since the last time, and a hero always keeps a weapon.
		if (bFits)
		{
			Setup.Slots[Slot].Item = Index;
			Setup.Slots[Slot].Quality = bFree ? Save.ArenaQuality : *Quality;
		}
		else if (Slot != static_cast<int32>(EEISlot::Weapon))
		{
			Setup.Slots[Slot].Item = INDEX_NONE;
		}
	}
	return Setup;
}

void UEIProfileComponent::SendSetups()
{
	ServerSendSetups(BuildSetup(false), BuildSetup(true));
}

void UEIProfileComponent::ServerSendSetups_Implementation(const FEIHeroSetup& ForBladewarden, const FEIHeroSetup& ForArcanist)
{
	ServerSetups[0] = ForBladewarden;
	ServerSetups[1] = ForArcanist;
	bHasSetups = true;
	if (AEIHeroCharacter* Hero = GetPlayer()->GetPawn<AEIHeroCharacter>())
	{
		ApplyTo(Hero);
	}
}

void UEIProfileComponent::ApplyTo(AEIHeroCharacter* Hero)
{
	const bool bArcanist = Hero->GetHeroClass() == EEIHeroClass::Arcanist;
	Hero->GetGear()->SetSetup(bHasSetups ? ServerSetups[bArcanist ? 1 : 0] : MakeStartSetup(bArcanist));
}
