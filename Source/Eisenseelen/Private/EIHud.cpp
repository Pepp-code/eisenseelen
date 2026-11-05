#include "EIHud.h"

#include "EIAnchorStone.h"
#include "EIBoss.h"
#include "EICasterComponent.h"
#include "EIChest.h"
#include "EIContracts.h"
#include "EIEnemy.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EIPlayerController.h"
#include "EIProfileComponent.h"
#include "EIRuneComponent.h"
#include "EISignalFire.h"
#include "EISkyBarge.h"
#include "EISwordComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const FLinearColor TextColor(0.95f, 0.95f, 0.92f);
	const FLinearColor LifeColor(0.75f, 0.12f, 0.1f);
	const FLinearColor RecoveringColor(0.3f, 0.8f, 0.35f);
	const FLinearColor RuneColor(0.55f, 0.75f, 1.0f);
	const FLinearColor ChargeColor(1.0f, 0.7f, 0.2f);
	const FLinearColor SpellColor(1.0f, 0.42f, 0.1f);
	const FLinearColor ManaColor(0.2f, 0.4f, 1.0f);
	const FLinearColor NoManaColor(0.9f, 0.1f, 0.1f);
	const FLinearColor BarBack(0.0f, 0.0f, 0.0f, 0.55f);
	constexpr float Margin = 24.0f;
	constexpr float BarWidth = 260.0f;
	constexpr float BarHeight = 16.0f;
	constexpr float CrosshairSize = 6.0f;
	const FLinearColor HurtColor(0.8f, 0.0f, 0.0f, 0.35f);
	const FLinearColor ThreatColor(1.0f, 0.15f, 0.05f);
	constexpr float ThreatRangeCm = 800.0f;
	constexpr float ThreatInViewDot = 0.6f;
	constexpr float ThreatMarkSize = 18.0f;
	constexpr float ThreatEdgeShare = 0.42f;
	constexpr float HitArrowSeconds = 1.5f;
	constexpr float HitArrowFromCenter = 130.0f;
	constexpr float HitArrowLength = 34.0f;
	constexpr float HitArrowThickness = 5.0f;
	constexpr float HitNoteFromBottom = 150.0f;
	constexpr float HitNoteWidth = 170.0f;
	constexpr float HitNoteLastSeconds = 3.0f;
	const FLinearColor FireMarkColor(1.0f, 0.55f, 0.1f);
	const FLinearColor BossMarkColor(1.0f, 0.2f, 0.25f);
	const FLinearColor AnchorMarkColor(0.75f, 0.45f, 1.0f);
	const FLinearColor BossLifeColor(0.75f, 0.1f, 0.15f);
	constexpr float BossBarWidth = 620.0f;
	constexpr float BossBarHeight = 18.0f;
	constexpr float BossBarY = 172.0f;
	constexpr float BossMarkAboveCm = 620.0f;
	constexpr double BossMarkFromCm = 3000.0;
	constexpr float CrossingFadePerSecond = 2.0f;
	const FLinearColor TextShadowColor(0.0f, 0.0f, 0.0f, 0.75f);
	constexpr float TextShadowOffset = 1.5f;
	constexpr float ContractMarkAboveCm = 220.0f;
	const FLinearColor RewardColor(1.0f, 0.85f, 0.45f);
	const FLinearColor BargeMarkColor(0.5f, 0.85f, 1.0f);
	const FLinearColor WarningTextColor(1.0f, 0.25f, 0.15f);
	constexpr float WorldMarkSize = 10.0f;
	const FLinearColor PanelColor(0.0f, 0.0f, 0.0f, 0.72f);
	const FLinearColor LostColor(0.65f, 0.65f, 0.65f);
	constexpr float PanelWidth = 760.0f;
	constexpr float PanelLine = 30.0f;
	constexpr float PanelPadding = 26.0f;
	constexpr float RewardNoteY = 230.0f;
	constexpr float MenuTextInset = 10.0f;
	constexpr float MenuTextDrop = 6.0f;
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float BoardMapSize = 300.0f;
	constexpr float BoardListWidth = 400.0f;
	constexpr float BoardY = 110.0f;
	constexpr float BoardLine = 22.0f;
	constexpr float BoardMarkSize = 12.0f;
	constexpr float BoardHeroSize = 8.0f;
	constexpr float BoardHeroAheadDistance = 9.0f;
	constexpr float BoardHeroAheadSize = 4.0f;
	const FLinearColor BoardFloorColor(0.14f, 0.14f, 0.16f, 0.9f);
}

void AEIHud::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color)
{
	DrawRect(BarBack, X, Y, Width, Height);
	DrawRect(Color, X, Y, Width * FMath::Clamp(Fraction, 0.0f, 1.0f), Height);
}

void AEIHud::DrawLine(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale)
{
	// A dark copy behind it keeps the text readable on a bright floor.
	DrawText(Text, TextShadowColor, X + TextShadowOffset, Y + TextShadowOffset, GEngine->GetMediumFont(), Scale);
	DrawText(Text, Color, X, Y, GEngine->GetMediumFont(), Scale);
}

