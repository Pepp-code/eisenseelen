#include "EIGearMenu.h"

#include "EICasterComponent.h"
#include "EIGearComponent.h"
#include "EIHeroCharacter.h"
#include "EIProfileComponent.h"
#include "EIRuneComponent.h"

namespace
{
	const FLinearColor PageColor(0.01f, 0.01f, 0.015f, 0.97f);
	const FLinearColor RowColor(1.0f, 1.0f, 1.0f, 0.06f);
	const FLinearColor RowHoverColor(1.0f, 1.0f, 1.0f, 0.16f);
	const FLinearColor RowActiveColor(1.0f, 0.75f, 0.25f, 0.28f);
	const FLinearColor TitleColor(0.95f, 0.95f, 0.92f);
	const FLinearColor NormalColor(0.85f, 0.85f, 0.82f);
	const FLinearColor QuietColor(0.6f, 0.6f, 0.58f);
	const FLinearColor BetterColor(0.45f, 0.9f, 0.45f);
	const FLinearColor WorseColor(1.0f, 0.45f, 0.35f);
	constexpr float PageMargin = 40.0f;
	constexpr float PagePadding = 24.0f;
	constexpr float TitleScale = 1.6f;
	constexpr float HeadHeight = 64.0f;
	constexpr float RowHeight = 34.0f;
	constexpr float RowGap = 4.0f;
	constexpr float LineHeight = 24.0f;
	constexpr float TextInset = 10.0f;
	constexpr float TextDrop = 6.0f;
	constexpr float SlotsShare = 0.33f;
	constexpr float ChoicesShare = 0.33f;
	constexpr float ColumnGap = 22.0f;
	constexpr float NearlyNothing = 0.0001f;
	constexpr float PointButtonWidth = 122.0f;
	constexpr float PageButtonWidth = 160.0f;
	constexpr int32 BookColumns = 3;
	constexpr float BookNameScale = 1.25f;
	constexpr float BossNameBrighten = 0.4f;
	constexpr float BookChanceShare = 0.55f;
	constexpr float BookStockShare = 0.76f;

	/** The values of an item at a quality, added to what stands in the array. */
	void AddItemStats(const FEIItemTable& Table, int32 Item, int32 Quality, float Sign, float (&Sums)[EIStatCount])
	{
		if (Table.Items.IsValidIndex(Item))
		{
			for (const FEIStatLine& Line : Table.Items[Item].Stats)
			{
				Sums[static_cast<int32>(Line.Stat)] += Sign * Line.Amount * Table.GetQualityFactor(Quality);
			}
		}
	}

	FString ItemLabel(const FEIItemTable& Table, const FEIWornItem& Worn)
	{
		return Table.Items.IsValidIndex(Worn.Item) ? FString::Printf(TEXT("%s (%s)"), *Table.Items[Worn.Item].Name, *Table.GetQualityName(Worn.Quality)) : FString(TEXT("leer"));
	}
}

bool FEIMenuElement::Contains(float PointX, float PointY) const
{
	return PointX >= X && PointX <= X + Width && PointY >= Y && PointY <= Y + Height;
}

void FEIGearMenu::AddText(float X, float Y, const FString& Text, const FLinearColor& Color, float Scale)
{
	FEIMenuElement Element;
	Element.X = X;
	Element.Y = Y;
	Element.Text = Text;
	Element.TextColor = Color;
	Element.TextScale = Scale;
	Elements.Add(MoveTemp(Element));
}

void FEIGearMenu::AddBox(float X, float Y, float Width, float Height, const FLinearColor& Color)
{
	FEIMenuElement Element;
	Element.X = X;
	Element.Y = Y;
	Element.Width = Width;
	Element.Height = Height;
	Element.BoxColor = Color;
	Elements.Add(MoveTemp(Element));
}

void FEIGearMenu::AddButton(float X, float Y, float Width, float Height, const FString& Text, const FLinearColor& TextColor, bool bActive, float MouseX, float MouseY, TFunction<void()> OnClick)
{
	FEIMenuElement Element;
	Element.X = X;
	Element.Y = Y;
	Element.Width = Width;
	Element.Height = Height;
	Element.Text = Text;
	Element.TextColor = TextColor;
	Element.BoxColor = bActive ? RowActiveColor : (Element.Contains(MouseX, MouseY) ? RowHoverColor : RowColor);
	Element.OnClick = MoveTemp(OnClick);
	Elements.Add(MoveTemp(Element));
}

