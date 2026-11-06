#include "EIPauseMenu.h"

#include "EIGameState.h"
#include "EIPlayerController.h"
#include "EISettings.h"

namespace
{
	const FLinearColor PageColor(0.01f, 0.01f, 0.015f, 0.97f);
	const FLinearColor RowColor(1.0f, 1.0f, 1.0f, 0.06f);
	const FLinearColor RowHoverColor(1.0f, 1.0f, 1.0f, 0.16f);
	const FLinearColor TitleColor(0.95f, 0.95f, 0.92f);
	const FLinearColor NormalColor(0.85f, 0.85f, 0.82f);
	const FLinearColor TurnColor(1.0f, 0.75f, 0.25f);
	constexpr float PageMargin = 160.0f;
	constexpr float PageWidth = 620.0f;
	constexpr float TitleDrop = 56.0f;
	constexpr float RowHeight = 52.0f;
	constexpr float RowGap = 8.0f;
	constexpr float KnobWidth = 54.0f;
	constexpr float ValueWidth = 170.0f;
	constexpr float TextDrop = 16.0f;
}

void FEIPauseMenu::AddText(float X, float Y, const FString& Text, const FLinearColor& Color, float Scale)
{
	FEIMenuElement Element;
	Element.X = X;
	Element.Y = Y;
	Element.Text = Text;
	Element.TextColor = Color;
	Element.TextScale = Scale;
	Elements.Add(MoveTemp(Element));
}

void FEIPauseMenu::AddBox(float X, float Y, float Width, float Height, const FLinearColor& Color)
{
	FEIMenuElement Element;
	Element.X = X;
	Element.Y = Y;
	Element.Width = Width;
	Element.Height = Height;
	Element.BoxColor = Color;
	Elements.Add(MoveTemp(Element));
}

void FEIPauseMenu::AddButton(float X, float Y, float Width, float Height, const FString& Text, bool bActive, float MouseX, float MouseY, TFunction<void()> OnClick)
{
	FEIMenuElement Element;
	Element.X = X;
	Element.Y = Y;
	Element.Width = Width;
	Element.Height = Height;
	Element.Text = Text;
	Element.TextColor = bActive ? TurnColor : NormalColor;
	Element.BoxColor = Element.Contains(MouseX, MouseY) ? RowHoverColor : RowColor;
	Element.OnClick = MoveTemp(OnClick);
	Elements.Add(MoveTemp(Element));
}

void FEIPauseMenu::AddKnob(AEIPlayerController& Player, const FString& Name, const FString& ValueText, float Left, float Y, float MouseX, float MouseY, TFunction<void(int32)> Turn)
{
	const float KnobsX = Left + PageWidth - KnobWidth * 2.0f - RowGap;
	AddText(Left + 32.0f, Y + TextDrop, Name, NormalColor);
	AddText(KnobsX - RowGap - ValueWidth, Y + TextDrop, ValueText, NormalColor);
	AddButton(KnobsX, Y, KnobWidth, RowHeight, TEXT("-"), false, MouseX, MouseY, [&Player, Turn]() { Turn(-1); Player.OnSettingsChanged(); });
	AddButton(KnobsX + KnobWidth + RowGap, Y, KnobWidth, RowHeight, TEXT("+"), false, MouseX, MouseY, [&Player, Turn]() { Turn(1); Player.OnSettingsChanged(); });
}

void FEIPauseMenu::AddSwitch(AEIPlayerController& Player, const FString& Name, bool bOn, float Left, float Y, float MouseX, float MouseY, TFunction<void()> Toggle)
{
	const float SwitchX = Left + PageWidth - KnobWidth * 2.0f - ValueWidth - RowGap;
	AddButton(SwitchX, Y, KnobWidth * 2.0f + RowGap + ValueWidth, RowHeight, bOn ? TEXT("An") : TEXT("Aus"), bOn, MouseX, MouseY, [&Player, Toggle]() { Toggle(); Player.OnSettingsChanged(); });
	AddText(Left + 32.0f, Y + TextDrop, Name, NormalColor);
}

