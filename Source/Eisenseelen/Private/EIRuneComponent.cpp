#include "EIRuneComponent.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"
#include "EIGearComponent.h"
#include "EIHeroCharacter.h"
#include "EIProjectile.h"
#include "EIRuneField.h"
#include "EIViewModelComponent.h"
#include "EIVisuals.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr double ServerClockTolerance = 0.1;
	constexpr float FloorSearchAboveCm = 200.0f;
	constexpr float FloorSearchBelowCm = 5000.0f;
	constexpr float PlaceToleranceCm = 300.0f;
	constexpr float StaffCastSeconds = 0.6f;
	const FLinearColor SteamColor(0.9f, 0.95f, 1.0f);
	constexpr float SteamCloudStrength = 0.35f;
	constexpr float SteamCloudShare = 0.85f;
	const FLinearColor ShatterColor(0.55f, 0.85f, 1.0f);
	const FLinearColor SparkColor(1.0f, 0.75f, 0.25f);
	constexpr float SparkBurstRadiusCm = 130.0f;

	bool ReadRune(const FJsonObject& Line, FEIRune& Out, FString& OutError)
	{
		FString Form;
		FString Element;
		if (!Line.TryGetStringField(TEXT("name"), Out.Name) || !Line.TryGetStringField(TEXT("zauber"), Out.SpellName))
		{
			OutError = TEXT("Einer Rune fehlt der Name oder der Zauber.");
			return false;
		}
		if (!Line.TryGetStringField(TEXT("form"), Form) || (Form != TEXT("kreis") && Form != TEXT("wand")))
		{
			OutError = FString::Printf(TEXT("%s braucht als form kreis oder wand."), *Out.Name);
			return false;
		}
		Out.bWall = Form == TEXT("wand");
		if (!Line.TryGetStringField(TEXT("element"), Element) || !EIElementFromKey(Element, Out.Element))
		{
			OutError = FString::Printf(TEXT("%s braucht als element feuer, eis oder blitz."), *Out.Name);
			return false;
		}
		return FEIDataFile::ReadNumber(Line, TEXT("schaden"), 0.0, 100000.0, Out.Damage, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("abklingzeit_s"), 0.0, 600.0, Out.CooldownSeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("groesse_m"), 0.1, 200.0, Out.SizeMeters, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("reichweite_m"), 0.0, 500.0, Out.RangeMeters, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("dauer_s"), 0.0, 600.0, Out.DurationSeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("takt_s"), 0.05, 60.0, Out.TickSeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("gefrieren_s"), 0.0, 60.0, Out.FreezeSeconds, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("verlangsamen_0_bis_1"), 0.0, 1.0, Out.SlowShare, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("brennen_pro_s"), 0.0, 100000.0, Out.BurnPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Line, TEXT("zustand_s"), 0.0, 600.0, Out.StatusSeconds, OutError);
	}

	bool ReadTable(const FJsonObject& Root, FEIRuneTable& Out, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
		if (!Root.TryGetArrayField(TEXT("runen"), Lines))
		{
			OutError = TEXT("Die Liste runen fehlt.");
			return false;
		}
		if (!FEIDataFile::ReadNumber(Root, TEXT("reaktion_fenster_s"), 0.0, 60.0, Out.ReactionWindowSeconds, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("dampfexplosion_schaden"), 0.0, 100000.0, Out.SteamDamage, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("dampfexplosion_radius_m"), 0.1, 200.0, Out.SteamRadiusMeters, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("dampfwolke_s"), 0.1, 60.0, Out.SteamCloudSeconds, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("splitterbruch_schaden"), 0.0, 100000.0, Out.ShatterDamage, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("splitterbruch_radius_m"), 0.1, 200.0, Out.ShatterRadiusMeters, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("funkenflug_schaden"), 0.0, 100000.0, Out.SparkDamage, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("funkenflug_radius_m"), 0.1, 200.0, Out.SparkRadiusMeters, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("funkenflug_ziele"), 0.0, 100.0, Out.SparkTargets, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("funkenflug_brennen_pro_s"), 0.0, 100000.0, Out.SparkBurnPerSecond, OutError)
			|| !FEIDataFile::ReadNumber(Root, TEXT("funkenflug_brennen_s"), 0.0, 60.0, Out.SparkBurnSeconds, OutError))
		{
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Line : *Lines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEIRune Rune;
			if (!Line->TryGetObject(Object) || !ReadRune(**Object, Rune, OutError))
			{
				return false;
			}
			Out.Runes.Add(Rune);
		}
		return true;
	}
}

int32 FEIRuneTable::FindByName(const FString& Name) const
{
	return Runes.IndexOfByPredicate([&Name](const FEIRune& Rune) { return Rune.Name == Name; });
}

bool FEIRuneTable::LoadFromString(const FString& Json, FEIRuneTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEIRuneTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}

bool FEIRuneTable::LoadFromProjectFile(FEIRuneTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("runen.json"), OutError);
	FEIRuneTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}

