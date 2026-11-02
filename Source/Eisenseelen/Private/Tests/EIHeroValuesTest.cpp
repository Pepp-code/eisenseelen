#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EIHeroValues.h"
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
	const FString Valid = TEXT("{\"lauftempo_m_pro_s\": 7, \"sprunghoehe_m\": 3, \"luftkontrolle_0_bis_1\": 0.25, \"maus_faktor\": 2}");

	FEIHeroValues Values;
	FString Error;
	TestTrue(TEXT("A complete file loads"), Parse(Valid, Values, Error));
	TestEqual(TEXT("Run speed"), Values.RunSpeedMetersPerSecond, 7.0f);
	TestEqual(TEXT("Jump height"), Values.JumpHeightMeters, 3.0f);
	TestEqual(TEXT("Air control"), Values.AirControl, 0.25f);
	TestEqual(TEXT("Mouse factor"), Values.MouseFactor, 2.0f);

	FEIHeroValues Untouched;
	TestFalse(TEXT("Broken JSON is refused"), Parse(TEXT("{ lauftempo"), Untouched, Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.RunSpeedMetersPerSecond, FEIHeroValues().RunSpeedMetersPerSecond);

	TestFalse(TEXT("A missing value is refused"), Parse(TEXT("{\"lauftempo_m_pro_s\": 7}"), Untouched, Error));
	TestTrue(TEXT("The error names the missing value"), Error.Contains(TEXT("sprunghoehe_m")));
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
	return true;
}

#endif
