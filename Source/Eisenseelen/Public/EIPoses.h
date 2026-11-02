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

/** All poses by name. The hand only moves along these curves, so no animation tool is involved. */
struct EISENSEELEN_API FEIPoseLibrary
{
	TMap<FName, FEIPoseCurve> Curves;

	const FEIPoseCurve* Find(FName Name) const { return Curves.Find(Name); }

	static bool LoadFromString(const FString& Json, FEIPoseLibrary& OutLibrary, FString& OutError);
	static bool LoadFromProjectFile(FEIPoseLibrary& OutLibrary, FString& OutError);
};
