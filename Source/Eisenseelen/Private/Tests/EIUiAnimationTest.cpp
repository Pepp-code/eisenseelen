#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EIUiAnimation.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIUiAnimationTest,
	"Eisenseelen.Data.UiAnimation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIUiAnimationTest::RunTest(const FString& Parameters)
{
	// The file of the game.
	FEIUiTimes Times;
	FString Error;
	if (!FEIUiTimes::LoadFromProjectFile(Times, Error))
	{
		AddError(Error);
	}
	TestEqual(TEXT("A soft fade takes a quarter second"), Times.FadeSeconds, 0.25f);
	TestEqual(TEXT("The ghost waits"), Times.GhostHoldSeconds, 0.6f);
	TestEqual(TEXT("The ghost needs three seconds for a whole bar"), Times.GhostDrainSeconds, 3.0f);
	TestEqual(TEXT("The pulse breathes once a second"), Times.PulsePeriod, 1.0f);
	TestEqual(TEXT("The pulse breathes by fifteen percent"), Times.PulseAmount, 0.15f);
	TestEqual(TEXT("A swing takes four tenths"), Times.SwingSeconds, 0.4f);
	TestEqual(TEXT("A swing starts at two fifths"), Times.SwingStartScale, 0.4f);
	TestEqual(TEXT("A swing shoots past the target"), Times.SwingOvershoot, 1.12f);

	// S1: the fade runs at constant speed and stops at both ends.
	FEIUiFade Fade;
	TestEqual(TEXT("The fade starts gone"), Fade.Alpha, 0.0f);
	Fade.Update(true, Times, 0.125f);
	TestEqual(TEXT("Half the time is half the alpha"), Fade.Alpha, 0.5f);
	Fade.Update(true, Times, 1.0f);
	TestEqual(TEXT("The fade stops at fully there"), Fade.Alpha, 1.0f);
	Fade.Update(false, Times, 0.25f);
	TestEqual(TEXT("The fade ends gone again"), Fade.Alpha, 0.0f);

	// S2: the ghost holds after a hit, then drains at its own pace.
	FEIUiGhost Ghost;
	Ghost.Update(1.0f, Times, 0.1f);
	TestEqual(TEXT("The ghost starts full"), Ghost.Fraction, 1.0f);
	Ghost.Update(0.4f, Times, 0.5f);
	TestEqual(TEXT("The ghost stands still while the hold lasts"), Ghost.Fraction, 1.0f);
	Ghost.Update(0.4f, Times, 0.1f);
	Ghost.Update(0.4f, Times, 1.5f);
	TestEqual(TEXT("The ghost drains a half bar in one and a half seconds"), Ghost.Fraction, 0.5f);
	Ghost.Update(0.9f, Times, 0.05f);
	TestEqual(TEXT("On a rise the ghost stays with the value"), Ghost.Fraction, 0.9f);
	Ghost.Update(0.2f, Times, 10.0f);
	TestEqual(TEXT("The ghost never runs below the value"), Ghost.Fraction, 0.2f);

	// S5: the breath swings around one, once per period.
	FEIUiPulse Pulse;
	TestEqual(TEXT("The breath starts in the middle"), Pulse.Factor(Times), 1.0f);
	Pulse.Advance(Times, 0.25f);
	TestTrue(TEXT("A quarter in it breathes out"), FMath::IsNearlyEqual(Pulse.Factor(Times), 1.15f, 0.001f));
	Pulse.Advance(Times, Times.PulsePeriod / 2.0f);
	TestTrue(TEXT("Three quarters in it breathes in"), FMath::IsNearlyEqual(Pulse.Factor(Times), 0.85f, 0.001f));
	Pulse.Advance(Times, 0.25f);
	TestTrue(TEXT("The breath comes back around"), FMath::IsNearlyEqual(Pulse.Factor(Times), 1.0f, 0.001f));

	// S8: the swing starts small, shoots past the target and settles on it.
	FEIUiSwing Swing;
	TestEqual(TEXT("What never began is not scaled"), Swing.Scale(100.0, Times), 1.0f);
	Swing.Begin(100.0);
	TestEqual(TEXT("The swing starts small"), Swing.Scale(100.0, Times), Times.SwingStartScale);
	TestTrue(TEXT("It shoots past the target"), FMath::IsNearlyEqual(Swing.Scale(100.0 + Times.SwingSeconds * 2.0f / 3.0f, Times), Times.SwingOvershoot, 0.001f));
	TestEqual(TEXT("It settles on the target"), Swing.Scale(100.0 + Times.SwingSeconds, Times), 1.0f);
	TestEqual(TEXT("It stays settled"), Swing.Scale(200.0, Times), 1.0f);

	return true;
}

#endif