UEIRuneComponent::UEIRuneComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEIRuneComponent::LoadValues()
{
	FString Error;
	if (!FEIRuneTable::LoadFromProjectFile(Table, Error))
	{
		FEIDataFile::ReportError(TEXT("runen.json"), Error);
	}
}

AEIHeroCharacter* UEIRuneComponent::GetHero() const
{
	return CastChecked<AEIHeroCharacter>(GetOwner());
}

const FEIRune* UEIRuneComponent::GetRune() const
{
	return Table.Runes.IsValidIndex(RuneIndex) ? &Table.Runes[RuneIndex] : nullptr;
}

void UEIRuneComponent::SetRune(int32 Index)
{
	if (Index != RuneIndex)
	{
		RuneIndex = Index;
		ReadyAt = 0.0;
	}
}

float UEIRuneComponent::GetCooldownSeconds() const
{
	const FEIRune* Rune = GetRune();
	return Rune != nullptr ? Rune->CooldownSeconds * GetHero()->GetClassValues().SpellCooldownFactor * GetHero()->GetGear()->GetCooldownFactor() : 0.0f;
}

float UEIRuneComponent::GetCooldownFraction() const
{
	const float Left = static_cast<float>(ReadyAt - GetWorld()->GetTimeSeconds());
	return FMath::Clamp(Left / FMath::Max(GetCooldownSeconds(), KINDA_SMALL_NUMBER), 0.0f, 1.0f);
}

void UEIRuneComponent::FindPlace(const FEIRune& Rune, FVector& OutLocation, float& OutYaw) const
{
	const AEIHeroCharacter* Hero = GetHero();
	OutYaw = static_cast<float>(Hero->GetViewRotation().Yaw);
	FVector Wanted = Hero->GetActorLocation();
	if (Rune.RangeMeters > 0.0f)
	{
		const FVector Eye = Hero->GetPawnViewLocation();
		const FVector End = Eye + Hero->GetViewRotation().Vector() * Rune.RangeMeters * CentimetersPerMeter;
		FHitResult Hit;
		Wanted = GetWorld()->LineTraceSingleByObjectType(Hit, Eye, End, FCollisionObjectQueryParams(ECC_WorldStatic)) ? Hit.ImpactPoint : End;
	}
	FHitResult Floor;
	const FVector From = Wanted + FVector(0.0f, 0.0f, FloorSearchAboveCm);
	const FVector To = Wanted - FVector(0.0f, 0.0f, FloorSearchBelowCm);
	OutLocation = GetWorld()->LineTraceSingleByObjectType(Floor, From, To, FCollisionObjectQueryParams(ECC_WorldStatic)) ? Floor.ImpactPoint : Wanted;
}

void UEIRuneComponent::CastRune()
{
	const FEIRune* Rune = GetRune();
	if (Rune == nullptr || GetWorld()->GetTimeSeconds() < ReadyAt)
	{
		return;
	}
	ReadyAt = GetWorld()->GetTimeSeconds() + GetCooldownSeconds();
	AEIHeroCharacter* Hero = GetHero();
	EIVisuals::PlaySound2D(this, TEXT("Feuerball"));
	if (Hero->GetViewModel() != nullptr)
	{
		Hero->GetViewModel()->PlayPoseIfIdle(Hero->GetHeroClass() == EEIHeroClass::Arcanist ? TEXT("stab_feuerball") : TEXT("schwert_zauber"), StaffCastSeconds);
	}
	FVector Location;
	float Yaw = 0.0f;
	FindPlace(*Rune, Location, Yaw);
	if (GetOwner()->HasAuthority())
	{
		SpawnField(*Rune, Location, Yaw);
	}
	else
	{
		ServerCast(Location, Yaw);
	}
}

void UEIRuneComponent::ServerCast_Implementation(FVector_NetQuantize Location, float Yaw)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const AEIHeroCharacter* Hero = GetHero();
	const FEIRune* Rune = GetRune();
	if (Rune == nullptr || Hero->IsDead() || Now + ServerClockTolerance < ReadyAt)
	{
		return;
	}
	ReadyAt = Now + GetCooldownSeconds();

	// A guest cannot cast farther than the spell reaches.
	const FVector Offset = FVector(Location) - Hero->GetActorLocation();
	const double Allowed = Rune->RangeMeters * CentimetersPerMeter + PlaceToleranceCm;
	const FVector Place = Offset.Size2D() <= Allowed ? FVector(Location) : Hero->GetActorLocation() + Offset.GetSafeNormal2D() * Allowed;
	SpawnField(*Rune, Place, Yaw);
}

