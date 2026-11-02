#include "EIHeroValues.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FString FEIHeroValues::GetProjectFilePath()
{
	return FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/held.json"));
}

bool FEIHeroValues::LoadFromString(const FString& Json, FEIHeroValues& OutValues, FString& OutError)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = FString::Printf(TEXT("Die Datei ist kein gueltiges JSON (Zeile %d)."), Reader->GetLineNumber());
		return false;
	}

	const auto ReadNumber = [&Root, &OutError](const TCHAR* Key, double Min, double Max, float& OutNumber)
	{
		double Number = 0.0;
		if (!Root->TryGetNumberField(Key, Number))
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
	};

	FEIHeroValues Loaded;
	if (!ReadNumber(TEXT("lauftempo_m_pro_s"), 1.0, 30.0, Loaded.RunSpeedMetersPerSecond)
		|| !ReadNumber(TEXT("sprunghoehe_m"), 0.2, 10.0, Loaded.JumpHeightMeters)
		|| !ReadNumber(TEXT("luftkontrolle_0_bis_1"), 0.0, 1.0, Loaded.AirControl)
		|| !ReadNumber(TEXT("maus_faktor"), 0.1, 5.0, Loaded.MouseFactor))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}

bool FEIHeroValues::LoadFromProjectFile(FEIHeroValues& OutValues, FString& OutError)
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *GetProjectFilePath()))
	{
		OutError = FString::Printf(TEXT("Die Datei %s fehlt."), *GetProjectFilePath());
		return false;
	}
	return LoadFromString(Json, OutValues, OutError);
}
