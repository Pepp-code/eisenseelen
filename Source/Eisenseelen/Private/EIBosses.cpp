#include "EIBosses.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"

namespace
{
	bool ReadBannbruch(const FJsonObject& Line, FEIBannbruch& Out, FString& OutError)
	{
		FString Kind;
		Line.TryGetStringField(TEXT("art"), Kind);
		if (Kind == TEXT("ankersteine"))
		{
			Out.Kind = EEIBannbruchKind::AnchorStones;
		}
		else if (Kind == TEXT("kessel"))
		{
			Out.Kind = EEIBannbruchKind::Kettle;
		}
		else if (Kind == TEXT("seelensteine"))
		{
			Out.Kind = EEIBannbruchKind::SoulStones;
		}
		else
		{
			OutError = FString::Printf(TEXT("Den Bannbruch mit der Art %s gibt es nicht. Es gibt ankersteine, kessel und seelensteine."), *Kind);
			return false;
		}
		if (!Line.TryGetStringField(TEXT("name"), Out.Name)
			|| !Line.TryGetStringField(TEXT("teil"), Out.PartName)
			|| !Line.TryGetStringField(TEXT("anleitung"), Out.Hint))
		{
			OutError = TEXT("Einem Bannbruch fehlt name, teil oder anleitung.");
			return false;
		}
		// Only the anchor stones are held for a time. The kettle fills itself, and the stones of the armor go in one by one.
		const bool bKettle = Out.Kind == EEIBannbruchKind::Kettle;
		float Count = 0.0f;
		if (!FEIDataFile::ReadNumber(Line, TEXT("anzahl"), 1.0, bKettle ? 500.0 : 20.0, Count, OutError)
			|| (Out.Kind == EEIBannbruchKind::AnchorStones && !FEIDataFile::ReadNumber(Line, TEXT("halten_s"), 0.1, 3600.0, Out.HoldSeconds, OutError))
			|| !FEIDataFile::ReadNumber(Line, TEXT("zone_m"), 1.0, 100.0, Out.ZoneMeters, OutError)
			|| !FEIDataFile::ReadNumber(Line, TEXT("abstand_m"), 1.0, 200.0, Out.DistanceMeters, OutError))
		{
			return false;
		}
		Out.Count = FMath::RoundToInt32(Count);
		return true;
	}

	bool ReadStomp(const FJsonObject& Line, FEIStomp& Out, FString& OutError)
	{
		Out.bHas = true;
		return FEIDataFile::ReadNumber(Line, TEXT("radius_m"), 0.5, 100.0, Out.RadiusMeters, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("ab_abstand_m"), 0.0, 100.0, Out.FromMeters, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("ankuendigung_s"), 0.0, 20.0, Out.WindupSeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("schaden"), 0.0, 100000.0, Out.Damage, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("pause_s"), 0.0, 600.0, Out.PauseSeconds, OutError);
	}

	bool ReadSummon(const FJsonObject& Line, FEISummon& Out, FString& OutError)
	{
		Out.bHas = true;
		if (!Line.TryGetStringField(TEXT("gruppe"), Out.GroupSize) || Out.GroupSize.IsEmpty())
		{
			OutError = TEXT("Beim Beschwoeren fehlt die gruppe (klein, mittel, gross oder elite).");
			return false;
		}
		return FEIDataFile::ReadNumber(Line, TEXT("alle_s"), 1.0, 600.0, Out.EverySeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("ankuendigung_s"), 0.0, 20.0, Out.WindupSeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("schutz_hoechstens_s"), 0.0, 600.0, Out.MaxShieldSeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("abstand_m"), 1.0, 100.0, Out.DistanceMeters, OutError);
	}

