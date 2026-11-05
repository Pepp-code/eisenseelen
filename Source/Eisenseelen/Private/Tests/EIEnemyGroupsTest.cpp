#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EIEnemyGroups.h"
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

	// Whatever the camps, the chest and the blood moon ask for has to exist, or nobody would come.
	FEIEnemyGroups Table;
	if (!FEIEnemyGroups::LoadFromProjectFile(Table, Error))
	{
		AddError(Error);
	}
	for (const FString& Size : { FString(TEXT("klein")), FString(TEXT("mittel")), Table.ChestGuards, Table.AltarWave.Size, Table.StoneWave.Size, Table.FightWave.Size, Table.PursuitWave.Size, Table.FireWave.Size })
	{
		TestNotNull(*FString::Printf(TEXT("There is a group of the size %s"), *Size), Table.Pick(Size));
	}
	TestTrue(TEXT("The blood moon stays a little after the task"), Table.BloodMoonLingerSeconds > 0.0f);

	// A small table, to see that the rules are kept.
	const FString Valid = TEXT("{\"truhe\": \"klein\", \"obergrenze\": 9, \"blutmond_nachlauf_s\": 4, ")
		TEXT("\"blutmond\": {\"altar\": {\"groesse\": \"klein\", \"alle_s\": 5}, \"ankerstein\": {\"groesse\": \"klein\", \"alle_s\": 6}, ")
		TEXT("\"bosskampf\": {\"groesse\": \"klein\", \"alle_s\": 7}, \"flucht\": {\"groesse\": \"klein\", \"alle_s\": 8}, \"signalfeuer\": {\"groesse\": \"gross\", \"alle_s\": 9}}, ")
		TEXT("\"gruppen\": [{\"name\": \"Probe\", \"groesse\": \"klein\", \"knochenlaeufer\": 3, \"schildtraeger\": 1}]}");
	FEIEnemyGroups Small;
	TestTrue(TEXT("A complete table loads"), FEIEnemyGroups::LoadFromString(Valid, Small, Error));
	TestEqual(TEXT("The wave time counts"), Small.AltarWave.EverySeconds, 5.0f);
	TestEqual(TEXT("Every task has its own wave"), Small.FireWave.Size, FString(TEXT("gross")));
	TestEqual(TEXT("The limit counts"), Small.ThreatCap, 9.0f);
	TestNotNull(TEXT("A group of its size comes"), Small.Pick(TEXT("klein")));
	TestNull(TEXT("A size nobody has brings nobody"), Small.Pick(TEXT("gross")));
	FEIEnemyGroups Untouched;
	TestFalse(TEXT("A blood moon without the signal fire is refused"), FEIEnemyGroups::LoadFromString(Valid.Replace(TEXT("\"signalfeuer\""), TEXT("\"feuer\"")), Untouched, Error));
	TestTrue(TEXT("The error names what is missing"), Error.Contains(TEXT("signalfeuer")));

	const FEIEnemyGroup& Group = Small.Groups[0];
	TestEqual(TEXT("A group has all its members"), Group.ListMembers(1.0f).Num(), 4);
	TestEqual(TEXT("More players bring more of them"), Group.ListMembers(2.0f).Num(), 8);
	TestEqual(TEXT("A factor of 0 brings nobody"), Group.ListMembers(0.0f).Num(), 0);
	TestEqual(TEXT("A low factor leaves one of each kind"), Group.ListMembers(0.1f).Num(), 2);

	const FString Empty = Valid.Replace(TEXT("\"knochenlaeufer\": 3, \"schildtraeger\": 1"), TEXT("\"knochenlaeufer\": 0"));
	TestFalse(TEXT("A group without anyone is refused"), FEIEnemyGroups::LoadFromString(Empty, Untouched, Error));
	TestTrue(TEXT("The error names the group"), Error.Contains(TEXT("Probe")));
	TestFalse(TEXT("Broken text is refused"), FEIEnemyGroups::LoadFromString(TEXT("{ gruppen"), Untouched, Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.Groups.Num(), 0);
	return true;
}

#endif
