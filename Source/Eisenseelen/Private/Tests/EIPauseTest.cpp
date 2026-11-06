#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EISettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEISettingsDataTest,
	"Eisenseelen.Data.Einstellungen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEISettingsDataTest::RunTest(const FString& Parameters)
{
	// The file of the game.
	FEISettings Settings;
	FString Error;
	if (!FEISettings::LoadFromProjectFile(Settings, Error))
	{
		AddError(Error);
	}
	TestEqual(TEXT("The mouse factor starts in the middle"), Settings.MouseFactor, 1.0f);
	TestFalse(TEXT("The view is not inverted"), Settings.bInvertLook);
	TestTrue(TEXT("The HUD is on"), Settings.bHudOn);
	TestTrue(TEXT("Damage numbers are on"), Settings.bDamageNumbers);
	TestEqual(TEXT("Effects are loud"), Settings.EffectsVolume, 1.0f);
	TestEqual(TEXT("Music is loud"), Settings.MusicVolume, 1.0f);

	// What the player chose comes back out of the file unchanged.
	FEISettings Chosen;
	Chosen.MouseFactor = 1.7f;
	Chosen.bInvertLook = true;
	Chosen.bHudOn = false;
	Chosen.bDamageNumbers = false;
	Chosen.EffectsVolume = 0.45f;
	Chosen.MusicVolume = 0.7f;
	FEISettings ReadBack;
	TestTrue(TEXT("The chosen settings load again"), ReadBack.FromJson(Chosen.ToJson(), Error));
	TestEqual(TEXT("The mouse factor survives"), ReadBack.MouseFactor, 1.7f);
	TestTrue(TEXT("Inverting survives"), ReadBack.bInvertLook);
	TestFalse(TEXT("The HUD state survives"), ReadBack.bHudOn);
	TestFalse(TEXT("The damage number state survives"), ReadBack.bDamageNumbers);
	TestEqual(TEXT("The effects volume survives"), ReadBack.EffectsVolume, 0.45f);
	TestEqual(TEXT("The music volume survives"), ReadBack.MusicVolume, 0.7f);

	// What the file does not say stays at the default.
	FEISettings Partly;
	TestTrue(TEXT("A file with one value loads"), Partly.FromJson(TEXT("{\"maus_empfindlichkeit\": 2.5}"), Error));
	TestEqual(TEXT("The one value is there"), Partly.MouseFactor, 2.5f);
	TestFalse(TEXT("The rest stays at the default"), Partly.bInvertLook);

	FEISettings Untouched;
	TestFalse(TEXT("Broken JSON is refused"), Untouched.FromJson(TEXT("{ maus"), Error));
	TestEqual(TEXT("A refused file changes nothing"), Untouched.MouseFactor, FEISettings().MouseFactor);

	// The knobs hold their range, in whole steps.
	FEISettings Knobs;
	Knobs.AddMouseFactor(-100);
	TestEqual(TEXT("The mouse factor stops at the bottom"), Knobs.MouseFactor, FEISettings::MouseFactorMin);
	Knobs.AddMouseFactor(1);
	TestEqual(TEXT("The mouse factor turns one step"), Knobs.MouseFactor, FEISettings::MouseFactorMin + FEISettings::MouseFactorStep);
	Knobs.MouseFactor = FEISettings::MouseFactorMax;
	Knobs.AddMouseFactor(1);
	TestEqual(TEXT("The mouse factor stops at the top"), Knobs.MouseFactor, FEISettings::MouseFactorMax);
	Knobs.AddEffectsVolume(-100);
	TestEqual(TEXT("The effects volume stops at silent"), Knobs.EffectsVolume, FEISettings::VolumeMin);
	Knobs.AddMusicVolume(100);
	TestEqual(TEXT("The music volume stops at full"), Knobs.MusicVolume, FEISettings::VolumeMax);

	// In tests nothing is written to the disk.
	TestFalse(TEXT("Tests do not write settings"), FEISettings::UsesDisk());
	return true;
}

#if WITH_EDITOR

#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHordeDirector.h"
#include "EIPauseMenu.h"
#include "EIRunDirector.h"
#include "EITestScript.h"
#include "EIPlayerController.h"

using namespace EITestScript;

namespace
{
	/** Makes the page anew, as the HUD would, and clicks the button with this text. */
	void ClickButton(const FEIPlayContext& Context, const TCHAR* Text)
	{
		FEIPauseMenu& Menu = Context.Player->GetPauseMenu();
		Menu.Build(*Context.Player, 1280.0f, 720.0f, -1.0f, -1.0f);
		const FEIMenuElement* Button = Menu.FindButton(Text);
		if (Button == nullptr)
		{
			Context.Test->AddError(FString::Printf(TEXT("The pause menu has no button %s"), Text));
			return;
		}
		Menu.Click(Button->X + 1.0f, Button->Y + 1.0f);
	}

	void BuildMenu(const FEIPlayContext& Context)
	{
		Context.Player->GetPauseMenu().Build(*Context.Player, 1280.0f, 720.0f, -1.0f, -1.0f);
	}