void FEIPauseMenu::Build(AEIPlayerController& Player, float ScreenWidth, float ScreenHeight, float MouseX, float MouseY)
{
	Elements.Reset();
	const AEIGameState* State = Player.GetWorld()->GetGameState<AEIGameState>();
	const EEIRunPhase Phase = State != nullptr ? State->GetRunPhase() : EEIRunPhase::None;
	const bool bLive = Phase == EEIRunPhase::Crossing || Phase == EEIRunPhase::Exploring || Phase == EEIRunPhase::SignalLit || Phase == EEIRunPhase::BargeWaiting;

	int32 Rows = 4;
	if (Page == EEIPausePage::Main && bLive)
	{
		Rows = 5;
	}
	else if (Page == EEIPausePage::Controls)
	{
		Rows = 10;
	}
	else if (Page == EEIPausePage::Settings)
	{
		Rows = 7;
	}
	const float Left = FMath::Max(PageMargin, ScreenWidth / 2.0f - PageWidth - 60.0f);
	const float PageHeight = TitleDrop + (RowHeight + RowGap) * Rows;
	const float Top = FMath::Max(PageMargin, (ScreenHeight - PageHeight) / 2.0f);
	AddBox(Left, Top, PageWidth, PageHeight, PageColor);
	AddText(Left + 32.0f, Top + 24.0f,
		Page == EEIPausePage::Main ? TEXT("Pause") : Page == EEIPausePage::Controls ? TEXT("Steuerung") : TEXT("Einstellungen"),
		TitleColor, 2.0f);

	switch (Page)
	{
	case EEIPausePage::Main:
		BuildMain(Player, Left, Top, PageWidth, MouseX, MouseY);
		break;
	case EEIPausePage::Controls:
		BuildControls(Left, Top);
		break;
	case EEIPausePage::Settings:
		BuildSettings(Player, Left, Top, PageWidth, MouseX, MouseY);
		break;
	}
}

void FEIPauseMenu::BuildMain(AEIPlayerController& Player, float Left, float Top, float Width, float MouseX, float MouseY)
{
	float Y = Top + TitleDrop;
	const float ButtonWidth = Width - 64.0f;
	const auto Row = [this, &Y, Left, ButtonWidth, MouseX, MouseY](const FString& Text, TFunction<void()> OnClick)
	{
		AddButton(Left + 32.0f, Y, ButtonWidth, RowHeight, Text, false, MouseX, MouseY, MoveTemp(OnClick));
		Y += RowHeight + RowGap;
	};
	Row(TEXT("Fortsetzen"), [&Player]() { Player.SetPauseOpen(false); });
	Row(TEXT("Steuerung"), [this]() { Page = EEIPausePage::Controls; });
	Row(TEXT("Einstellungen"), [this]() { Page = EEIPausePage::Settings; });
	Row(TEXT("Neustart"), [&Player]() { Player.RequestRestartRun(); });
	// Leaving only makes sense while the run is still going. After it is over the settlement says what to press.
	const AEIGameState* State = Player.GetWorld()->GetGameState<AEIGameState>();
	const EEIRunPhase Phase = State != nullptr ? State->GetRunPhase() : EEIRunPhase::None;
	const bool bLive = Phase == EEIRunPhase::Crossing || Phase == EEIRunPhase::Exploring || Phase == EEIRunPhase::SignalLit || Phase == EEIRunPhase::BargeWaiting;
	if (bLive)
	{
		Row(TEXT("Lauf verlassen"), [&Player]() { Player.RequestLeaveRun(); });
	}
}

