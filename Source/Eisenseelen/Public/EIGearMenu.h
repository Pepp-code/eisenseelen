#pragma once

#include "CoreMinimal.h"
#include "EIItems.h"

class AEIHeroCharacter;
class UEIProfileComponent;

/** One thing on the menu page: a text, a box or a button. A width of 0 means there is no box. */
struct FEIMenuElement
{
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
	FString Text;
	FLinearColor TextColor = FLinearColor::White;
	FLinearColor BoxColor = FLinearColor::Transparent;
	float TextScale = 1.0f;

	/** Empty for what cannot be clicked. */
	TFunction<void()> OnClick;

	bool Contains(float PointX, float PointY) const;
};

/** The three pages of the menu, chosen by the tabs at its top. */
enum class EEIMenuPage : uint8
{
	Gear,
	Forge,
	LootBook
};

/**
 * The menu with its three pages. The gear page has the seven slots on the left, what fits the chosen slot in the middle,
 * and the sum of all values on the right. The forge shows what the level opens and gold buys. It only makes a list of
 * what is on the page, and the HUD draws that list. So it can be tested without a screen, and all it changes goes through
 * the player's profile.
 */
class EISENSEELEN_API FEIGearMenu
{
public:
	/** Makes the page anew. The mouse only decides which row lights up and which item gets compared. */
	void Build(UEIProfileComponent& Profile, const AEIHeroCharacter& Hero, float ScreenWidth, float ScreenHeight, float MouseX, float MouseY);

	const TArray<FEIMenuElement>& GetElements() const { return Elements; }

	/** Clicks whatever is at this point on the page as it was last made. */
	void Click(float X, float Y);

	/** The first button with this text in it. Null if there is none. */
	const FEIMenuElement* FindButton(const FString& Text) const;

	/** Whether any element shows this text. */
	bool Shows(const FString& Text) const;

	EEISlot GetSelectedSlot() const { return SelectedSlot; }
	void SelectSlot(EEISlot Slot) { SelectedSlot = Slot; }

	EEIMenuPage GetPage() const { return Page; }
	void ShowPage(EEIMenuPage NewPage) { Page = NewPage; }

	/** Whether the loot book shows instead of the gear. */
	bool IsShowingLootBook() const { return Page == EEIMenuPage::LootBook; }
	void ShowLootBook(bool bShow) { Page = bShow ? EEIMenuPage::LootBook : EEIMenuPage::Gear; }

private:
	void AddText(float X, float Y, const FString& Text, const FLinearColor& Color, float Scale = 1.0f);
	void AddBox(float X, float Y, float Width, float Height, const FLinearColor& Color);
	void AddButton(float X, float Y, float Width, float Height, const FString& Text, const FLinearColor& TextColor, bool bActive, float MouseX, float MouseY, TFunction<void()> OnClick);

	void BuildTabs(float Left, float Top, float MouseX, float MouseY);
	void BuildPurse(const UEIProfileComponent& Profile, float X, float Y);
	void BuildLootBook(const UEIProfileComponent& Profile, float Left, float Top, float InnerWidth);
	void BuildSlots(UEIProfileComponent& Profile, float X, float Y, float Width, float MouseX, float MouseY);
	int32 BuildChoices(UEIProfileComponent& Profile, float X, float Y, float Width, float MouseX, float MouseY);
	void BuildSums(const UEIProfileComponent& Profile, const AEIHeroCharacter& Hero, float X, float Y);
	void BuildComparison(const UEIProfileComponent& Profile, const AEIHeroCharacter& Hero, int32 Item, float X, float Y);
	void BuildLevel(const UEIProfileComponent& Profile, float X, float Bottom);
	void BuildForge(UEIProfileComponent& Profile, const AEIHeroCharacter& Hero, float Left, float Top, float InnerWidth, float Bottom, float MouseX, float MouseY);
	void BuildForgeSlots(const UEIProfileComponent& Profile, float X, float Y, float Width, float MouseX, float MouseY);
	int32 BuildOffers(UEIProfileComponent& Profile, float X, float Y, float Width, float MouseX, float MouseY);

	TArray<FEIMenuElement> Elements;
	EEISlot SelectedSlot = EEISlot::Weapon;
	EEIMenuPage Page = EEIMenuPage::Gear;
};
