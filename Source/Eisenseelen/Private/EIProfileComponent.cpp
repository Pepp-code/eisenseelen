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

int32 UEIProfileComponent::GetLevel() const
{
	return IsFreeChoice() ? FMath::RoundToInt32(Table.MaxLevel) : Table.GetLevel(Save.Experience);
}

int32 UEIProfileComponent::GetAttributePoints(int32 Attribute) const
{
	const FEIHeroSetup Setup = BuildSetup(GetCurrentClass() == EEIHeroClass::Arcanist);
	return Setup.Attributes.IsValidIndex(Attribute) ? Setup.Attributes[Attribute] : 0;
}

int32 UEIProfileComponent::GetFreePoints() const
{
	int32 Free = GetLevel() - 1;
	for (int32 Index = 0; Index < Table.Attributes.Num(); ++Index)
	{
		Free -= GetAttributePoints(Index);
	}
	return Free;
}

void UEIProfileComponent::ChangeAttribute(int32 Attribute, int32 Change)
{
	if (!CanChangeGear() || !Table.Attributes.IsValidIndex(Attribute) || (Change > 0 && GetFreePoints() < Change))
	{
		return;
	}
	const int32 Points = FMath::Max(GetAttributePoints(Attribute) + Change, 0);
	GetSavedClass(GetCurrentClass() == EEIHeroClass::Arcanist).Attributes.Add(Table.Attributes[Attribute].Name, Points);
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
	// Never more points than the level gives, whatever the file says.
	int32 PointsLeft = GetLevel() - 1;
	Setup.Attributes.SetNum(Table.Attributes.Num());
	for (int32 Index = 0; Index < Table.Attributes.Num(); ++Index)
	{
		Setup.Attributes[Index] = FMath::Clamp(Saved.Attributes.FindRef(Table.Attributes[Index].Name), 0, PointsLeft);
		PointsLeft -= Setup.Attributes[Index];
	}
	return Setup;
}

void UEIProfileComponent::SendSetups()
{
	TArray<FEIWornItem> Owned;
	for (const TPair<FString, int32>& Pair : Save.Items)
	{
		FEIWornItem Entry;
		Entry.Item = Table.FindItem(Pair.Key);
		Entry.Quality = Pair.Value;
		if (Table.Items.IsValidIndex(Entry.Item))
		{
			Owned.Add(Entry);
		}
	}
	ServerSendSetups(BuildSetup(false), BuildSetup(true), Owned);
}

