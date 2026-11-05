#pragma once

#include "CoreMinimal.h"
#include "EIItems.h"

/** What one class has put on, by the names of the items, so that a save survives new lines in the data files. */
struct EISENSEELEN_API FEISavedClass
{
	/** One entry per slot. Empty text is an empty slot. */
	TArray<FString> Slots;

	FEISavedClass() { Slots.SetNum(EISlotCount); }
};

/**
 * What stays between two runs, as a file of the player. Every player has their own on their own machine. The test arena
 * has a part of its own, where everything is free, so that trying things out there changes nothing in a real run.
 */
struct EISENSEELEN_API FEISaveGame
{
	int32 Experience = 0;
	int32 Gold = 0;

	/** Name of a faction and the points earned with it. */
	TMap<FString, int32> FactionPoints;

	/** Name of an ingredient and how many there are. */
	TMap<FString, int32> Ingredients;

	/** Every ingredient a boss has dropped for this player, even in a run that failed. Only these have a name in the loot book. */
	TSet<FString> FoundIngredients;

	/** The names of all items this player owns. */
	TSet<FString> Items;

	/** By class: first the blade warden, then the arcanist. */
	FEISavedClass Classes[2];
	FEISavedClass ArenaClasses[2];

	/** What a new player begins with. */
	static FEISaveGame MakeNew(const FEIItemTable& Table);

	FString ToJson() const;

	/** Unchanged if the text is not valid. */
	bool FromJson(const FString& Json, FString& OutError);

	/** Tests and preview pictures keep their hands off the player's file. */
	static bool UsesDisk();
	static FString GetPath();

	/** The new file only takes the place of the old one when it is complete. */
	bool SaveToFile(const FString& Path) const;
};
