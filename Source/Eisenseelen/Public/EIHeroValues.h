#pragma once

#include "CoreMinimal.h"

/** The hero's movement values from Data/held.json. The defaults here only apply when that file is broken. */
struct EISENSEELEN_API FEIHeroValues
{
	float RunSpeedMetersPerSecond = 5.5f;
	float JumpHeightMeters = 1.0f;
	float GravityFactor = 1.6f;
	float AirControl = 0.55f;
	float MouseFactor = 1.0f;
	float DashSpeedBonusMetersPerSecond = 6.0f;
	float DashHopMetersPerSecond = 3.0f;
	float DashCooldownSeconds = 1.0f;
	float SpeedChainWindowSeconds = 0.12f;
	float MaxSpeedMetersPerSecond = 22.0f;

	/** OutValues only changes when the whole file is valid. */
	static bool LoadFromString(const FString& Json, FEIHeroValues& OutValues, FString& OutError);
	static bool LoadFromProjectFile(FEIHeroValues& OutValues, FString& OutError);
};
