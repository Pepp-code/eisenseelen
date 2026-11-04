#include "EIGearComponent.h"

#include "EIDataFile.h"
#include "EIHeroCharacter.h"
#include "EIOmenComponent.h"
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

int32 UEIGearComponent::GetWornQuality(EEISlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot);
	return Setup.Slots.IsValidIndex(Index) ? Setup.Slots[Index].Quality : 0;
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

float UEIGearComponent::GetElementBonus(EEIElement Element) const
{
	switch (Element)
	{
	case EEIElement::Fire: return GetStat(EEIStat::FireDamage);
	case EEIElement::Ice: return GetStat(EEIStat::IceDamage);
	default: return GetStat(EEIStat::LightningDamage);
	}
}

float UEIGearComponent::GetWeaponQualityFactor() const
{
	return Table.GetQualityFactor(GetWornQuality(EEISlot::Weapon));
}

float UEIGearComponent::GetSwordFactor() const
{
	return GetWeaponQualityFactor() * (1.0f + GetStat(EEIStat::AttackPower));
}

float UEIGearComponent::GetShotFactor() const
{
	return GetWeaponQualityFactor() * (1.0f + GetStat(EEIStat::AttackPower) + GetElementBonus(GetWeaponElement()));
}

float UEIGearComponent::GetSpellFactor() const
{
	return GetWeaponQualityFactor() * (1.0f + GetStat(EEIStat::SpellPower) + GetElementBonus(GetWeaponElement()));
}

float UEIGearComponent::GetRuneFactor(EEIElement Element) const
{
	return Table.GetQualityFactor(GetWornQuality(EEISlot::Rune)) * (1.0f + GetStat(EEIStat::SpellPower) + GetElementBonus(Element));
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
	TMap<FString, int32> Pieces;
	for (int32 Slot = 0; Slot < EISlotCount; ++Slot)
	{
		const FEIItem* Item = GetWorn(static_cast<EEISlot>(Slot));
		if (Item == nullptr)
		{
			continue;
		}
		const float Factor = Table.GetQualityFactor(GetWornQuality(static_cast<EEISlot>(Slot)));
		for (const FEIStatLine& Line : Item->Stats)
		{
			Add(Line.Stat, Line.Amount * Factor);
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
	const UEIOmenComponent* Omens = GetHero()->GetOmens();
	for (int32 Stat = 0; Stat < EIStatCount; ++Stat)
	{
		Stats[Stat] += Omens->GetBonus(static_cast<EEIStat>(Stat));
	}
	GetHero()->OnGearChanged();
}