void AEIHud::DrawHUD()
{
	Super::DrawHUD();
	DrawScreen();
	if (const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>())
	{
		DrawCrossing(State);
	}
}

void AEIHud::DrawCrossing(const AEIGameState* State)
{
	const bool bCrossing = State->GetRunPhase() == EEIRunPhase::Crossing;
	CrossingFade = FMath::FInterpConstantTo(CrossingFade, bCrossing ? 1.0f : 0.0f, RenderDelta, CrossingFadePerSecond);
	if (CrossingFade <= 0.0f)
	{
		return;
	}
	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, CrossingFade), 0.0f, 0.0f, Canvas->SizeX, Canvas->SizeY);
	if (bCrossing && CrossingFade >= 1.0f)
	{
		DrawLine(TEXT("Die Barke bringt euch ins Gebiet ..."), Canvas->SizeX / 2.0f - 220.0f, Canvas->SizeY / 2.0f - 20.0f, TextColor, 2.0f);
	}
}

void AEIHud::DrawScreen()
{
	const AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(GetOwningPawn());
	const float Width = Canvas->SizeX;
	const float Height = Canvas->SizeY;
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const bool bHasRun = State != nullptr && State->GetRunPhase() != EEIRunPhase::None;
	const FEIRune* Rune = Hero != nullptr ? Hero->GetRunes()->GetRune() : nullptr;
	const FString RuneKey = Rune != nullptr ? FString::Printf(TEXT("E %s   "), *Rune->SpellName) : FString();
	const FString SpellName = Hero != nullptr ? Hero->GetCaster()->GetSpell().Name : FString();
	AEIPlayerController* Player = Cast<AEIPlayerController>(PlayerOwner);
	if (Hero != nullptr && Player != nullptr && Player->IsMenuOpen())
	{
		DrawMenu(Player, *Hero);
		return;
	}
	const TCHAR* GearKey = Player != nullptr && Player->GetProfile()->CanChangeGear() ? TEXT("I Ausr\u00FCstung   ") : TEXT("");
	DrawLine(FString::Printf(TEXT("WASD laufen   Shift sprinten   Leertaste springen   Q %s   %sF benutzen   C Klasse   %s%sR Neustart"), *SpellName, *RuneKey, GearKey, bHasRun ? TEXT("M Karte   ") : TEXT("H Horde   ")), Margin, Margin, TextColor);
	if (Hero == nullptr)
	{
		return;
	}

	int32 Enemies = 0;
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		Enemies += It->IsDead() ? 0 : 1;
	}
	const bool bHorde = State != nullptr && State->IsHordeActive();
	const bool bArcanist = Hero->GetHeroClass() == EEIHeroClass::Arcanist;
	const TCHAR* ClassName = bArcanist ? TEXT("Arkanist") : TEXT("Klingenwacht");
	const TCHAR* HordeNote = bHasRun ? TEXT("") : (bHorde ? TEXT("Horde an   ") : TEXT("Horde aus   "));
	const FString ClassKeys = bArcanist ? FString::Printf(TEXT("Linke Maus Schuss   Rechte Maus halten: %s"), *SpellName) : FString(TEXT("Linke Maus Hieb (halten: schwerer Hieb)   Rechte Maus halten: blocken, kurz vor dem Treffer: Parade"));
	// On the run map most enemies stand far away in their camps, so that number would say nothing.
	const FString EnemyNote = bHasRun ? FString() : FString::Printf(TEXT("   %sGegner %d"), HordeNote, Enemies);
	const int32 Level = Player != nullptr ? Player->GetProfile()->GetLevel() : 1;
	DrawLine(FString::Printf(TEXT("%s, Level %d   %s   %s%s"), ClassName, Level, Rune != nullptr ? *Rune->Name : TEXT("Keine Rune"), *ClassKeys, *EnemyNote), Margin, Margin + 24.0f, TextColor);

	if (Hero->GetHurtFlash() > 0.0f)
	{
		FLinearColor Flash = HurtColor;
		Flash.A *= FMath::Min(1.0f, Hero->GetHurtFlash() / 0.35f);
		DrawRect(Flash, 0.0f, 0.0f, Width, Height);
	}
	if (bHasRun)
	{
		DrawRun(Hero, State);
	}
	if (Hero->IsDead())
	{
		// Once all have fallen the settlement says what happened.
		if (State == nullptr || State->GetRunPhase() != EEIRunPhase::Failed)
		{
			DrawLine(TEXT("Gefallen. R startet neu."), Width / 2.0f - 170.0f, Height / 2.0f - 20.0f, TextColor, 2.0f);
		}
		return;
	}
	DrawThreats(Hero);
	DrawLastHit(Hero);

	DrawRect(TextColor, Width / 2.0f - CrosshairSize / 2.0f, Height / 2.0f - CrosshairSize / 2.0f, CrosshairSize, CrosshairSize);

	const UEISwordComponent* Sword = Hero->GetSword();
	if (Sword->GetBlockFlash() > 0.0f)
	{
		DrawLine(Sword->WasParry() ? TEXT("Parade!") : TEXT("Geblockt"), Width / 2.0f - 50.0f, Height / 2.0f + 60.0f, TextColor, 1.5f);
	}

	const UEIHealthComponent* Health = Hero->GetHealth();
	const float LifeY = Height - Margin - BarHeight;
	DrawBar(Margin, LifeY, BarWidth, BarHeight, Health->GetHealth() / FMath::Max(Health->GetMaxHealth(), 1.0f), Health->IsRecovering() ? RecoveringColor : LifeColor);
	DrawLine(FString::Printf(TEXT("Leben %.0f / %.0f%s"), Health->GetHealth(), Health->GetMaxHealth(), Health->IsRecovering() ? TEXT("   erholt sich") : TEXT("")), Margin, LifeY - 24.0f, TextColor);

	const float Speed = static_cast<float>(Hero->GetVelocity().Size2D()) / 100.0f;
	DrawLine(FString::Printf(TEXT("%.1f m/s"), Speed), Margin + BarWidth + Margin + BarWidth / 2.0f + Margin, LifeY - 8.0f, TextColor);

	const UEICasterComponent* Caster = Hero->GetCaster();
	const float SpellX = Margin + BarWidth + Margin;
	const float SpellY = LifeY - 56.0f;
	DrawBar(SpellX, SpellY, BarWidth / 2.0f, BarHeight, 1.0f - Caster->GetSpellCooldownFraction(), SpellColor);
	DrawLine(FString::Printf(TEXT("Q %s"), *SpellName), SpellX, SpellY - 24.0f, TextColor);
	if (Rune != nullptr)
	{
		const float RuneX = SpellX + BarWidth / 2.0f + Margin;
		DrawBar(RuneX, SpellY, BarWidth / 2.0f, BarHeight, 1.0f - Hero->GetRunes()->GetCooldownFraction(), RuneColor);
		DrawLine(FString::Printf(TEXT("E %s"), *Rune->SpellName), RuneX, SpellY - 24.0f, TextColor);
	}

	if (Caster->GetMaxMana() > 0.0f)
	{
		DrawBar(Margin, SpellY, BarWidth, BarHeight, Caster->GetMana() / Caster->GetMaxMana(), ManaColor);
		if (Caster->GetManaDeniedFlash() > 0.0f)
		{
			DrawRect(NoManaColor, Margin, SpellY, BarWidth, BarHeight);
		}
		DrawLine(FString::Printf(TEXT("Mana %.0f / %.0f"), Caster->GetMana(), Caster->GetMaxMana()), Margin, SpellY - 24.0f, TextColor);
	}

	const float Charge = bArcanist ? Caster->GetChargeFraction() : Hero->GetSword()->GetChargeFraction();
	if (Charge > 0.25f)
	{
		const FLinearColor ChargeBarColor = bArcanist && !Caster->CanAffordSpell() ? NoManaColor : (Charge >= 1.0f ? FLinearColor::White : ChargeColor);
		DrawBar(Width / 2.0f - 60.0f, Height / 2.0f + 40.0f, 120.0f, 8.0f, Charge, ChargeBarColor);
	}
}