void FEIGearMenu::Build(UEIProfileComponent& Profile, const AEIHeroCharacter& Hero, float ScreenWidth, float ScreenHeight, float MouseX, float MouseY)
{
	Elements.Reset();
	const float PageX = PageMargin;
	const float PageY = PageMargin;
	const float PageWidth = ScreenWidth - PageMargin * 2.0f;
	const float PageHeight = ScreenHeight - PageMargin * 2.0f;
	AddBox(PageX, PageY, PageWidth, PageHeight, PageColor);

	const float Left = PageX + PagePadding;
	const float Top = PageY + PagePadding;
	const float InnerWidth = PageWidth - PagePadding * 2.0f;
	AddButton(Left + InnerWidth - PageButtonWidth, Top, PageButtonWidth, RowHeight, bShowLootBook ? TEXT("Ausrüstung") : TEXT("Beutebuch"), NormalColor, false, MouseX, MouseY,
		[this]() { bShowLootBook = !bShowLootBook; });
	if (bShowLootBook)
	{
		BuildLootBook(Profile, Left, Top, InnerWidth);
		return;
	}

	const bool bArcanist = Profile.GetCurrentClass() == EEIHeroClass::Arcanist;
	AddText(Left, Top, FString::Printf(TEXT("Ausrüstung: %s"), bArcanist ? TEXT("Arkanist") : TEXT("Klingenwacht")), TitleColor, TitleScale);
	const FString Hint = Profile.IsFreeChoice()
		? TEXT("Testarena: alles ist frei und zählt nur hier.   C wechselt die Klasse   I schließt")
		: TEXT("C wechselt die Klasse   I schließt");
	AddText(Left, Top + LineHeight * 1.5f, Hint, QuietColor);

	const float ColumnsTop = Top + HeadHeight;
	const float SlotsWidth = InnerWidth * SlotsShare;
	const float ChoicesX = Left + SlotsWidth + ColumnGap;
	const float ChoicesWidth = InnerWidth * ChoicesShare;
	const float SumsX = ChoicesX + ChoicesWidth + ColumnGap;
	if (!Profile.IsFreeChoice())
	{
		FString Purse = FString::Printf(TEXT("Gold %d"), Profile.GetSave().Gold);
		for (const FEIFaction& Faction : Profile.GetContractTable().Factions)
		{
			Purse += FString::Printf(TEXT("   %s %d"), *Faction.Name, Profile.GetSave().FactionPoints.FindRef(Faction.Name));
		}
		AddText(ChoicesX, Top, Purse, TitleColor);
		FString Ingredients;
		for (const TPair<FString, int32>& Pair : Profile.GetSave().Ingredients)
		{
			Ingredients += FString::Printf(TEXT("%s%s %d"), Ingredients.IsEmpty() ? TEXT("Zutaten: ") : TEXT(", "), *Pair.Key, Pair.Value);
		}
		if (!Ingredients.IsEmpty())
		{
			AddText(ChoicesX, Top + LineHeight * 1.5f, Ingredients, NormalColor);
		}
	}
	BuildSlots(Profile, Left, ColumnsTop, SlotsWidth, MouseX, MouseY);
	const int32 Hovered = BuildChoices(Profile, ChoicesX, ColumnsTop, ChoicesWidth, MouseX, MouseY);
	BuildSums(Profile, Hero, SumsX, ColumnsTop);
	BuildComparison(Profile, Hero, Hovered, Left, ColumnsTop + (RowHeight + RowGap) * EISlotCount + LineHeight * 2.0f);
	BuildAttributes(Profile, SumsX, PageY + PageHeight - PagePadding, MouseX, MouseY);

	if (Profile.IsFreeChoice())
	{
		const int32 Quality = Profile.GetSave().ArenaQuality;
		const int32 Count = FMath::Max(Profile.GetTable().QualityFactors.Num(), 1);
		UEIProfileComponent* ProfilePointer = &Profile;
		AddButton(Left, PageY + PageHeight - PagePadding - RowHeight, SlotsWidth, RowHeight, FString::Printf(TEXT("Qualität: %s"), *Profile.GetTable().GetQualityName(Quality)), NormalColor, false, MouseX, MouseY,
			[ProfilePointer, Quality, Count]() { ProfilePointer->SetArenaQuality((Quality + 1) % Count); });
	}
}

