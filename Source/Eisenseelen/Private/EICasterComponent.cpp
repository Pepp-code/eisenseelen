#include "EICasterComponent.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIHeroCharacter.h"
#include "EIProjectile.h"
#include "EIViewModelComponent.h"
#include "EIVisuals.h"
#include "Engine/World.h"

namespace
{
	constexpr float AimRangeCm = 20000.0f;
	constexpr float MuzzleForwardCm = 60.0f;
	// The owner's clock runs slightly ahead of the server's, so the server allows a little early firing.
	constexpr double ServerClockTolerance = 0.1;
	constexpr float ManaDeniedFlashSeconds = 0.3f;

	bool ReadProjectile(const FJsonObject& Staff, const TCHAR* Name, FEIProjectileValues& Out, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Projectile = nullptr;
		if (!Staff.TryGetObjectField(Name, Projectile))
		{
			OutError = FString::Printf(TEXT("Der Abschnitt %s fehlt."), Name);
			return false;
		}
		return FEIDataFile::ReadNumber(**Projectile, TEXT("schaden"), 0.0, 10000.0, Out.Damage, OutError)
			&& FEIDataFile::ReadNumber(**Projectile, TEXT("radius_m"), 0.1, 50.0, Out.RadiusMeters, OutError)
			&& FEIDataFile::ReadNumber(**Projectile, TEXT("tempo_m_pro_s"), 1.0, 500.0, Out.SpeedMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(**Projectile, TEXT("mana_kosten"), 0.0, 10000.0, Out.ManaCost, OutError);
	}
}

FEICasterValues::FEICasterValues()
{
	Fireball.Damage = 120.0f;
	Fireball.RadiusMeters = 4.0f;
	Fireball.SpeedMetersPerSecond = 30.0f;
	Fireball.ManaCost = 40.0f;
}

bool FEICasterValues::LoadFromProjectFile(FEICasterValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("waffen.json"), OutError);
	const TSharedPtr<FJsonObject>* Staff = nullptr;
	if (!Root.IsValid())
	{
		return false;
	}
	if (!Root->TryGetObjectField(TEXT("feuerstab"), Staff))
	{
		OutError = TEXT("Der Abschnitt feuerstab fehlt.");
		return false;
	}
	FEICasterValues Loaded;
	if (!ReadProjectile(**Staff, TEXT("schuss"), Loaded.Bolt, OutError)
		|| !ReadProjectile(**Staff, TEXT("feuerball"), Loaded.Fireball, OutError)
		|| !FEIDataFile::ReadNumber(**Staff, TEXT("halten_fuer_feuerball_s"), 0.05, 5.0, Loaded.HoldForFireballSeconds, OutError)
		|| !FEIDataFile::ReadNumber(**Staff, TEXT("abklingzeit_taste_q_s"), 0.0, 120.0, Loaded.SpellCooldownSeconds, OutError))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}

UEICasterComponent::UEICasterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UEICasterComponent::LoadValues()
{
	FString Error;
	if (!FEICasterValues::LoadFromProjectFile(Values, Error))
	{
		FEIDataFile::ReportError(TEXT("waffen.json"), Error);
	}
}

AEIHeroCharacter* UEICasterComponent::GetHero() const
{
	return CastChecked<AEIHeroCharacter>(GetOwner());
}

float UEICasterComponent::GetChargeFraction() const
{
	if (!bHolding)
	{
		return 0.0f;
	}
	return FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - HoldStartedAt) / Values.HoldForFireballSeconds, 0.0f, 1.0f);
}

float UEICasterComponent::GetSpellCooldownSeconds() const
{
	return Values.SpellCooldownSeconds * GetHero()->GetClassValues().SpellCooldownFactor;
}

float UEICasterComponent::GetSpellCooldownFraction() const
{
	const float Left = static_cast<float>(SpellReadyAt - GetWorld()->GetTimeSeconds());
	return FMath::Clamp(Left / FMath::Max(GetSpellCooldownSeconds(), KINDA_SMALL_NUMBER), 0.0f, 1.0f);
}

float UEICasterComponent::GetMaxMana() const
{
	return GetHero()->GetClassValues().Mana;
}

float UEICasterComponent::GetMana() const
{
	const float Regained = static_cast<float>(GetWorld()->GetTimeSeconds() - ManaStampedAt) * GetHero()->GetClassValues().ManaPerSecond;
	return FMath::Min(ManaAtStamp + Regained, GetMaxMana());
}

void UEICasterComponent::SpendMana(float Amount)
{
	ManaAtStamp = FMath::Max(0.0f, GetMana() - Amount);
	ManaStampedAt = GetWorld()->GetTimeSeconds();
}

void UEICasterComponent::RefillMana()
{
	ManaAtStamp = GetMaxMana();
	ManaStampedAt = GetWorld()->GetTimeSeconds();
}

bool UEICasterComponent::CanAfford(float Cost) const
{
	return GetMaxMana() <= 0.0f || GetMana() >= Cost;
}

bool UEICasterComponent::CanAffordFireball() const
{
	return CanAfford(Values.Fireball.ManaCost);
}