void AEIHud::DrawWorldMark(const FVector& Location, const FString& Label, const FLinearColor& Color)
{
	const FVector OnScreen = Project(Location);
	// Behind the camera the projection has no depth.
	if (OnScreen.Z <= 0.0)
	{
		return;
	}
	DrawRect(Color, OnScreen.X - WorldMarkSize / 2.0f, OnScreen.Y - WorldMarkSize / 2.0f, WorldMarkSize, WorldMarkSize);
	DrawLine(Label, OnScreen.X + WorldMarkSize, OnScreen.Y - WorldMarkSize, TextColor);
}

void AEIHud::DrawMenu(AEIPlayerController* Player, const AEIHeroCharacter& Hero)
{
	float MouseX = -1.0f;
	float MouseY = -1.0f;
	Player->GetMousePosition(MouseX, MouseY);
	FEIGearMenu& Menu = Player->GetMenu();
	Menu.Build(*Player->GetProfile(), Hero, Canvas->SizeX, Canvas->SizeY, MouseX, MouseY);
	for (const FEIMenuElement& Element : Menu.GetElements())
	{
		if (Element.Width > 0.0f)
		{
			DrawRect(Element.BoxColor, Element.X, Element.Y, Element.Width, Element.Height);
		}
		if (!Element.Text.IsEmpty())
		{
			const bool bRow = Element.Width > 0.0f;
			DrawLine(Element.Text, Element.X + (bRow ? MenuTextInset : 0.0f), Element.Y + (bRow ? MenuTextDrop : 0.0f), Element.TextColor, Element.TextScale);
		}
	}
}