void FEIPauseMenu::BuildControls(float Left, float Top)
{
	float Y = Top + TitleDrop;
	const auto Line = [this, &Y, Left](const TCHAR* Keys, const TCHAR* What)
	{
		AddText(Left + 32.0f, Y + TextDrop, FString::Printf(TEXT("%s   %s"), Keys, What), NormalColor);
		Y += RowHeight * 0.75f;
	};
	Line(TEXT("WASD"), TEXT("Laufen"));
	Line(TEXT("Maus"), TEXT("Umsehen"));
	Line(TEXT("Shift (halten)"), TEXT("Sprinten"));
	Line(TEXT("Leertaste"), TEXT("Springen"));
	Line(TEXT("Linke Maustaste"), TEXT("Angreifen (Klingenwacht: halten f\u00FCr schweren Hieb)"));
	Line(TEXT("Rechte Maustaste (halten)"), TEXT("Arkanist: Zauber   Klingenwacht: blocken, kurz vor dem Treffer: Parade"));
	Line(TEXT("Q"), TEXT("Zauber der Klasse"));
	Line(TEXT("E"), TEXT("Zauber der Rune"));
	Line(TEXT("F"), TEXT("Benutzen: Barke, Signalfeuer, Schmiede, Jagdbrett"));
	Line(TEXT("C"), TEXT("Zwischen Klingenwacht und Arkanist wechseln"));
	Line(TEXT("I"), TEXT("Ausr\u00FCstung, Schmiede, Beutebuch, Gravuren"));
	Line(TEXT("M"), TEXT("Karte der Vertr\u00E4ge"));
	Line(TEXT("H"), TEXT("Testarena: Horde an und aus"));
	Line(TEXT("R"), TEXT("Testarena: Neustart. Im Lauf: dieses Men\u00FC"));
	Line(TEXT("Esc"), TEXT("Dieses Men\u00FC"));
}

void FEIPauseMenu::BuildSettings(AEIPlayerController& Player, float Left, float Top, float Width, float MouseX, float MouseY)
{
	FEISettings& Settings = Player.GetSettings();
	float Y = Top + TitleDrop;
	AddKnob(Player, TEXT("Maus-Empfindlichkeit"), FString::Printf(TEXT("x%.1f"), Settings.MouseFactor), Left, Y, MouseX, MouseY, [&Settings](int32 Steps) { Settings.AddMouseFactor(Steps); });
	Y += RowHeight + RowGap;
	AddKnob(Player, TEXT("Effekte-Lautst\u00E4rke"), FString::Printf(TEXT("%.0f %%"), Settings.EffectsVolume * 100.0f), Left, Y, MouseX, MouseY, [&Settings](int32 Steps) { Settings.AddEffectsVolume(Steps); });
	Y += RowHeight + RowGap;
	AddKnob(Player, TEXT("Musik-Lautst\u00E4rke"), FString::Printf(TEXT("%.0f %%"), Settings.MusicVolume * 100.0f), Left, Y, MouseX, MouseY, [&Settings](int32 Steps) { Settings.AddMusicVolume(Steps); });
	Y += RowHeight + RowGap;
	AddSwitch(Player, TEXT("Blick invertieren"), Settings.bInvertLook, Left, Y, MouseX, MouseY, [&Settings]() { Settings.bInvertLook = !Settings.bInvertLook; });
	Y += RowHeight + RowGap;
	AddSwitch(Player, TEXT("HUD anzeigen"), Settings.bHudOn, Left, Y, MouseX, MouseY, [&Settings]() { Settings.bHudOn = !Settings.bHudOn; });
	Y += RowHeight + RowGap;
	AddSwitch(Player, TEXT("Schadenszahlen"), Settings.bDamageNumbers, Left, Y, MouseX, MouseY, [&Settings]() { Settings.bDamageNumbers = !Settings.bDamageNumbers; });
	Y += RowHeight + RowGap;
	AddButton(Left + 32.0f, Y, Width - 64.0f, RowHeight, TEXT("Zur\u00FCck"), false, MouseX, MouseY, [this]() { Page = EEIPausePage::Main; });
}

void FEIPauseMenu::Click(float X, float Y)
{
	for (int32 Index = Elements.Num() - 1; Index >= 0; --Index)
	{
		if (Elements[Index].OnClick && Elements[Index].Contains(X, Y))
		{
			Elements[Index].OnClick();
			return;
		}
	}
}

const FEIMenuElement* FEIPauseMenu::FindButton(const FString& Text) const
{
	return Elements.FindByPredicate([&Text](const FEIMenuElement& Element) { return Element.OnClick && Element.Text.Contains(Text); });
}

bool FEIPauseMenu::Shows(const FString& Text) const
{
	return Elements.ContainsByPredicate([&Text](const FEIMenuElement& Element) { return Element.Text.Contains(Text); });
}
