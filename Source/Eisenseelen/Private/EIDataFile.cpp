#include "EIDataFile.h"

#include "Dom/JsonObject.h"
#include "Eisenseelen.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FEIDataFile::bDataLoaded = false;
TArray<FEIWorldMutatorDef> FEIDataFile::CachedMutators;
TArray<FEISoulCargoDef> FEIDataFile::CachedSoulCargo;
FEIIslandEventsDef FEIDataFile::CachedIslandEvents;

TSharedPtr<FJsonObject> FEIDataFile::Load(const FString& FileName, FString& OutError)
{
	FString Json;
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data"), FileName);
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		OutError = TEXT("Die Datei fehlt.");
		return nullptr;
	}
	return Parse(Json, OutError);
}

TSharedPtr<FJsonObject> FEIDataFile::Parse(const FString& Json, FString& OutError)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = FString::Printf(TEXT("Die Datei ist kein gueltiges JSON (Zeile %d)."), Reader->GetLineNumber());
		return nullptr;
	}
	return Root;
}

bool FEIDataFile::ReadNumber(const FJsonObject& Object, const TCHAR* Key, double Min, double Max, float& OutNumber, FString& OutError)
{
	double Number = 0.0;
	if (!Object.TryGetNumberField(Key, Number))
	{
		OutError = FString::Printf(TEXT("Der Wert %s fehlt oder ist keine Zahl."), Key);
		return false;
	}
	if (Number < Min || Number > Max)
	{
		OutError = FString::Printf(TEXT("Der Wert %s muss zwischen %g und %g liegen, ist aber %g."), Key, Min, Max, Number);
		return false;
	}
	OutNumber = static_cast<float>(Number);
	return true;
}

void FEIDataFile::ReportError(const FString& FileName, const FString& Error)
{
	const FString Message = FString::Printf(TEXT("Data/%s: %s Das Spiel nutzt die Standardwerte."), *FileName, *Error);
	UE_LOG(LogEisenseelen, Error, TEXT("%s"), *Message);
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 30.0f, FColor::Red, Message);
	}
}

void FEIDataFile::ReloadData()
{
	bDataLoaded = false;
	EnsureLoaded();
}

void FEIDataFile::EnsureLoaded()
{
	if (bDataLoaded)
	{
		return;
	}
	bDataLoaded = true;
	CachedMutators.Reset();
	CachedSoulCargo.Reset();
	CachedIslandEvents = FEIIslandEventsDef();

	FString Error;
	if (TSharedPtr<FJsonObject> WeltObj = Load(TEXT("welt.json"), Error))
	{
		const TArray<TSharedPtr<FJsonValue>>* MutatorsJson = nullptr;
		if (WeltObj->TryGetArrayField(TEXT("welt_mutatoren"), MutatorsJson) && MutatorsJson)
		{
			for (const TSharedPtr<FJsonValue>& Val : *MutatorsJson)
			{
				const TSharedPtr<FJsonObject> Obj = Val->AsObject();
				if (!Obj.IsValid()) continue;
				FEIWorldMutatorDef Def;
				Def.Id = FName(*Obj->GetStringField(TEXT("id")));
				Def.DisplayName = Obj->GetStringField(TEXT("anzeigename"));
				Def.Description = Obj->GetStringField(TEXT("beschreibung"));
				Obj->TryGetNumberField(TEXT("intervall_sekunden"), Def.IntervalSeconds);
				Obj->TryGetNumberField(TEXT("runenfeld_groesse_faktor"), Def.RuneFieldScaleFactor);
				Obj->TryGetNumberField(TEXT("licht_faktor"), Def.LightFactor);
				CachedMutators.Add(Def);
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* CargoJson = nullptr;
		if (WeltObj->TryGetArrayField(TEXT("seelenfracht_typen"), CargoJson) && CargoJson)
		{
			for (const TSharedPtr<FJsonValue>& Val : *CargoJson)
			{
				const TSharedPtr<FJsonObject> Obj = Val->AsObject();
				if (!Obj.IsValid()) continue;
				FEISoulCargoDef Def;
				Def.Id = FName(*Obj->GetStringField(TEXT("id")));
				Def.DisplayName = Obj->GetStringField(TEXT("anzeigename"));
				Obj->TryGetNumberField(TEXT("tempo_faktor_traeger"), Def.CarrierSpeedFactor);
				Obj->TryGetNumberField(TEXT("wurf_schaden"), Def.ThrowDamage);
				Obj->TryGetNumberField(TEXT("wurf_radius"), Def.ThrowRadius);
				Obj->TryGetNumberField(TEXT("kessel_ladung_bonus"), Def.KettleChargeBonus);
				int32 SoulVal = 0;
				if (Obj->TryGetNumberField(TEXT("beute_seelen_wert"), SoulVal))
				{
					Def.SoulValue = SoulVal;
				}
				FString SoundWurf, SoundExplosion;
				if (Obj->TryGetStringField(TEXT("sound_wurf"), SoundWurf))
				{
					Def.ThrowSoundId = FName(*SoundWurf);
				}
				if (Obj->TryGetStringField(TEXT("sound_explosion"), SoundExplosion))
				{
					Def.ExplosionSoundId = FName(*SoundExplosion);
				}
				CachedSoulCargo.Add(Def);
			}
		}
	}

	// Try loading island events from waldinsel.json or lauf.json
	TSharedPtr<FJsonObject> IslandObj = Load(TEXT("waldinsel.json"), Error);
	if (!IslandObj.IsValid())
	{
		IslandObj = Load(TEXT("lauf.json"), Error);
	}
	if (IslandObj.IsValid())
	{
		const TSharedPtr<FJsonObject>* EventsObj = nullptr;
		if (IslandObj->TryGetObjectField(TEXT("insel_events"), EventsObj) && EventsObj && (*EventsObj).IsValid())
		{
			(*EventsObj)->TryGetNumberField(TEXT("fluchtruhe_chance"), CachedIslandEvents.CursedChestChance);
			(*EventsObj)->TryGetNumberField(TEXT("fluchtruhe_dauer_sekunden"), CachedIslandEvents.CursedChestDurationSeconds);
			(*EventsObj)->TryGetNumberField(TEXT("fluchtruhe_radius"), CachedIslandEvents.CursedChestRadius);
			(*EventsObj)->TryGetNumberField(TEXT("kessel_ueberladung_zeitfenster"), CachedIslandEvents.KettleOverchargeWindowSeconds);
			(*EventsObj)->TryGetNumberField(TEXT("kessel_ueberladung_beute_faktor"), CachedIslandEvents.KettleOverchargeRewardMultiplier);
		}
	}
}

const FEIWorldMutatorDef* FEIDataFile::FindWorldMutator(FName MutatorId)
{
	EnsureLoaded();
	for (const FEIWorldMutatorDef& Def : CachedMutators)
	{
		if (Def.Id == MutatorId)
		{
			return &Def;
		}
	}
	return nullptr;
}

const FEISoulCargoDef* FEIDataFile::FindSoulCargoDef(FName CargoId)
{
	EnsureLoaded();
	for (const FEISoulCargoDef& Def : CachedSoulCargo)
	{
		if (Def.Id == CargoId)
		{
			return &Def;
		}
	}
	return nullptr;
}

const FEIIslandEventsDef& FEIDataFile::GetIslandEventsDef()
{
	EnsureLoaded();
	return CachedIslandEvents;
}

const TArray<FEIWorldMutatorDef>& FEIDataFile::GetWorldMutators()
{
	EnsureLoaded();
	return CachedMutators;
}