void AEIHud::DrawSettlement(const AEIHeroCharacter* Hero, const AEIGameState* State, const FString& Clock)
{
	const AEIPlayerController* Player = Cast<AEIPlayerController>(PlayerOwner);
	if (Player == nullptr)
	{
		return;
	}
	const UEIProfileComponent* Profile = Player->GetProfile();
	const TArray<FEILootLine>& Loot = Profile->GetLoot();
	const bool bEscaped = State->GetRunPhase() == EEIRunPhase::Escaped;

	// Without the barge only a share stays, and it says how much of what.
	TArray<FString> Earned;
	const auto AddEarned = [Profile, bEscaped, &Earned](const FString& Name, int32 Amount)
	{
		Earned.Add(bEscaped
			? FString::Printf(TEXT("%s: +%d"), *Name, Amount)
			: FString::Printf(TEXT("%s: %d von %d bleiben"), *Name, Profile->GetSecured(Amount), Amount));
	};
	AddEarned(TEXT("Gold"), Profile->GetRunGold());
	for (const FEIFaction& Faction : Profile->GetContractTable().Factions)
	{
		const int32 Points = Profile->GetRunPoints(Faction.Name);
		if (Points > 0)
		{
			AddEarned(Faction.Name + TEXT("punkte"), Points);
		}
	}

	const int32 LootLines = Loot.IsEmpty() ? 0 : Loot.Num() + 1;
	const int32 IngredientLines = Profile->GetRunIngredients().IsEmpty() ? 0 : 1;
	const float PanelHeight = PanelPadding * 2.0f + PanelLine * (4 + Earned.Num() + LootLines + IngredientLines) + 40.0f;
	const float X = Canvas->SizeX / 2.0f - PanelWidth / 2.0f;
	float Y = FMath::Max(Margin, Canvas->SizeY / 2.0f - PanelHeight / 2.0f);
	DrawRect(PanelColor, X, Y, PanelWidth, PanelHeight);
	const float TextX = X + PanelPadding;
	Y += PanelPadding;
	DrawLine(bEscaped ? TEXT("Entkommen!") : TEXT("Lauf gescheitert"), TextX, Y, bEscaped ? TextColor : WarningTextColor, 2.0f);
	Y += 40.0f + PanelLine / 2.0f;
	DrawLine(FString::Printf(TEXT("Laufzeit %s, %d von %d Nebenauftr\u00E4gen erf\u00FCllt"), *Clock, State->CountContractsDone(), State->GetContracts().Num()), TextX, Y, TextColor);
	Y += PanelLine;
	for (const FString& Line : Earned)
	{
		DrawLine(Line, TextX, Y, RewardColor);
		Y += PanelLine;
	}
	DrawLine(FString::Printf(TEXT("Erfahrung: +%d, sie bleibt in jedem Fall. Du bist Level %d."), Profile->GetRunExperience(), Profile->GetLevel()), TextX, Y, TextColor);
	Y += PanelLine;
	if (!Profile->GetRunIngredients().IsEmpty())
	{
		DrawLine(FString::Printf(TEXT("%s%s"), bEscaped ? TEXT("Zutaten: ") : TEXT("Diese Zutaten sind verloren: "), *FString::Join(Profile->GetRunIngredients(), TEXT(", "))), TextX, Y, bEscaped ? RewardColor : LostColor);
		Y += PanelLine;
	}
	if (!Loot.IsEmpty())
	{
		DrawLine(bEscaped ? TEXT("Gefunden:") : TEXT("Diese Funde sind verloren:"), TextX, Y, TextColor);
		Y += PanelLine;
	}
	for (const FEILootLine& Line : Loot)
	{
		const FEIItemTable& Table = Profile->GetTable();
		const FString Name = Table.Items.IsValidIndex(Line.Item) ? Table.Items[Line.Item].Name : FString();
		DrawLine(FString::Printf(TEXT("%s: %s"), *Name, *Line.Result), TextX + PanelPadding, Y, bEscaped ? RewardColor : LostColor);
		Y += PanelLine;
	}
	Y += PanelLine / 2.0f;
	DrawLine(TEXT("I Ausr\u00FCstung   R zur\u00FCck zur Anlegestelle"), TextX, Y, TextColor);
}

void AEIHud::DrawContracts(const AEIHeroCharacter* Hero, const AEIGameState* State, const FEIContractTable& Table, float PromptY)
{
	const float CenterX = Canvas->SizeX / 2.0f;
	for (const FEIContract& Contract : State->GetContracts())
	{
		const FEIContractTask* Task = Table.FindTask(Contract.Kind);
		const FEIFaction* Faction = Table.FindFaction(Contract.Faction);
		if (Contract.bDone || Task == nullptr || Faction == nullptr)
		{
			continue;
		}
		const FString Label = FString::Printf(TEXT("%s: %s %.0f m"), *Faction->Name, *Task->ShortName, FVector::Dist(Hero->GetActorLocation(), Contract.Location) / CentimetersPerMeter);
		DrawWorldMark(Contract.Location + FVector(0.0f, 0.0f, ContractMarkAboveCm), Label, Faction->Color);
		if (Contract.bHeld && FVector::Dist2D(Hero->GetActorLocation(), Contract.Location) <= Task->ZoneMeters * CentimetersPerMeter)
		{
			DrawLine(FString::Printf(TEXT("%s: noch %.0f s halten"), *Task->ShortName, Contract.HoldLeft), CenterX - 130.0f, PromptY - 80.0f, Faction->Color, 1.5f);
		}
	}
	int32 GuardsLeft = 0;
	for (TActorIterator<AEIChest> It(GetWorld()); It; ++It)
	{
		if (!It->IsInUse() || It->IsOpen())
		{
			continue;
		}
		GuardsLeft += It->GetGuardsLeft();
		if (It->IsInReach(Hero))
		{
			const bool bFree = It->GetState() == EEIChestState::Free;
			DrawLine(bFree ? TEXT("F: Truhe \u00F6ffnen") : TEXT("Besiege erst die W\u00E4chter!"), CenterX - (bFree ? 95.0f : 150.0f), PromptY - 40.0f, bFree ? TextColor : WarningTextColor, 1.5f);
		}
	}
	if (GuardsLeft > 0)
	{
		DrawLine(FString::Printf(TEXT("W\u00E4chter: %d"), GuardsLeft), CenterX - 70.0f, 200.0f, WarningTextColor, 1.5f);
	}
}

