#include "EILooks.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIEffects.h"
#include "EIEnemy.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* LookFileName = TEXT("aussehen.json");
	constexpr double MaxScale = 1000.0;
	constexpr double MinScale = 0.001;
	constexpr double MaxOffsetCm = 100000.0;
	constexpr double FullTurn = 360.0;

	const FEILookTable* TestOverride = nullptr;

	bool ReadThree(const FJsonObject& Object, const FString& Field, double Min, double Max, FVector& OutVector, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
		if (!Object.TryGetArrayField(Field, Array) || Array->Num() != 3)
		{
			OutError = FString::Printf(TEXT("%s braucht drei Zahlen, zum Beispiel [0, 0, 0]."), *Field);
			return false;
		}
		double Numbers[3] = {0.0, 0.0, 0.0};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (!(*Array)[Index]->TryGetNumber(Numbers[Index]) || Numbers[Index] < Min || Numbers[Index] > Max)
			{
				OutError = FString::Printf(TEXT("%s braucht drei Zahlen zwischen %g und %g."), *Field, Min, Max);
				return false;
			}
		}
		OutVector = FVector(Numbers[0], Numbers[1], Numbers[2]);
		return true;
	}

	bool ReadPath(const FJsonObject& Object, const FString& Field, bool bClass, FString& OutPath, FString& OutError)
	{
		FString Text;
		if (!Object.TryGetStringField(Field, Text))
		{
			OutError = FString::Printf(TEXT("%s muss ein Text in Anfuehrungszeichen sein."), *Field);
			return false;
		}
		if (Text.TrimStartAndEnd().IsEmpty())
		{
			OutPath.Reset();
			return true;
		}
		OutPath = bClass ? FEILookTable::NormalizeClassPath(Text) : FEILookTable::NormalizeObjectPath(Text);
		if (OutPath.IsEmpty())
		{
			OutError = FString::Printf(TEXT("%s \"%s\" ist kein Pfad im Projekt. Er beginnt mit /Game/, zum Beispiel /Game/Kunst/Gegner/SK_Laeufer."), *Field, *Text);
			return false;
		}
		return true;
	}

	bool IsKnownKey(const FString& Key)
	{
		FString Prefix;
		FString Name;
		if (!Key.Split(TEXT("."), &Prefix, &Name) || Name.IsEmpty())
		{
			return false;
		}
		if (Prefix == TEXT("held") || Prefix == TEXT("hand"))
		{
			return FEILookTable::GetClassNames().Contains(Name);
		}
		if (Prefix == TEXT("ort"))
		{
			return FEILookTable::GetPlaceNames().Contains(Name);
		}
		if (Prefix == TEXT("effekt"))
		{
			return AEIEffect::GetNames().Contains(FName(*Name));
		}
		if (Prefix == TEXT("gegner"))
		{
			for (int32 Index = 0; Index < EIEnemyKindCount; ++Index)
			{
				if (Name == EIEnemyKindKey(static_cast<EEIEnemyKind>(Index)))
				{
					return true;
				}
			}
			return false;
		}
		// Bosses, items and weapon forms are named in their own files, and the tests compare the names.
		return Prefix == TEXT("boss") || Prefix == TEXT("item") || Prefix == TEXT("form");
	}

	bool ReadEntry(const FJsonObject& Object, FEILookEntry& OutEntry, FString& OutError)
	{
		static const TSet<FString> Fields = {
			TEXT("modell"), TEXT("skalierung"), TEXT("versatz_cm"), TEXT("drehung_grad"), TEXT("anim_blueprint"),
			TEXT("animationen"), TEXT("sockel"), TEXT("leuchtteile_behalten")};
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Object.Values)
		{
			if (!Field.Key.StartsWith(TEXT("_")) && !Fields.Contains(Field.Key))
			{
				OutError = FString::Printf(TEXT("Den Wert %s gibt es nicht. Erlaubt sind modell, skalierung, versatz_cm, drehung_grad, anim_blueprint, animationen, sockel und leuchtteile_behalten."), *Field.Key);
				return false;
			}
		}
		if (Object.HasField(TEXT("modell")) && !ReadPath(Object, TEXT("modell"), false, OutEntry.Model, OutError))
		{
			return false;
		}
		if (Object.HasField(TEXT("skalierung")))
		{
			double Uniform = 0.0;
			if (Object.TryGetNumberField(TEXT("skalierung"), Uniform))
			{
				if (Uniform < MinScale || Uniform > MaxScale)
				{
					OutError = FString::Printf(TEXT("skalierung muss zwischen %g und %g liegen."), MinScale, MaxScale);
					return false;
				}
				OutEntry.Scale = FVector(Uniform);
			}
			else if (!ReadThree(Object, TEXT("skalierung"), MinScale, MaxScale, OutEntry.Scale, OutError))
			{
				OutError += TEXT(" Oder eine einzelne Zahl fuer alle Richtungen.");
				return false;
			}
		}
		if (Object.HasField(TEXT("versatz_cm")) && !ReadThree(Object, TEXT("versatz_cm"), -MaxOffsetCm, MaxOffsetCm, OutEntry.OffsetCm, OutError))
		{
			return false;
		}
		if (Object.HasField(TEXT("drehung_grad")))
		{
			// In the order of the editor's Details panel: X, Y, Z.
			FVector Angles = FVector::ZeroVector;
			if (!ReadThree(Object, TEXT("drehung_grad"), -FullTurn, FullTurn, Angles, OutError))
			{
				return false;
			}
			OutEntry.Rotation = FRotator(Angles.Y, Angles.Z, Angles.X);
		}
		if (Object.HasField(TEXT("anim_blueprint")) && !ReadPath(Object, TEXT("anim_blueprint"), true, OutEntry.AnimBlueprint, OutError))
		{
			return false;
		}
		if (Object.HasField(TEXT("animationen")))
		{
			const TSharedPtr<FJsonObject>* Animations = nullptr;
			if (!Object.TryGetObjectField(TEXT("animationen"), Animations))
			{
				OutError = TEXT("animationen muss eine Liste in geschweiften Klammern sein, zum Beispiel { \"laufen\": \"/Game/Kunst/A_Laufen\" }.");
				return false;
			}
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Animation : (*Animations)->Values)
			{
				if (Animation.Key.StartsWith(TEXT("_")))
				{
					continue;
				}
				const FName Event(*Animation.Key);
				if (!FEILookTable::GetEvents().Contains(Event))
				{
					FString Known;
					for (const FName& Name : FEILookTable::GetEvents())
					{
						Known += (Known.IsEmpty() ? TEXT("") : TEXT(", ")) + Name.ToString();
					}
					OutError = FString::Printf(TEXT("Die Animation %s gibt es nicht. Moeglich sind: %s."), *Animation.Key, *Known);
					return false;
				}
				FString Path;
				if (!ReadPath(**Animations, Animation.Key, false, Path, OutError))
				{
					return false;
				}
				if (!Path.IsEmpty())
				{
					OutEntry.Animations.Add(Event, Path);
				}
			}
		}
		if (Object.HasField(TEXT("sockel")))
		{
			FString Socket;
			if (!Object.TryGetStringField(TEXT("sockel"), Socket) || Socket.TrimStartAndEnd().IsEmpty())
			{
				OutError = TEXT("sockel muss der Name eines Sockels am Skelett sein, zum Beispiel \"waffe_r\".");
				return false;
			}
			OutEntry.Socket = FName(*Socket.TrimStartAndEnd());
		}
		if (Object.HasField(TEXT("leuchtteile_behalten")))
		{
			bool bKeep = true;
			double Number = 0.0;
			if (Object.TryGetBoolField(TEXT("leuchtteile_behalten"), bKeep))
			{
				OutEntry.bKeepGlowParts = bKeep;
			}
			else if (Object.TryGetNumberField(TEXT("leuchtteile_behalten"), Number) && (Number == 0.0 || Number == 1.0))
			{
				OutEntry.bKeepGlowParts = Number == 1.0;
			}
			else
			{
				OutError = TEXT("leuchtteile_behalten muss true oder false sein.");
				return false;
			}
		}
		return true;
	}

	bool ReadTable(const FJsonObject& Root, FEILookTable& OutTable, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Looks = nullptr;
		if (!Root.TryGetObjectField(TEXT("aussehen"), Looks))
		{
			OutError = TEXT("Der Abschnitt aussehen fehlt.");
			return false;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : (*Looks)->Values)
		{
			if (Entry.Key.StartsWith(TEXT("_")))
			{
				continue;
			}
			if (!IsKnownKey(Entry.Key))
			{
				OutError = FString::Printf(TEXT("%s ist kein bekannter Name. Er beginnt mit held., hand., gegner., boss., item., form., ort. oder effekt. und nennt etwas, das es im Spiel gibt."), *Entry.Key);
				return false;
			}
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Entry.Value->TryGetObject(Object))
			{
				OutError = FString::Printf(TEXT("%s muss in geschweiften Klammern stehen."), *Entry.Key);
				return false;
			}
			FEILookEntry Look;
			if (!ReadEntry(**Object, Look, OutError))
			{
				OutError = FString::Printf(TEXT("%s: %s"), *Entry.Key, *OutError);
				return false;
			}
			OutTable.Entries.Add(Entry.Key, MoveTemp(Look));
		}
		return true;
	}
}

