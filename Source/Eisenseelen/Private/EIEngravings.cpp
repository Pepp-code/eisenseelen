#include "EIEngravings.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"

namespace
{
	constexpr int32 MaxPrice = 1000000;

	const TCHAR* const RuleKeys[EIEngravingRuleCount] = {
		TEXT("reichweite"),
		TEXT("ziele_mehr"),
		TEXT("taumeln_laenger"),
		TEXT("q_schaden"),
		TEXT("q_abklingzeit"),
		TEXT("parade_staerkt_hieb"),
		TEXT("schwerer_hieb_element"),
		TEXT("schuss_radius"),
		TEXT("schuss_tempo"),
		TEXT("schuss_durchschlag"),
		TEXT("schuss_verlangsamung"),
		TEXT("schuss_verlangsamung_s"),
		TEXT("schuss_element"),
		TEXT("starker_schuss_alle"),
		TEXT("starker_schuss_bonus"),
	};

	bool ReadPrice(const FJsonObject& Object, const TCHAR* Key, const FString& Owner, int32& OutPrice, FString& OutError)
	{
		double Number = 0.0;
		if (!Object.TryGetNumberField(Key, Number) || Number < 0.0 || Number > MaxPrice)
		{
			OutError = FString::Printf(TEXT("%s braucht als %s eine Zahl von 0 bis %d."), *Owner, Key, MaxPrice);
			return false;
		}
		OutPrice = FMath::RoundToInt32(Number);
		return true;
	}

	bool ReadEngraving(const FJsonObject& Object, const FString& Owner, FEIEngraving& Out, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Effects = nullptr;
		if (!Object.TryGetStringField(TEXT("name"), Out.Name) || Out.Name.IsEmpty() || !Object.TryGetStringField(TEXT("text"), Out.Text) || !Object.TryGetObjectField(TEXT("wirkung"), Effects))
		{
			OutError = FString::Printf(TEXT("Jede Wahl in %s braucht name, text und wirkung."), *Owner);
			return false;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Effects)->Values)
		{
			double Amount = 0.0;
			if (!Pair.Value->TryGetNumber(Amount))
			{
				OutError = FString::Printf(TEXT("Bei der Gravur %s ist %s keine Zahl."), *Out.Name, *Pair.Key);
				return false;
			}
			EEIStat Stat;
			EEIEngravingRule Rule;
			if (EIStatFromKey(Pair.Key, Stat))
			{
				Out.Stats.Add({ Stat, static_cast<float>(Amount) });
			}
			else if (EIEngravingRuleFromKey(Pair.Key, Rule))
			{
				Out.Rules.Add({ Rule, static_cast<float>(Amount) });
			}
			else
			{
				OutError = FString::Printf(TEXT("Die Gravur %s hat die unbekannte Wirkung %s."), *Out.Name, *Pair.Key);
				return false;
			}
		}
		return true;
	}

	bool ReadForm(const FString& Key, const FJsonObject& Object, FEIEngravingForm& Out, FString& OutError)
	{
		Out.FormKey = Key;
		const TArray<TSharedPtr<FJsonValue>>* Tiers = nullptr;
		if (!Object.TryGetArrayField(TEXT("stufen"), Tiers) || Tiers->IsEmpty())
		{
			OutError = FString::Printf(TEXT("Die Grundform %s braucht eine Liste stufen."), *Key);
			return false;
		}
		TSet<FString> Names;
		for (int32 Index = 0; Index < Tiers->Num(); ++Index)
		{
			const FString Owner = FString::Printf(TEXT("Stufe %d von %s"), Index + 1, *Key);
			const TSharedPtr<FJsonObject>* TierObject = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
			FEIEngravingTier& Tier = Out.Tiers.AddDefaulted_GetRef();
			if (!(*Tiers)[Index]->TryGetObject(TierObject) || !(*TierObject)->TryGetStringField(TEXT("thema"), Tier.Theme) || !(*TierObject)->TryGetArrayField(TEXT("wahl"), Choices) || Choices->IsEmpty())
			{
				OutError = FString::Printf(TEXT("%s braucht ein thema und eine Liste wahl."), *Owner);
				return false;
			}
			if (!ReadPrice(**TierObject, TEXT("gold"), Owner, Tier.Gold, OutError) || !ReadPrice(**TierObject, TEXT("punkte"), Owner, Tier.Points, OutError))
			{
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Value : *Choices)
			{
				const TSharedPtr<FJsonObject>* ChoiceObject = nullptr;
				FEIEngraving& Choice = Tier.Choices.AddDefaulted_GetRef();
				if (!Value->TryGetObject(ChoiceObject))
				{
					OutError = FString::Printf(TEXT("In %s ist eine Wahl kein Abschnitt."), *Owner);
					return false;
				}
				if (!ReadEngraving(**ChoiceObject, Owner, Choice, OutError))
				{
					return false;
				}
				if (Names.Contains(Choice.Name))
				{
					OutError = FString::Printf(TEXT("Die Gravur %s gibt es bei %s zweimal."), *Choice.Name, *Key);
					return false;
				}
				Names.Add(Choice.Name);
			}
		}
		return true;
	}

	bool ReadTable(const FJsonObject& Root, FEIEngravingTable& Out, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Factions = nullptr;
		const TSharedPtr<FJsonObject>* Forms = nullptr;
		if (!Root.TryGetObjectField(TEXT("fraktion_je_element"), Factions) || !Root.TryGetObjectField(TEXT("grundformen"), Forms))
		{
			OutError = TEXT("Es fehlen die Abschnitte fraktion_je_element oder grundformen.");
			return false;
		}
		const TCHAR* const ElementKeys[4] = { TEXT("feuer"), TEXT("eis"), TEXT("blitz"), TEXT("wind") };
		for (int32 Index = 0; Index < 4; ++Index)
		{
			EEIElement Element;
			if (!(*Factions)->TryGetStringField(ElementKeys[Index], Out.FactionNames[Index]) || Out.FactionNames[Index].IsEmpty() || !EIElementFromKey(ElementKeys[Index], Element) || static_cast<int32>(Element) != Index)
			{
				OutError = FString::Printf(TEXT("fraktion_je_element braucht eine Fraktion für %s."), ElementKeys[Index]);
				return false;
			}
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Forms)->Values)
		{
			const TSharedPtr<FJsonObject>* FormObject = nullptr;
			if (!Pair.Value->TryGetObject(FormObject))
			{
				OutError = FString::Printf(TEXT("Die Grundform %s ist kein Abschnitt."), *Pair.Key);
				return false;
			}
			if (!ReadForm(Pair.Key, **FormObject, Out.Forms.AddDefaulted_GetRef(), OutError))
			{
				return false;
			}
		}
		return true;
	}
}

