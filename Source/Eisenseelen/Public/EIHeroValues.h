#pragma once

#include "CoreMinimal.h"

/** The hero's movement values from Data/held.json. The defaults here only apply when that file is broken. */
struct EISENSEELEN_API FEIHeroValues
{
	float RunSpeedMetersPerSecond = 5.5f;
	float SprintSpeedMetersPerSecond = 8.0f;
	float BlockSpeedMetersPerSecond = 3.3f;
	float JumpHeightMeters = 1.0f;
	float GravityFactor = 1.6f;
	float AirControl = 0.55f;
	float AirTurnDegreesPerSecond = 200.0f;
	float MouseFactor = 1.0f;
	float LifeBackAfterSeconds = 6.0f;
	float LifeBackSharePerSecond = 0.1f;
	float JumpBufferSeconds = 0.12f;

	/** OutValues only changes when the whole file is valid. */
	static bool LoadFromString(const FString& Json, FEIHeroValues& OutValues, FString& OutError);
	static bool LoadFromProjectFile(FEIHeroValues& OutValues, FString& OutError);
};
