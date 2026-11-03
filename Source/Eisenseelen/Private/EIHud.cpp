#include "EIHud.h"

#include "EICasterComponent.h"
#include "EIEnemy.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
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
	const FLinearColor DodgeColor(0.3f, 0.7f, 1.0f);
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
}

void AEIHud::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color)
{
	DrawRect(BarBack, X, Y, Width, Height);
	DrawRect(Color, X, Y, Width * FMath::Clamp(Fraction, 0.0f, 1.0f), Height);
}

void AEIHud::DrawLine(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale)
{
	DrawText(Text, Color, X, Y, GEngine->GetMediumFont(), Scale);
}

void AEIHud::DrawHUD()
{
	Super::DrawHUD();

	const AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(GetOwningPawn());
	const float Width = Canvas->SizeX;
	const float Height = Canvas->SizeY;
	DrawLine(TEXT("WASD laufen   Shift sprinten   Leertaste springen   V ausweichen   Linke Maus Angriff (halten: stark)   Q Feuerball   E Schlachtruf   C Klasse   H Horde   R Neustart"), Margin, Margin, TextColor);
	if (Hero == nullptr)
	{
		return;
	}

	int32 Enemies = 0;
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		Enemies += It->IsDead() ? 0 : 1;
	}
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const bool bHorde = State != nullptr && State->IsHordeActive();
	const TCHAR* ClassName = Hero->GetHeroClass() == EEIHeroClass::Arcanist ? TEXT("Arkanist") : TEXT("Klingenwacht");
	DrawLine(FString::Printf(TEXT("%s   Horde %s   Gegner %d"), ClassName, bHorde ? TEXT("an") : TEXT("aus"), Enemies), Margin, Margin + 24.0f, TextColor);

	if (Hero->GetHurtFlash() > 0.0f)
	{
		FLinearColor Flash = HurtColor;
		Flash.A *= FMath::Min(1.0f, Hero->GetHurtFlash() / 0.35f);
		DrawRect(Flash, 0.0f, 0.0f, Width, Height);
	}
	if (Hero->IsDead())
	{
		DrawLine(TEXT("Gefallen. R startet neu."), Width / 2.0f - 170.0f, Height / 2.0f - 20.0f, TextColor, 2.0f);
		return;
	}
	DrawThreats(Hero);

	DrawRect(TextColor, Width / 2.0f - CrosshairSize / 2.0f, Height / 2.0f - CrosshairSize / 2.0f, CrosshairSize, CrosshairSize);

	const UEIHealthComponent* Health = Hero->GetHealth();
	const float LifeY = Height - Margin - BarHeight;
	DrawBar(Margin, LifeY, BarWidth, BarHeight, Health->GetHealth() / FMath::Max(Health->GetMaxHealth(), 1.0f), LifeColor);
	DrawLine(FString::Printf(TEXT("Leben %.0f / %.0f"), Health->GetHealth(), Health->GetMaxHealth()), Margin, LifeY - 24.0f, TextColor);

	const UEIHeroMovementComponent* Movement = Hero->GetHeroMovement();
	const float DodgeCooldown = FMath::Max(Movement->GetDodgeCooldownSeconds(), KINDA_SMALL_NUMBER);
	DrawBar(Margin + BarWidth + Margin, LifeY, BarWidth / 2.0f, BarHeight, 1.0f - Movement->GetDodgeCooldownRemaining() / DodgeCooldown, DodgeColor);
	DrawLine(TEXT("Ausweichen"), Margin + BarWidth + Margin, LifeY - 24.0f, TextColor);

	const float Speed = static_cast<float>(Hero->GetVelocity().Size2D()) / 100.0f;
	DrawLine(FString::Printf(TEXT("%.1f m/s"), Speed), Margin + BarWidth + Margin + BarWidth / 2.0f + Margin, LifeY - 8.0f, TextColor);

	const UEICasterComponent* Caster = Hero->GetCaster();
	const float SpellX = Margin + BarWidth + Margin;
	const float SpellY = LifeY - 56.0f;
	DrawBar(SpellX, SpellY, BarWidth / 2.0f, BarHeight, 1.0f - Caster->GetSpellCooldownFraction(), SpellColor);
	DrawLine(TEXT("Q Feuerball"), SpellX, SpellY - 24.0f, TextColor);

	if (Caster->GetMaxMana() > 0.0f)
	{
		DrawBar(Margin, SpellY, BarWidth, BarHeight, Caster->GetMana() / Caster->GetMaxMana(), ManaColor);
		if (Caster->GetManaDeniedFlash() > 0.0f)
		{
			DrawRect(NoManaColor, Margin, SpellY, BarWidth, BarHeight);
		}
		DrawLine(FString::Printf(TEXT("Mana %.0f / %.0f"), Caster->GetMana(), Caster->GetMaxMana()), Margin, SpellY - 24.0f, TextColor);
	}

	const bool bArcanist = Hero->GetHeroClass() == EEIHeroClass::Arcanist;
	const float Charge = bArcanist ? Caster->GetChargeFraction() : Hero->GetSword()->GetChargeFraction();
	if (Charge > 0.25f)
	{
		const FLinearColor ChargeBarColor = bArcanist && !Caster->CanAffordFireball() ? NoManaColor : (Charge >= 1.0f ? FLinearColor::White : ChargeColor);
		DrawBar(Width / 2.0f - 60.0f, Height / 2.0f + 40.0f, 120.0f, 8.0f, Charge, ChargeBarColor);
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
