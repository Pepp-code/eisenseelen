#pragma once

#include "CoreMinimal.h"

/**
 * One line of Data/aussehen.json: which real model goes over the grey-box placeholder of one thing, and how it moves.
 * An empty model keeps the placeholder, so the game plays the same with or without art.
 */
struct EISENSEELEN_API FEILookEntry
{
	/** A full object path such as /Game/Kunst/Gegner/SK_Laeufer.SK_Laeufer. Empty keeps the placeholder. */
	FString Model;
	FVector Scale = FVector::OneVector;
	FVector OffsetCm = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;

	/** The class path of an animation blueprint, ending in _C. Empty plays the animations below one by one. */
	FString AnimBlueprint;

	/** What happens, such as "laufen" or "tod", to the object path of its animation. */
	TMap<FName, FString> Animations;

	/** Where a weapon or a piece of armour sits on a figure with a skeleton. */
	FName Socket = TEXT("waffe_r");

	/** Whether the glowing placeholder parts, such as crystals, fire or the edge of a blade, stay over the model. */
	bool bKeepGlowParts = true;

	bool HasModel() const { return !Model.IsEmpty(); }
	FTransform GetTransform() const { return FTransform(Rotation, OffsetCm, Scale); }
};

/**
 * All of Data/aussehen.json. Keys: held.<class> for the body others see, hand.<class> for the own hand, gegner.<kind>,
 * boss.<name>, item.<name>, form.<weapon form> and ort.<place>.
 */
struct EISENSEELEN_API FEILookTable
{
	TMap<FString, FEILookEntry> Entries;

	const FEILookEntry* Find(const FString& Key) const;

	/** The first of these keys whose entry names a model. Null if none does. */
	const FEILookEntry* FindFirstModel(const TArray<FString>& Keys, FString* OutKey = nullptr) const;

	/** Every moment an animation can belong to. */
	static const TArray<FName>& GetEvents();

	/** What plays instead when there is no animation for this moment. None if nothing does. */
	static FName GetFallback(FName Event);

	/** The fixed names after held. and hand., and after ort. */
	static const TArray<FString>& GetClassNames();
	static const TArray<FString>& GetPlaceNames();
	static const TArray<FString>& GetPartNames();

	/** Turns "SkeletalMesh'/Game/A/B.B'" or "/Game/A/B" into "/Game/A/B.B". Empty when it is no path in /Game or /Engine. */
	static FString NormalizeObjectPath(const FString& Path);

	/** The same for a blueprint, whose class ends in _C. */
	static FString NormalizeClassPath(const FString& Path);

	/** OutTable only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEILookTable& OutTable, FString& OutError);
	static bool LoadFromProjectFile(FEILookTable& OutTable, FString& OutError);

	/** The table of the project. It is read again whenever the file changes, and an error shows once and keeps all placeholders. */
	static const FEILookTable& Get();

	/** For tests: Get returns this table instead, until it is set back to null. */
	static void SetTestOverride(const FEILookTable* Table);
};
