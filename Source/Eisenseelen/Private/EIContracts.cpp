#include "EIContracts.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"

namespace
{
	constexpr float MaxHoldSeconds = 3600.0f;
	constexpr float MaxZoneMeters = 100.0f;

	bool ReadKind(const FString& Key, EEIContractKind& Out)
	{
		if (Key == TEXT("anfuehrer"))
		{
			Out = EEIContractKind::Leader;
		}
		else if (Key == TEXT("schrein"))
		{
			Out = EEIContractKind::Shrine;
		}
		else if (Key == TEXT("truhe"))
		{
			Out = EEIContractKind::Chest;
		}
		else if (Key == TEXT("altar"))
		{
			Out = EEIContractKind::Altar;
		}
		else
		{
			return false;
		}
		return true;
	}

	bool ReadFaction(const FJsonObject& Line, FEIFaction& Out, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Color = nullptr;
		if (!Line.TryGetStringField(TEXT("name"), Out.Name) || !Line.TryGetArrayField(TEXT("farbe"), Color) || Color->Num() != 3)
		{
			OutError = TEXT("Einer Fraktion fehlt der Name oder die Farbe aus drei Zahlen.");
			return false;
		}
		double Parts[3] = {};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (!(*Color)[Index]->TryGetNumber(Parts[Index]))
			{
				OutError = FString::Printf(TEXT("Die Farbe der Fraktion %s besteht nicht aus Zahlen."), *Out.Name);
				return false;
			}
		}
		Out.Color = FLinearColor(static_cast<float>(Parts[0]), static_cast<float>(Parts[1]), static_cast<float>(Parts[2]));
		return true;
	}

	bool ReadTask(const FJsonObject& Line, FEIContractTask& Out, FString& OutError)
	{
		FString Kind;
		if (!Line.TryGetStringField(TEXT("art"), Kind) || !ReadKind(Kind, Out.Kind))
		{
			OutError = FString::Printf(TEXT("Die Aufgabe mit der Art %s gibt es nicht."), *Kind);
			return false;
		}
		if (!Line.TryGetStringField(TEXT("name"), Out.Name) || !Line.TryGetStringField(TEXT("kurz"), Out.ShortName))
		{
			OutError = FString::Printf(TEXT("Der Aufgabe %s fehlt name oder kurz."), *Kind);
			return false;
		}
		const bool bHeld = Out.Kind == EEIContractKind::Shrine || Out.Kind == EEIContractKind::Altar;
		return !bHeld
			|| (FEIDataFile::ReadNumber(Line, TEXT("halten_s"), 0.1, MaxHoldSeconds, Out.HoldSeconds, OutError)
				&& FEIDataFile::ReadNumber(Line, TEXT("zone_m"), 1.0, MaxZoneMeters, Out.ZoneMeters, OutError));
	}

	bool ReadTable(const FJsonObject& Root, FEIContractTable& Out, FString& OutError)
	{
		if (!FEIDataFile::ReadNumber(Root, TEXT("vertraege_pro_lauf"), 0.0, 100.0, Out.ContractsPerRun, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("vertraege_bis_zur_fackel"), 0.0, 100.0, Out.ContractsToFlare, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("mindestabstand_m"), 0.0, 1000.0, Out.MinDistanceMeters, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("gold"), 0.0, 1000000.0, Out.Gold, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("punkte"), 0.0, 1000000.0, Out.Points, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("verderbnis_bonus_je_stufe"), 0.0, 100.0, Out.CorruptionBonusPerStage, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("beim_scheitern_bleibt"), 0.0, 1.0, Out.KeptOnFailure, OutError))
		{
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* FactionLines = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* TaskLines = nullptr;
		if (!Root.TryGetArrayField(TEXT("fraktionen"), FactionLines) || FactionLines->IsEmpty()
			|| !Root.TryGetArrayField(TEXT("aufgaben"), TaskLines) || TaskLines->IsEmpty())
		{
			OutError = TEXT("Die Listen fraktionen und aufgaben müssen da sein und dürfen nicht leer sein.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Line : *FactionLines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEIFaction Faction;
			if (!Line->TryGetObject(Object) || !ReadFaction(**Object, Faction, OutError))
			{
				return false;
			}
			Out.Factions.Add(Faction);
		}
		for (const TSharedPtr<FJsonValue>& Line : *TaskLines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEIContractTask Task;
			if (!Line->TryGetObject(Object) || !ReadTask(**Object, Task, OutError))
			{
				return false;
			}
			Out.Tasks.Add(Task);
		}
		return true;
	}

	TArray<int32> ShuffledTurns(int32 Choices, int32 Count)
	{
		// Whole rounds in a random order, so that two of the same only come once all others have had their turn.
		TArray<int32> Order;
		for (int32 Index = 0; Index < Choices; ++Index)
		{
			Order.Insert(Index, FMath::RandRange(0, Order.Num()));
		}
		TArray<int32> Turns;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Turns.Insert(Order[Index % Choices], FMath::RandRange(0, Turns.Num()));
		}
		return Turns;
	}
}

const FEIContractTask* FEIContractTable::FindTask(EEIContractKind Kind) const
{
	return Tasks.FindByPredicate([Kind](const FEIContractTask& Task) { return Task.Kind == Kind; });
}

const FEIFaction* FEIContractTable::FindFaction(int32 Faction) const
{
	return Factions.IsValidIndex(Faction) ? &Factions[Faction] : nullptr;
}

int32 FEIContractTable::GetGold(int32 Stage) const
{
	return FMath::RoundToInt32(Gold * (1.0f + CorruptionBonusPerStage * Stage));
}

int32 FEIContractTable::GetPoints(int32 Stage) const
{
	return FMath::RoundToInt32(Points * (1.0f + CorruptionBonusPerStage * Stage));
}

int32 FEIContractTable::GetKeptOnFailure(int32 Amount) const
{
	return FMath::FloorToInt32(Amount * KeptOnFailure);
}

TArray<FEIContract> FEIContractTable::RollContracts() const
{
	TArray<FEIContract> Contracts;
	if (Tasks.IsEmpty() || Factions.IsEmpty())
	{
		return Contracts;
	}
	const int32 Count = FMath::RoundToInt32(ContractsPerRun);
	const TArray<int32> TaskTurns = ShuffledTurns(Tasks.Num(), Count);
	const TArray<int32> FactionTurns = ShuffledTurns(Factions.Num(), Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FEIContractTask& Task = Tasks[TaskTurns[Index]];
		FEIContract Contract;
		Contract.Kind = Task.Kind;
		Contract.Faction = FactionTurns[Index];
		Contract.HoldLeft = Task.HoldSeconds;
		Contracts.Add(Contract);
	}
	Contracts.StableSort([](const FEIContract& First, const FEIContract& Second) { return First.Faction < Second.Faction; });
	return Contracts;
}

bool FEIContractTable::LoadFromString(const FString& Json, FEIContractTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEIContractTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}

bool FEIContractTable::LoadFromProjectFile(FEIContractTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("vertraege.json"), OutError);
	const TSharedPtr<FJsonObject> Map = Root.IsValid() ? FEIDataFile::Load(TEXT("laufkarte.json"), OutError) : nullptr;
	FEIContractTable Loaded;
	if (!Root.IsValid() || !Map.IsValid() || !ReadTable(*Root, Loaded, OutError)
		|| !FEIDataFile::ReadNumber(*Map, TEXT("boden_m"), 1.0, 100000.0, Loaded.MapMeters, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}
