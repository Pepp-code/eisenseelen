#include "EIBosses.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"

namespace
{
	bool ReadBannbruch(const FJsonObject& Line, FEIBannbruch& Out, FString& OutError)
	{
		FString Kind;
		if (!Line.TryGetStringField(TEXT("art"), Kind) || Kind != TEXT("ankersteine"))
		{
			OutError = FString::Printf(TEXT("Den Bannbruch mit der Art %s gibt es nicht."), *Kind);
			return false;
		}
		Out.Kind = EEIBannbruchKind::AnchorStones;
		if (!Line.TryGetStringField(TEXT("name"), Out.Name)
			|| !Line.TryGetStringField(TEXT("teil"), Out.PartName)
			|| !Line.TryGetStringField(TEXT("anleitung"), Out.Hint))
		{
			OutError = TEXT("Einem Bannbruch fehlt name, teil oder anleitung.");
			return false;
		}
		float Count = 0.0f;
		if (!FEIDataFile::ReadNumber(Line, TEXT("anzahl"), 1.0, 20.0, Count, OutError)
			|| !FEIDataFile::ReadNumber(Line, TEXT("halten_s"), 0.1, 3600.0, Out.HoldSeconds, OutError)
			|| !FEIDataFile::ReadNumber(Line, TEXT("zone_m"), 1.0, 100.0, Out.ZoneMeters, OutError)
			|| !FEIDataFile::ReadNumber(Line, TEXT("abstand_m"), 1.0, 200.0, Out.DistanceMeters, OutError))
		{
			return false;
		}
		Out.Count = FMath::RoundToInt32(Count);
		return true;
	}

	bool ReadBoss(const FJsonObject& Line, FEIBossValues& Out, FString& OutError)
	{
		if (!Line.TryGetStringField(TEXT("name"), Out.Name) || !Line.TryGetStringField(TEXT("auftritt"), Out.Entrance))
		{
			OutError = TEXT("Einem Boss fehlt name oder auftritt.");
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Color = nullptr;
		const TSharedPtr<FJsonObject>* Bannbruch = nullptr;
		const TSharedPtr<FJsonObject>* Fight = nullptr;
		const TSharedPtr<FJsonObject>* Stomp = nullptr;
		const TSharedPtr<FJsonObject>* Loot = nullptr;
		if (!Line.TryGetArrayField(TEXT("farbe"), Color) || Color->Num() != 3
			|| !Line.TryGetObjectField(TEXT("bannbruch"), Bannbruch)
			|| !Line.TryGetObjectField(TEXT("werte"), Fight)
			|| !Line.TryGetObjectField(TEXT("stampfen"), Stomp)
			|| !Line.TryGetObjectField(TEXT("beute"), Loot))
		{
			OutError = FString::Printf(TEXT("Dem Boss %s fehlt farbe, bannbruch, werte, stampfen oder beute."), *Out.Name);
			return false;
		}
		double Parts[3] = {};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			(*Color)[Index]->TryGetNumber(Parts[Index]);
		}
		Out.Color = FLinearColor(static_cast<float>(Parts[0]), static_cast<float>(Parts[1]), static_cast<float>(Parts[2]));
		if (!ReadBannbruch(**Bannbruch, Out.Bannbruch, OutError)
			|| !FEIEnemyValues::ReadFrom(**Fight, Out.Fight, OutError)
			|| !FEIDataFile::ReadNumber(**Stomp, TEXT("radius_m"), 0.5, 100.0, Out.Stomp.RadiusMeters, OutError)
			|| !FEIDataFile::ReadNumber(**Stomp, TEXT("ab_abstand_m"), 0.0, 100.0, Out.Stomp.FromMeters, OutError)
			|| !FEIDataFile::ReadNumber(**Stomp, TEXT("ankuendigung_s"), 0.0, 20.0, Out.Stomp.WindupSeconds, OutError)
			|| !FEIDataFile::ReadNumber(**Stomp, TEXT("schaden"), 0.0, 100000.0, Out.Stomp.Damage, OutError)
			|| !FEIDataFile::ReadNumber(**Stomp, TEXT("pause_s"), 0.0, 600.0, Out.Stomp.PauseSeconds, OutError)
			|| !FEIDataFile::ReadNumber(**Loot, TEXT("gold_von"), 0.0, 1000000.0, Out.GoldFrom, OutError)
			|| !FEIDataFile::ReadNumber(**Loot, TEXT("gold_bis"), 0.0, 1000000.0, Out.GoldTo, OutError))
		{
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
			|| !FEIDataFile::ReadNumber(Root, TEXT("kampf_welle_alle_s"), 0.1, 3600.0, Out.FightWaveEverySeconds, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("verfolgung_welle_alle_s"), 0.1, 3600.0, Out.PursuitWaveEverySeconds, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("signalfeuer_mindestabstand_m"), 0.0, 10000.0, Out.FireMinDistanceMeters, OutError))
		{
			return false;
		}
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