const TCHAR* EIEngravingRuleKey(EEIEngravingRule Rule)
{
	return RuleKeys[static_cast<int32>(Rule)];
}

bool EIEngravingRuleFromKey(const FString& Key, EEIEngravingRule& OutRule)
{
	for (int32 Index = 0; Index < EIEngravingRuleCount; ++Index)
	{
		if (Key == RuleKeys[Index])
		{
			OutRule = static_cast<EEIEngravingRule>(Index);
			return true;
		}
	}
	return false;
}

bool FEIEngravingForm::Find(const FString& Name, int32& OutTier, int32& OutChoice) const
{
	for (int32 Tier = 0; Tier < Tiers.Num(); ++Tier)
	{
		const int32 Choice = Tiers[Tier].Choices.IndexOfByPredicate([&Name](const FEIEngraving& Engraving) { return Engraving.Name == Name; });
		if (Choice != INDEX_NONE)
		{
			OutTier = Tier;
			OutChoice = Choice;
			return true;
		}
	}
	return false;
}

const FEIEngravingForm* FEIEngravingTable::FindForm(const FString& FormKey) const
{
	return Forms.FindByPredicate([&FormKey](const FEIEngravingForm& Form) { return Form.FormKey == FormKey; });
}

bool FEIEngravingTable::LoadFromString(const FString& Json, FEIEngravingTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEIEngravingTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = MoveTemp(Loaded);
	return true;
}

bool FEIEngravingTable::LoadFromProjectFile(FEIEngravingTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("gravuren.json"), OutError);
	FEIEngravingTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = MoveTemp(Loaded);
	return true;
}
