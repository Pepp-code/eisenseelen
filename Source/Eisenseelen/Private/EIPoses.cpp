#include "EIPoses.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"

namespace
{
	bool ReadCurve(const FString& Name, const FJsonObject& Object, FEIPoseCurve& OutCurve, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
		if (!Object.TryGetArrayField(TEXT("schluessel"), Keys) || Keys->IsEmpty())
		{
			OutError = FString::Printf(TEXT("Die Pose %s hat keine Schluessel."), *Name);
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Keys)
		{
			const TSharedPtr<FJsonObject>* Key = nullptr;
			if (!Value->TryGetObject(Key))
			{
				OutError = FString::Printf(TEXT("Die Pose %s hat einen ungueltigen Schluessel."), *Name);
				return false;
			}
			float T = 0.0f;
			float Forward = 0.0f;
			float Right = 0.0f;
			float Up = 0.0f;
			float Pitch = 0.0f;
			float Yaw = 0.0f;
			float Roll = 0.0f;
			constexpr double Far = 500.0;
			constexpr double FullTurn = 360.0;
			if (!FEIDataFile::ReadNumber(**Key, TEXT("t"), 0.0, 1.0, T, OutError)
				|| !FEIDataFile::ReadNumber(**Key, TEXT("vor"), -Far, Far, Forward, OutError)
				|| !FEIDataFile::ReadNumber(**Key, TEXT("rechts"), -Far, Far, Right, OutError)
				|| !FEIDataFile::ReadNumber(**Key, TEXT("hoch"), -Far, Far, Up, OutError)
				|| !FEIDataFile::ReadNumber(**Key, TEXT("nick"), -FullTurn, FullTurn, Pitch, OutError)
				|| !FEIDataFile::ReadNumber(**Key, TEXT("gier"), -FullTurn, FullTurn, Yaw, OutError)
				|| !FEIDataFile::ReadNumber(**Key, TEXT("roll"), -FullTurn, FullTurn, Roll, OutError))
			{
				OutError = FString::Printf(TEXT("Pose %s: %s"), *Name, *OutError);
				return false;
			}
			OutCurve.Position.AddPoint(T, FVector(Forward, Right, Up));
			OutCurve.Rotation.AddPoint(T, FVector(Pitch, Yaw, Roll));
		}
		for (FInterpCurvePointVector& Point : OutCurve.Position.Points)
		{
			Point.InterpMode = CIM_CurveAuto;
		}
		for (FInterpCurvePointVector& Point : OutCurve.Rotation.Points)
		{
			Point.InterpMode = CIM_CurveAuto;
		}
		OutCurve.Position.AutoSetTangents();
		OutCurve.Rotation.AutoSetTangents();

		const TSharedPtr<FJsonObject>* Arc = nullptr;
		if (Object.TryGetObjectField(TEXT("bogen"), Arc))
		{
			constexpr double FullTurn = 360.0;
			return FEIDataFile::ReadNumber(**Arc, TEXT("gier_von"), -FullTurn, FullTurn, OutCurve.ArcYawFrom, OutError)
				&& FEIDataFile::ReadNumber(**Arc, TEXT("gier_bis"), -FullTurn, FullTurn, OutCurve.ArcYawTo, OutError)
				&& FEIDataFile::ReadNumber(**Arc, TEXT("nick_von"), -FullTurn, FullTurn, OutCurve.ArcPitchFrom, OutError)
				&& FEIDataFile::ReadNumber(**Arc, TEXT("nick_bis"), -FullTurn, FullTurn, OutCurve.ArcPitchTo, OutError);
		}
		return true;
	}

	bool ReadLibrary(const FJsonObject& Root, FEIPoseLibrary& OutLibrary, FString& OutError)
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Root.Values)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Entry.Value->TryGetObject(Object))
			{
				OutError = FString::Printf(TEXT("Die Pose %s ist kein Objekt."), *Entry.Key);
				return false;
			}
			FEIPoseCurve Curve;
			if (!ReadCurve(Entry.Key, **Object, Curve, OutError))
			{
				return false;
			}
			OutLibrary.Curves.Add(FName(*Entry.Key), MoveTemp(Curve));
		}
		return true;
	}
}

bool FEIHandMotion::LoadFromProjectFile(FEIHandMotion& OutMotion, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("animation.json"), OutError);
	FEIHandMotion Loaded;
	if (!Root.IsValid()
		|| !FEIDataFile::ReadNumber(*Root, TEXT("atmen_cm"), 0.0, 50.0, Loaded.BreathCm, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("atmen_pro_s"), 0.0, 10.0, Loaded.BreathsPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("wippen_hoch_cm"), 0.0, 50.0, Loaded.BobUpCm, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("wippen_seite_cm"), 0.0, 50.0, Loaded.BobSideCm, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("schrittlaenge_m"), 0.1, 20.0, Loaded.StrideMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("sprint_vor_cm"), -100.0, 100.0, Loaded.SprintForwardCm, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("sprint_rechts_cm"), -100.0, 100.0, Loaded.SprintRightCm, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("sprint_hoch_cm"), -100.0, 100.0, Loaded.SprintUpCm, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("sprint_nick_grad"), -180.0, 180.0, Loaded.SprintPitchDegrees, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("sprint_gier_grad"), -180.0, 180.0, Loaded.SprintYawDegrees, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("sprint_roll_grad"), -180.0, 180.0, Loaded.SprintRollDegrees, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("luft_hoch_cm"), -100.0, 100.0, Loaded.AirUpCm, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("landen_tief_cm"), 0.0, 100.0, Loaded.LandingDipCm, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("maus_nachziehen_grad_je_grad_pro_s"), 0.0, 1.0, Loaded.LookLag, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("maus_nachziehen_max_grad"), 0.0, 45.0, Loaded.LookLagMaxDegrees, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("sichtfeld_grad"), 30.0, 150.0, Loaded.FieldOfViewDegrees, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("sichtfeld_sprint_plus_grad"), 0.0, 60.0, Loaded.SprintFieldOfViewPlusDegrees, OutError))
	{
		return false;
	}
	OutMotion = Loaded;
	return true;
}

FTransform FEIPoseCurve::Evaluate(float T) const
{
	const FVector Angles = Rotation.Eval(T);
	return FTransform(FRotator(Angles.X, Angles.Y, Angles.Z), Position.Eval(T));
}

bool FEIPoseLibrary::LoadFromString(const FString& Json, FEIPoseLibrary& OutLibrary, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEIPoseLibrary Loaded;
	if (!Root.IsValid() || !ReadLibrary(*Root, Loaded, OutError))
	{
		return false;
	}
	OutLibrary = MoveTemp(Loaded);
	return true;
}

bool FEIPoseLibrary::LoadFromProjectFile(FEIPoseLibrary& OutLibrary, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("posen.json"), OutError);
	FEIPoseLibrary Loaded;
	if (!Root.IsValid() || !ReadLibrary(*Root, Loaded, OutError))
	{
		return false;
	}
	OutLibrary = MoveTemp(Loaded);
	return true;
}
