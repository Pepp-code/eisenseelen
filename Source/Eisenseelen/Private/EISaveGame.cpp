#include "EISaveGame.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreMiscDefines.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	const TCHAR* const ClassKeys[2] = { TEXT("klingenwacht"), TEXT("arkanist") };

	TSharedRef<FJsonObject> WriteClass(const FEISavedClass& Saved)
	{
		const TSharedRef<FJsonObject> Slots = MakeShared<FJsonObject>();
		for (int32 Index = 0; Index < EISlotCount; ++Index)
		{
			if (!Saved.Slots[Index].IsEmpty())
			{
				Slots->SetStringField(EISlotKey(static_cast<EEISlot>(Index)), Saved.Slots[Index]);
			}
		}
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetObjectField(TEXT("plaetze"), Slots);
		return Object;
	}

	void ReadClass(const FJsonObject& Parent, const TCHAR* Key, FEISavedClass& Out)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		const TSharedPtr<FJsonObject>* Slots = nullptr;
		if (!Parent.TryGetObjectField(Key, Object) || !(*Object)->TryGetObjectField(TEXT("plaetze"), Slots))
		{
			return;
		}
		for (int32 Index = 0; Index < EISlotCount; ++Index)
		{
			FString Name;
			Out.Slots[Index] = (*Slots)->TryGetStringField(EISlotKey(static_cast<EEISlot>(Index)), Name) ? Name : FString();
		}
	}
}

FEISaveGame FEISaveGame::MakeNew(const FEIItemTable& Table)
{
	FEISaveGame Save;
	for (const FString& Name : Table.StartItems)
	{
		Save.Items.Add(Name, 0);
	}
	for (int32 ClassIndex = 0; ClassIndex < 2; ++ClassIndex)
	{
		for (int32 Slot = 0; Slot < EISlotCount; ++Slot)
		{
			const int32 Item = Table.GetStartItem(static_cast<EEISlot>(Slot), ClassIndex == 1);
			const FString Name = Table.Items.IsValidIndex(Item) ? Table.Items[Item].Name : FString();
			Save.Classes[ClassIndex].Slots[Slot] = Name;
			Save.ArenaClasses[ClassIndex].Slots[Slot] = Name;
		}
	}
	return Save;
}

FString FEISaveGame::ToJson() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), 1);

	const TSharedRef<FJsonObject> Owned = MakeShared<FJsonObject>();
	for (const TPair<FString, int32>& Pair : Items)
	{
		Owned->SetNumberField(Pair.Key, Pair.Value);
	}
	Root->SetObjectField(TEXT("items"), Owned);

	const TSharedRef<FJsonObject> Arena = MakeShared<FJsonObject>();
	Arena->SetNumberField(TEXT("qualitaet"), ArenaQuality);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Root->SetObjectField(ClassKeys[Index], WriteClass(Classes[Index]));
		Arena->SetObjectField(ClassKeys[Index], WriteClass(ArenaClasses[Index]));
	}
	Root->SetObjectField(TEXT("testarena"), Arena);

	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(Root, Writer);
	return Json;
}

bool FEISaveGame::FromJson(const FString& Json, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	const TSharedPtr<FJsonObject>* Owned = nullptr;
	if (!Root.IsValid())
	{
		return false;
	}
	if (!Root->TryGetObjectField(TEXT("items"), Owned))
	{
		OutError = TEXT("Die Liste items fehlt.");
		return false;
	}
	FEISaveGame Loaded = *this;
	Loaded.Items.Reset();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Owned)->Values)
	{
		double Quality = 0.0;
		Pair.Value->TryGetNumber(Quality);
		Loaded.Items.Add(Pair.Key, FMath::RoundToInt32(Quality));
	}
	const TSharedPtr<FJsonObject>* Arena = nullptr;
	const bool bHasArena = Root->TryGetObjectField(TEXT("testarena"), Arena);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		ReadClass(*Root, ClassKeys[Index], Loaded.Classes[Index]);
		if (bHasArena)
		{
			ReadClass(**Arena, ClassKeys[Index], Loaded.ArenaClasses[Index]);
		}
	}
	double Quality = 0.0;
	if (bHasArena && (*Arena)->TryGetNumberField(TEXT("qualitaet"), Quality))
	{
		Loaded.ArenaQuality = FMath::RoundToInt32(Quality);
	}
	*this = Loaded;
	return true;
}

bool FEISaveGame::UsesDisk()
{
	return !GIsAutomationTesting && !FParse::Param(FCommandLine::Get(), TEXT("KeinSpielstand"));
}

FString FEISaveGame::GetPath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Spielstand.json"));
}

bool FEISaveGame::SaveToFile(const FString& Path) const
{
	const FString Temporary = Path + TEXT(".neu");
	return FFileHelper::SaveStringToFile(ToJson(), *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
		&& IFileManager::Get().Move(*Path, *Temporary, true);
}
