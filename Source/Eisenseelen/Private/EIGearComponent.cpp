#include "EIGearComponent.h"

#include "EIDataFile.h"
#include "EIHeroCharacter.h"
#include "Net/UnrealNetwork.h"

UEIGearComponent::UEIGearComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEIGearComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEIGearComponent, Setup);
}

void UEIGearComponent::LoadValues()
{
	FString Error;
	if (!FEIItemTable::LoadFromProjectFiles(Table, Error))
	{
		FEIDataFile::ReportError(TEXT("items.json"), Error);
	}
	if (!FEIEngravingTable::LoadFromProjectFile(Engravings, Error))
	{
		FEIDataFile::ReportError(TEXT("gravuren.json"), Error);
	}
}

AEIHeroCharacter* UEIGearComponent::GetHero() const
{
	return CastChecked<AEIHeroCharacter>(GetOwner());
}

void UEIGearComponent::SetSetup(const FEIHeroSetup& NewSetup)
{
	Setup = NewSetup;
	Recalculate();
	GetHero()->RefreshLife();
}

void UEIGearComponent::OnRep_Setup()
{
	Recalculate();
}

const FEIItem* UEIGearComponent::GetWorn(EEISlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot);
	return Setup.Slots.IsValidIndex(Index) && Table.Items.IsValidIndex(Setup.Slots[Index].Item) ? &Table.Items[Setup.Slots[Index].Item] : nullptr;
}

const FEIWeaponForm* UEIGearComponent::GetWeaponForm() const
{
	const FEIItem* Weapon = GetWorn(EEISlot::Weapon);
	return Weapon != nullptr ? Table.GetForm(*Weapon) : nullptr;
}

EEIElement UEIGearComponent::GetWeaponElement() const
{
	const FEIItem* Weapon = GetWorn(EEISlot::Weapon);
	return Weapon != nullptr ? Weapon->Element : EEIElement::Fire;
}

const FEIEngravingForm* UEIGearComponent::GetEngravingForm() const
{
	const FEIWeaponForm* Form = GetWeaponForm();
	return Form != nullptr ? Engravings.FindForm(Form->Key) : nullptr;
}

TArray<FString> UEIGearComponent::GetActiveEngravings() const
{
	TArray<FString> Names;
	if (const FEIEngravingForm* Form = GetEngravingForm())
	{
		for (int32 Tier = 0; Tier < FMath::Min(Form->Tiers.Num(), Setup.Engravings.Num()); ++Tier)
		{
			if (Form->Tiers[Tier].Choices.IsValidIndex(Setup.Engravings[Tier]))
			{
				Names.Add(Form->Tiers[Tier].Choices[Setup.Engravings[Tier]].Name);
			}
		}
	}
	return Names;
}

float UEIGearComponent::GetElementBonus(EEIElement Element) const
{
	return GetStat(EIElementDamageStat(Element));
}

float UEIGearComponent::GetSwordFactor() const
{
	return 1.0f + GetStat(EEIStat::AttackPower);
}

float UEIGearComponent::GetShotFactor() const
{
	return 1.0f + GetStat(EEIStat::AttackPower) + GetElementBonus(GetWeaponElement());
}

float UEIGearComponent::GetSpellFactor() const
{
	return 1.0f + GetStat(EEIStat::SpellPower) + GetElementBonus(GetWeaponElement());
}

float UEIGearComponent::GetRuneFactor(EEIElement Element) const
{
	return 1.0f + GetStat(EEIStat::SpellPower) + GetElementBonus(Element);
}

float UEIGearComponent::GetCooldownFactor() const
{
	return FMath::Max(1.0f - Table.MaxCooldownCut, 1.0f + GetStat(EEIStat::Cooldown));
}

float UEIGearComponent::GetSpeedFactor() const
{
	return 1.0f + FMath::Min(Table.MaxSpeedBonus, GetStat(EEIStat::Speed));
}

float UEIGearComponent::GetManaCostFactor() const
{
	return FMath::Max(1.0f - Table.MaxManaCostCut, 1.0f + GetStat(EEIStat::ManaCost));
}

float UEIGearComponent::GetArmorShare() const
{
	return FMath::Clamp(GetStat(EEIStat::Armor), 0.0f, Table.MaxArmor);
}

void UEIGearComponent::Recalculate()
{
	const auto Add = [this](EEIStat Stat, float Amount) { Stats[static_cast<int32>(Stat)] += Amount; };

	FMemory::Memzero(Stats);
	FMemory::Memzero(Rules);
	TMap<FString, int32> Pieces;
	for (int32 Slot = 0; Slot < EISlotCount; ++Slot)
	{
		const FEIItem* Item = GetWorn(static_cast<EEISlot>(Slot));
		if (Item == nullptr)
		{
			continue;
		}
		for (const FEIStatLine& Line : Item->Stats)
		{
			Add(Line.Stat, Line.Amount);
		}
		if (!Item->Set.IsEmpty())
		{
			++Pieces.FindOrAdd(Item->Set);
		}
	}
	for (const TPair<FString, int32>& Pair : Pieces)
	{
		if (const FEIArmorSet* Set = Table.FindSet(Pair.Key))
		{
			for (const FEIStatLine& Line : Set->PerTwoPieces)
			{
				Add(Line.Stat, Line.Amount * (Pair.Value / 2));
			}
		}
	}
	if (const FEIEngravingForm* Form = GetEngravingForm())
	{
		for (int32 Tier = 0; Tier < FMath::Min(Form->Tiers.Num(), Setup.Engravings.Num()); ++Tier)
		{
			if (!Form->Tiers[Tier].Choices.IsValidIndex(Setup.Engravings[Tier]))
			{
				continue;
			}
			const FEIEngraving& Engraving = Form->Tiers[Tier].Choices[Setup.Engravings[Tier]];
			for (const FEIStatLine& Line : Engraving.Stats)
			{
				Add(Line.Stat, Line.Amount);
			}
			for (const FEIRuleLine& Line : Engraving.Rules)
			{
				Rules[static_cast<int32>(Line.Rule)] += Line.Amount;
			}
		}
	}
	GetHero()->OnGearChanged();
}
