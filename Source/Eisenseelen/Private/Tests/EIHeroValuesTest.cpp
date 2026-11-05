#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EIHeroValues.h"
#include "EIPoses.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIHeroValuesParseTest,
	"Eisenseelen.Data.HeroValues.Parse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIHeroValuesParseTest::RunTest(const FString& Parameters)
{
	const auto Parse = [](const FString& Json, FEIHeroValues& OutValues, FString& OutError)
	{
		return FEIHeroValues::LoadFromString(Json, OutValues, OutError);
	};
	const FString Valid = TEXT("{\"lauftempo_m_pro_s\": 7, \"sprint_tempo_m_pro_s\": 9, \"blocken_tempo_m_pro_s\": 2, \"sprunghoehe_m\": 3, \"schwerkraft_faktor\": 2, \"luftkontrolle_0_bis_1\": 0.25, \"luft_lenkung_grad_pro_s\": 150, \"maus_faktor\": 2, ")
		TEXT("\"leben_erholung_nach_s\": 4, \"leben_erholung_anteil_pro_s\": 0.2, \"sprung_puffer_s\": 0.2}");

	FEIHeroValues Values;
	FString Error;
	TestTrue(TEXT("A complete file loads"), Parse(Valid, Values, Error));
	TestEqual(TEXT("Run speed"), Values.RunSpeedMetersPerSecond, 7.0f);
	TestEqual(TEXT("Sprint speed"), Values.SprintSpeedMetersPerSecond, 9.0f);
	TestEqual(TEXT("Block speed"), Values.BlockSpeedMetersPerSecond, 2.0f);
	TestEqual(TEXT("Jump height"), Values.JumpHeightMeters, 3.0f);
	TestEqual(TEXT("Air control"), Values.AirControl, 0.25f);
	TestEqual(TEXT("Air turn rate"), Values.AirTurnDegreesPerSecond, 150.0f);
	TestEqual(TEXT("Mouse factor"), Values.MouseFactor, 2.0f);
	TestEqual(TEXT("Gravity"), Values.GravityFactor, 2.0f);
	TestEqual(TEXT("Life back after"), Values.LifeBackAfterSeconds, 4.0f);
	TestEqual(TEXT("Life back per second"), Values.LifeBackSharePerSecond, 0.2f);
	TestEqual(TEXT("Jump buffer"), Values.JumpBufferSeconds, 0.2f);

	FEIHeroValues Untouched;
	TestFalse(TEXT("Broken JSON is refused"), Parse(TEXT("{ lauftempo"), Untouched, Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.RunSpeedMetersPerSecond, FEIHeroValues().RunSpeedMetersPerSecond);

	TestFalse(TEXT("A missing value is refused"), Parse(TEXT("{\"lauftempo_m_pro_s\": 7}"), Untouched, Error));
	TestTrue(TEXT("The error names the missing value"), Error.Contains(TEXT("sprint_tempo_m_pro_s")));
	TestEqual(TEXT("A partly valid file changes nothing"), Untouched.RunSpeedMetersPerSecond, FEIHeroValues().RunSpeedMetersPerSecond);

	const FString TooFast = Valid.Replace(TEXT("\"lauftempo_m_pro_s\": 7"), TEXT("\"lauftempo_m_pro_s\": 500"));
	TestFalse(TEXT("A value out of range is refused"), Parse(TooFast, Untouched, Error));
	TestTrue(TEXT("The error names the bad value"), Error.Contains(TEXT("lauftempo_m_pro_s")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIHeroValuesProjectFileTest,
	"Eisenseelen.Data.HeroValues.ProjectFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIHeroValuesProjectFileTest::RunTest(const FString& Parameters)
{
	FEIHeroValues Values;
	FString Error;
	if (!FEIHeroValues::LoadFromProjectFile(Values, Error))
	{
		AddError(Error);
	}
	FEIHandMotion Motion;
	if (!FEIHandMotion::LoadFromProjectFile(Motion, Error))
	{
		AddError(Error);
	}
	return true;
}

#endif
