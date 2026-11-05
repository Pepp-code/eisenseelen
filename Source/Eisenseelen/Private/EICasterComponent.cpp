#include "EICasterComponent.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIGearComponent.h"
#include "EIHeroCharacter.h"
#include "EIProjectile.h"
#include "EIRuneComponent.h"
#include "EIViewModelComponent.h"
#include "EIVisuals.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float AimRangeCm = 20000.0f;
	constexpr float MuzzleForwardCm = 60.0f;
	// The owner's clock runs slightly ahead of the server's, so the server allows a little early firing.
	constexpr double ServerClockTolerance = 0.1;
	constexpr float ManaDeniedFlashSeconds = 0.3f;

	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float SlowestChargeFactor = 0.1f;
	// The flash starts at the hand, because straight ahead of the eye its end would fill the view.
	const FVector ChainStartCm(100.0f, 24.0f, -30.0f);

	bool ReadSpell(const FJsonObject& Spells, const TCHAR* Key, FEISpellValues& Out, TSharedPtr<FJsonObject>& OutObject, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Spell = nullptr;
		if (!Spells.TryGetObjectField(Key, Spell) || !(*Spell)->TryGetStringField(TEXT("name"), Out.Name))
		{
			OutError = FString::Printf(TEXT("Der Zauber %s fehlt oder hat keinen Namen."), Key);
			return false;
		}
		OutObject = *Spell;
		return FEIDataFile::ReadNumber(**Spell, TEXT("schaden"), 0.0, 100000.0, Out.Damage, OutError)
			&& FEIDataFile::ReadNumber(**Spell, TEXT("mana_kosten"), 0.0, 10000.0, Out.ManaCost, OutError)
			&& FEIDataFile::ReadNumber(**Spell, TEXT("abklingzeit_s"), 0.0, 600.0, Out.CooldownSeconds, OutError);
	}

	bool ReadFlight(const FJsonObject& Spell, FEISpellValues& Out, FString& OutError)
	{
		return FEIDataFile::ReadNumber(Spell, TEXT("radius_m"), 0.1, 50.0, Out.RadiusMeters, OutError)
			&& FEIDataFile::ReadNumber(Spell, TEXT("tempo_m_pro_s"), 1.0, 500.0, Out.SpeedMetersPerSecond, OutError);
	}

	bool IsChainTarget(const AActor* Actor)
	{
		const AEIEnemy* Enemy = Cast<AEIEnemy>(Actor);
		return Actor != nullptr && !Actor->IsA<AEIHeroCharacter>() && Actor->CanBeDamaged() && (Enemy == nullptr || !Enemy->IsDead());
	}
}

FEICasterValues::FEICasterValues()
{
	Fireball.Damage = 120.0f;
	Fireball.RadiusMeters = 4.0f;
	Fireball.SpeedMetersPerSecond = 30.0f;
	Fireball.ManaCost = 40.0f;
}

const FEISpellValues& FEICasterValues::GetSpell(EEIElement Element) const
{
	switch (Element)
	{
	case EEIElement::Ice: return IceLance;
	case EEIElement::Lightning: return ChainLightning;
	default: return Fireball;
	}
}

