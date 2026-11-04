#include "EIEnemyGroups.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"

namespace
{
	const FEIGroupStage NoStage;
	constexpr int32 MaxStage = 100;

	bool ReadGroup(const FJsonObject& Line, FEIEnemyGroup& Out, FString& OutError)
	{
		if (!Line.TryGetStringField(TEXT("name"), Out.Name) || !Line.TryGetStringField(TEXT("groesse"), Out.Size))
		{
			OutError = TEXT("Einer Gruppe fehlt der Name oder die Größe.");
			return false;
		}
		float FromStage = 0.0f;
		float ToStage = 0.0f;
		if (!FEIDataFile::ReadNumber(Line, TEXT("ab_stufe"), 0.0, MaxStage, FromStage, OutError)
			|| !FEIDataFile::ReadNumber(Line, TEXT("bis_stufe"), 0.0, MaxStage, ToStage, OutError))
		{
			return false;
		}
		Out.FromStage = FMath::RoundToInt(FromStage);
		Out.ToStage = FMath::RoundToInt(ToStage);
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

	bool ReadStage(const FJsonObject& Line, FEIGroupStage& Out, FString& OutError)
	{
		if (!Line.TryGetStringField(TEXT("lager"), Out.Camp)
			|| !Line.TryGetStringField(TEXT("nachschub"), Out.Reinforcement)
			|| !Line.TryGetStringField(TEXT("truhe"), Out.Chest)
			|| !Line.TryGetStringField(TEXT("altar"), Out.AltarWave)
			|| !Line.TryGetStringField(TEXT("fackel"), Out.FlareWave))
		{
			OutError = TEXT("Einer Stufe fehlt lager, nachschub, truhe, altar oder fackel.");
			return false;
		}
		return FEIDataFile::ReadNumber(Line, TEXT("nachschub_alle_s"), 0.0, 3600.0, Out.ReinforcementEverySeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("obergrenze"), 0.0, 10000.0, Out.ThreatCap, OutError);
	}

	bool ReadTable(const FJsonObject& Root, FEIEnemyGroups& Out, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* GroupLines = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* StageLines = nullptr;
		if (!Root.TryGetArrayField(TEXT("gruppen"), GroupLines) || !Root.TryGetArrayField(TEXT("stufen"), StageLines) || StageLines->IsEmpty())
		{
			OutError = TEXT("Die Listen gruppen und stufen müssen da sein, und stufen darf nicht leer sein.");
			return false;
		}
		if (!FEIDataFile::ReadNumber(Root, TEXT("altar_welle_alle_s"), 0.1, 3600.0, Out.AltarWaveEverySeconds, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("fackel_welle_alle_s"), 0.1, 3600.0, Out.FlareWaveEverySeconds, OutError))
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
		for (const TSharedPtr<FJsonValue>& Line : *StageLines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEIGroupStage Stage;
			if (!Line->TryGetObject(Object) || !ReadStage(**Object, Stage, OutError))
			{
				return false;
			}
			Out.Stages.Add(Stage);
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

const FEIEnemyGroup* FEIEnemyGroups::Pick(const FString& Size, int32 Stage) const
{
	TArray<const FEIEnemyGroup*> Fitting;
	for (const FEIEnemyGroup& Group : Groups)
	{
		if (Group.Size == Size && Group.FromStage <= Stage && Stage <= Group.ToStage)
		{
			Fitting.Add(&Group);
		}
	}
	return Fitting.IsEmpty() ? nullptr : Fitting[FMath::RandRange(0, Fitting.Num() - 1)];
}

const FEIGroupStage& FEIEnemyGroups::GetStage(int32 Stage) const
{
	return Stages.IsEmpty() ? NoStage : Stages[FMath::Clamp(Stage, 0, Stages.Num() - 1)];
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
