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

	/** Items that got a new name. A file from before still holds the old one. */
	FString RenamedItem(const FString& Name)
	{
		if (Name == TEXT("Sturmklinge"))
		{
			return TEXT("Donnerklinge");
		}
		if (Name == TEXT("Sturmrune"))
		{
			return TEXT("Donnerrune");
		}
		return Name;
	}

	TArray<TSharedPtr<FJsonValue>> WriteNames(const TSet<FString>& Names)
	{
		TArray<FString> Sorted = Names.Array();
		Sorted.Sort();
		TArray<TSharedPtr<FJsonValue>> Values;
		for (const FString& Name : Sorted)
		{
			Values.Add(MakeShared<FJsonValueString>(Name));
		}
		return Values;
	}

	TSet<FString> ReadNames(const FJsonObject& Parent, const TCHAR* Key)
	{
		TSet<FString> Names;
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (Parent.TryGetArrayField(Key, Values))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				FString Name;
				if (Value->TryGetString(Name) && !Name.IsEmpty())
				{
					Names.Add(Name);
				}
			}
		}
		return Names;
	}

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
			Out.Slots[Index] = (*Slots)->TryGetStringField(EISlotKey(static_cast<EEISlot>(Index)), Name) ? RenamedItem(Name) : FString();
		}
	}
}

FEISaveGame FEISaveGame::MakeNew(const FEIItemTable& Table)
{
	FEISaveGame Save;
	for (const FString& Name : Table.StartItems)
	{
		Save.Items.Add(Name);
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
	Root->SetNumberField(TEXT("version"), 3);
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

	TArray<FString> OwnedNames = Items.Array();
	OwnedNames.Sort();
	TArray<TSharedPtr<FJsonValue>> Owned;
	for (const FString& Name : OwnedNames)
	{
		Owned.Add(MakeShared<FJsonValueString>(Name));
	}
	Root->SetArrayField(TEXT("items"), Owned);

	const TSharedRef<FJsonObject> Arena = MakeShared<FJsonObject>();
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Root->SetObjectField(ClassKeys[Index], WriteClass(Classes[Index]));
		Arena->SetObjectField(ClassKeys[Index], WriteClass(ArenaClasses[Index]));
	}
	Root->SetObjectField(TEXT("testarena"), Arena);

	const TSharedRef<FJsonObject> Engraved = MakeShared<FJsonObject>();
	for (const TPair<FString, FEISavedEngravings>& Pair : Engravings)
	{
		const TSharedRef<FJsonObject> Weapon = MakeShared<FJsonObject>();
		Weapon->SetArrayField(TEXT("gekauft"), WriteNames(Pair.Value.Owned));
		Weapon->SetArrayField(TEXT("aktiv"), WriteNames(Pair.Value.Active));
		Weapon->SetArrayField(TEXT("aktiv_testarena"), WriteNames(Pair.Value.ArenaActive));
		Engraved->SetObjectField(Pair.Key, Weapon);
	}
	Root->SetObjectField(TEXT("gravuren"), Engraved);

	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(Root, Writer);
	return Json;
}

bool FEISaveGame::FromJson(const FString& Json, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	if (!Root.IsValid())
	{
		return false;
	}
	// Since version 2 the items are a list of names. Before, every name had a quality, and owning was enough.
	TSet<FString> OwnedNames;
	const TArray<TSharedPtr<FJsonValue>>* OwnedList = nullptr;
	const TSharedPtr<FJsonObject>* OwnedObject = nullptr;
	if (Root->TryGetArrayField(TEXT("items"), OwnedList))
	{
		for (const TSharedPtr<FJsonValue>& Value : *OwnedList)
		{
			FString Name;
			if (Value->TryGetString(Name) && !Name.IsEmpty())
			{
				OwnedNames.Add(RenamedItem(Name));
			}
		}
	}
	else if (Root->TryGetObjectField(TEXT("items"), OwnedObject))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*OwnedObject)->Values)
		{
			OwnedNames.Add(RenamedItem(Pair.Key));
		}
	}
	else
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
	Loaded.Items = MoveTemp(OwnedNames);
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
	// Since version 3 the weapons have engravings. A file from before has none.
	Loaded.Engravings.Reset();
	const TSharedPtr<FJsonObject>* Engraved = nullptr;
	if (Root->TryGetObjectField(TEXT("gravuren"), Engraved))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Engraved)->Values)
		{
			const TSharedPtr<FJsonObject>* Weapon = nullptr;
			if (Pair.Value->TryGetObject(Weapon))
			{
				FEISavedEngravings& Saved = Loaded.Engravings.Add(RenamedItem(Pair.Key));
				Saved.Owned = ReadNames(**Weapon, TEXT("gekauft"));
				Saved.Active = ReadNames(**Weapon, TEXT("aktiv")).Intersect(Saved.Owned);
				Saved.ArenaActive = ReadNames(**Weapon, TEXT("aktiv_testarena"));
			}
		}
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
