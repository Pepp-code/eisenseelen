#include "EIHud.h"

#include "EIAltar.h"
#include "EIBeacon.h"
#include "EICasterComponent.h"
#include "EIChest.h"
#include "EIEnemy.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EIOmenComponent.h"
#include "EIPlayerController.h"
#include "EIProfileComponent.h"
#include "EIRuneComponent.h"
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
	const FLinearColor BeaconMarkColor(1.0f, 0.55f, 0.1f);
	const FLinearColor AltarMarkColor(0.7f, 0.3f, 1.0f);
	const FLinearColor TextShadowColor(0.0f, 0.0f, 0.0f, 0.75f);
	constexpr float TextShadowOffset = 1.5f;
	const FLinearColor ChestMarkColor(1.0f, 0.8f, 0.25f);
	const FLinearColor GuardedChestMarkColor(1.0f, 0.3f, 0.15f);
	const FLinearColor OmenColor(1.0f, 0.85f, 0.45f);
	constexpr float ChestMarkRangeCm = 4000.0f;
	const FLinearColor BargeMarkColor(0.5f, 0.85f, 1.0f);
	const FLinearColor WarningTextColor(1.0f, 0.25f, 0.15f);
	constexpr float WorldMarkSize = 10.0f;
	const FLinearColor PanelColor(0.0f, 0.0f, 0.0f, 0.72f);
	const FLinearColor LostColor(0.65f, 0.65f, 0.65f);
	constexpr float PanelWidth = 760.0f;
	constexpr float PanelLine = 30.0f;
	constexpr float PanelPadding = 26.0f;
	constexpr float RewardNoteY = 230.0f;
	constexpr float CentimetersPerMeter = 100.0f;
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

	const AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(GetOwningPawn());
	const float Width = Canvas->SizeX;
	const float Height = Canvas->SizeY;
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const bool bHasRun = State != nullptr && State->GetRunPhase() != EEIRunPhase::None;
	const FEIRune* Rune = Hero != nullptr ? Hero->GetRunes()->GetRune() : nullptr;
	const FString RuneKey = Rune != nullptr ? FString::Printf(TEXT("E %s   "), *Rune->SpellName) : FString();
	const FString SpellName = Hero != nullptr ? Hero->GetCaster()->GetSpell().Name : FString();
	const TCHAR* NextRuneKey = !bHasRun || State->GetRunPhase() == EEIRunPhase::OnBarge ? TEXT("T Rune   ") : TEXT("");
	DrawLine(FString::Printf(TEXT("WASD laufen   Shift sprinten   Leertaste springen   Q %s   %sF benutzen   C Klasse   %s%sR Neustart"), *SpellName, *RuneKey, NextRuneKey, bHasRun ? TEXT("") : TEXT("H Horde   ")), Margin, Margin, TextColor);
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
	DrawLine(FString::Printf(TEXT("%s   %s   %s%s"), ClassName, Rune != nullptr ? *Rune->Name : TEXT("Keine Rune"), *ClassKeys, *EnemyNote), Margin, Margin + 24.0f, TextColor);

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
	DrawOmens(Hero, SpellY - 56.0f);

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

