#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/**
 * The tuning files in Data/. They are read when play starts, so a changed value only needs another press of Play.
 * Errors are in German, because the developer reads them on screen after editing a file by hand.
 */
struct EISENSEELEN_API FEIDataFile
{
	static TSharedPtr<FJsonObject> Load(const FString& FileName, FString& OutError);
	static TSharedPtr<FJsonObject> Parse(const FString& Json, FString& OutError);
	static bool ReadNumber(const FJsonObject& Object, const TCHAR* Key, double Min, double Max, float& OutNumber, FString& OutError);

	/** Editing a file by hand must not stop the game: says what is wrong, and the game plays on with default values. */
	static void ReportError(const FString& FileName, const FString& Error);
};