bool FEICasterValues::LoadFromProjectFile(FEICasterValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("waffen.json"), OutError);
	const TSharedPtr<FJsonObject>* Staff = nullptr;
	const TSharedPtr<FJsonObject>* Spells = nullptr;
	if (!Root.IsValid())
	{
		return false;
	}
	if (!Root->TryGetObjectField(TEXT("stab"), Staff) || !Root->TryGetObjectField(TEXT("zauber"), Spells))
	{
		OutError = TEXT("Der Abschnitt stab oder zauber fehlt.");
		return false;
	}
	FEICasterValues Loaded;
	TSharedPtr<FJsonObject> Spell;
	if (!FEIDataFile::ReadNumber(**Staff, TEXT("halten_fuer_zauber_s"), 0.05, 5.0, Loaded.HoldForSpellSeconds, OutError)
		|| !ReadSpell(**Spells, TEXT("feuerball"), Loaded.Fireball, Spell, OutError)
		|| !ReadFlight(*Spell, Loaded.Fireball, OutError)
		|| !ReadSpell(**Spells, TEXT("eislanze"), Loaded.IceLance, Spell, OutError)
		|| !ReadFlight(*Spell, Loaded.IceLance, OutError)
		|| !FEIDataFile::ReadNumber(*Spell, TEXT("verlangsamen_0_bis_1"), 0.0, 1.0, Loaded.IceLance.SlowShare, OutError)
		|| !FEIDataFile::ReadNumber(*Spell, TEXT("zustand_s"), 0.0, 600.0, Loaded.IceLance.StatusSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Spell, TEXT("gefrieren_s"), 0.0, 60.0, Loaded.IceLance.FreezeSeconds, OutError)
		|| !ReadSpell(**Spells, TEXT("kettenblitz"), Loaded.ChainLightning, Spell, OutError)
		|| !FEIDataFile::ReadNumber(*Spell, TEXT("zielhilfe_m"), 0.1, 50.0, Loaded.ChainLightning.RadiusMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Spell, TEXT("reichweite_m"), 1.0, 500.0, Loaded.ChainLightning.RangeMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Spell, TEXT("spruenge"), 0.0, 100.0, Loaded.ChainLightning.Jumps, OutError)
		|| !FEIDataFile::ReadNumber(*Spell, TEXT("sprung_m"), 0.1, 200.0, Loaded.ChainLightning.JumpMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Spell, TEXT("abzug_je_sprung_0_bis_1"), 0.0, 1.0, Loaded.ChainLightning.JumpFalloff, OutError))
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
	if (!FEICasterValues::LoadFromProjectFile(BaseValues, Error))
	{
		FEIDataFile::ReportError(TEXT("waffen.json"), Error);
	}
	Values = BaseValues;
}

void UEICasterComponent::ApplyGear(const UEIGearComponent& Gear)
{
	Values = BaseValues;
	Values.HoldForSpellSeconds /= FMath::Max(SlowestChargeFactor, 1.0f + Gear.GetStat(EEIStat::ChargeSpeed));
	const FEIWeaponForm* Form = Gear.GetWeaponForm();
	if (Form != nullptr && Form->bStaff)
	{
		Values.Bolt.Damage = Form->BoltDamage;
		Values.Bolt.RadiusMeters = Form->BoltRadiusMeters;
		Values.Bolt.SpeedMetersPerSecond = Form->BoltSpeedMetersPerSecond;
		Values.Bolt.ManaCost = Form->BoltMana;
		Values.bBoltPierces = Form->bBoltPierces;
	}
}

const FEISpellValues& UEICasterComponent::GetSpell() const
{
	return Values.GetSpell(GetHero()->GetGear()->GetWeaponElement());
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
	return FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - HoldStartedAt) / Values.HoldForSpellSeconds, 0.0f, 1.0f);
}

float UEICasterComponent::GetSpellCooldownSeconds() const
{
	return GetSpell().CooldownSeconds * GetHero()->GetClassValues().SpellCooldownFactor * GetHero()->GetGear()->GetCooldownFactor();
}

float UEICasterComponent::GetSpellCooldownFraction() const
{
	const float Left = static_cast<float>(SpellReadyAt - GetWorld()->GetTimeSeconds());
	return FMath::Clamp(Left / FMath::Max(GetSpellCooldownSeconds(), KINDA_SMALL_NUMBER), 0.0f, 1.0f);
}

float UEICasterComponent::GetMaxMana() const
{
	// A class without mana stays without, whatever a helmet says.
	const float ClassMana = GetHero()->GetClassValues().Mana;
	return ClassMana > 0.0f ? ClassMana + GetHero()->GetGear()->GetStat(EEIStat::Mana) : 0.0f;
}