void FEIGearMenu::BuildSlots(UEIProfileComponent& Profile, float X, float Y, float Width, float MouseX, float MouseY)
{
	AddText(X, Y, TEXT("Plätze"), TitleColor);
	float RowY = Y + LineHeight * 1.25f;
	for (int32 Index = 0; Index < EISlotCount; ++Index)
	{
		const EEISlot Slot = static_cast<EEISlot>(Index);
		const FString Text = FString::Printf(TEXT("%s: %s"), EISlotName(Slot), *ItemLabel(Profile.GetTable(), Profile.GetEquipped(Slot)));
		AddButton(X, RowY, Width, RowHeight, Text, NormalColor, Slot == SelectedSlot, MouseX, MouseY, [this, Slot]() { SelectedSlot = Slot; });
		RowY += RowHeight + RowGap;
	}
}

int32 FEIGearMenu::BuildChoices(UEIProfileComponent& Profile, float X, float Y, float Width, float MouseX, float MouseY)
{
	const EEISlot Slot = SelectedSlot;
	const int32 Equipped = Profile.GetEquipped(Slot).Item;
	const FEIItemTable& Table = Profile.GetTable();
	UEIProfileComponent* ProfilePointer = &Profile;
	AddText(X, Y, FString::Printf(TEXT("Items für %s"), EISlotName(Slot)), TitleColor);
	float RowY = Y + LineHeight * 1.25f;
	int32 Hovered = INDEX_NONE;
	const TArray<FEIWornItem> Choices = Profile.ListChoices(Slot);
	if (Choices.IsEmpty())
	{
		AddText(X + TextInset, RowY + TextDrop, Slot == EEISlot::Crystal ? TEXT("Herzkristalle kommen später.") : TEXT("Noch nichts für diese Klasse gefunden."), QuietColor);
	}
	for (const FEIWornItem& Choice : Choices)
	{
		const int32 Item = Choice.Item;
		AddButton(X, RowY, Width, RowHeight, ItemLabel(Table, Choice), NormalColor, Item == Equipped, MouseX, MouseY, [ProfilePointer, Slot, Item]() { ProfilePointer->Equip(Slot, Item); });
		if (Elements.Last().Contains(MouseX, MouseY))
		{
			Hovered = Item;
		}
		RowY += RowHeight + RowGap;
	}
	// A hero always keeps a weapon, but everything else can come off again.
	if (Slot != EEISlot::Weapon && Equipped != INDEX_NONE)
	{
		AddButton(X, RowY, Width, RowHeight, TEXT("Ablegen"), QuietColor, false, MouseX, MouseY, [ProfilePointer, Slot]() { ProfilePointer->Equip(Slot, INDEX_NONE); });
	}
	return Hovered;
}

void FEIGearMenu::BuildSums(const UEIProfileComponent& Profile, const AEIHeroCharacter& Hero, float X, float Y)
{
	AddText(X, Y, TEXT("Werte"), TitleColor);
	float LineY = Y + LineHeight * 1.25f;
	const UEIGearComponent* Gear = Hero.GetGear();
	bool bAny = false;
	for (int32 Index = 0; Index < EIStatCount; ++Index)
	{
		const EEIStat Stat = static_cast<EEIStat>(Index);
		const float Amount = Gear->GetStat(Stat);
		if (FMath::Abs(Amount) > NearlyNothing)
		{
			AddText(X, LineY, EIDescribeStat(Stat, Amount), NormalColor);
			LineY += LineHeight;
			bAny = true;
		}
	}
	if (!bAny)
	{
		AddText(X, LineY, TEXT("Keine Boni"), QuietColor);
		LineY += LineHeight;
	}
	LineY += LineHeight / 2.0f;
	AddText(X, LineY, FString::Printf(TEXT("Zauber auf Q: %s"), *Hero.GetCaster()->GetSpell().Name), NormalColor);
	LineY += LineHeight;
	const FEIRune* Rune = Hero.GetRunes()->GetRune();
	AddText(X, LineY, FString::Printf(TEXT("Zauber auf E: %s"), Rune != nullptr ? *Rune->SpellName : TEXT("keiner")), NormalColor);
}

