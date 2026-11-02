#pragma once

#include "CoreMinimal.h"

/**
 * The hero's tuning values from Data/held.json. They are read every time play starts, so a changed value only needs
 * another press of Play. The defaults here only apply when that file is broken.
 */
struct EISENSEELEN_API FEIHeroValues
{
	float RunSpeedMetersPerSecond = 5.5f;
	float JumpHeightMeters = 2.0f;
	float AirControl = 0.55f;
	float MouseFactor = 1.0f;

	static FString GetProjectFilePath();

	/** On failure OutError says in German what to fix, because the developer reads it on screen. */
	static bool LoadFromString(const FString& Json, FEIHeroValues& OutValues, FString& OutError);
	static bool LoadFromProjectFile(FEIHeroValues& OutValues, FString& OutError);
};