void AEIHud::DrawContractBoard(const AEIHeroCharacter* Hero, const AEIGameState* State, const FEIContractTable& Table)
{
	const TArray<FEIContract>& Contracts = State->GetContracts();
	const float Width = PanelPadding * 3.0f + BoardMapSize + BoardListWidth;
	const float Height = PanelPadding * 2.0f + BoardMapSize;
	const float X = Canvas->SizeX - Width - Margin;
	DrawRect(PanelColor, X, BoardY, Width, Height);

	// Up on the map is the way the hero looks with no turn at all, and right is right.
	const float MapX = X + PanelPadding;
	const float MapY = BoardY + PanelPadding;
	const float Scale = BoardMapSize / FMath::Max(Table.MapMeters * CentimetersPerMeter, 1.0f);
	const auto ToMap = [MapX, MapY, Scale](const FVector& Location)
	{
		return FVector2D(MapX + BoardMapSize / 2.0f + Location.Y * Scale, MapY + BoardMapSize / 2.0f - Location.X * Scale);
	};
	DrawRect(BoardFloorColor, MapX, MapY, BoardMapSize, BoardMapSize);

	const float ListX = MapX + BoardMapSize + PanelPadding;
	float ListY = MapY;
	const int32 Stage = State->GetCorruptionStage();
	for (int32 Index = 0; Index < Contracts.Num(); ++Index)
	{
		const FEIContract& Contract = Contracts[Index];
		const FEIContractTask* Task = Table.FindTask(Contract.Kind);
		const FEIFaction* Faction = Table.FindFaction(Contract.Faction);
		if (Task == nullptr || Faction == nullptr)
		{
			continue;
		}
		const FLinearColor Color = Contract.bDone ? LostColor : Faction->Color;
		const FVector2D Mark = ToMap(Contract.Location);
		DrawRect(Color, Mark.X - BoardMarkSize / 2.0f, Mark.Y - BoardMarkSize / 2.0f, BoardMarkSize, BoardMarkSize);
		DrawLine(FString::FromInt(Index + 1), Mark.X + BoardMarkSize / 2.0f + 2.0f, Mark.Y - BoardMarkSize, TextColor);

		const float Distance = static_cast<float>(FVector::Dist(Hero->GetActorLocation(), Contract.Location)) / CentimetersPerMeter;
		DrawLine(FString::Printf(TEXT("%d  %s: %s, %.0f m"), Index + 1, *Faction->Name, *Task->Name, Distance), ListX, ListY, Color);
		ListY += BoardLine;
		const FString Pay = Contract.bDone ? FString(TEXT("erf\u00FCllt")) : FString::Printf(TEXT("%d Gold, %d %spunkte"), Table.GetGold(Stage), Table.GetPoints(Stage), *Faction->Name);
		DrawLine(Pay, ListX + 22.0f, ListY, Contract.bDone ? LostColor : TextColor);
		ListY += BoardLine * 1.25f;
	}

	for (TActorIterator<AEISignalFire> It(GetWorld()); It; ++It)
	{
		if (It->IsInUse())
		{
			const FVector2D Mark = ToMap(It->GetActorLocation());
			DrawRect(FireMarkColor, Mark.X - BoardMarkSize / 2.0f, Mark.Y - BoardMarkSize / 2.0f, BoardMarkSize, BoardMarkSize);
			DrawLine(TEXT("S"), Mark.X + BoardMarkSize / 2.0f + 2.0f, Mark.Y - BoardMarkSize, TextColor);
			DrawLine(FString::Printf(TEXT("S  Signalfeuer, %.0f m"), FVector::Dist(Hero->GetActorLocation(), It->GetActorLocation()) / CentimetersPerMeter), ListX, ListY, FireMarkColor);
		}
	}
	for (TActorIterator<AEIBoss> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			const FVector2D Mark = ToMap(It->GetActorLocation());
			DrawRect(BossMarkColor, Mark.X - BoardMarkSize / 2.0f, Mark.Y - BoardMarkSize / 2.0f, BoardMarkSize, BoardMarkSize);
			DrawLine(TEXT("B"), Mark.X + BoardMarkSize / 2.0f + 2.0f, Mark.Y - BoardMarkSize, TextColor);
			if (const FEIBossValues* Values = It->GetBossValues())
			{
				DrawLine(FString::Printf(TEXT("B  %s, %.0f m"), *Values->Name, FVector::Dist(Hero->GetActorLocation(), It->GetActorLocation()) / CentimetersPerMeter), ListX, ListY + BoardLine, BossMarkColor);
			}
		}
	}
	const FVector2D HeroMark = ToMap(Hero->GetActorLocation());
	const FVector Ahead = Hero->GetViewRotation().Vector().GetSafeNormal2D();
	const FVector2D AheadMark = HeroMark + FVector2D(Ahead.Y, -Ahead.X) * BoardHeroAheadDistance;
	DrawRect(FLinearColor::White, HeroMark.X - BoardHeroSize / 2.0f, HeroMark.Y - BoardHeroSize / 2.0f, BoardHeroSize, BoardHeroSize);
	DrawRect(FLinearColor::White, AheadMark.X - BoardHeroAheadSize / 2.0f, AheadMark.Y - BoardHeroAheadSize / 2.0f, BoardHeroAheadSize, BoardHeroAheadSize);
}