const FEILookEntry* FEILookTable::Find(const FString& Key) const
{
	return Entries.Find(Key);
}

const FEILookEntry* FEILookTable::FindFirstModel(const TArray<FString>& Keys, FString* OutKey) const
{
	for (const FString& Key : Keys)
	{
		const FEILookEntry* Entry = Entries.Find(Key);
		if (Entry != nullptr && Entry->HasModel())
		{
			if (OutKey != nullptr)
			{
				*OutKey = Key;
			}
			return Entry;
		}
	}
	return nullptr;
}

const TArray<FName>& FEILookTable::GetEvents()
{
	static const TArray<FName> Events = {
		// What runs on and on, chosen by how fast it moves.
		TEXT("stehen"), TEXT("laufen"), TEXT("rennen"), TEXT("springen"),
		// Enemies and bosses.
		TEXT("ausholen"), TEXT("schlag"), TEXT("wurf"), TEXT("ansturm"), TEXT("stampfen"), TEXT("beschwoeren"), TEXT("verschwinden"),
		TEXT("treffer"), TEXT("taumeln"), TEXT("bemerkt"), TEXT("tod"),
		// Heroes.
		TEXT("hieb"), TEXT("schwerer_hieb"), TEXT("blocken"), TEXT("schuss"), TEXT("zauber"), TEXT("rune"),
		// Places: the chest opens, the fire burns, the grave breaks open.
		TEXT("aktiv")};
	return Events;
}

