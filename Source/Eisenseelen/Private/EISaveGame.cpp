#include "EISaveGame.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
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
		const TSharedRef<FJsonObject> Attributes = MakeShared<FJsonObject>();
		for (const TPair<FString, int32>& Pair : Saved.Attributes)
		{
			Attributes->SetNumberField(Pair.Key, Pair.Value);
		}
		Object->SetObjectField(TEXT("attribute"), Attributes);
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
		Out.Attributes.Reset();
		const TSharedPtr<FJsonObject>* Attributes = nullptr;
		if ((*Object)->TryGetObjectField(TEXT("attribute"), Attributes))
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Attributes)->Values)
			{
				double Points = 0.0;
				Pair.Value->TryGetNumber(Points);
				Out.Attributes.Add(Pair.Key, FMath::RoundToInt32(Points));
			}
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
	Root->SetNumberField(TEXT("erfahrung"), Experience);
	Root->SetNumberField(TEXT("gold"), Gold);
	const TSharedRef<FJsonObject> Points = MakeShared<FJsonObject>();
	for (const TPair<FString, int32>& Pair : FactionPoints)
	{
		Points->SetNumberField(Pair.Key, Pair.Value);
	}
	Root->SetObjectField(TEXT("fraktionspunkte"), Points);
	const TSharedRef<FJsonObject> Found = MakeShared<FJsonObject>();
	for (const TPair<FString, int32>& Pair : Ingredients)
	{
		Found->SetNumberField(Pair.Key, Pair.Value);
	}
	Root->SetObjectField(TEXT("zutaten"), Found);
	TArray<FString> Seen = FoundIngredients.Array();
	Seen.Sort();
	TArray<TSharedPtr<FJsonValue>> SeenValues;
	for (const FString& Name : Seen)
	{
		SeenValues.Add(MakeShared<FJsonValueString>(Name));
	}
	Root->SetArrayField(TEXT("gefundene_zutaten"), SeenValues);

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
	double Number = 0.0;
	if (Root->TryGetNumberField(TEXT("erfahrung"), Number))
	{
		Loaded.Experience = FMath::RoundToInt32(Number);
	}
	if (Root->TryGetNumberField(TEXT("gold"), Number))
	{
		Loaded.Gold = FMath::RoundToInt32(Number);
	}
	Loaded.FactionPoints.Reset();
	const TSharedPtr<FJsonObject>* Points = nullptr;
	if (Root->TryGetObjectField(TEXT("fraktionspunkte"), Points))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Points)->Values)
		{
			double Amount = 0.0;
			Pair.Value->TryGetNumber(Amount);
			Loaded.FactionPoints.Add(Pair.Key, FMath::RoundToInt32(Amount));
		}
	}
	Loaded.Items.Reset();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Owned)->Values)
	{
		double Quality = 0.0;
		Pair.Value->TryGetNumber(Quality);
		Loaded.Items.Add(Pair.Key, FMath::RoundToInt32(Quality));
	}
	Loaded.Ingredients.Reset();
	const TSharedPtr<FJsonObject>* Found = nullptr;
	if (Root->TryGetObjectField(TEXT("zutaten"), Found))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Found)->Values)
		{
			double Amount = 0.0;
			Pair.Value->TryGetNumber(Amount);
			Loaded.Ingredients.Add(Pair.Key, FMath::RoundToInt32(Amount));
		}
	}
	// Whatever is in the bag has been found, also in a file from before the loot book.
	Loaded.FoundIngredients.Reset();
	const TArray<TSharedPtr<FJsonValue>>* Seen = nullptr;
	if (Root->TryGetArrayField(TEXT("gefundene_zutaten"), Seen))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Seen)
		{
			FString Name;
			if (Value->TryGetString(Name) && !Name.IsEmpty())
			{
				Loaded.FoundIngredients.Add(Name);
			}
		}
	}
	for (const TPair<FString, int32>& Pair : Loaded.Ingredients)
	{
		Loaded.FoundIngredients.Add(Pair.Key);
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