void UEIRuneComponent::SpawnField(const FEIRune& Rune, const FVector& Location, float Yaw)
{
	AEIHeroCharacter* Hero = GetHero();
	const FTransform Spawn(FRotator(0.0f, Yaw, 0.0f), Location);
	AEIRuneField* Field = GetWorld()->SpawnActorDeferred<AEIRuneField>(AEIRuneField::StaticClass(), Spawn, Hero, Hero, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Field != nullptr)
	{
		const UEIGearComponent* Gear = Hero->GetGear();
		Field->Configure(Rune, Rune.Damage * Gear->GetRuneFactor(Rune.Element), Gear->GetStatusFactor());
		Field->FinishSpawning(Spawn);
	}
}

void UEIRuneComponent::HitWithElement(AEIEnemy* Enemy, EEIElement Element)
{
	if (Element == EEIElement::Lightning && Enemy->IsFrozen())
	{
		Shatter(Enemy);
		return;
	}
	switch (Enemy->MarkElement(Element, Table.ReactionWindowSeconds))
	{
	case EEIReaction::Steam:
		SteamExplosion(Enemy);
		break;
	case EEIReaction::Sparks:
		SparksFly(Enemy);
		break;
	default:
		break;
	}
}

void UEIRuneComponent::HurtAround(const FVector& Location, float RadiusCm, float Damage)
{
	AEIHeroCharacter* Hero = GetHero();
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(RadiusCm));
	TSet<AActor*> Done;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (Target == nullptr || Done.Contains(Target) || Target->IsA<AEIHeroCharacter>() || !Target->CanBeDamaged())
		{
			continue;
		}
		Done.Add(Target);
		UGameplayStatics::ApplyDamage(Target, Damage, Hero->GetController(), Hero, nullptr);
	}
}

void UEIRuneComponent::SteamExplosion(const AEIEnemy* Enemy)
{
	const FVector Location = Enemy->GetActorLocation();
	HurtAround(Location, Table.SteamRadiusMeters * CentimetersPerMeter, Table.SteamDamage * GetHero()->GetGear()->GetReactionFactor());
	MulticastSteam(Location);
}

void UEIRuneComponent::Shatter(AEIEnemy* Enemy)
{
	const FVector Location = Enemy->GetActorLocation();
	const float RadiusCm = Table.ShatterRadiusMeters * CentimetersPerMeter;
	Enemy->Unfreeze();
	HurtAround(Location, RadiusCm, Table.ShatterDamage * GetHero()->GetGear()->GetReactionFactor());
	MulticastBurst(Location, RadiusCm, ShatterColor);
}

void UEIRuneComponent::SparksFly(AEIEnemy* Enemy)
{
	AEIHeroCharacter* Hero = GetHero();
	const FVector From = Enemy->GetActorLocation();
	const double Reach = Table.SparkRadiusMeters * CentimetersPerMeter;
	TArray<AEIEnemy*> Nearby;
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), From) <= Reach)
		{
			Nearby.Add(*It);
		}
	}
	Nearby.Sort([&From](const AEIEnemy& A, const AEIEnemy& B) { return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From); });
	// The enemy itself comes first, because it is the nearest, and it counts on top of the others.
	const int32 Targets = FMath::Min(Nearby.Num(), FMath::RoundToInt(Table.SparkTargets) + 1);
	for (int32 Index = 0; Index < Targets; ++Index)
	{
		AEIEnemy* Target = Nearby[Index];
		MulticastBurst(Target->GetActorLocation(), SparkBurstRadiusCm, SparkColor);
		Target->ApplyBurn(Table.SparkBurnPerSecond, Table.SparkBurnSeconds * Hero->GetGear()->GetStatusFactor());
		UGameplayStatics::ApplyDamage(Target, Table.SparkDamage * Hero->GetGear()->GetReactionFactor(), Hero->GetController(), Hero, nullptr);
	}
}

void UEIRuneComponent::MulticastBurst_Implementation(FVector_NetQuantize Location, float RadiusCm, FLinearColor Color)
{
	EIVisuals::PlaySoundAt(this, TEXT("ExplosionKlein"), Location);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AEIExplosionFX* FX = GetWorld()->SpawnActor<AEIExplosionFX>(Location, FRotator::ZeroRotator, Params))
	{
		FX->Setup(RadiusCm, Color);
	}
}

void UEIRuneComponent::MulticastSteam_Implementation(FVector_NetQuantize Location)
{
	EIVisuals::PlaySoundAt(this, TEXT("ExplosionGross"), Location);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AEIExplosionFX* FX = GetWorld()->SpawnActor<AEIExplosionFX>(Location, FRotator::ZeroRotator, Params))
	{
		FX->Setup(Table.SteamRadiusMeters * CentimetersPerMeter, SteamColor);
	}
	// The cloud that stays hides what is behind it.
	if (AEIExplosionFX* Cloud = GetWorld()->SpawnActor<AEIExplosionFX>(Location, FRotator::ZeroRotator, Params))
	{
		Cloud->SetupCloud(Table.SteamRadiusMeters * CentimetersPerMeter * SteamCloudShare, SteamColor, Table.SteamCloudSeconds, SteamCloudStrength);
	}
}