FName FEILookTable::GetFallback(FName Event)
{
	static const TMap<FName, FName> Fallbacks = {
		{TEXT("rennen"), TEXT("laufen")},
		{TEXT("laufen"), TEXT("stehen")},
		{TEXT("schwerer_hieb"), TEXT("hieb")},
		{TEXT("wurf"), TEXT("schlag")},
		{TEXT("stampfen"), TEXT("schlag")},
		{TEXT("beschwoeren"), TEXT("ausholen")},
		{TEXT("verschwinden"), TEXT("ausholen")}};
	const FName* Found = Fallbacks.Find(Event);
	return Found != nullptr ? *Found : NAME_None;
}

const TArray<FString>& FEILookTable::GetClassNames()
{
	static const TArray<FString> Names = {TEXT("klingenwacht"), TEXT("arkanist")};
	return Names;
}

const TArray<FString>& FEILookTable::GetPlaceNames()
{
	static const TArray<FString> Names = {
		TEXT("schmiede"), TEXT("jagdbrett"), TEXT("altar"), TEXT("ankerstein"), TEXT("truhe"), TEXT("signalfeuer"),
		TEXT("seelenkessel"), TEXT("grab"), TEXT("trainingspuppe"), TEXT("barke"), TEXT("sprungfeld"), TEXT("lager")};
	return Names;
}

FString FEILookTable::NormalizeObjectPath(const FString& Path)
{
	FString Result = Path.TrimStartAndEnd();
	// "Copy Reference" in the editor gives Class'/Game/A/B.B'.
	int32 Quote = INDEX_NONE;
	if (Result.FindChar(TEXT('\''), Quote))
	{
		Result = Result.Mid(Quote + 1);
		Result.RemoveFromEnd(TEXT("'"));
	}
	Result.TrimStartAndEndInline();
	if ((!Result.StartsWith(TEXT("/Game/")) && !Result.StartsWith(TEXT("/Engine/"))) || Result.Contains(TEXT(" ")) || Result.Contains(TEXT("\\")))
	{
		return FString();
	}
	const int32 LastSlash = Result.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	const int32 Dot = Result.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (Dot == INDEX_NONE || Dot < LastSlash)
	{
		const FString Object = Result.Mid(LastSlash + 1);
		return Object.IsEmpty() ? FString() : Result + TEXT(".") + Object;
	}
	// Neither the name before the dot nor the one after it may be empty.
	return Dot <= LastSlash + 1 || Dot == Result.Len() - 1 ? FString() : Result;
}

FString FEILookTable::NormalizeClassPath(const FString& Path)
{
	const FString Result = NormalizeObjectPath(Path);
	return Result.IsEmpty() || Result.EndsWith(TEXT("_C")) ? Result : Result + TEXT("_C");
}

bool FEILookTable::LoadFromString(const FString& Json, FEILookTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEILookTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = MoveTemp(Loaded);
	return true;
}

bool FEILookTable::LoadFromProjectFile(FEILookTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(LookFileName, OutError);
	FEILookTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = MoveTemp(Loaded);
	return true;
}

const FEILookTable& FEILookTable::Get()
{
	if (TestOverride != nullptr)
	{
		return *TestOverride;
	}
	static FEILookTable Cached;
	static FDateTime CachedStamp;
	static bool bRead = false;
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data"), LookFileName);
	const FDateTime Stamp = IFileManager::Get().GetTimeStamp(*Path);
	if (bRead && Stamp == CachedStamp)
	{
		return Cached;
	}
	bRead = true;
	CachedStamp = Stamp;
	FString Error;
	if (!LoadFromProjectFile(Cached, Error))
	{
		Cached = FEILookTable();
		FEIDataFile::ReportError(LookFileName, Error);
	}
	return Cached;
}

void FEILookTable::SetTestOverride(const FEILookTable* Table)
{
	TestOverride = Table;
}
