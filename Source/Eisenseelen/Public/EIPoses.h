#pragma once

#include "CoreMinimal.h"
#include "Math/InterpCurve.h"

/** One movement of the hand in the first-person view, from Data/posen.json. T runs from 0 to 1. */
struct EISENSEELEN_API FEIPoseCurve
{
	FInterpCurveVector Position;
	FInterpCurveVector Rotation;

	// The direction the blade sweeps during the hit window, in degrees around the view direction.
	float ArcYawFrom = 0.0f;
	float ArcYawTo = 0.0f;
	float ArcPitchFrom = 0.0f;
	float ArcPitchTo = 0.0f;

	FTransform Evaluate(float T) const;
};

/** How the hand and the view move with the hero, on top of the poses, from Data/animation.json. */
struct EISENSEELEN_API FEIHandMotion
{
	float BreathCm = 0.35f;
	float BreathsPerSecond = 0.3f;
	float BobUpCm = 1.3f;
	float BobSideCm = 1.1f;
	float StrideMeters = 1.7f;
	float SprintForwardCm = -4.0f;
	float SprintRightCm = 5.0f;
	float SprintUpCm = -9.0f;
	float SprintPitchDegrees = -22.0f;
	float SprintYawDegrees = 8.0f;
	float SprintRollDegrees = 12.0f;
	float AirUpCm = 3.0f;
	float LandingDipCm = 6.0f;
	float LookLag = 0.012f;
	float LookLagMaxDegrees = 4.0f;
	float FieldOfViewDegrees = 90.0f;
	float SprintFieldOfViewPlusDegrees = 6.0f;

	static bool LoadFromProjectFile(FEIHandMotion& OutMotion, FString& OutError);
};

/** All poses by name. The hand only moves along these curves, so no animation tool is involved. */
struct EISENSEELEN_API FEIPoseLibrary
{
	TMap<FName, FEIPoseCurve> Curves;

	const FEIPoseCurve* Find(FName Name) const { return Curves.Find(Name); }

	static bool LoadFromString(const FString& Json, FEIPoseLibrary& OutLibrary, FString& OutError);
	static bool LoadFromProjectFile(FEIPoseLibrary& OutLibrary, FString& OutError);
};