void AEIHud::DrawBoss(const AEIHeroCharacter* Hero, const AEIGameState* State, float LineY, float PromptY)
{
	const float CenterX = Canvas->SizeX / 2.0f;
	const AEIBoss* Boss = nullptr;
	for (TActorIterator<AEIBoss> It(GetWorld()); It; ++It)
	{
		Boss = *It;
	}
	const FEIBossValues* Values = Boss != nullptr ? Boss->GetBossValues() : nullptr;
	const EEIBossStage Stage = State->GetBossStage();
	if (Stage == EEIBossStage::Down)
	{
		DrawLine(TEXT("Der Boss ist gefallen. Lauf zum Signalfeuer, und erst in der Barke ist die Beute sicher!"), Margin, LineY, RewardColor);
		return;
	}
	if (Values == nullptr || Stage == EEIBossStage::None)
	{
		return;
	}
	// Close by, the bar says all of it, and the mark would lie on top of it.
	const double FromBoss = FVector::Dist(Hero->GetActorLocation(), Boss->GetActorLocation());
	if (!Boss->IsDead() && (Stage != EEIBossStage::Awake || FromBoss > BossMarkFromCm))
	{
		const FVector Mark = Boss->GetActorLocation() + FVector(0.0f, 0.0f, BossMarkAboveCm);
		DrawWorldMark(Mark, FString::Printf(TEXT("%s %.0f m"), *Values->Name, FVector::Dist(Hero->GetActorLocation(), Boss->GetActorLocation()) / CentimetersPerMeter), BossMarkColor);
	}
	if (Stage == EEIBossStage::Sealed)
	{
		DrawLine(FString::Printf(TEXT("%s: %d von %d. %s"), *Values->Bannbruch.Name, State->GetBannbruchDone(), State->GetBannbruchTotal(), *Values->Bannbruch.Hint), Margin, LineY, RewardColor);
		for (TActorIterator<AEIAnchorStone> It(GetWorld()); It; ++It)
		{
			if (It->IsBroken())
			{
				continue;
			}
			DrawWorldMark(It->GetMarkLocation(), FString::Printf(TEXT("%s %.0f m"), *Values->Bannbruch.PartName, FVector::Dist(Hero->GetActorLocation(), It->GetActorLocation()) / CentimetersPerMeter), AnchorMarkColor);
			if (It->IsHeld() && It->IsInZone(Hero))
			{
				DrawLine(FString::Printf(TEXT("%s: noch %.0f s halten"), *Values->Bannbruch.PartName, It->GetHoldLeft()), CenterX - 150.0f, PromptY - 80.0f, AnchorMarkColor, 1.5f);
			}
		}
		return;
	}
	DrawLine(FString::Printf(TEXT("Besiege: %s"), *Values->Name), Margin, LineY, RewardColor);
	const UEIHealthComponent* Life = Boss->GetHealth();
	DrawLine(Values->Name, CenterX - BossBarWidth / 2.0f, BossBarY - 24.0f, TextColor);
	DrawBar(CenterX - BossBarWidth / 2.0f, BossBarY, BossBarWidth, BossBarHeight, Life->GetHealth() / FMath::Max(Life->GetMaxHealth(), 1.0f), BossLifeColor);
}

void AEIHud::DrawDock(const AEIHeroCharacter* Hero, const AEIGameState* State, float LineY, float PromptY)
{
	const float CenterX = Canvas->SizeX / 2.0f;
	int32 Heroes = 0;
	int32 Aboard = 0;
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			++Heroes;
			Aboard += It->IsAboard() ? 1 : 0;
		}
	}
	DrawLine(TEXT("Steig in die Barke, dann beginnt der Lauf. Vorher w\u00E4hlst du mit C die Klasse und mit I die Ausr\u00FCstung."), Margin, LineY, TextColor);
	if (State->IsCastingOff())
	{
		DrawLine(TEXT("Die Barke legt ab"), CenterX - 110.0f, PromptY - 40.0f, RewardColor, 1.5f);
	}
	else if (Aboard > 0)
	{
		DrawLine(FString::Printf(TEXT("An Bord: %d von %d"), Aboard, Heroes), Margin, LineY + 24.0f, RewardColor);
	}
	for (TActorIterator<AEISkyBarge> It(GetWorld()); It; ++It)
	{
		if (Hero->IsAboard())
		{
			DrawLine(TEXT("F: Aussteigen"), CenterX - 70.0f, PromptY, TextColor, 1.5f);
		}
		else if (It->IsInBoardingZone(Hero))
		{
			DrawLine(TEXT("F: Einsteigen"), CenterX - 70.0f, PromptY, TextColor, 1.5f);
		}
		else
		{
			DrawWorldMark(It->GetActorLocation(), FString::Printf(TEXT("Barke %.0f m"), FVector::Dist(Hero->GetActorLocation(), It->GetActorLocation()) / CentimetersPerMeter), BargeMarkColor);
		}
	}
}

