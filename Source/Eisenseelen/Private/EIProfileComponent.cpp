#include "EIProfileComponent.h"

#include "EIDataFile.h"
#include "EIGameState.h"
#include "EIGearComponent.h"
#include "EIPlayerController.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

namespace
{
	constexpr float RewardNoteSeconds = 5.0f;
	constexpr int32 NamedUnlocks = 3;

	bool AllowsGearChange(EEIRunPhase Phase)
	{
		return Phase == EEIRunPhase::None || Phase == EEIRunPhase::AtDock || Phase == EEIRunPhase::Escaped || Phase == EEIRunPhase::Failed;
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
	if (!FEIContractTable::LoadFromProjectFile(ContractTable, Error))
	{
		FEIDataFile::ReportError(TEXT("nebenauftraege.json"), Error);
	}
	if (!FEIBossTable::LoadFromProjectFile(BossTable, Error))
	{
		FEIDataFile::ReportError(TEXT("bosse.json"), Error);
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
	RewardNoteRemaining = FMath::Max(0.0f, RewardNoteRemaining - DeltaTime);
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
		if (Item.Slot == Slot && Item.FitsClass(bArcanist) && (bFree || Save.Items.Contains(Item.Name)))
		{
			FEIWornItem Choice;
			Choice.Item = Index;
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

int32 UEIProfileComponent::GetLevel() const
{
	return IsFreeChoice() ? FMath::RoundToInt32(Table.MaxLevel) : Table.GetLevel(Save.Experience);
}

bool UEIProfileComponent::Owns(int32 Item) const
{
	return Table.Items.IsValidIndex(Item) && (IsFreeChoice() || Save.Items.Contains(Table.Items[Item].Name));
}

bool UEIProfileComponent::IsUnlocked(int32 Item) const
{
	return Table.Items.IsValidIndex(Item) && GetLevel() >= Table.Items[Item].FromLevel;
}

EEIOffer UEIProfileComponent::GetOffer(int32 Item) const
{
	if (Owns(Item))
	{
		return EEIOffer::Owned;
	}
	if (!IsUnlocked(Item))
	{
		return EEIOffer::Locked;
	}
	return Save.Gold >= Table.Items[Item].Price ? EEIOffer::Buyable : EEIOffer::TooPoor;
}

bool UEIProfileComponent::Buy(int32 Item)
{
	if (!IsOwnerSide() || IsFreeChoice() || !CanChangeGear() || GetOffer(Item) != EEIOffer::Buyable)
	{
		return false;
	}
	const FEIItem& Bought = Table.Items[Item];
	Save.Gold -= Bought.Price;
	Save.Items.Add(Bought.Name);
	WriteSave();
	// What is bought goes on at once, if it fits the hero who stands at the forge.
	Equip(Bought.Slot, Item);
	ClientShowNote_Implementation(FString::Printf(TEXT("Gekauft: %s"), *Bought.Name));
	EIVisuals::PlaySound2D(this, TEXT("Sprungfeld"));
	return true;
}

bool UEIProfileComponent::BuyByName(const FString& Name)
{
	return Buy(Table.FindItem(Name));
}

TArray<int32> UEIProfileComponent::ListOffers(EEISlot Slot) const
{
	const bool bArcanist = GetCurrentClass() == EEIHeroClass::Arcanist;
	TArray<int32> Offers;
	for (int32 Index = 0; Index < Table.Items.Num(); ++Index)
	{
		if (Table.Items[Index].Slot == Slot && Table.Items[Index].FitsClass(bArcanist))
		{
			Offers.Add(Index);
		}
	}
	// Sorted the way they open, so that the list reads like a way ahead.
	Offers.StableSort([this](int32 Left, int32 Right) { return Table.Items[Left].FromLevel < Table.Items[Right].FromLevel; });
	return Offers;
}

int32 UEIProfileComponent::GetNextUnlockLevel(TArray<int32>& OutItems) const
{
	OutItems.Reset();
	const bool bArcanist = GetCurrentClass() == EEIHeroClass::Arcanist;
	for (int32 Level = GetLevel() + 1; Level <= FMath::RoundToInt32(Table.MaxLevel); ++Level)
	{
		for (const int32 Item : Table.GetUnlockedAt(Level))
		{
			if (Table.Items[Item].FitsClass(bArcanist))
			{
				OutItems.Add(Item);
			}
		}
		if (!OutItems.IsEmpty())
		{
			return Level;
		}
	}
	return 0;
}

FString UEIProfileComponent::DescribeUnlocks(int32 FirstLevel, int32 LastLevel) const
{
	TArray<FString> Names;
	for (int32 Level = FirstLevel; Level <= LastLevel; ++Level)
	{
		for (const int32 Item : Table.GetUnlockedAt(Level))
		{
			Names.Add(Table.Items[Item].Name);
		}
	}
	if (Names.IsEmpty())
	{
		return FString();
	}
	FString Text = FString::Join(TArrayView<const FString>(Names.GetData(), FMath::Min(Names.Num(), NamedUnlocks)), TEXT(", "));
	if (Names.Num() > NamedUnlocks)
	{
		Text += FString::Printf(TEXT(" und %d weitere"), Names.Num() - NamedUnlocks);
	}
	return Text;
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
		const bool bFits = Table.Items.IsValidIndex(Index) && static_cast<int32>(Table.Items[Index].Slot) == Slot && Table.Items[Index].FitsClass(bArcanist) && (bFree || Save.Items.Contains(Table.Items[Index].Name));
		// The data files may have changed since the last time, and a hero always keeps a weapon.
		if (bFits)
		{
			Setup.Slots[Slot].Item = Index;
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
	const bool bFirst = !bHasSetups;
	ServerSetups[0] = ForBladewarden;
	ServerSetups[1] = ForArcanist;
	bHasSetups = true;
	// During a run a hero keeps what they flew out with. Only somebody joining late still needs to get dressed.
	AEIHeroCharacter* Hero = GetPlayer()->GetPawn<AEIHeroCharacter>();
	if (Hero != nullptr && (bFirst || CanChangeGear()))
	{
		ApplyTo(Hero);
	}
}

void UEIProfileComponent::ApplyTo(AEIHeroCharacter* Hero)
{
	const bool bArcanist = Hero->GetHeroClass() == EEIHeroClass::Arcanist;
	Hero->GetGear()->SetSetup(bHasSetups ? ServerSetups[bArcanist ? 1 : 0] : MakeStartSetup(bArcanist));
}

void UEIProfileComponent::GrantEnemyReward(float Experience)
{
	ClientReceiveReward(FMath::RoundToInt32(Experience));
}

void UEIProfileComponent::ClientReceiveReward_Implementation(int32 Experience)
{
	const int32 LevelBefore = Table.GetLevel(Save.Experience);
	Save.Experience += Experience;
	RunExperience += Experience;
	const int32 LevelAfter = Table.GetLevel(Save.Experience);

	// An ordinary kill does not interrupt with a note every time, only a new level does.
	if (LevelAfter <= LevelBefore)
	{
		return;
	}
	const FString Unlocks = DescribeUnlocks(LevelBefore + 1, LevelAfter);
	ClientShowNote_Implementation(Unlocks.IsEmpty()
		? FString::Printf(TEXT("Level %d!"), LevelAfter)
		: FString::Printf(TEXT("Level %d! Neu in der Schmiede: %s"), LevelAfter, *Unlocks));
	EIVisuals::PlaySound2D(this, TEXT("Sprungfeld"));
}

void UEIProfileComponent::ClientShowNote_Implementation(const FString& Text)
{
	RewardText = Text;
	RewardNoteRemaining = RewardNoteSeconds;
}

void UEIProfileComponent::GrantContract(int32 Faction, int32 Gold, int32 Points)
{
	ClientReceiveContract(Faction, Gold, Points);
}

void UEIProfileComponent::ClientReceiveContract_Implementation(int32 Faction, int32 Gold, int32 Points)
{
	const FEIFaction* Found = ContractTable.FindFaction(Faction);
	if (Found == nullptr)
	{
		return;
	}
	RunGold += Gold;
	RunPoints.FindOrAdd(Found->Name) += Points;
	ClientShowNote_Implementation(FString::Printf(TEXT("%s-Auftrag erf\u00FCllt: +%d Gold, +%d %spunkte"), *Found->Name, Gold, Points, *Found->Name));
	EIVisuals::PlaySound2D(this, TEXT("Sprungfeld"));
}

void UEIProfileComponent::GrantBossLoot(const FString& BossName, int32 Gold, const TArray<FString>& Ingredients)
{
	ClientReceiveBossLoot(BossName, Gold, Ingredients);
}

void UEIProfileComponent::ClientReceiveBossLoot_Implementation(const FString& BossName, int32 Gold, const TArray<FString>& Ingredients)
{
	RunGold += Gold;
	RunIngredients.Append(Ingredients);
	// Seen is seen: the loot book names it from now on, even if the run fails.
	Save.FoundIngredients.Append(Ingredients);
	FString Note = FString::Printf(TEXT("%s ist gefallen: +%d Gold"), *BossName, Gold);
	for (const FString& Ingredient : Ingredients)
	{
		Note += TEXT(", ") + Ingredient;
	}
	ClientShowNote_Implementation(Note);
	EIVisuals::PlaySound2D(this, TEXT("Sprungfeld"));
}

int32 UEIProfileComponent::GetSecured(int32 Amount) const
{
	return bLastRunEscaped ? Amount : ContractTable.GetKeptOnFailure(Amount);
}

void UEIProfileComponent::EndRun(bool bEscaped)
{
	ClientEndRun(bEscaped);
}

void UEIProfileComponent::ClientEndRun_Implementation(bool bEscaped)
{
	bLastRunEscaped = bEscaped;
	Save.Gold += GetSecured(RunGold);
	for (const TPair<FString, int32>& Pair : RunPoints)
	{
		Save.FactionPoints.FindOrAdd(Pair.Key) += GetSecured(Pair.Value);
	}
	if (bEscaped)
	{
		for (const FString& Ingredient : RunIngredients)
		{
			Save.Ingredients.FindOrAdd(Ingredient) += 1;
		}
	}
	WriteSave();
	SendSetups();
}

void UEIProfileComponent::StartFresh()
{
	LoadTable();
	ClientStartFresh();
}

void UEIProfileComponent::ClientStartFresh_Implementation()
{
	LoadTable();
	RunExperience = 0;
	RunGold = 0;
	RunPoints.Reset();
	RunIngredients.Reset();
	RewardNoteRemaining = 0.0f;
	WriteSave();
	SendSetups();
}
