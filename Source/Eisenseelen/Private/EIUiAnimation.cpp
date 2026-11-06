#include "EIUiAnimation.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"

bool FEIUiTimes::LoadFromProjectFile(FEIUiTimes& OutTimes, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("ui.json"), OutError);
	FEIUiTimes Loaded;
	if (!Root.IsValid()
		|| !FEIDataFile::ReadNumber(*Root, TEXT("einblenden_s"), 0.01, 5.0, Loaded.FadeSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("geist_verzoegerung_s"), 0.0, 5.0, Loaded.GhostHoldSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("geist_abbau_s"), 0.05, 30.0, Loaded.GhostDrainSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("puls_periode_s"), 0.05, 10.0, Loaded.PulsePeriod, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("puls_ausschlag"), 0.0, 1.0, Loaded.PulseAmount, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("durchschwingen_s"), 0.05, 5.0, Loaded.SwingSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("durchschwingen_start"), 0.0, 1.0, Loaded.SwingStartScale, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("durchschwingen_ueberziel"), 1.0, 2.0, Loaded.SwingOvershoot, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("schadenszahl_lebensdauer_s"), 0.05, 10.0, Loaded.DamageNumberSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("schadenszahl_aufstieg_px"), 0.0, 500.0, Loaded.DamageNumberRise, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("toast_einschieben_s"), 0.0, 5.0, Loaded.ToastSlideSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("toast_stand_s"), 0.1, 30.0, Loaded.ToastHoldSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("treffer_aufschlag_s"), 0.0, 5.0, Loaded.HitStrikeSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("treffer_abklingen_s"), 0.05, 10.0, Loaded.HitFadeSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("boss_ruck_px"), 0.0, 100.0, Loaded.BossShakePixels, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("boss_ruck_s"), 0.05, 10.0, Loaded.BossShakeSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("karte_aufbau_s"), 0.0, 10.0, Loaded.MapBuildSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("karte_marken_versatz_s"), 0.0, 1.0, Loaded.MapMarkStagger, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("slot_aufblinken_s"), 0.0, 5.0, Loaded.SlotBlinkSeconds, OutError))
	{
		return false;
	}
	OutTimes = Loaded;
	return true;
}

void FEIUiFade::Update(bool bVisible, const FEIUiTimes& Times, float DeltaSeconds)
{
	const float Speed = 1.0f / FMath::Max(Times.FadeSeconds, 0.01f);
	Alpha = FMath::Clamp(Alpha + (bVisible ? Speed : -Speed) * DeltaSeconds, 0.0f, 1.0f);
}

void FEIUiGhost::Update(float Target, const FEIUiTimes& Times, float DeltaSeconds)
{
	Target = FMath::Clamp(Target, 0.0f, 1.0f);
	if (Target < LastTarget)
	{
		// Every fresh hit makes the ghost wait again, so the last damage stays readable.
		HoldLeft = Times.GhostHoldSeconds;
	}
	LastTarget = Target;
	if (Target >= Fraction)
	{
		Fraction = Target;
		HoldLeft = 0.0f;
		return;
	}
	if (HoldLeft > 0.0f)
	{
		// What the frame has left after the hold drains the ghost in the same step, so no frame is lost.
		if (DeltaSeconds < HoldLeft)
		{
			HoldLeft -= DeltaSeconds;
			return;
		}
		DeltaSeconds -= HoldLeft;
		HoldLeft = 0.0f;
	}
	Fraction = FMath::Max(Target, Fraction - DeltaSeconds / FMath::Max(Times.GhostDrainSeconds, 0.01f));
}

void FEIUiPulse::Advance(const FEIUiTimes& Times, float DeltaSeconds)
{
	Phase = FMath::Fmod(Phase + FMath::Max(DeltaSeconds, 0.0f), FMath::Max(Times.PulsePeriod, 0.01f));
}

float FEIUiPulse::Factor(const FEIUiTimes& Times) const
{
	return 1.0f + Times.PulseAmount * FMath::Sin(2.0 * PI * Phase / FMath::Max(Times.PulsePeriod, 0.01f));
}

float FEIUiSwing::Scale(double Now, const FEIUiTimes& Times) const
{
	if (StartedAt < 0.0)
	{
		return 1.0f;
	}
	const float Elapsed = FMath::Clamp(static_cast<float>((Now - StartedAt) / FMath::Max(Times.SwingSeconds, 0.01f)), 0.0f, 1.0f);
	// Two thirds of the way it grows past the target, then it settles back to it.
	constexpr float GrowShare = 2.0f / 3.0f;
	if (Elapsed < GrowShare)
	{
		return FMath::Lerp(Times.SwingStartScale, Times.SwingOvershoot, FMath::SmoothStep(0.0f, 1.0f, Elapsed / GrowShare));
	}
	return FMath::Lerp(Times.SwingOvershoot, 1.0f, (Elapsed - GrowShare) / (1.0f - GrowShare));
}