	/** No enemies get in the way of a test about menus. */
	void SwitchOffDirector(const FEIPlayContext& Context)
	{
		UEIRunDirector* Run = Context.World->GetAuthGameMode<AEIGameMode>()->GetRun();
		FEIEnemyGroups& Groups = Run->GetGroups();
		for (FEIWaveRule* Wave : { &Groups.AltarWave, &Groups.StoneWave, &Groups.KettleWave, &Groups.FightWave, &Groups.PursuitWave, &Groups.FireWave })
		{
			Wave->Size.Empty();
		}
		FEIRunValues Values = Run->GetValues();
		Values.CampFactor = 0.0f;
		Run->ApplyValues(Values);
		Run->GetContractTable().Tasks.RemoveAll([](const FEIContractTask& Task) { return Task.Kind == EEIContractKind::Leader; });
		Run->GetBossTable().Bosses.Empty();
		// Without ResetRun the run would still remember a boss place from the full table.
		Run->ResetRun();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIPauseMenuArenaTest,
	"Eisenseelen.Play.Pause.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIPauseMenuArenaTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;

	// Esc opens the pause menu.
	AddTap(Steps, EKeys::Escape);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("Esc opens the pause menu"), Context.Player->IsPauseOpen());
		BuildMenu(Context);
		Context.Test->TestTrue(TEXT("The page says Pause"), Context.Player->GetPauseMenu().Shows(TEXT("Pause")));
	}));

	// Esc closes it again.
	AddTap(Steps, EKeys::Escape);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("Esc closes the pause menu again"), Context.Player->IsPauseOpen());
	}));

	// The button does the same.
	AddTap(Steps, EKeys::Escape);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		ClickButton(Context, TEXT("Fortsetzen"));
		Context.Test->TestFalse(TEXT("Keep playing closes the menu"), Context.Player->IsPauseOpen());
	}));

	// The settings page turns the knobs and flips the switches.
	AddTap(Steps, EKeys::Escape);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		ClickButton(Context, TEXT("Einstellungen"));
		BuildMenu(Context);
		Context.Test->TestTrue(TEXT("The settings page shows the mouse"), Context.Player->GetPauseMenu().Shows(TEXT("Maus-Empfindlichkeit")));
	}));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Before = Context.Player->GetSettings().MouseFactor;
		ClickButton(Context, TEXT("+"));
		Context.Test->TestEqual(TEXT("Plus turns the mouse factor up"), Context.Player->GetSettings().MouseFactor, Before + FEISettings::MouseFactorStep, 0.001f);
		ClickButton(Context, TEXT("-"));
		Context.Test->TestEqual(TEXT("Minus turns it back down"), Context.Player->GetSettings().MouseFactor, Before, 0.001f);
	}));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("The view is not inverted yet"), Context.Player->GetSettings().bInvertLook);
		ClickButton(Context, TEXT("Aus"));
		Context.Test->TestTrue(TEXT("The switch inverts the view"), Context.Player->GetSettings().bInvertLook);
	}));

	// Back on the main page the arena still restarts right away.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		ClickButton(Context, TEXT("Zur\u00FCck"));
		ClickButton(Context, TEXT("Neustart"));
	}));
	Steps.Add(Wait(0.3));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("A restart closes the menu"), Context.Player->IsPauseOpen());
		Context.Test->TestTrue(TEXT("The hero still stands there"), Context.Hero != nullptr && !Context.Hero->IsDead());
	}));

	// In the arena R keeps its old meaning: everything anew, no pause menu.
	AddTap(Steps, EKeys::R);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("R does not open a pause menu in the arena"), Context.Player->IsPauseOpen());
	}));

	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIPauseRunTest,
	"Eisenseelen.Play.Pause.Run",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIPauseRunTest::RunTest(const FString& Parameters)
{
	TArray<FEIPlayStep> Steps;

	// The run waits at the dock.
	Steps.Add(WaitUntil(TEXT("the run stands at the dock"), 20.0, [](const FEIPlayContext& Context)
	{
		return Context.World->GetGameState<AEIGameState>()->GetRunPhase() == EEIRunPhase::AtDock;
	}));

	// One wrong press of R must not throw the run away. It only opens the pause menu.
	AddTap(Steps, EKeys::R);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("R opens the pause menu at the dock"), Context.Player->IsPauseOpen());
		Context.Test->TestEqual(TEXT("The run still stands at the dock"), Context.World->GetGameState<AEIGameState>()->GetRunPhase(), EEIRunPhase::AtDock);
		BuildMenu(Context);
		Context.Test->TestNull(TEXT("At the dock the run cannot be left"), Context.Player->GetPauseMenu().FindButton(TEXT("Lauf verlassen")));
	}));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		ClickButton(Context, TEXT("Fortsetzen"));
		Context.Test->TestFalse(TEXT("Keep playing closes the menu"), Context.Player->IsPauseOpen());
	}));

	// Out in the land the menu offers leaving, and leaving fails the run.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		SwitchOffDirector(Context);
		Context.World->GetAuthGameMode<AEIGameMode>()->GetRun()->BeginRunNow();
	}));
	Steps.Add(WaitUntil(TEXT("the team arrives"), 30.0, [](const FEIPlayContext& Context)
	{
		return Context.World->GetGameState<AEIGameState>()->GetRunPhase() == EEIRunPhase::Exploring;
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The hero lives"), !Context.Hero->IsDead());
	}));

	// Out in the land Esc opens the menu, and the menu offers leaving.
	AddTap(Steps, EKeys::Escape);
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("Esc opens the pause menu in the land"), Context.Player->IsPauseOpen());
		BuildMenu(Context);
		Context.Test->TestNotNull(TEXT("The run can be left"), Context.Player->GetPauseMenu().FindButton(TEXT("Lauf verlassen")));
	}));

	// Leaving fails the run, as if the hero had fallen.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		ClickButton(Context, TEXT("Lauf verlassen"));
	}));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Leaving fails the run"), Context.World->GetGameState<AEIGameState>()->GetRunPhase(), EEIRunPhase::Failed);
		Context.Test->TestFalse(TEXT("The menu is closed after leaving"), Context.Player->IsPauseOpen());
		Context.Test->TestTrue(TEXT("The hero still lives after leaving"), !Context.Hero->IsDead());
	}));

	return RunInMap(this, RunMap, MoveTemp(Steps));
}

#endif

#endif
