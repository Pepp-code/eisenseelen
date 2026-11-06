#pragma once

#include "CoreMinimal.h"
#include "EIGearMenu.h"

class AEIPlayerController;

/** The pages of the pause menu. */
enum class EEIPausePage : uint8
{
	Main,
	Controls,
	Settings
};

/**
 * The pause menu on Esc: keep playing, see the keys, change the settings, start anew or leave the run.
 * Like the gear menu it only makes a list of what is on the page, and the HUD draws that list.
 * Every change goes through the player controller, so it works without a screen and can be tested.
 */
class EISENSEELEN_API FEIPauseMenu
{
public:
	/** Makes the page anew. The mouse only decides which row lights up. */
	void Build(AEIPlayerController& Player, float ScreenWidth, float ScreenHeight, float MouseX, float MouseY);

	const TArray<FEIMenuElement>& GetElements() const { return Elements; }

	/** Clicks whatever is at this point on the page as it was last made. */
	void Click(float X, float Y);

	/** The first button with this text in it. Null if there is none. */
	const FEIMenuElement* FindButton(const FString& Text) const;

	/** Whether any element shows this text. */
	bool Shows(const FString& Text) const;

	EEIPausePage GetPage() const { return Page; }
	void ShowPage(EEIPausePage NewPage) { Page = NewPage; }

private:
	void AddText(float X, float Y, const FString& Text, const FLinearColor& Color, float Scale = 1.0f);
	void AddBox(float X, float Y, float Width, float Height, const FLinearColor& Color);
	void AddButton(float X, float Y, float Width, float Height, const FString& Text, bool bActive, float MouseX, float MouseY, TFunction<void()> OnClick);
	void AddKnob(AEIPlayerController& Player, const FString& Name, const FString& ValueText, float Left, float Y, float MouseX, float MouseY, TFunction<void(int32)> Turn);
	void AddSwitch(AEIPlayerController& Player, const FString& Name, bool bOn, float Left, float Y, float MouseX, float MouseY, TFunction<void()> Toggle);
	void BuildMain(AEIPlayerController& Player, float Left, float Top, float Width, float MouseX, float MouseY);
	void BuildControls(float Left, float Top);
	void BuildSettings(AEIPlayerController& Player, float Left, float Top, float Width, float MouseX, float MouseY);

	TArray<FEIMenuElement> Elements;
	EEIPausePage Page = EEIPausePage::Main;
};
