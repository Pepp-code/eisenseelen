#include "EIItems.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"

namespace
{
	struct FStatInfo
	{
		const TCHAR* Key;
		const TCHAR* Name;
		bool bAbsolute;
	};

	const FStatInfo StatInfos[EIStatCount] = {
		{ TEXT("angriffskraft"), TEXT("Angriffskraft"), false },
		{ TEXT("angriffstempo"), TEXT("Angriffstempo"), false },
		{ TEXT("aufladetempo"), TEXT("Aufladetempo"), false },
		{ TEXT("zaubermacht"), TEXT("Zaubermacht"), false },
		{ TEXT("feuerschaden"), TEXT("Feuerschaden"), false },
		{ TEXT("eisschaden"), TEXT("Eisschaden"), false },
		{ TEXT("blitzschaden"), TEXT("Blitzschaden"), false },
		{ TEXT("abklingzeit"), TEXT("Abklingzeit"), false },
		{ TEXT("zustandsdauer"), TEXT("Zustandsdauer"), false },
		{ TEXT("reaktionsschaden"), TEXT("Reaktionsschaden"), false },
		{ TEXT("leben"), TEXT("Leben"), true },
		{ TEXT("ruestung"), TEXT("Rüstung"), false },
		{ TEXT("blockstaerke"), TEXT("Blockstärke"), false },
		{ TEXT("tempo"), TEXT("Tempo"), false },
		{ TEXT("mana"), TEXT("Mana"), true },
		{ TEXT("manarueckkehr"), TEXT("Manarückkehr"), false },
		{ TEXT("manakosten"), TEXT("Manakosten"), false }
	};

	const TCHAR* const SlotKeys[EISlotCount] = { TEXT("waffe"), TEXT("rune"), TEXT("helm"), TEXT("brustpanzer"), TEXT("handschuhe"), TEXT("schuhe"), TEXT("herzkristall") };
	const TCHAR* const SlotNames[EISlotCount] = { TEXT("Waffe"), TEXT("Rune"), TEXT("Helm"), TEXT("Brustpanzer"), TEXT("Handschuhe"), TEXT("Schuhe"), TEXT("Herzkristall") };

