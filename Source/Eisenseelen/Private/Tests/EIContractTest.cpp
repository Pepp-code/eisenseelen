#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Count.h"
#include "EIContracts.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIContractDataTest,
	"Eisenseelen.Data.Contracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIContractDataTest::RunTest(const FString& Parameters)
{
	FString Error;

	// The file of the game.
	FEIContractTable Table;
	if (!FEIContractTable::LoadFromProjectFile(Table, Error))
	{
		AddError(Error);
	}
	TestEqual(TEXT("Six contracts in a run"), FMath::RoundToInt32(Table.ContractsPerRun), 6);
	TestEqual(TEXT("Four contracts until the flare"), FMath::RoundToInt32(Table.ContractsToFlare), 4);
	TestEqual(TEXT("Four kinds of task"), Table.Tasks.Num(), 4);
	TestEqual(TEXT("A failed run keeps 30 percent"), Table.KeptOnFailure, 0.3f);
	TestEqual(TEXT("The overview knows the map"), Table.MapMeters, 300.0f);

	// A small table, to see that the rules are kept.
	const FString Valid = TEXT("{\"vertraege_pro_lauf\": 6, \"vertraege_bis_zur_fackel\": 4, \"mindestabstand_m\": 40, \"gold\": 100, \"punkte\": 10, ")
		TEXT("\"verderbnis_bonus_je_stufe\": 0.1, \"beim_scheitern_bleibt\": 0.3, ")
		TEXT("\"fraktionen\": [{\"name\": \"Eins\", \"farbe\": [1, 0, 0]}, {\"name\": \"Zwei\", \"farbe\": [0, 1, 0]}, {\"name\": \"Drei\", \"farbe\": [0, 0, 1]}], ")
		TEXT("\"aufgaben\": [{\"art\": \"anfuehrer\", \"name\": \"Jagen\", \"kurz\": \"J\"}, {\"art\": \"schrein\", \"name\": \"Schrein\", \"kurz\": \"S\", \"halten_s\": 15, \"zone_m\": 6}, ")
		TEXT("{\"art\": \"truhe\", \"name\": \"Truhe\", \"kurz\": \"T\"}, {\"art\": \"altar\", \"name\": \"Altar\", \"kurz\": \"A\", \"halten_s\": 30, \"zone_m\": 8}]}");
	FEIContractTable Small;
	TestTrue(TEXT("A complete table loads"), FEIContractTable::LoadFromString(Valid, Small, Error));
	TestEqual(TEXT("The factions count"), Small.Factions.Num(), 3);
	TestEqual(TEXT("A faction has its color"), Small.Factions[1].Color, FLinearColor(0.0f, 1.0f, 0.0f));
	TestEqual(TEXT("A held task has its time"), Small.FindTask(EEIContractKind::Altar)->HoldSeconds, 30.0f);
	TestEqual(TEXT("A held task has its zone"), Small.FindTask(EEIContractKind::Shrine)->ZoneMeters, 6.0f);
	TestNull(TEXT("A faction that does not exist is nothing"), Small.FindFaction(3));

	// What a contract pays.
	TestEqual(TEXT("At the start it pays the plain gold"), Small.GetGold(0), 100);
	TestEqual(TEXT("Every stage of the corruption pays more gold"), Small.GetGold(3), 130);
	TestEqual(TEXT("Every stage pays more points too"), Small.GetPoints(5), 15);
	TestEqual(TEXT("A failed run keeps its share"), Small.GetKeptOnFailure(100), 30);
	TestEqual(TEXT("And never rounds up"), Small.GetKeptOnFailure(25), 7);

	// Whatever the dice say: every faction has its turn, and so does every task.
	const auto CheckRoll = [this](const FEIContractTable& Rolled, int32 MostOfFaction, int32 LeastOfFaction)
	{
		const TArray<FEIContract> Contracts = Rolled.RollContracts();
		TestEqual(TEXT("A roll brings all contracts"), Contracts.Num(), FMath::RoundToInt32(Rolled.ContractsPerRun));
		for (int32 Faction = 0; Faction < Rolled.Factions.Num(); ++Faction)
		{
			const int32 Count = Algo::CountIf(Contracts, [Faction](const FEIContract& Contract) { return Contract.Faction == Faction; });
			TestTrue(TEXT("No faction has too many"), Count <= MostOfFaction);
			TestTrue(TEXT("No faction has too few"), Count >= LeastOfFaction);
		}
		for (const FEIContractTask& Task : Rolled.Tasks)
		{
			const int32 Count = Algo::CountIf(Contracts, [&Task](const FEIContract& Contract) { return Contract.Kind == Task.Kind; });
			TestTrue(TEXT("Every task comes up"), Count >= 1);
			TestTrue(TEXT("No task comes up more than twice"), Count <= 2);
		}
		for (int32 Index = 1; Index < Contracts.Num(); ++Index)
		{
			TestTrue(TEXT("The list is sorted by faction"), Contracts[Index - 1].Faction <= Contracts[Index].Faction);
		}
		for (const FEIContract& Contract : Contracts)
		{
			TestEqual(TEXT("A contract starts with the time of its task"), Contract.HoldLeft, Rolled.FindTask(Contract.Kind)->HoldSeconds);
		}
	};
	for (int32 Roll = 0; Roll < 20; ++Roll)
	{
		CheckRoll(Small, 2, 2);
	}

	// A fourth faction is one more line in the file, and then two factions have one contract less.
	FEIContractTable WithWind = Small;
	FEIFaction Wind;
	Wind.Name = TEXT("Wind");
	WithWind.Factions.Add(Wind);
	for (int32 Roll = 0; Roll < 20; ++Roll)
	{
		CheckRoll(WithWind, 2, 1);
	}

	// What must not pass.
	FEIContractTable Untouched;
	const FString UnknownTask = Valid.Replace(TEXT("\"art\": \"truhe\""), TEXT("\"art\": \"ruine\""));
	TestFalse(TEXT("A task nobody knows is refused"), FEIContractTable::LoadFromString(UnknownTask, Untouched, Error));
	TestTrue(TEXT("The error names it"), Error.Contains(TEXT("ruine")));
	const FString NoColor = Valid.Replace(TEXT("\"farbe\": [1, 0, 0]"), TEXT("\"farbe\": [1, 0]"));
	TestFalse(TEXT("A faction needs its whole color"), FEIContractTable::LoadFromString(NoColor, Untouched, Error));
	const FString NoHold = Valid.Replace(TEXT("\"halten_s\": 30, "), TEXT(""));
	TestFalse(TEXT("A held task needs its time"), FEIContractTable::LoadFromString(NoHold, Untouched, Error));
	const FString NoFactions = Valid.Replace(TEXT("\"fraktionen\""), TEXT("\"parteien\""));
	TestFalse(TEXT("Without factions nothing goes"), FEIContractTable::LoadFromString(NoFactions, Untouched, Error));
	const FString TooMuchKept = Valid.Replace(TEXT("\"beim_scheitern_bleibt\": 0.3"), TEXT("\"beim_scheitern_bleibt\": 3"));
	TestFalse(TEXT("More than all cannot stay"), FEIContractTable::LoadFromString(TooMuchKept, Untouched, Error));
	TestFalse(TEXT("Broken text is refused"), FEIContractTable::LoadFromString(TEXT("{ fraktionen"), Untouched, Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.Factions.Num(), 0);
	return true;
}

#endif