void AEIHud::DrawOmens(const AEIHeroCharacter* Hero, float BottomY)
{
	const UEIOmenComponent* Omens = Hero->GetOmens();
	const TArray<FEIOmen>& Table = Omens->GetTable().Omens;
	float LineY = BottomY;
	for (int32 Index = 0; Index < Table.Num(); ++Index)
	{
		const int32 Count = Omens->CountOf(Index);
		if (Count > 0)
		{
			DrawLine(Count > 1 ? FString::Printf(TEXT("%s x%d"), *Table[Index].Name, Count) : Table[Index].Name, Margin, LineY, OmenColor);
			LineY -= 22.0f;
		}
	}
	if (Omens->GetFoundNote() > 0.0f && Table.IsValidIndex(Omens->GetLastFound()))
	{
		const FEIOmen& Found = Table[Omens->GetLastFound()];
		DrawLine(FString::Printf(TEXT("%s: %s"), *Found.Name, *Found.Describe()), Canvas->SizeX / 2.0f - 200.0f, 150.0f, OmenColor, 1.5f);
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
	const float PanelHeight = PanelPadding * 2.0f + PanelLine * (6 + Loot.Num()) + 40.0f;
	const float X = Canvas->SizeX / 2.0f - PanelWidth / 2.0f;
	float Y = FMath::Max(Margin, Canvas->SizeY / 2.0f - PanelHeight / 2.0f);
	DrawRect(PanelColor, X, Y, PanelWidth, PanelHeight);
	const float TextX = X + PanelPadding;
	Y += PanelPadding;
	DrawLine(bEscaped ? TEXT("Entkommen!") : TEXT("Lauf gescheitert"), TextX, Y, bEscaped ? TextColor : WarningTextColor, 2.0f);
	Y += 40.0f + PanelLine / 2.0f;
	DrawLine(FString::Printf(TEXT("Laufzeit %s, %d Omen"), *Clock, Hero->GetOmens()->CountAll()), TextX, Y, TextColor);
	Y += PanelLine;
	DrawLine(FString::Printf(TEXT("Erfahrung: +%d, sie bleibt in jedem Fall"), Profile->GetRunExperience()), TextX, Y, TextColor);
	Y += PanelLine;
	DrawLine(Loot.IsEmpty() ? FString(TEXT("Keine Beute")) : FString(bEscaped ? TEXT("Beute:") : TEXT("Diese Beute ist verloren:")), TextX, Y, TextColor);
	Y += PanelLine;
	for (const FEILootLine& Line : Loot)
	{
		const FEIItemTable& Table = Profile->GetTable();
		const FString Name = Table.Items.IsValidIndex(Line.Item) ? Table.Items[Line.Item].Name : FString();
		DrawLine(FString::Printf(TEXT("%s: %s"), *Name, *Line.Result), TextX + PanelPadding, Y, bEscaped ? OmenColor : LostColor);
		Y += PanelLine;
	}
	Y += PanelLine / 2.0f;
	DrawLine(TEXT("R startet einen neuen Lauf"), TextX, Y, TextColor);
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
	if (const AEIPlayerController* Player = Cast<AEIPlayerController>(PlayerOwner); Player != nullptr && Player->GetProfile()->GetRewardNote() > 0.0f)
	{
		DrawLine(Player->GetProfile()->GetRewardText(), CenterX - 260.0f, RewardNoteY, OmenColor, 1.5f);
	}
	const float PromptY = Canvas->SizeY / 2.0f + 100.0f;
	const auto DistanceLabel = [Hero](const TCHAR* Name, const FVector& Location)
	{
		return FString::Printf(TEXT("%s %.0f m"), Name, FVector::Dist(Hero->GetActorLocation(), Location) / CentimetersPerMeter);
	};
	const auto DrawActiveAltar = [this, &DistanceLabel]()
	{
		for (TActorIterator<AEIAltar> It(GetWorld()); It; ++It)
		{
			if (It->IsActive())
			{
				DrawWorldMark(It->GetCrystalLocation(), DistanceLabel(TEXT("Altar"), It->GetActorLocation()), AltarMarkColor);
			}
		}
	};

	const EEIRunPhase Phase = State->GetRunPhase();
	if (Phase != EEIRunPhase::OnBarge && Phase != EEIRunPhase::Escaped && Phase != EEIRunPhase::Failed)
	{
		int32 GuardsLeft = 0;
		for (TActorIterator<AEIChest> It(GetWorld()); It; ++It)
		{
			GuardsLeft += It->IsInUse() ? It->GetGuardsLeft() : 0;
			if (!It->IsInUse() || It->IsOpenFor(Hero) || FVector::Dist(Hero->GetActorLocation(), It->GetActorLocation()) > ChestMarkRangeCm)
			{
				continue;
			}
			const bool bFree = It->GetState() == EEIChestState::Free;
			if (!It->IsInReach(Hero))
			{
				DrawWorldMark(It->GetMarkLocation(), DistanceLabel(bFree ? TEXT("Truhe") : TEXT("Bewachte Truhe"), It->GetActorLocation()), bFree ? ChestMarkColor : GuardedChestMarkColor);
			}
			else if (bFree)
			{
				DrawLine(TEXT("F: Truhe \u00F6ffnen"), CenterX - 95.0f, PromptY - 40.0f, TextColor, 1.5f);
			}
			else
			{
				DrawLine(TEXT("Besiege erst die W\u00E4chter!"), CenterX - 150.0f, PromptY - 40.0f, WarningTextColor, 1.5f);
			}
		}
		if (GuardsLeft > 0)
		{
			DrawLine(FString::Printf(TEXT("W\u00E4chter: %d"), GuardsLeft), CenterX - 70.0f, 190.0f, WarningTextColor, 1.5f);
		}
	}

	switch (Phase)
	{
	case EEIRunPhase::OnBarge:
		DrawLine(TEXT("Spring von der Barke, dann beginnt der Lauf. Mit C w\u00E4hlst du vorher die Klasse, mit T die Rune."), Margin, LineY, TextColor);
		DrawActiveAltar();
		break;

	case EEIRunPhase::Exploring:
		DrawLine(FString::Printf(TEXT("%s   St\u00F6re das Ritual: Halte den Altar"), *Clock), Margin, LineY, TextColor);
		DrawActiveAltar();
		break;

	case EEIRunPhase::Ritual:
		DrawLine(FString::Printf(TEXT("%s   Altar %d von %d: noch %.0f s halten"), *Clock, State->GetAltarsBroken() + 1, State->GetAltarsTotal(), State->GetAltarHoldLeft()), Margin, LineY, TextColor);
		DrawActiveAltar();
		if (!State->IsAltarHeld())
		{
			DrawLine(TEXT("Geh zum Altar!"), CenterX - 90.0f, PromptY, TextColor, 1.5f);
		}
		break;

	case EEIRunPhase::RitualDone:
		DrawLine(FString::Printf(TEXT("%s   Das Ritual ist gest\u00F6rt. Entz\u00FCnde ein Leuchtfeuer, um die Barke zu rufen"), *Clock), Margin, LineY, TextColor);
		for (TActorIterator<AEIBeacon> It(GetWorld()); It; ++It)
		{
			DrawWorldMark(It->GetFireLocation(), DistanceLabel(TEXT("Leuchtfeuer"), It->GetActorLocation()), BeaconMarkColor);
			if (It->IsInReach(Hero))
			{
				DrawLine(TEXT("F: Leuchtfeuer entz\u00FCnden"), CenterX - 120.0f, PromptY, TextColor, 1.5f);
			}
		}
		break;

	case EEIRunPhase::BeaconLit:
		DrawLine(FString::Printf(TEXT("%s   Halte das Feuer noch %.0f s, bis die Barke da ist"), *Clock, State->GetBeaconHoldLeft()), Margin, LineY, TextColor);
		for (TActorIterator<AEIBeacon> It(GetWorld()); It; ++It)
		{
			if (It->IsLit())
			{
				DrawWorldMark(It->GetFireLocation(), DistanceLabel(TEXT("Feuer"), It->GetActorLocation()), BeaconMarkColor);
			}
		}
		if (!State->IsBeaconHeld())
		{
			DrawLine(TEXT("Zur\u00FCck zum Feuer!"), CenterX - 110.0f, PromptY, WarningTextColor, 1.5f);
		}
		break;

	case EEIRunPhase::BargeWaiting:
		DrawLine(FString::Printf(TEXT("%s   Die Barke ist da. Steig ein!"), *Clock), Margin, LineY, TextColor);
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