void FEIGearMenu::BuildComparison(const UEIProfileComponent& Profile, const AEIHeroCharacter& Hero, int32 Item, float X, float Y)
{
	const FEIItemTable& Table = Profile.GetTable();
	if (!Table.Items.IsValidIndex(Item))
	{
		return;
	}
	const FEIItem& Entry = Table.Items[Item];
	const FEIWornItem Worn = Profile.GetEquipped(SelectedSlot);
	AddText(X, Y, FString::Printf(TEXT("%s statt %s:"), *Entry.Name, Table.Items.IsValidIndex(Worn.Item) ? *Table.Items[Worn.Item].Name : TEXT("nichts")), TitleColor);
	float LineY = Y + LineHeight;

	if (const FEIWeaponForm* Form = Table.GetForm(Entry))
	{
		AddText(X, LineY, FString::Printf(TEXT("%s, Zauber auf Q: %s"), *Form->Name, *Hero.GetCaster()->GetValues().GetSpell(Entry.Element).Name), NormalColor);
		LineY += LineHeight;
	}
	else if (Entry.Slot == EEISlot::Rune)
	{
		const FEIRuneTable& Runes = Hero.GetRunes()->GetTable();
		const int32 Rune = Runes.FindByName(Entry.Name);
		AddText(X, LineY, FString::Printf(TEXT("Zauber auf E: %s"), Runes.Runes.IsValidIndex(Rune) ? *Runes.Runes[Rune].SpellName : TEXT("keiner")), NormalColor);
		LineY += LineHeight;
	}
	if (const FEIArmorSet* Set = Table.FindSet(Entry.Set))
	{
		for (const FEIStatLine& Line : Set->PerTwoPieces)
		{
			AddText(X, LineY, FString::Printf(TEXT("%s: je 2 Teile %s"), *Set->Name, *EIDescribeStat(Line.Stat, Line.Amount)), NormalColor);
			LineY += LineHeight;
		}
	}

	float Change[EIStatCount] = {};
	int32 Quality = 0;
	for (const FEIWornItem& Choice : Profile.ListChoices(SelectedSlot))
	{
		Quality = Choice.Item == Item ? Choice.Quality : Quality;
	}
	AddItemStats(Table, Item, Quality, 1.0f, Change);
	AddItemStats(Table, Worn.Item, Worn.Quality, -1.0f, Change);
	for (int32 Index = 0; Index < EIStatCount; ++Index)
	{
		if (FMath::Abs(Change[Index]) > NearlyNothing)
		{
			// Less cooldown and lower mana costs are the good direction.
			const EEIStat Stat = static_cast<EEIStat>(Index);
			const bool bLessIsMore = Stat == EEIStat::Cooldown || Stat == EEIStat::ManaCost;
			AddText(X, LineY, EIDescribeStat(Stat, Change[Index]), (Change[Index] > 0.0f) != bLessIsMore ? BetterColor : WorseColor);
			LineY += LineHeight;
		}
	}
}

void FEIGearMenu::BuildAttributes(UEIProfileComponent& Profile, float X, float Bottom, float MouseX, float MouseY)
{
	const FEIItemTable& Table = Profile.GetTable();
	const int32 Rows = Table.Attributes.Num();
	float RowY = Bottom - (RowHeight + RowGap) * Rows;
	const int32 Level = Profile.GetLevel();
	const FString Head = Profile.IsFreeChoice() || Level >= FMath::RoundToInt32(Table.MaxLevel)
		? FString::Printf(TEXT("Level %d, noch %d Punkte frei"), Level, Profile.GetFreePoints())
		: FString::Printf(TEXT("Level %d, %d von %d Erfahrung, noch %d Punkte frei"), Level, Profile.GetSave().Experience, Table.GetExperienceForLevel(Level + 1), Profile.GetFreePoints());
	AddText(X, RowY - LineHeight * 1.25f, Head, TitleColor);
	UEIProfileComponent* ProfilePointer = &Profile;
	for (int32 Index = 0; Index < Rows; ++Index)
	{
		const FEIAttribute& Attribute = Table.Attributes[Index];
		const int32 Points = Profile.GetAttributePoints(Index);
		AddButton(X, RowY, PointButtonWidth, RowHeight, FString::Printf(TEXT("- %s"), *Attribute.Name), NormalColor, false, MouseX, MouseY, [ProfilePointer, Index]() { ProfilePointer->ChangeAttribute(Index, -1); });
		AddButton(X + PointButtonWidth + RowGap, RowY, PointButtonWidth, RowHeight, FString::Printf(TEXT("+ %s"), *Attribute.Name), NormalColor, false, MouseX, MouseY, [ProfilePointer, Index]() { ProfilePointer->ChangeAttribute(Index, 1); });
		AddText(X + (PointButtonWidth + RowGap) * 2.0f + TextInset, RowY + TextDrop, FString::Printf(TEXT("%d: %s"), Points, *EIDescribeStat(Attribute.Stat, Attribute.PerPoint * Points)), NormalColor);
		RowY += RowHeight + RowGap;
	}
}