	bool ReadStatLines(const FJsonObject& Object, const TCHAR* Field, const FString& Owner, TArray<FEIStatLine>& OutLines, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Lines = nullptr;
		if (!Object.TryGetObjectField(Field, Lines))
		{
			return true;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Lines)->Values)
		{
			FEIStatLine Line;
			double Amount = 0.0;
			if (!EIStatFromKey(Pair.Key, Line.Stat))
			{
				OutError = FString::Printf(TEXT("%s hat den unbekannten Wert %s."), *Owner, *Pair.Key);
				return false;
			}
			if (!Pair.Value->TryGetNumber(Amount))
			{
				OutError = FString::Printf(TEXT("Bei %s ist %s keine Zahl."), *Owner, *Pair.Key);
				return false;
			}
			Line.Amount = static_cast<float>(Amount);
			OutLines.Add(Line);
		}
		return true;
	}

	bool ReadForm(const FString& Key, const FJsonObject& Line, FEIWeaponForm& Out, FString& OutError)
	{
		Out.Key = Key;
		FString ClassText;
		if (!Line.TryGetStringField(TEXT("name"), Out.Name) || !Line.TryGetStringField(TEXT("klasse"), ClassText) || (ClassText != TEXT("klingenwacht") && ClassText != TEXT("arkanist")))
		{
			OutError = FString::Printf(TEXT("Die Grundform %s braucht einen Namen und als klasse klingenwacht oder arkanist."), *Key);
			return false;
		}
		Out.bStaff = ClassText == TEXT("arkanist");
		if (!Out.bStaff)
		{
			return FEIDataFile::ReadNumber(Line, TEXT("leichter_hieb_schaden"), 0.0, 100000.0, Out.LightDamage, OutError)
				&& FEIDataFile::ReadNumber(Line, TEXT("leichter_hieb_s"), 0.05, 5.0, Out.LightSeconds, OutError)
				&& FEIDataFile::ReadNumber(Line, TEXT("schwerer_hieb_schaden"), 0.0, 100000.0, Out.HeavyDamage, OutError)
				&& FEIDataFile::ReadNumber(Line, TEXT("schwerer_hieb_s"), 0.05, 5.0, Out.HeavySeconds, OutError)
				&& FEIDataFile::ReadNumber(Line, TEXT("aufladen_s"), 0.1, 5.0, Out.ChargeSeconds, OutError)
				&& FEIDataFile::ReadNumber(Line, TEXT("taumeln_s"), 0.0, 30.0, Out.StaggerSeconds, OutError)
				&& FEIDataFile::ReadNumber(Line, TEXT("laenge_faktor"), 0.1, 5.0, Out.LengthFactor, OutError);
		}
		float Pierces = 0.0f;
		const bool bRead = FEIDataFile::ReadNumber(Line, TEXT("schuss_schaden"), 0.0, 100000.0, Out.BoltDamage, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("schuss_radius_m"), 0.1, 50.0, Out.BoltRadiusMeters, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("schuss_tempo_m_pro_s"), 1.0, 500.0, Out.BoltSpeedMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("schuss_mana"), 0.0, 10000.0, Out.BoltMana, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("durchschlag_0_oder_1"), 0.0, 1.0, Pierces, OutError);
		Out.bBoltPierces = Pierces >= 0.5f;
		return bRead;
	}

	bool ReadItem(const FJsonObject& Line, const FEIItemTable& Table, FEIItem& Out, FString& OutError)
	{
		if (!Line.TryGetStringField(TEXT("name"), Out.Name))
		{
			OutError = TEXT("Einem Item fehlt der Name.");
			return false;
		}
		FString SlotText;
		if (!Line.TryGetStringField(TEXT("platz"), SlotText))
		{
			OutError = FString::Printf(TEXT("%s hat keinen Platz."), *Out.Name);
			return false;
		}
		int32 SlotIndex = INDEX_NONE;
		for (int32 Index = 0; Index < EISlotCount; ++Index)
		{
			SlotIndex = SlotText == SlotKeys[Index] ? Index : SlotIndex;
		}
		if (SlotIndex == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("%s hat den unbekannten Platz %s."), *Out.Name, *SlotText);
			return false;
		}
		Out.Slot = static_cast<EEISlot>(SlotIndex);

		FString ClassText;
		if (Line.TryGetStringField(TEXT("klasse"), ClassText))
		{
			if (ClassText != TEXT("alle") && ClassText != TEXT("klingenwacht") && ClassText != TEXT("arkanist"))
			{
				OutError = FString::Printf(TEXT("%s braucht als klasse alle, klingenwacht oder arkanist."), *Out.Name);
				return false;
			}
			Out.bForBladewarden = ClassText != TEXT("arkanist");
			Out.bForArcanist = ClassText != TEXT("klingenwacht");
		}
		Line.TryGetStringField(TEXT("art"), Out.Set);
		if (!ReadStatLines(Line, TEXT("werte"), Out.Name, Out.Stats, OutError))
		{
			return false;
		}
		if (Out.Slot != EEISlot::Weapon)
		{
			return true;
		}

		// A weapon: its form decides who can use it, and its element what its spell is.
		FString FormText;
		FString ElementText;
		if (!Line.TryGetStringField(TEXT("grundform"), FormText) || !Line.TryGetStringField(TEXT("element"), ElementText))
		{
			OutError = FString::Printf(TEXT("Die Waffe %s braucht eine grundform und ein element."), *Out.Name);
			return false;
		}
		Out.Form = Table.Forms.IndexOfByPredicate([&FormText](const FEIWeaponForm& Form) { return Form.Key == FormText; });
		if (Out.Form == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("%s hat die unbekannte Grundform %s."), *Out.Name, *FormText);
			return false;
		}
		if (!EIElementFromKey(ElementText, Out.Element))
		{
			OutError = FString::Printf(TEXT("%s braucht als element feuer, eis oder blitz."), *Out.Name);
			return false;
		}
		Out.bForBladewarden = !Table.Forms[Out.Form].bStaff;
		Out.bForArcanist = Table.Forms[Out.Form].bStaff;
		const EEIStat ElementStat = Out.Element == EEIElement::Fire ? EEIStat::FireDamage : (Out.Element == EEIElement::Ice ? EEIStat::IceDamage : EEIStat::LightningDamage);
		Out.Stats.Add({ ElementStat, Table.WeaponElementBonus });
		return true;
	}

	bool ReadTable(const FJsonObject& Items, const FJsonObject& Weapons, FEIItemTable& Out, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Forms = nullptr;
		if (!Weapons.TryGetObjectField(TEXT("grundformen"), Forms))
		{
			OutError = TEXT("Der Abschnitt grundformen in waffen.json fehlt.");
			return false;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Forms)->Values)
		{
			const TSharedPtr<FJsonObject>* Line = nullptr;
			FEIWeaponForm Form;
			if (!Pair.Value->TryGetObject(Line) || !ReadForm(Pair.Key, **Line, Form, OutError))
			{
				return false;
			}
			Out.Forms.Add(Form);
		}

		const TArray<TSharedPtr<FJsonValue>>* Qualities = nullptr;
		if (!Items.TryGetArrayField(TEXT("qualitaeten"), Qualities) || Qualities->IsEmpty())
		{
			OutError = TEXT("Die Liste qualitaeten fehlt.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Qualities)
		{
			const TSharedPtr<FJsonObject>* Line = nullptr;
			FString Name;
			float Factor = 1.0f;
			if (!Value->TryGetObject(Line) || !(*Line)->TryGetStringField(TEXT("name"), Name) || !FEIDataFile::ReadNumber(**Line, TEXT("faktor"), 0.01, 100.0, Factor, OutError))
			{
				OutError = TEXT("Einer Qualität fehlt der Name oder der Faktor.");
				return false;
			}
			Out.QualityNames.Add(Name);
			Out.QualityFactors.Add(Factor);
		}

		if (!FEIDataFile::ReadNumber(Items, TEXT("tempo_bonus_max"), 0.0, 10.0, Out.MaxSpeedBonus, OutError)
			|| !FEIDataFile::ReadNumber(Items, TEXT("ruestung_max"), 0.0, 1.0, Out.MaxArmor, OutError)
			|| !FEIDataFile::ReadNumber(Items, TEXT("abklingzeit_abzug_max"), 0.0, 1.0, Out.MaxCooldownCut, OutError)
			|| !FEIDataFile::ReadNumber(Items, TEXT("manakosten_abzug_max"), 0.0, 1.0, Out.MaxManaCostCut, OutError)
			|| !FEIDataFile::ReadNumber(Items, TEXT("waffe_element_bonus"), 0.0, 100.0, Out.WeaponElementBonus, OutError))
		{
			return false;
		}

		const TSharedPtr<FJsonObject>* Rewards = nullptr;
		if (!Items.TryGetObjectField(TEXT("belohnung"), Rewards))
		{
			OutError = TEXT("Der Abschnitt belohnung fehlt.");
			return false;
		}
		if (!FEIDataFile::ReadNumber(**Rewards, TEXT("doppelt_erfahrung"), 0.0, 1000000.0, Out.Rewards.DuplicateExperience, OutError)
			|| !FEIDataFile::ReadNumber(**Rewards, TEXT("umkreis_m"), 1.0, 1000.0, Out.Rewards.RadiusMeters, OutError))
		{
			return false;
		}

		const TSharedPtr<FJsonObject>* Experience = nullptr;
		if (!Items.TryGetObjectField(TEXT("erfahrung"), Experience))
		{
			OutError = TEXT("Der Abschnitt erfahrung fehlt.");
			return false;
		}
		if (!FEIDataFile::ReadNumber(**Experience, TEXT("level_max"), 1.0, 1000.0, Out.MaxLevel, OutError)
			|| !FEIDataFile::ReadNumber(**Experience, TEXT("erste_stufe"), 1.0, 1000000.0, Out.FirstLevelExperience, OutError)
			|| !FEIDataFile::ReadNumber(**Experience, TEXT("zuwachs_je_level"), 0.0, 1000000.0, Out.LevelExperienceGrowth, OutError))
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* Attributes = nullptr;
		if (!Items.TryGetArrayField(TEXT("attribute"), Attributes))
		{
			OutError = TEXT("Die Liste attribute fehlt.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Attributes)
		{
			const TSharedPtr<FJsonObject>* Line = nullptr;
			FEIAttribute Attribute;
			FString StatText;
			if (!Value->TryGetObject(Line) || !(*Line)->TryGetStringField(TEXT("name"), Attribute.Name))
			{
				OutError = TEXT("Einem Attribut fehlt der Name.");
				return false;
			}
			if (!(*Line)->TryGetStringField(TEXT("wert"), StatText) || !EIStatFromKey(StatText, Attribute.Stat))
			{
				OutError = FString::Printf(TEXT("%s hat keinen bekannten Wert."), *Attribute.Name);
				return false;
			}
			if (!FEIDataFile::ReadNumber(**Line, TEXT("je_punkt"), 0.0, 100000.0, Attribute.PerPoint, OutError))
			{
				return false;
			}
			Out.Attributes.Add(Attribute);
		}

		const TArray<TSharedPtr<FJsonValue>>* Sets = nullptr;
		if (Items.TryGetArrayField(TEXT("arten"), Sets))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Sets)
			{
				const TSharedPtr<FJsonObject>* Line = nullptr;
				FEIArmorSet Set;
				if (!Value->TryGetObject(Line) || !(*Line)->TryGetStringField(TEXT("name"), Set.Name))
				{
					OutError = TEXT("Einer Art fehlt der Name.");
					return false;
				}
				if (!ReadStatLines(**Line, TEXT("je_zwei_teile"), Set.Name, Set.PerTwoPieces, OutError))
				{
					return false;
				}
				Out.Sets.Add(Set);
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
		if (!Items.TryGetArrayField(TEXT("items"), Lines))
		{
			OutError = TEXT("Die Liste items fehlt.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Lines)
		{
			const TSharedPtr<FJsonObject>* Line = nullptr;
			FEIItem Item;
			if (!Value->TryGetObject(Line))
			{
				OutError = TEXT("Eine Zeile der Liste items ist kein Item.");
				return false;
			}
			if (!ReadItem(**Line, Out, Item, OutError))
			{
				return false;
			}
			if (Out.FindItem(Item.Name) != INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("%s steht zweimal in der Liste."), *Item.Name);
				return false;
			}
			if (!Item.Set.IsEmpty() && Out.FindSet(Item.Set) == nullptr)
			{
				OutError = FString::Printf(TEXT("%s gehört zur unbekannten Art %s."), *Item.Name, *Item.Set);
				return false;
			}
			Out.Items.Add(Item);
		}

		const TArray<TSharedPtr<FJsonValue>>* Start = nullptr;
		if (!Items.TryGetArrayField(TEXT("start_items"), Start))
		{
			OutError = TEXT("Die Liste start_items fehlt.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Start)
		{
			const FString Name = Value->AsString();
			if (Out.FindItem(Name) == INDEX_NONE)
			{
				OutError = FString::Printf(TEXT("Das Start-Item %s gibt es nicht."), *Name);
				return false;
			}
			Out.StartItems.Add(Name);
		}
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (Out.GetStartItem(EEISlot::Weapon, Index == 1) == INDEX_NONE)
			{
				OutError = TEXT("Jede Klasse braucht eine Start-Waffe.");
				return false;
			}
		}
		return true;
	}
}

const TCHAR* EIStatKey(EEIStat Stat)
{
	return StatInfos[static_cast<int32>(Stat)].Key;
}

bool EIStatFromKey(const FString& Key, EEIStat& OutStat)
{
	for (int32 Index = 0; Index < EIStatCount; ++Index)
	{
		if (Key == StatInfos[Index].Key)
		{
			OutStat = static_cast<EEIStat>(Index);
			return true;
		}
	}
	return false;
}

const TCHAR* EIStatName(EEIStat Stat)
{
	return StatInfos[static_cast<int32>(Stat)].Name;
}

const TCHAR* EISlotName(EEISlot Slot)
{
	return SlotNames[static_cast<int32>(Slot)];
}

const TCHAR* EISlotKey(EEISlot Slot)
{
	return SlotKeys[static_cast<int32>(Slot)];
}

FString EIDescribeStat(EEIStat Stat, float Amount)
{
	const TCHAR* Sign = Amount < 0.0f ? TEXT("-") : TEXT("+");
	const float Size = FMath::Abs(Amount);
	if (StatInfos[static_cast<int32>(Stat)].bAbsolute)
	{
		return FString::Printf(TEXT("%s%.0f %s"), Sign, Size, EIStatName(Stat));
	}
	// Half a percent is a real number here, and a whole one should not show a zero behind the comma.
	const float Percent = Size * 100.0f;
	const bool bWhole = FMath::IsNearlyEqual(Percent, FMath::RoundToFloat(Percent), 0.01f);
	return bWhole ? FString::Printf(TEXT("%s%.0f %% %s"), Sign, Percent, EIStatName(Stat)) : FString::Printf(TEXT("%s%.1f %% %s"), Sign, Percent, EIStatName(Stat));
}

int32 FEIItemTable::FindItem(const FString& Name) const
{
	return Items.IndexOfByPredicate([&Name](const FEIItem& Item) { return Item.Name == Name; });
}

const FEIArmorSet* FEIItemTable::FindSet(const FString& Name) const
{
	return Sets.FindByPredicate([&Name](const FEIArmorSet& Set) { return Set.Name == Name; });
}

const FEIWeaponForm* FEIItemTable::GetForm(const FEIItem& Item) const
{
	return Forms.IsValidIndex(Item.Form) ? &Forms[Item.Form] : nullptr;
}

float FEIItemTable::GetQualityFactor(int32 Quality) const
{
	return QualityFactors.IsValidIndex(Quality) ? QualityFactors[Quality] : 1.0f;
}

FString FEIItemTable::GetQualityName(int32 Quality) const
{
	return QualityNames.IsValidIndex(Quality) ? QualityNames[Quality] : FString();
}

int32 FEIItemTable::GetExperienceForLevel(int32 Level) const
{
	const int32 Steps = FMath::Max(Level - 1, 0);
	return FMath::RoundToInt32(Steps * FirstLevelExperience + LevelExperienceGrowth * Steps * (Steps - 1) / 2.0f);
}

int32 FEIItemTable::GetLevel(int32 Experience) const
{
	int32 Level = 1;
	while (Level < FMath::RoundToInt32(MaxLevel) && Experience >= GetExperienceForLevel(Level + 1))
	{
		++Level;
	}
	return Level;
}

int32 FEIItemTable::GetStartItem(EEISlot Slot, bool bArcanist) const
{
	for (const FString& Name : StartItems)
	{
		const int32 Index = FindItem(Name);
		if (Items.IsValidIndex(Index) && Items[Index].Slot == Slot && Items[Index].FitsClass(bArcanist))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool FEIItemTable::LoadFromStrings(const FString& ItemsJson, const FString& WeaponsJson, FEIItemTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Items = FEIDataFile::Parse(ItemsJson, OutError);
	const TSharedPtr<FJsonObject> Weapons = Items.IsValid() ? FEIDataFile::Parse(WeaponsJson, OutError) : nullptr;
	FEIItemTable Loaded;
	if (!Items.IsValid() || !Weapons.IsValid() || !ReadTable(*Items, *Weapons, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}

bool FEIItemTable::LoadFromProjectFiles(FEIItemTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Items = FEIDataFile::Load(TEXT("items.json"), OutError);
	const TSharedPtr<FJsonObject> Weapons = Items.IsValid() ? FEIDataFile::Load(TEXT("waffen.json"), OutError) : nullptr;
	FEIItemTable Loaded;
	if (!Items.IsValid() || !Weapons.IsValid() || !ReadTable(*Items, *Weapons, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}
