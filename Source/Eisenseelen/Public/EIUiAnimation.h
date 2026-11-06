#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/**
 * The times of the HUD animations from Data/ui.json, so the developer can tune them without a build.
 * Numbers no step uses yet are here too, so every later animation reads the same file.
 */
struct EISENSEELEN_API FEIUiTimes
{
	// S1: how long a soft fade takes from gone to fully there.
	float FadeSeconds = 0.25f;
	// S2: how long the ghost bar stands still after the value falls, and how long it needs for a whole bar.
	float GhostHoldSeconds = 0.6f;
	float GhostDrainSeconds = 3.0f;
	// S5: the pulse breathes size or alpha once per period, by this much.
	float PulsePeriod = 1.0f;
	float PulseAmount = 0.15f;
	// S8: a big line swings in from the start scale, over the target, back to it.
	float SwingSeconds = 0.4f;
	float SwingStartScale = 0.4f;
	float SwingOvershoot = 1.12f;
	// S3 damage numbers, S4 toasts, S6 hit flash, S7 boss shake, S9 map build, S10 slot blink.
	float DamageNumberSeconds = 0.8f;
	float DamageNumberRise = 40.0f;
	float ToastSlideSeconds = 0.2f;
	float ToastHoldSeconds = 2.5f;
	float HitStrikeSeconds = 0.15f;
	float HitFadeSeconds = 0.4f;
	float BossShakePixels = 8.0f;
	float BossShakeSeconds = 0.5f;
	float MapBuildSeconds = 0.3f;
	float MapMarkStagger = 0.03f;
	float SlotBlinkSeconds = 0.2f;

	static bool LoadFromProjectFile(FEIUiTimes& OutTimes, FString& OutError);
};

/** S1: an alpha that runs to 0 or 1 at constant speed, so panels and bars come and go without a jump. */
struct EISENSEELEN_API FEIUiFade
{
	float Alpha = 0.0f;

	void Update(bool bVisible, const FEIUiTimes& Times, float DeltaSeconds);
};

/**
 * S2: the lighter bar behind a value bar. While the value falls the ghost stands still for the hold time,
 * then drains towards it at its own pace; when the value rises the ghost stays with the value.
 */
struct EISENSEELEN_API FEIUiGhost
{
	float Fraction = 1.0f;

	void Update(float Target, const FEIUiTimes& Times, float DeltaSeconds);

private:
	float LastTarget = 1.0f;
	float HoldLeft = 0.0f;
};

/** S5: a slow breath of size or alpha for what wants attention. */
struct EISENSEELEN_API FEIUiPulse
{
	void Advance(const FEIUiTimes& Times, float DeltaSeconds);
	float Factor(const FEIUiTimes& Times) const;

private:
	float Phase = 0.0f;
};

/** S8: a big line swings from small past its size and back, instead of just appearing. */
struct EISENSEELEN_API FEIUiSwing
{
	void Begin(double Now) { StartedAt = Now; }
	float Scale(double Now, const FEIUiTimes& Times) const;

private:
	double StartedAt = -1.0;
};