	bool ReadVanish(const FJsonObject& Line, FEIVanish& Out, FString& OutError)
	{
		Out.bHas = true;
		if (!FEIDataFile::ReadNumber(Line, TEXT("alle_s"), 1.0, 600.0, Out.EverySeconds, OutError)
			|| !FEIDataFile::ReadNumber(Line, TEXT("ankuendigung_s"), 0.0, 20.0, Out.WindupSeconds, OutError)
			|| !FEIDataFile::ReadNumber(Line, TEXT("weite_m"), 1.0, 200.0, Out.RangeMeters, OutError)
			|| !FEIDataFile::ReadNumber(Line, TEXT("mindestens_m"), 0.0, 200.0, Out.MinMeters, OutError))
		{
			return false;
		}
		if (Out.MinMeters > Out.RangeMeters)
		{
			OutError = TEXT("Beim Verschwinden liegt mindestens_m ueber weite_m.");
			return false;
		}
		return true;
	}

	bool ReadBoss(const FJsonObject& Line, FEIBossValues& Out, FString& OutError)
	{
		if (!Line.TryGetStringField(TEXT("name"), Out.Name) || !Line.TryGetStringField(TEXT("auftritt"), Out.Entrance))
		{
			OutError = TEXT("Einem Boss fehlt name oder auftritt.");
			return false;
		}
		// Where a boss lives is up to it. But a gebiet that is there must also say something.
		if (Line.TryGetStringField(TEXT("gebiet"), Out.Area) && Out.Area.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Beim Boss %s ist gebiet leer."), *Out.Name);
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Color = nullptr;
		const TSharedPtr<FJsonObject>* Bannbruch = nullptr;
		const TSharedPtr<FJsonObject>* Fight = nullptr;
		const TSharedPtr<FJsonObject>* Stomp = nullptr;
		const TSharedPtr<FJsonObject>* Summon = nullptr;
		const TSharedPtr<FJsonObject>* Vanish = nullptr;
		const TSharedPtr<FJsonObject>* Loot = nullptr;
		if (!Line.TryGetArrayField(TEXT("farbe"), Color) || Color->Num() != 3
			|| !Line.TryGetObjectField(TEXT("bannbruch"), Bannbruch)
			|| !Line.TryGetObjectField(TEXT("werte"), Fight)
			|| !Line.TryGetObjectField(TEXT("beute"), Loot))
		{
			OutError = FString::Printf(TEXT("Dem Boss %s fehlt farbe, bannbruch, werte oder beute."), *Out.Name);
			return false;
		}
		double Parts[3] = {};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			(*Color)[Index]->TryGetNumber(Parts[Index]);
		}
		Out.Color = FLinearColor(static_cast<float>(Parts[0]), static_cast<float>(Parts[1]), static_cast<float>(Parts[2]));
		Out.Stomp.bHas = false;
		// Stomping, calling and vanishing are each up to the boss.
		if (!ReadBannbruch(**Bannbruch, Out.Bannbruch, OutError)
			|| !FEIEnemyValues::ReadFrom(**Fight, Out.Fight, OutError)
			|| (Line.TryGetObjectField(TEXT("stampfen"), Stomp) && !ReadStomp(**Stomp, Out.Stomp, OutError))
			|| (Line.TryGetObjectField(TEXT("beschwoeren"), Summon) && !ReadSummon(**Summon, Out.Summon, OutError))
			|| (Line.TryGetObjectField(TEXT("verschwinden"), Vanish) && !ReadVanish(**Vanish, Out.Vanish, OutError))
			|| !FEIDataFile::ReadNumber(**Loot, TEXT("gold_von"), 0.0, 1000000.0, Out.GoldFrom, OutError)
			|| !FEIDataFile::ReadNumber(**Loot, TEXT("gold_bis"), 0.0, 1000000.0, Out.GoldTo, OutError))
		{
			if (!OutError.StartsWith(TEXT("Beim Boss")) && !OutError.StartsWith(TEXT("Dem Boss")))
			{
				OutError = FString::Printf(TEXT("Beim Boss %s: %s"), *Out.Name, *OutError);
			}
			return false;
		}
		Out.Fight.Name = Out.Name;
		if (Out.GoldTo < Out.GoldFrom)
		{
			OutError = FString::Printf(TEXT("Beim Boss %s liegt gold_bis unter gold_von."), *Out.Name);
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
		if (!(*Loot)->TryGetArrayField(TEXT("zutaten"), Items))
		{
			OutError = FString::Printf(TEXT("Beim Boss %s fehlt die Liste zutaten."), *Out.Name);
			return false;
		}
		for (const TSharedPtr<FJsonValue>& ItemLine : *Items)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEILootItem Item;
			if (!ItemLine->TryGetObject(Object)
				|| !(*Object)->TryGetStringField(TEXT("name"), Item.Name)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("chance"), 0.0, 1.0, Item.Chance, OutError))
			{
				if (OutError.IsEmpty())
				{
					OutError = FString::Printf(TEXT("Beim Boss %s fehlt einer Zutat der Name."), *Out.Name);
				}
				return false;
			}
			Out.Loot.Add(Item);
		}
		return true;
	}

	bool ReadTable(const FJsonObject& Root, FEIBossTable& Out, FString& OutError)
	{
		if (!FEIDataFile::ReadNumber(Root, TEXT("platz_leine_m"), 1.0, 1000.0, Out.LeashMeters, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("erholung_anteil_pro_s"), 0.0, 10.0, Out.RecoverySharePerSecond, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("leben_je_weiterem_spieler"), 0.0, 10.0, Out.LifePerExtraPlayer, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("signalfeuer_mindestabstand_m"), 0.0, 10000.0, Out.FireMinDistanceMeters, OutError))
		{
			return false;
		}
		float Letters = 0.0f;
		if (!FEIDataFile::ReadNumber(Root, TEXT("jagdbriefe"), 1.0, 4.0, Letters, OutError))
		{
			return false;
		}
		Out.HuntLetters = FMath::RoundToInt32(Letters);
		const TArray<TSharedPtr<FJsonValue>>* Words = nullptr;
		if (!Root.TryGetArrayField(TEXT("chance_worte"), Words) || Words->IsEmpty())
		{
			OutError = TEXT("Die Liste chance_worte muss da sein und darf nicht leer sein.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& WordLine : *Words)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEIChanceWord Word;
			if (!WordLine->TryGetObject(Object)
				|| !(*Object)->TryGetStringField(TEXT("wort"), Word.Word)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("ab"), 0.0, 1.0, Word.From, OutError))
			{
				if (OutError.IsEmpty())
				{
					OutError = TEXT("Einem Eintrag in chance_worte fehlt das wort.");
				}
				return false;
			}
			Out.ChanceWords.Add(Word);
		}
		Out.ChanceWords.Sort([](const FEIChanceWord& A, const FEIChanceWord& B) { return A.From > B.From; });
		const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
		if (!Root.TryGetArrayField(TEXT("bosse"), Lines) || Lines->IsEmpty())
		{
			OutError = TEXT("Die Liste bosse muss da sein und darf nicht leer sein.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Line : *Lines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEIBossValues Boss;
			if (!Line->TryGetObject(Object) || !ReadBoss(**Object, Boss, OutError))
			{
				return false;
			}
			Out.Bosses.Add(Boss);
		}
		return true;
	}
}

int32 FEIBossValues::RollGold() const
{
	return FMath::RandRange(FMath::RoundToInt32(GoldFrom), FMath::RoundToInt32(GoldTo));
}

TArray<FString> FEIBossValues::RollLoot() const
{
	TArray<FString> Found;
	for (const FEILootItem& Item : Loot)
	{
		if (FMath::FRand() < Item.Chance)
		{
			Found.Add(Item.Name);
		}
	}
	return Found;
}

FString FEIBossTable::DescribeChance(float Chance) const
{
	for (const FEIChanceWord& Word : ChanceWords)
	{
		if (Chance >= Word.From)
		{
			return Word.Word;
		}
	}
	return ChanceWords.IsEmpty() ? FString() : ChanceWords.Last().Word;
}

bool FEIBossTable::LoadFromString(const FString& Json, FEIBossTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEIBossTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}

bool FEIBossTable::LoadFromProjectFile(FEIBossTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("bosse.json"), OutError);
	FEIBossTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}