void UEIProfileComponent::ServerSendSetups_Implementation(const FEIHeroSetup& ForBladewarden, const FEIHeroSetup& ForArcanist, const TArray<FEIWornItem>& Owned)
{
	const bool bFirst = !bHasSetups;
	ServerSetups[0] = ForBladewarden;
	ServerSetups[1] = ForArcanist;
	ServerOwned = Owned;
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

int32 UEIProfileComponent::GetCorruptionStage() const
{
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	return State != nullptr ? State->GetCorruptionStage() : 0;
}

int32 UEIProfileComponent::ScaleExperience(float Experience) const
{
	return FMath::RoundToInt32(Experience * (1.0f + Table.Rewards.ExperiencePerStage * GetCorruptionStage()));
}

int32 UEIProfileComponent::RollItem(int32 Quality)
{
	const AEIHeroCharacter* Hero = GetPlayer()->GetPawn<AEIHeroCharacter>();
	const bool bArcanist = Hero != nullptr && Hero->GetHeroClass() == EEIHeroClass::Arcanist;
	const int32 MaxQuality = FMath::Max(Table.QualityFactors.Num() - 1, 0);
	TArray<int32> Fitting;
	TArray<int32> CanRise;
	TArray<int32> New;
	for (int32 Index = 0; Index < Table.Items.Num(); ++Index)
	{
		const FEIItem& Item = Table.Items[Index];
		if (Item.Slot == EEISlot::Crystal || !Item.FitsClass(bArcanist))
		{
			continue;
		}
		Fitting.Add(Index);
		const FEIWornItem* Owned = ServerOwned.FindByPredicate([Index](const FEIWornItem& Entry) { return Entry.Item == Index; });
		if (Owned == nullptr)
		{
			New.Add(Index);
		}
		else if (Owned->Quality < MaxQuality)
		{
			CanRise.Add(Index);
		}
	}
	const TArray<int32>& Choice = !New.IsEmpty() ? New : (!CanRise.IsEmpty() ? CanRise : Fitting);
	if (Choice.IsEmpty())
	{
		return INDEX_NONE;
	}
	const int32 Picked = Choice[FMath::RandRange(0, Choice.Num() - 1)];

	// The next reward of this run should know about this one, long before the owner has it for good.
	if (FEIWornItem* Owned = ServerOwned.FindByPredicate([Picked](const FEIWornItem& Entry) { return Entry.Item == Picked; }))
	{
		Owned->Quality = FMath::Min(Owned->Quality + 1, MaxQuality);
	}
	else
	{
		FEIWornItem Entry;
		Entry.Item = Picked;
		Entry.Quality = Quality;
		ServerOwned.Add(Entry);
	}
	return Picked;
}

void UEIProfileComponent::GrantEnemyReward(float Experience, float ItemChance)
{
	const int32 Quality = GetCorruptionStage() >= FMath::RoundToInt32(Table.Rewards.ForgedFromStage) ? 1 : 0;
	const int32 Item = FMath::FRand() < ItemChance ? RollItem(Quality) : INDEX_NONE;
	ClientReceiveReward(Item, Quality, ScaleExperience(Experience));
}

void UEIProfileComponent::ClientReceiveReward_Implementation(int32 Item, int32 Quality, int32 Experience)
{
	const int32 LevelBefore = Table.GetLevel(Save.Experience);
	Save.Experience += Experience;
	RunExperience += Experience;
	const int32 LevelAfter = Table.GetLevel(Save.Experience);
	const FString LevelNote = LevelAfter > LevelBefore ? FString::Printf(TEXT("Level %d! Einen neuen Punkt verteilst du in der Ausr\u00FCstung."), LevelAfter) : FString();

	// An ordinary kill does not interrupt with a note every time.
	const bool bFound = Table.Items.IsValidIndex(Item);
	if (!bFound && LevelNote.IsEmpty())
	{
		return;
	}
	if (bFound)
	{
		FEILootLine Line;
		Line.Item = Item;
		Line.Quality = Quality;
		Loot.Add(Line);
	}
	const FString FoundNote = bFound ? FString::Printf(TEXT("Gefunden: %s"), *Table.Items[Item].Name) : FString();
	ClientShowNote_Implementation(FoundNote + (bFound && !LevelNote.IsEmpty() ? TEXT("   ") : TEXT("")) + LevelNote);
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

FString UEIProfileComponent::AddToSave(int32 Item, int32 Quality)
{
	const FEIItem& Entry = Table.Items[Item];
	const int32 MaxQuality = FMath::Max(Table.QualityFactors.Num() - 1, 0);
	int32* Owned = Save.Items.Find(Entry.Name);
	if (Owned == nullptr)
	{
		Save.Items.Add(Entry.Name, Quality);
		return FString::Printf(TEXT("neu, %s"), *Table.GetQualityName(Quality));
	}
	if (*Owned < MaxQuality)
	{
		*Owned = FMath::Max(*Owned + 1, Quality);
		return FString::Printf(TEXT("steigt auf %s"), *Table.GetQualityName(*Owned));
	}
	const int32 Experience = FMath::RoundToInt32(Table.Rewards.DuplicateExperience);
	Save.Experience += Experience;
	RunExperience += Experience;
	return FString::Printf(TEXT("schon %s, +%d Erfahrung"), *Table.GetQualityName(*Owned), Experience);
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
	for (FEILootLine& Line : Loot)
	{
		Line.Result = bEscaped ? AddToSave(Line.Item, Line.Quality) : FString(TEXT("verloren"));
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
	Loot.Reset();
	RunExperience = 0;
	RunGold = 0;
	RunPoints.Reset();
	RunIngredients.Reset();
	RewardNoteRemaining = 0.0f;
	WriteSave();
	SendSetups();
}