void FEIGearMenu::BuildLootBook(const UEIProfileComponent& Profile, float Left, float Top, float InnerWidth)
{
	const FEIBossTable& Bosses = Profile.GetBossTable();
	const FEISaveGame& Save = Profile.GetSave();
	int32 Found = 0;
	int32 Total = 0;
	for (const FEIBossValues& Boss : Bosses.Bosses)
	{
		for (const FEILootItem& Item : Boss.Loot)
		{
			Found += Profile.HasFound(Item.Name) ? 1 : 0;
			++Total;
		}
	}
	AddText(Left, Top, TEXT("Beutebuch"), TitleColor, TitleScale);
	AddText(Left, Top + LineHeight * 1.5f, FString::Printf(TEXT("%d von %d Zutaten gefunden. Was noch fehlt, steht als ???   I schließt"), Found, Total), QuietColor);
	if (Bosses.Bosses.IsEmpty())
	{
		AddText(Left, Top + HeadHeight, TEXT("Noch kein Boss bekannt."), QuietColor);
		return;
	}

	const float Width = (InnerWidth - ColumnGap * (BookColumns - 1)) / BookColumns;
	float RowTop = Top + HeadHeight;
	float RowBottom = RowTop;
	for (int32 Index = 0; Index < Bosses.Bosses.Num(); ++Index)
	{
		const int32 Column = Index % BookColumns;
		if (Index > 0 && Column == 0)
		{
			RowTop = RowBottom + LineHeight;
		}
		const FEIBossValues& Boss = Bosses.Bosses[Index];
		const float X = Left + (Width + ColumnGap) * Column;
		// Some bosses are dark, and the page is dark too.
		const FLinearColor NameColor = FMath::Lerp(Boss.Color, FLinearColor::White, BossNameBrighten).CopyWithNewOpacity(1.0f);
		AddText(X, RowTop, Boss.Name, NameColor, BookNameScale);
		AddText(X, RowTop + LineHeight * 1.5f, FString::Printf(TEXT("Bannbruch: %s"), *Boss.Bannbruch.Name), QuietColor);
		float LineY = RowTop + LineHeight * 2.75f;
		for (const FEILootItem& Item : Boss.Loot)
		{
			const bool bFound = Profile.HasFound(Item.Name);
			AddBox(X, LineY, Width, RowHeight, RowColor);
			AddText(X + TextInset, LineY + TextDrop, bFound ? Item.Name : TEXT("???"), bFound ? NormalColor : QuietColor);
			AddText(X + Width * BookChanceShare, LineY + TextDrop, Bosses.DescribeChance(Item.Chance), QuietColor);
			const int32 Stock = Save.Ingredients.FindRef(Item.Name);
			if (Stock > 0)
			{
				AddText(X + Width * BookStockShare, LineY + TextDrop, FString::Printf(TEXT("im Lager: %d"), Stock), NormalColor);
			}
			LineY += RowHeight + RowGap;
		}
		RowBottom = FMath::Max(RowBottom, LineY);
	}
}

void FEIGearMenu::Click(float X, float Y)
{
	for (int32 Index = Elements.Num() - 1; Index >= 0; --Index)
	{
		if (Elements[Index].OnClick && Elements[Index].Contains(X, Y))
		{
			// The click may change what the next page shows, so this one is done after it.
			const TFunction<void()> OnClick = Elements[Index].OnClick;
			OnClick();
			return;
		}
	}
}

const FEIMenuElement* FEIGearMenu::FindButton(const FString& Text) const
{
	return Elements.FindByPredicate([&Text](const FEIMenuElement& Element) { return Element.OnClick && Element.Text.Contains(Text); });
}

bool FEIGearMenu::Shows(const FString& Text) const
{
	return Elements.ContainsByPredicate([&Text](const FEIMenuElement& Element) { return Element.Text.Contains(Text); });
}