float UEICasterComponent::GetMana() const
{
	const float Regained = static_cast<float>(GetWorld()->GetTimeSeconds() - ManaStampedAt) * GetHero()->GetClassValues().ManaPerSecond * ManaFactor;
	return FMath::Min(ManaAtStamp + Regained, GetMaxMana());
}

void UEICasterComponent::SpendMana(float Amount)
{
	ManaAtStamp = FMath::Max(0.0f, GetMana() - Amount);
	ManaStampedAt = GetWorld()->GetTimeSeconds();
}

void UEICasterComponent::SetManaFactor(float Factor)
{
	// What came back until now counts at the old rate.
	SpendMana(0.0f);
	ManaFactor = Factor;
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

bool UEICasterComponent::CanAffordSpell() const
{
	return CanAfford(GetManaCost(EEICast::ChargedSpell));
}

void UEICasterComponent::FireBolt()
{
	FireAsOwner(EEICast::Bolt);
}

float UEICasterComponent::GetManaCost(EEICast Kind) const
{
	if (Kind == EEICast::Spell)
	{
		return 0.0f;
	}
	const float Cost = Kind == EEICast::Bolt ? Values.Bolt.ManaCost : GetSpell().ManaCost;
	return Cost * GetHero()->GetGear()->GetManaCostFactor();
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
		FireAsOwner(EEICast::ChargedSpell);
	}
}

