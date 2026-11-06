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
	TestEqual(TEXT("Three side jobs in a run"), FMath::RoundToInt32(Table.ContractsPerRun), 3);
	TestEqual(TEXT("Four kinds of task"), Table.Tasks.Num(), 4);
	TestEqual(TEXT("A failed run keeps 30 percent"), Table.KeptOnFailure, 0.3f);
	TestEqual(TEXT("The overview knows the map"), Table.MapMeters, 600.0f);

	// A small table, to see that the rules are kept.
	const FString Valid = TEXT("{\"auftraege_pro_lauf\": 3, \"mindestabstand_m\": 40, \"gold\": 100, \"punkte\": 10, ")
		TEXT("\"beim_scheitern_bleibt\": 0.3, ")
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
	TestEqual(TEXT("It pays its gold"), Small.GetGold(), 100);
	TestEqual(TEXT("It pays its points"), Small.GetPoints(), 10);
	TestEqual(TEXT("A failed run keeps its share"), Small.GetKeptOnFailure(100), 30);
	TestEqual(TEXT("And never rounds up"), Small.GetKeptOnFailure(25), 7);

	// Whatever the dice say: no task comes twice before all others have been there, and the list is in order.
	const auto CheckRoll = [this](const FEIContractTable& Rolled)
	{
		const TArray<FEIContract> Contracts = Rolled.RollContracts();
		TestEqual(TEXT("A roll brings all side jobs"), Contracts.Num(), FMath::RoundToInt32(Rolled.ContractsPerRun));
		const int32 MostOfTask = FMath::DivideAndRoundUp(Contracts.Num(), Rolled.Tasks.Num());
		for (const FEIContractTask& Task : Rolled.Tasks)
		{
			const int32 Count = Algo::CountIf(Contracts, [&Task](const FEIContract& Contract) { return Contract.Kind == Task.Kind; });
			TestTrue(TEXT("No task comes up too often"), Count <= MostOfTask);
		}
		for (int32 Index = 1; Index < Contracts.Num(); ++Index)
		{
			TestTrue(TEXT("The list is sorted by faction"), Contracts[Index - 1].Faction <= Contracts[Index].Faction);
		}
		for (const FEIContract& Contract : Contracts)
		{
			TestEqual(TEXT("A side job starts with the time of its task"), Contract.HoldLeft, Rolled.FindTask(Contract.Kind)->HoldSeconds);
			TestNotNull(TEXT("A side job has a faction"), Rolled.FindFaction(Contract.Faction));
		}
	};
	for (int32 Roll = 0; Roll < 20; ++Roll)
	{
		CheckRoll(Small);
	}

	// The factions are pure chance, so that nobody knows beforehand what the map pays. Over many rolls both show up.
	bool bSameFactionTwice = false;
	bool bAllDifferent = false;
	for (int32 Roll = 0; Roll < 100; ++Roll)
	{
		const TArray<FEIContract> Contracts = Small.RollContracts();
		const bool bTwice = Contracts[0].Faction == Contracts[1].Faction || Contracts[1].Faction == Contracts[2].Faction;
		bSameFactionTwice = bSameFactionTwice || bTwice;
		bAllDifferent = bAllDifferent || !bTwice;
	}
	TestTrue(TEXT("Sometimes two jobs share a faction"), bSameFactionTwice);
	TestTrue(TEXT("Sometimes all three differ"), bAllDifferent);

	// A fourth faction is one more line in the file.
	FEIContractTable WithWind = Small;
	FEIFaction Wind;
	Wind.Name = TEXT("Wind");
	WithWind.Factions.Add(Wind);
	for (int32 Roll = 0; Roll < 20; ++Roll)
	{
		CheckRoll(WithWind);
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