void AEIHud::DrawRun(const AEIHeroCharacter* Hero, const AEIGameState* State)
{
	const int32 Seconds = FMath::Max(0, FMath::FloorToInt(State->GetRunSeconds()));
	FString Clock = FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60);
	if (State->IsBloodMoon())
	{
		Clock += TEXT("   Blutmond");
	}
	else if (State->GetCorruptionStage() > 0)
	{
		Clock += FString::Printf(TEXT("   Verderbnis %d"), State->GetCorruptionStage());
	}
	if (State->GetCorruptionNote() > 0.0f)
	{
		DrawLine(State->IsBloodMoon() ? TEXT("Der Blutmond steigt auf") : TEXT("Die Verderbnis w\u00E4chst"), Canvas->SizeX / 2.0f - 140.0f, 110.0f, WarningTextColor, 1.5f);
	}
	const float LineY = Margin + 48.0f;
	const float CenterX = Canvas->SizeX / 2.0f;
	const AEIPlayerController* Player = Cast<AEIPlayerController>(PlayerOwner);
	if (Player == nullptr)
	{
		return;
	}
	const UEIProfileComponent* Profile = Player->GetProfile();
	const FEIContractTable& Table = Profile->GetContractTable();
	const float PromptY = Canvas->SizeY / 2.0f + 100.0f;
	const auto DistanceLabel = [Hero](const TCHAR* Name, const FVector& Location)
	{
		return FString::Printf(TEXT("%s %.0f m"), Name, FVector::Dist(Hero->GetActorLocation(), Location) / CentimetersPerMeter);
	};

	const EEIRunPhase Phase = State->GetRunPhase();
	if (Phase == EEIRunPhase::Exploring || Phase == EEIRunPhase::SignalLit || Phase == EEIRunPhase::BargeWaiting)
	{
		DrawContracts(Hero, State, Table, PromptY);
		if (Player->IsMapOpen())
		{
			DrawContractBoard(Hero, State, Table);
		}
	}
	if (Profile->GetRewardNote() > 0.0f)
	{
		DrawLine(Profile->GetRewardText(), CenterX - 260.0f, RewardNoteY, RewardColor, 1.5f);
	}

	if (Phase == EEIRunPhase::Exploring || Phase == EEIRunPhase::SignalLit || Phase == EEIRunPhase::BargeWaiting)
	{
		DrawBoss(Hero, State, LineY + 24.0f, PromptY);
	}

	// What this run has earned so far, which is only safe in the barge.
	FString Purse = FString::Printf(TEXT("Beute: %d Gold"), Profile->GetRunGold());
	for (const FEIFaction& Faction : Table.Factions)
	{
		const int32 Points = Profile->GetRunPoints(Faction.Name);
		if (Points > 0)
		{
			Purse += FString::Printf(TEXT(", %d %s"), Points, *Faction.Name);
		}
	}

	switch (Phase)
	{
	case EEIRunPhase::AtDock:
		DrawDock(Hero, State, LineY, PromptY);
		break;

	case EEIRunPhase::Exploring:
		DrawLine(FString::Printf(TEXT("%s   Nebenauftr\u00E4ge: %d von %d   %s   M zeigt die Karte"), *Clock, State->CountContractsDone(), State->GetContracts().Num(), *Purse), Margin, LineY, TextColor);
		if (State->GetBossStage() == EEIBossStage::None)
		{
			DrawLine(TEXT("Entz\u00FCnde das Signalfeuer, dann holt dich die Barke ab."), Margin, LineY + 24.0f, RewardColor);
		}
		for (TActorIterator<AEISignalFire> It(GetWorld()); It; ++It)
		{
			if (It->IsInUse())
			{
				DrawWorldMark(It->GetFireLocation(), DistanceLabel(TEXT("Signalfeuer"), It->GetActorLocation()), FireMarkColor);
				if (It->IsInReach(Hero))
				{
					const bool bReady = State->CanLightSignal();
					DrawLine(bReady ? TEXT("F: Signalfeuer entz\u00FCnden") : TEXT("Das Signalfeuer brennt erst nach dem Boss"), CenterX - (bReady ? 125.0f : 200.0f), PromptY, bReady ? TextColor : WarningTextColor, 1.5f);
				}
			}
		}
		break;

	case EEIRunPhase::SignalLit:
		DrawLine(FString::Printf(TEXT("%s   Halte das Feuer noch %.0f s, bis die Barke da ist   %s"), *Clock, State->GetSignalHoldLeft(), *Purse), Margin, LineY, TextColor);
		for (TActorIterator<AEISignalFire> It(GetWorld()); It; ++It)
		{
			if (It->IsLit())
			{
				DrawWorldMark(It->GetFireLocation(), DistanceLabel(TEXT("Signalfeuer"), It->GetActorLocation()), FireMarkColor);
			}
		}
		if (!State->IsSignalHeld())
		{
			DrawLine(TEXT("Zur\u00FCck zum Signalfeuer!"), CenterX - 130.0f, PromptY, WarningTextColor, 1.5f);
		}
		break;

	case EEIRunPhase::BargeWaiting:
		DrawLine(FString::Printf(TEXT("%s   Die Barke ist da. Steig ein!   %s"), *Clock, *Purse), Margin, LineY, TextColor);
		for (TActorIterator<AEISkyBarge> It(GetWorld()); It; ++It)
		{
			DrawWorldMark(It->GetActorLocation(), DistanceLabel(TEXT("Barke"), It->GetActorLocation()), BargeMarkColor);
			if (It->IsInBoardingZone(Hero))
			{
				DrawLine(TEXT("F: Einsteigen"), CenterX - 70.0f, PromptY, TextColor, 1.5f);
			}
		}
		break;

	case EEIRunPhase::Escaped:
	case EEIRunPhase::Failed:
		DrawSettlement(Hero, State, Clock);
		break;

	default:
		break;
	}
}

