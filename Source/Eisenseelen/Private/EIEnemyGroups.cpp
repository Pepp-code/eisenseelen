#include "EIEnemyGroups.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"

namespace
{
	bool ReadGroup(const FJsonObject& Line, FEIEnemyGroup& Out, FString& OutError)
	{
		if (!Line.TryGetStringField(TEXT("name"), Out.Name) || !Line.TryGetStringField(TEXT("groesse"), Out.Size))
		{
			OutError = TEXT("Einer Gruppe fehlt der Name oder die Größe.");
			return false;
		}
		int32 Everyone = 0;
		for (int32 Kind = 0; Kind < EIEnemyKindCount; ++Kind)
		{
			const FString Key = EIEnemyKindKey(static_cast<EEIEnemyKind>(Kind));
			float Count = 0.0f;
			if (Line.HasField(Key) && !FEIDataFile::ReadNumber(Line, *Key, 0.0, 100.0, Count, OutError))
			{
				return false;
			}
			Out.Counts[Kind] = FMath::RoundToInt(Count);
			Everyone += Out.Counts[Kind];
		}
		if (Everyone == 0)
		{
			OutError = FString::Printf(TEXT("In der Gruppe %s ist niemand."), *Out.Name);
			return false;
		}
		return true;
	}

	bool ReadWave(const FJsonObject& Waves, const TCHAR* Key, FEIWaveRule& Out, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Line = nullptr;
		if (!Waves.TryGetObjectField(Key, Line) || !(*Line)->TryGetStringField(TEXT("groesse"), Out.Size))
		{
			OutError = FString::Printf(TEXT("Im Abschnitt blutmond fehlt %s mit seiner groesse."), Key);
			return false;
		}
		return FEIDataFile::ReadNumber(**Line, TEXT("alle_s"), 0.1, 3600.0, Out.EverySeconds, OutError);
	}

	bool ReadTable(const FJsonObject& Root, FEIEnemyGroups& Out, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* GroupLines = nullptr;
		const TSharedPtr<FJsonObject>* Waves = nullptr;
		if (!Root.TryGetArrayField(TEXT("gruppen"), GroupLines) || !Root.TryGetObjectField(TEXT("blutmond"), Waves) || !Root.TryGetStringField(TEXT("truhe"), Out.ChestGuards))
		{
			OutError = TEXT("Die Liste gruppen und die Abschnitte blutmond und truhe müssen da sein.");
			return false;
		}
		if (!FEIDataFile::ReadNumber(Root, TEXT("obergrenze"), 0.0, 10000.0, Out.ThreatCap, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("blutmond_nachlauf_s"), 0.0, 600.0, Out.BloodMoonLingerSeconds, OutError)
			|| !ReadWave(**Waves, TEXT("altar"), Out.AltarWave, OutError)
			|| !ReadWave(**Waves, TEXT("ankerstein"), Out.StoneWave, OutError)
			|| !ReadWave(**Waves, TEXT("kessel"), Out.KettleWave, OutError)
			|| !ReadWave(**Waves, TEXT("bosskampf"), Out.FightWave, OutError)
			|| !ReadWave(**Waves, TEXT("flucht"), Out.PursuitWave, OutError)
			|| !ReadWave(**Waves, TEXT("signalfeuer"), Out.FireWave, OutError))
		{
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Line : *GroupLines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEIEnemyGroup Group;
			if (!Line->TryGetObject(Object) || !ReadGroup(**Object, Group, OutError))
			{
				return false;
			}
			Out.Groups.Add(Group);
		}
		return true;
	}
}

TArray<EEIEnemyKind> FEIEnemyGroup::ListMembers(float CountFactor) const
{
	TArray<EEIEnemyKind> Members;
	if (CountFactor <= 0.0f)
	{
		return Members;
	}
	for (int32 Kind = 0; Kind < EIEnemyKindCount; ++Kind)
	{
		const int32 Count = Counts[Kind] > 0 ? FMath::Max(1, FMath::RoundToInt(Counts[Kind] * CountFactor)) : 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Members.Add(static_cast<EEIEnemyKind>(Kind));
		}
	}
	return Members;
}

const FEIEnemyGroup* FEIEnemyGroups::Pick(const FString& Size) const
{
	TArray<const FEIEnemyGroup*> Fitting;
	for (const FEIEnemyGroup& Group : Groups)
	{
		if (Group.Size == Size)
		{
			Fitting.Add(&Group);
		}
	}
	return Fitting.IsEmpty() ? nullptr : Fitting[FMath::RandRange(0, Fitting.Num() - 1)];
}

bool FEIEnemyGroups::LoadFromString(const FString& Json, FEIEnemyGroups& OutGroups, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEIEnemyGroups Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutGroups = Loaded;
	return true;
}

bool FEIEnemyGroups::LoadFromProjectFile(FEIEnemyGroups& OutGroups, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("gruppen.json"), OutError);
	FEIEnemyGroups Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutGroups = Loaded;
	return true;
}