void UEICasterComponent::CastSpell()
{
	if (GetWorld()->GetTimeSeconds() >= SpellReadyAt)
	{
		FireAsOwner(EEICast::Spell);
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
	if (Kind == EEICast::Spell)
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
	else if (Hero->GetViewModel() != nullptr)
	{
		constexpr float SwordCastSeconds = 0.5f;
		Hero->GetViewModel()->PlayPoseIfIdle(TEXT("schwert_zauber"), SwordCastSeconds);
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
	const float ClockAllowance = static_cast<float>(ServerClockTolerance) * Hero->GetClassValues().ManaPerSecond * ManaFactor;
	if (Hero->IsDead() || !CanAfford(Cost - ClockAllowance))
	{
		return;
	}
	if (Kind == EEICast::Spell)
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
	const UEIGearComponent* Gear = Hero->GetGear();
	const EEIElement Element = Gear->GetWeaponElement();
	// A charge of the staff is a shot, and only the spell key is a spell.
	const float Factor = Kind == EEICast::Spell ? Gear->GetSpellFactor() : Gear->GetShotFactor();
	if (Kind != EEICast::Bolt && Element == EEIElement::Lightning)
	{
		CastChain(Target, Values.ChainLightning.Damage * Factor);
		return;
	}

	FEIProjectileSetup Setup;
	Setup.Element = Element;
	if (Kind == EEICast::Bolt)
	{
		Setup.Damage = Values.Bolt.Damage * Factor;
		Setup.RadiusCm = Values.Bolt.RadiusMeters * CentimetersPerMeter;
		Setup.SpeedCm = Values.Bolt.SpeedMetersPerSecond * CentimetersPerMeter;
		Setup.bPierces = Values.bBoltPierces;
	}
	else
	{
		const FEISpellValues& Spell = GetSpell();
		Setup.Damage = Spell.Damage * Factor;
		Setup.RadiusCm = Spell.RadiusMeters * CentimetersPerMeter;
		Setup.SpeedCm = Spell.SpeedMetersPerSecond * CentimetersPerMeter;
		Setup.bBig = true;
		Setup.bCarriesElement = true;
		Setup.bPierces = Element == EEIElement::Ice;
		Setup.SlowShare = Spell.SlowShare;
		Setup.SlowSeconds = Spell.StatusSeconds * Gear->GetStatusFactor();
		Setup.FreezeSeconds = Spell.FreezeSeconds * Gear->GetStatusFactor();
	}
	const FVector Eye = Hero->GetPawnViewLocation();
	const FVector Muzzle = Eye + Hero->GetViewRotation().Vector() * MuzzleForwardCm;
	const FRotator Direction = (Target - Muzzle).Rotation();
	const FTransform Spawn(Direction, Muzzle);
	AEIProjectile* Projectile = GetWorld()->SpawnActorDeferred<AEIProjectile>(AEIProjectile::StaticClass(), Spawn, Hero, Hero, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile != nullptr)
	{
		Projectile->Configure(Setup);
		Projectile->FinishSpawning(Spawn);
	}
}

void UEICasterComponent::CastChain(const FVector& Target, float Damage)
{
	AEIHeroCharacter* Hero = GetHero();
	const FEISpellValues& Spell = Values.ChainLightning;
	const FVector Eye = Hero->GetPawnViewLocation();
	const FVector End = Eye + (Target - Eye).GetSafeNormal() * Spell.RangeMeters * CentimetersPerMeter;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(EIChain), false, Hero);

	// Walls stop the first strike, and it forgives a slightly poor aim.
	FHitResult Wall;
	const FVector Stop = GetWorld()->LineTraceSingleByObjectType(Wall, Eye, End, FCollisionObjectQueryParams(ECC_WorldStatic), Query) ? FVector(Wall.ImpactPoint) : End;
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Eye, Stop, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Spell.RadiusMeters * CentimetersPerMeter), Query);
	AActor* Current = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		if (IsChainTarget(Hit.GetActor()))
		{
			Current = Hit.GetActor();
			break;
		}
	}

	TArray<FVector_NetQuantize> Points;
	Points.Add(Eye + Hero->GetViewRotation().RotateVector(ChainStartCm));
	if (Current == nullptr)
	{
		Points.Add(Stop);
	}
	TArray<AActor*> Struck;
	for (int32 Jump = 0; Current != nullptr; ++Jump)
	{
		Struck.Add(Current);
		Points.Add(Current->GetActorLocation());
		if (AEIEnemy* Enemy = Cast<AEIEnemy>(Current))
		{
			Hero->GetRunes()->HitWithElement(Enemy, EEIElement::Lightning);
		}
		UGameplayStatics::ApplyDamage(Current, Damage * FMath::Max(0.0f, 1.0f - Spell.JumpFalloff * Jump), Hero->GetController(), Hero, UEISpellDamage::StaticClass());
		Current = Jump < FMath::RoundToInt(Spell.Jumps) ? FindNextChainTarget(Current->GetActorLocation(), Struck) : nullptr;
	}
	MulticastChain(Points);
}

AActor* UEICasterComponent::FindNextChainTarget(const FVector& From, const TArray<AActor*>& Struck) const
{
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, From, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Values.ChainLightning.JumpMeters * CentimetersPerMeter));
	AActor* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!IsChainTarget(Candidate) || Struck.Contains(Candidate))
		{
			continue;
		}
		const double Distance = FVector::DistSquared(Candidate->GetActorLocation(), From);
		FCollisionQueryParams WallQuery(SCENE_QUERY_STAT(EIChainWall), false, Candidate);
		if (Distance < NearestDistance && !GetWorld()->LineTraceTestByObjectType(From, Candidate->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), WallQuery))
		{
			Nearest = Candidate;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

void UEICasterComponent::MulticastChain_Implementation(const TArray<FVector_NetQuantize>& Points)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		if (AEIBeamFX* Beam = GetWorld()->SpawnActor<AEIBeamFX>(Points[Index - 1], FRotator::ZeroRotator, Params))
		{
			Beam->Setup(Points[Index - 1], Points[Index], EIElementColor(EEIElement::Lightning));
		}
		EIVisuals::PlaySoundAt(this, TEXT("ExplosionKlein"), Points[Index]);
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