void AEIHud::DrawLastHit(const AEIHeroCharacter* Hero)
{
	const float Left = Hero->GetHitNoteLeft();
	if (Left <= 0.0f)
	{
		return;
	}
	FLinearColor NoteColor = RewardColor;
	NoteColor.A = FMath::Min(1.0f, Left);
	DrawLine(Hero->GetLastHitNote(), Canvas->SizeX / 2.0f - HitNoteWidth, Canvas->SizeY - HitNoteFromBottom, NoteColor, 1.25f);

	// An arrow around the crosshair points to whoever did it, seen from above: up on the screen is straight ahead.
	const float ArrowLeft = Left - (HitNoteLastSeconds - HitArrowSeconds);
	if (ArrowLeft <= 0.0f)
	{
		return;
	}
	const FRotator View = Hero->GetViewRotation();
	const FVector Forward = View.Vector().GetSafeNormal2D();
	const FVector Right = FRotationMatrix(View).GetUnitAxis(EAxis::Y).GetSafeNormal2D();
	const FVector ToSource = (Hero->GetLastHitFrom() - Hero->GetActorLocation()).GetSafeNormal2D();
	const FVector2D Pointing(FVector::DotProduct(ToSource, Right), -FVector::DotProduct(ToSource, Forward));
	const FVector2D Center(Canvas->SizeX / 2.0f, Canvas->SizeY / 2.0f);
	const FVector2D Tip = Center + Pointing * HitArrowFromCenter;
	const FVector2D Back = Tip - Pointing * HitArrowLength;
	const FVector2D Side(-Pointing.Y, Pointing.X);
	FLinearColor ArrowColor = ThreatColor;
	ArrowColor.A = FMath::Min(1.0f, ArrowLeft / (HitArrowSeconds * 0.5f));
	for (const float Sign : { -1.0f, 1.0f })
	{
		const FVector2D Wing = Back + Side * Sign * HitArrowLength;
		AHUD::DrawLine(static_cast<float>(Tip.X), static_cast<float>(Tip.Y), static_cast<float>(Wing.X), static_cast<float>(Wing.Y), ArrowColor, HitArrowThickness);
	}
}

void AEIHud::DrawThreats(const AEIHeroCharacter* Hero)
{
	const FRotator View = Hero->GetViewRotation();
	const FVector Forward = View.Vector().GetSafeNormal2D();
	const FVector Right = FRotationMatrix(View).GetUnitAxis(EAxis::Y).GetSafeNormal2D();
	const float CenterX = Canvas->SizeX / 2.0f;
	const float CenterY = Canvas->SizeY / 2.0f;
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		const FVector ToEnemy = It->GetActorLocation() - Hero->GetActorLocation();
		if (!It->IsTelegraphing() || ToEnemy.Size2D() > ThreatRangeCm)
		{
			continue;
		}
		const FVector Direction = ToEnemy.GetSafeNormal2D();
		const float Ahead = static_cast<float>(FVector::DotProduct(Direction, Forward));
		if (Ahead > ThreatInViewDot)
		{
			continue;
		}
		const float Side = static_cast<float>(FVector::DotProduct(Direction, Right));
		const float MarkX = CenterX + Side * CenterX * ThreatEdgeShare * 2.0f;
		const float MarkY = CenterY - Ahead * CenterY * ThreatEdgeShare * 2.0f;
		DrawRect(ThreatColor, MarkX - ThreatMarkSize / 2.0f, MarkY - ThreatMarkSize / 2.0f, ThreatMarkSize, ThreatMarkSize);
	}
}