const FEIProjectileValues& UEICasterComponent::GetProjectile(EEICast Kind) const
{
	return Kind == EEICast::Bolt ? Values.Bolt : Values.Fireball;
}

void UEICasterComponent::FireBolt()
{
	FireAsOwner(EEICast::Bolt);
}

float UEICasterComponent::GetManaCost(EEICast Kind) const
{
	return Kind == EEICast::SpellFireball ? 0.0f : GetProjectile(Kind).ManaCost;
}

void UEICasterComponent::PressCharge()
{
	bHolding = true;
	HoldStartedAt = GetWorld()->GetTimeSeconds();
}

void UEICasterComponent::ReleaseCharge()
{
	if (!bHolding)
	{
		return;
	}
	const bool bCharged = GetChargeFraction() >= 1.0f;
	bHolding = false;
	if (GetHero()->GetViewModel() != nullptr)
	{
		GetHero()->GetViewModel()->ClearHold();
	}
	if (bCharged)
	{
		FireAsOwner(EEICast::ChargedFireball);
	}
}

void UEICasterComponent::CastFireball()
{
	if (GetWorld()->GetTimeSeconds() >= SpellReadyAt)
	{
		FireAsOwner(EEICast::SpellFireball);
	}
}

void UEICasterComponent::Deactivate()
{
	bHolding = false;
	if (GetHero()->GetViewModel() != nullptr)
	{
		GetHero()->GetViewModel()->ClearHold();
	}
}

FVector UEICasterComponent::FindAimPoint() const
{
	const AEIHeroCharacter* Hero = GetHero();
	const FVector Eye = Hero->GetPawnViewLocation();
	const FVector End = Eye + Hero->GetViewRotation().Vector() * AimRangeCm;
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(EIAim), false, Hero);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	return GetWorld()->LineTraceSingleByObjectType(Hit, Eye, End, Objects, Query) ? Hit.ImpactPoint : End;
}

void UEICasterComponent::FireAsOwner(EEICast Kind)
{
	const float Cost = GetManaCost(Kind);
	if (!CanAfford(Cost))
	{
		ManaDeniedRemaining = ManaDeniedFlashSeconds;
		return;
	}
	SpendMana(Cost);
	if (Kind == EEICast::SpellFireball)
	{
		SpellReadyAt = GetWorld()->GetTimeSeconds() + GetSpellCooldownSeconds();
	}
	const FVector Target = FindAimPoint();
	AEIHeroCharacter* Hero = GetHero();
	const bool bFireball = Kind != EEICast::Bolt;
	EIVisuals::PlaySound2D(this, bFireball ? TEXT("Feuerball") : TEXT("Feuerstab"));
	if (Hero->GetViewModel() != nullptr && Hero->GetHeroClass() == EEIHeroClass::Arcanist)
	{
		constexpr float StaffShotSeconds = 0.35f;
		constexpr float StaffFireballSeconds = 0.6f;
		Hero->GetViewModel()->PlayPose(bFireball ? TEXT("stab_feuerball") : TEXT("stab_schuss"), bFireball ? StaffFireballSeconds : StaffShotSeconds);
	}
	if (GetOwner()->HasAuthority())
	{
		SpawnProjectile(Kind, Target);
	}
	else
	{
		ServerFire(Kind, Target);
	}
}

void UEICasterComponent::ServerFire_Implementation(EEICast Kind, FVector_NetQuantize Target)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const AEIHeroCharacter* Hero = GetHero();
	const float Cost = GetManaCost(Kind);
	const float ClockAllowance = static_cast<float>(ServerClockTolerance) * Hero->GetClassValues().ManaPerSecond;
	if (Hero->IsDead() || !CanAfford(Cost - ClockAllowance))
	{
		return;
	}
	if (Kind == EEICast::SpellFireball)
	{
		if (Now + ServerClockTolerance < SpellReadyAt)
		{
			return;
		}
		SpellReadyAt = Now + GetSpellCooldownSeconds();
	}
	else if (Hero->GetHeroClass() != EEIHeroClass::Arcanist)
	{
		return;
	}
	SpendMana(Cost);
	SpawnProjectile(Kind, Target);
}

void UEICasterComponent::SpawnProjectile(EEICast Kind, const FVector& Target)
{
	AEIHeroCharacter* Hero = GetHero();
	const FVector Eye = Hero->GetPawnViewLocation();
	const FVector Muzzle = Eye + Hero->GetViewRotation().Vector() * MuzzleForwardCm;
	const FRotator Direction = (Target - Muzzle).Rotation();
	const FTransform Spawn(Direction, Muzzle);
	AEIProjectile* Projectile = GetWorld()->SpawnActorDeferred<AEIProjectile>(AEIProjectile::StaticClass(), Spawn, Hero, Hero, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile != nullptr)
	{
		Projectile->Configure(GetProjectile(Kind), Kind != EEICast::Bolt);
		Projectile->FinishSpawning(Spawn);
	}
}

void UEICasterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ManaDeniedRemaining = FMath::Max(0.0f, ManaDeniedRemaining - DeltaTime);
	if (bHolding && GetHero()->GetViewModel() != nullptr)
	{
		GetHero()->GetViewModel()->HoldPose(TEXT("stab_laden"), GetChargeFraction());
	}
}
