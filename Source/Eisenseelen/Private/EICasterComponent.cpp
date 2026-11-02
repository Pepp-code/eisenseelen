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
	constexpr double ServerCooldownTolerance = 0.1;

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
			&& FEIDataFile::ReadNumber(**Projectile, TEXT("abklingzeit_s"), 0.0, 120.0, Out.CooldownSeconds, OutError);
	}
}

FEICasterValues::FEICasterValues()
{
	Fireball.Damage = 120.0f;
	Fireball.RadiusMeters = 4.0f;
	Fireball.SpeedMetersPerSecond = 30.0f;
	Fireball.CooldownSeconds = 6.0f;
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
		|| !FEIDataFile::ReadNumber(**Staff, TEXT("halten_fuer_feuerball_s"), 0.05, 5.0, Loaded.HoldForFireballSeconds, OutError))
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

float UEICasterComponent::GetFireballCooldownSeconds() const
{
	return Values.Fireball.CooldownSeconds * GetHero()->GetClassValues().SpellCooldownFactor;
}

float UEICasterComponent::GetFireballCooldownFraction() const
{
	const float Left = static_cast<float>(FireballReadyAt - GetWorld()->GetTimeSeconds());
	return FMath::Clamp(Left / FMath::Max(GetFireballCooldownSeconds(), KINDA_SMALL_NUMBER), 0.0f, 1.0f);
}

void UEICasterComponent::PressStaff()
{
	bHolding = true;
	HoldStartedAt = GetWorld()->GetTimeSeconds();
}

void UEICasterComponent::ReleaseStaff()
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
		CastFireball();
	}
	else if (GetWorld()->GetTimeSeconds() >= BoltReadyAt)
	{
		FireAsOwner(EEIProjectileKind::Bolt);
	}
}

void UEICasterComponent::CastFireball()
{
	if (GetWorld()->GetTimeSeconds() >= FireballReadyAt)
	{
		FireAsOwner(EEIProjectileKind::Fireball);
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

void UEICasterComponent::FireAsOwner(EEIProjectileKind Kind)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const FVector Target = FindAimPoint();
	AEIHeroCharacter* Hero = GetHero();
	const bool bFireball = Kind == EEIProjectileKind::Fireball;
	if (bFireball)
	{
		FireballReadyAt = Now + GetFireballCooldownSeconds();
	}
	else
	{
		BoltReadyAt = Now + Values.Bolt.CooldownSeconds;
	}
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

void UEICasterComponent::ServerFire_Implementation(EEIProjectileKind Kind, FVector_NetQuantize Target)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (GetHero()->IsDead())
	{
		return;
	}
	if (Kind == EEIProjectileKind::Fireball)
	{
		if (Now + ServerCooldownTolerance < FireballReadyAt)
		{
			return;
		}
		FireballReadyAt = Now + GetFireballCooldownSeconds();
	}
	else
	{
		if (GetHero()->GetHeroClass() != EEIHeroClass::Arcanist || Now + ServerCooldownTolerance < BoltReadyAt)
		{
			return;
		}
		BoltReadyAt = Now + Values.Bolt.CooldownSeconds;
	}
	SpawnProjectile(Kind, Target);
}

void UEICasterComponent::SpawnProjectile(EEIProjectileKind Kind, const FVector& Target)
{
	AEIHeroCharacter* Hero = GetHero();
	const FVector Eye = Hero->GetPawnViewLocation();
	const FVector Muzzle = Eye + Hero->GetViewRotation().Vector() * MuzzleForwardCm;
	const FRotator Direction = (Target - Muzzle).Rotation();
	const FTransform Spawn(Direction, Muzzle);
	AEIProjectile* Projectile = GetWorld()->SpawnActorDeferred<AEIProjectile>(AEIProjectile::StaticClass(), Spawn, Hero, Hero, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile != nullptr)
	{
		Projectile->Configure(Kind == EEIProjectileKind::Fireball ? Values.Fireball : Values.Bolt, Kind == EEIProjectileKind::Fireball);
		Projectile->FinishSpawning(Spawn);
	}
}

void UEICasterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bHolding && GetHero()->GetViewModel() != nullptr)
	{
		GetHero()->GetViewModel()->HoldPose(TEXT("stab_laden"), GetChargeFraction());
	}
}
