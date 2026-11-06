#include "EISettings.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EISaveGame.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	bool ReadSwitch(const FJsonObject& Object, const TCHAR* Key, bool& OutValue)
	{
		double Number = 0.0;
		if (Object.TryGetNumberField(Key, Number))
		{
			OutValue = Number >= 0.5;
		}
		return true;
	}
}

bool FEISettings::LoadFromProjectFile(FEISettings& OutSettings, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("einstellungen.json"), OutError);
	FEISettings Loaded;
	if (!Root.IsValid()
		|| !FEIDataFile::ReadNumber(*Root, TEXT("maus_empfindlichkeit"), MouseFactorMin, MouseFactorMax, Loaded.MouseFactor, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("lautstaerke_effekte"), VolumeMin, VolumeMax, Loaded.EffectsVolume, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("lautstaerke_musik"), VolumeMin, VolumeMax, Loaded.MusicVolume, OutError))
	{
		return false;
	}
	ReadSwitch(*Root, TEXT("blick_invertieren_0_oder_1"), Loaded.bInvertLook);
	ReadSwitch(*Root, TEXT("hud_an_0_oder_1"), Loaded.bHudOn);
	ReadSwitch(*Root, TEXT("schadenszahlen_an_0_oder_1"), Loaded.bDamageNumbers);
	OutSettings = Loaded;
	return true;
}

FString FEISettings::ToJson() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), 1);
	Root->SetNumberField(TEXT("maus_empfindlichkeit"), MouseFactor);
	Root->SetNumberField(TEXT("blick_invertieren"), bInvertLook ? 1 : 0);
	Root->SetNumberField(TEXT("hud_an"), bHudOn ? 1 : 0);
	Root->SetNumberField(TEXT("schadenszahlen_an"), bDamageNumbers ? 1 : 0);
	Root->SetNumberField(TEXT("lautstaerke_effekte"), EffectsVolume);
	Root->SetNumberField(TEXT("lautstaerke_musik"), MusicVolume);
	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(Root, Writer);
	return Json;
}

bool FEISettings::FromJson(const FString& Json, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	if (!Root.IsValid())
	{
		return false;
	}
	FEISettings Loaded = *this;
	// What the file does not say stays at the default, so a missing knob is no error.
	FEIDataFile::ReadNumber(*Root, TEXT("maus_empfindlichkeit"), MouseFactorMin, MouseFactorMax, Loaded.MouseFactor, OutError);
	FEIDataFile::ReadNumber(*Root, TEXT("lautstaerke_effekte"), VolumeMin, VolumeMax, Loaded.EffectsVolume, OutError);
	FEIDataFile::ReadNumber(*Root, TEXT("lautstaerke_musik"), VolumeMin, VolumeMax, Loaded.MusicVolume, OutError);
	ReadSwitch(*Root, TEXT("blick_invertieren"), Loaded.bInvertLook);
	ReadSwitch(*Root, TEXT("hud_an"), Loaded.bHudOn);
	ReadSwitch(*Root, TEXT("schadenszahlen_an"), Loaded.bDamageNumbers);
	OutError.Reset();
	*this = Loaded;
	return true;
}

bool FEISettings::UsesDisk()
{
	return FEISaveGame::UsesDisk();
}

FString FEISettings::GetPath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Einstellungen.json"));
}

bool FEISettings::SaveToFile(const FString& Path) const
{
	const FString Temporary = Path + TEXT(".neu");
	return FFileHelper::SaveStringToFile(ToJson(), *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
		&& IFileManager::Get().Move(*Path, *Temporary, true);
}

void FEISettings::AddMouseFactor(int32 Steps)
{
	MouseFactor = FMath::Clamp(FMath::RoundToFloat((MouseFactor + MouseFactorStep * Steps) / MouseFactorStep) * MouseFactorStep, MouseFactorMin, MouseFactorMax);
}

void FEISettings::AddEffectsVolume(int32 Steps)
{
	EffectsVolume = FMath::Clamp(FMath::RoundToFloat((EffectsVolume + VolumeStep * Steps) / VolumeStep) * VolumeStep, VolumeMin, VolumeMax);
}

void FEISettings::AddMusicVolume(int32 Steps)
{
	MusicVolume = FMath::Clamp(FMath::RoundToFloat((MusicVolume + VolumeStep * Steps) / VolumeStep) * VolumeStep, VolumeMin, VolumeMax);
}
