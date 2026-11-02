#include "EIDataFile.h"

#include "Dom/JsonObject.h"
#include "Eisenseelen.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
