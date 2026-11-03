#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EIEnemyGroups.h"
#include "EIRunDirector.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIEnemyGroupsTest,
	"Eisenseelen.Data.EnemyGroups",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIEnemyGroupsTest::RunTest(const FString& Parameters)
{
	FString Error;

	// Every kind of enemy has its numbers.
	for (int32 Kind = 0; Kind < EIEnemyKindCount; ++Kind)
	{
		FEIEnemyValues Values;
		if (!FEIEnemyValues::LoadFromProjectFile(static_cast<EEIEnemyKind>(Kind), Values, Error))
		{
			AddError(Error);
		}
	}

	// Whatever a stage asks for has to exist in that stage, or nobody would come.
	FEIEnemyGroups Table;
	if (!FEIEnemyGroups::LoadFromProjectFile(Table, Error))
	{
		AddError(Error);
	}
	FEIRunValues RunValues;
	if (!FEIRunValues::LoadFromProjectFile(RunValues, Error))
	{
		AddError(Error);
	}
	TestEqual(TEXT("Every stage of the corruption has its line"), Table.Stages.Num(), FMath::RoundToInt(RunValues.CorruptionMaxStage) + 1);
	for (int32 Stage = 0; Stage < Table.Stages.Num(); ++Stage)
	{
		const FEIGroupStage& Row = Table.GetStage(Stage);
		for (const FString* Size : { &Row.Camp, &Row.Chest, &Row.AltarWave, &Row.BeaconWave })
		{
			TestNotNull(*FString::Printf(TEXT("Stage %d has a group of the size %s"), Stage, **Size), Table.Pick(*Size, Stage));
		}
		if (Row.ReinforcementEverySeconds > 0.0f)
		{
			TestNotNull(*FString::Printf(TEXT("Stage %d sends reinforcements of the size %s"), Stage, *Row.Reinforcement), Table.Pick(Row.Reinforcement, Stage));
		}
	}
	TestEqual(TEXT("At the start nothing comes by itself"), Table.GetStage(0).ReinforcementEverySeconds, 0.0f);
	TestTrue(TEXT("From the first stage on reinforcements come"), Table.GetStage(1).ReinforcementEverySeconds > 0.0f);

	// A small table, to see that the rules are kept.
	const FString Valid = TEXT("{\"altar_welle_alle_s\": 5, \"leuchtfeuer_welle_alle_s\": 6, ")
		TEXT("\"gruppen\": [{\"name\": \"Probe\", \"groesse\": \"klein\", \"ab_stufe\": 1, \"bis_stufe\": 2, \"knochenlaeufer\": 3, \"schildtraeger\": 1}], ")
		TEXT("\"stufen\": [{\"lager\": \"klein\", \"nachschub\": \"\", \"nachschub_alle_s\": 0, \"truhe\": \"klein\", \"altar\": \"klein\", \"leuchtfeuer\": \"klein\", \"obergrenze\": 9},")
		TEXT("{\"lager\": \"klein\", \"nachschub\": \"klein\", \"nachschub_alle_s\": 30, \"truhe\": \"klein\", \"altar\": \"klein\", \"leuchtfeuer\": \"klein\", \"obergrenze\": 12}]}");
	FEIEnemyGroups Small;
	TestTrue(TEXT("A complete table loads"), FEIEnemyGroups::LoadFromString(Valid, Small, Error));
	TestEqual(TEXT("The wave time counts"), Small.AltarWaveEverySeconds, 5.0f);
	TestNull(TEXT("A group does not come before its stage"), Small.Pick(TEXT("klein"), 0));
	TestNotNull(TEXT("A group comes in its stage"), Small.Pick(TEXT("klein"), 2));
	TestNull(TEXT("A group does not come after its stage"), Small.Pick(TEXT("klein"), 3));
	TestNull(TEXT("A size nobody has brings nobody"), Small.Pick(TEXT("gross"), 2));
	TestEqual(TEXT("A stage beyond the last counts as the last"), Small.GetStage(9).ThreatCap, 12.0f);

	const FEIEnemyGroup& Group = Small.Groups[0];
	TestEqual(TEXT("A group has all its members"), Group.ListMembers(1.0f).Num(), 4);
	TestEqual(TEXT("More players bring more of them"), Group.ListMembers(2.0f).Num(), 8);
	TestEqual(TEXT("A factor of 0 brings nobody"), Group.ListMembers(0.0f).Num(), 0);
	TestEqual(TEXT("A low factor leaves one of each kind"), Group.ListMembers(0.1f).Num(), 2);

	FEIEnemyGroups Untouched;
	const FString Empty = Valid.Replace(TEXT("\"knochenlaeufer\": 3, \"schildtraeger\": 1"), TEXT("\"knochenlaeufer\": 0"));
	TestFalse(TEXT("A group without anyone is refused"), FEIEnemyGroups::LoadFromString(Empty, Untouched, Error));
	TestTrue(TEXT("The error names the group"), Error.Contains(TEXT("Probe")));
	TestFalse(TEXT("Broken text is refused"), FEIEnemyGroups::LoadFromString(TEXT("{ gruppen"), Untouched, Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.Groups.Num(), 0);
	return true;
}

#endif
