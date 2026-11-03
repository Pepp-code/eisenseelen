#include "EIHordeDirector.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIHeroCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float TraceStartHeightCm = 3000.0f;
	constexpr float TraceDepthCm = 500.0f;
	constexpr float StandHeightCm = 92.0f;
	// Higher than this, the place is on top of something and not on the ground.
	constexpr float MaxGroundHeightCm = 50.0f;
	constexpr int32 PlaceTries = 8;
	constexpr float InViewDot = 0.35f;
	constexpr float LeftBehindCheckSeconds = 1.0f;

	bool CanSee(const AEIHeroCharacter& Hero, const FVector& Location)
	{
		const FVector Eye = Hero.GetPawnViewLocation();
		if (FVector::DotProduct(Hero.GetViewRotation().Vector(), (Location - Eye).GetSafeNormal()) < InViewDot)
		{
			return false;
		}
		FCollisionQueryParams Query(SCENE_QUERY_STAT(EISpawnSight), false, &Hero);
		return !Hero.GetWorld()->LineTraceTestByObjectType(Eye, Location, FCollisionObjectQueryParams(ECC_WorldStatic), Query);
	}
}

bool FEIHordeValues::LoadFromProjectFile(FEIHordeValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("gegner.json"), OutError);
	const TSharedPtr<FJsonObject>* Horde = nullptr;
	if (!Root.IsValid())
	{
		return false;
	}
	if (!Root->TryGetObjectField(TEXT("horde"), Horde))
	{
		OutError = TEXT("Der Abschnitt horde fehlt.");
		return false;
	}
	FEIHordeValues Loaded;
	const TSharedPtr<FJsonObject>* Director = nullptr;
	if (!Root->TryGetObjectField(TEXT("direktor"), Director))
	{
		OutError = TEXT("Der Abschnitt direktor fehlt.");
		return false;
	}
	if (!FEIDataFile::ReadNumber(**Horde, TEXT("gleichzeitig_max"), 0.0, 200.0, Loaded.MaxAlive, OutError)
		|| !FEIDataFile::ReadNumber(**Horde, TEXT("neue_pro_s"), 0.1, 100.0, Loaded.SpawnsPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(**Horde, TEXT("abstand_min_m"), 1.0, 500.0, Loaded.MinDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Horde, TEXT("abstand_max_m"), 1.0, 500.0, Loaded.MaxDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Horde, TEXT("zu_weit_weg_m"), 1.0, 1000.0, Loaded.TooFarMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Director, TEXT("bedrohung_voll"), 0.0, 10000.0, Loaded.Director.FullThreat, OutError)
		|| !FEIDataFile::ReadNumber(**Director, TEXT("anteil_1_spieler"), 0.0, 10.0, Loaded.Director.PlayerShare[0], OutError)
		|| !FEIDataFile::ReadNumber(**Director, TEXT("anteil_2_spieler"), 0.0, 10.0, Loaded.Director.PlayerShare[1], OutError)
		|| !FEIDataFile::ReadNumber(**Director, TEXT("anteil_3_spieler"), 0.0, 10.0, Loaded.Director.PlayerShare[2], OutError)
		|| !FEIDataFile::ReadNumber(**Director, TEXT("anteil_4_spieler"), 0.0, 10.0, Loaded.Director.PlayerShare[3], OutError)
		|| !FEIDataFile::ReadNumber(**Director, TEXT("anteil_erkundung"), 0.0, 10.0, Loaded.Director.ExploringShare, OutError)
		|| !FEIDataFile::ReadNumber(**Director, TEXT("anteil_leuchtfeuer"), 0.0, 10.0, Loaded.Director.BeaconShare, OutError)
		|| !FEIDataFile::ReadNumber(**Director, TEXT("bedrohung_spitze"), 0.0, 10000.0, Loaded.Director.PeakThreat, OutError))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}

UEIHordeDirector::UEIHordeDirector()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEIHordeDirector::LoadValues()
{
	FString Error;
	if (!FEIHordeValues::LoadFromProjectFile(Values, Error))
	{
		FEIDataFile::ReportError(TEXT("gegner.json"), Error);
	}
}

void UEIHordeDirector::SetHordeActive(bool bActive)
{
	bHordeActive = bActive;
	SpawnBudget = 0.0f;
	SetThreatBudget(Values.MaxAlive, 1.0f);
}

void UEIHordeDirector::SetThreatBudget(float Threat, float SpawnRateShare)
{
	ThreatBudget = Threat;
	SpawnShare = SpawnRateShare;
}

float UEIHordeDirector::CountThreat() const
{
	float Threat = 0.0f;
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		Threat += It->IsDead() ? 0.0f : It->GetValues().Threat;
	}
	return Threat;
}

int32 UEIHordeDirector::CountEnemies() const
{
	int32 Count = 0;
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		Count += It->IsDead() ? 0 : 1;
	}
	return Count;
}

void UEIHordeDirector::RemoveAllEnemies()
{
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
}

AEIEnemy* UEIHordeDirector::SpawnOne()
{
	TArray<AEIHeroCharacter*> Heroes;
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			Heroes.Add(*It);
		}
	}
	if (Heroes.IsEmpty())
	{
		return nullptr;
	}
	const float MinDistance = Values.MinDistanceMeters * CentimetersPerMeter;
	for (int32 Try = 0; Try < PlaceTries; ++Try)
	{
		const FVector Center = Heroes[FMath::RandRange(0, Heroes.Num() - 1)]->GetActorLocation();
		const float Distance = FMath::FRandRange(Values.MinDistanceMeters, Values.MaxDistanceMeters) * CentimetersPerMeter;
		const float Angle = FMath::FRandRange(0.0f, UE_TWO_PI);
		const FVector Ground(Center.X + FMath::Cos(Angle) * Distance, Center.Y + FMath::Sin(Angle) * Distance, 0.0f);

		FHitResult Floor;
		const FVector TraceStart = Ground + FVector(0.0f, 0.0f, TraceStartHeightCm);
		const FVector TraceEnd = Ground - FVector(0.0f, 0.0f, TraceDepthCm);
		if (!GetWorld()->LineTraceSingleByObjectType(Floor, TraceStart, TraceEnd, FCollisionObjectQueryParams(ECC_WorldStatic)) || Floor.ImpactPoint.Z > MaxGroundHeightCm)
		{
			continue;
		}
		const FVector Location = Floor.ImpactPoint + FVector(0.0f, 0.0f, StandHeightCm);
		const bool bBadPlace = Heroes.ContainsByPredicate([&Location, MinDistance](const AEIHeroCharacter* Hero)
		{
			return FVector::Dist2D(Hero->GetActorLocation(), Location) < MinDistance || CanSee(*Hero, Location);
		});
		if (bBadPlace)
		{
			continue;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		return GetWorld()->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Location, FRotator::ZeroRotator, Params);
	}
	return nullptr;
}

void UEIHordeDirector::RemoveEnemiesLeftBehind()
{
	const double TooFar = Values.TooFarMeters * CentimetersPerMeter;
	for (TActorIterator<AEIEnemy> Enemy(GetWorld()); Enemy; ++Enemy)
	{
		bool bNearHero = false;
		for (TActorIterator<AEIHeroCharacter> Hero(GetWorld()); Hero; ++Hero)
		{
			bNearHero = bNearHero || FVector::Dist2D(Enemy->GetActorLocation(), Hero->GetActorLocation()) <= TooFar;
		}
		if (!bNearHero)
		{
			Enemy->Destroy();
		}
	}
}

void UEIHordeDirector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bHordeActive)
	{
		return;
	}
	// Whoever runs away from the enemies must not outrun the danger: the budget comes back around the hero.
	LeftBehindCheckRemaining -= DeltaTime;
	if (LeftBehindCheckRemaining <= 0.0f)
	{
		LeftBehindCheckRemaining = LeftBehindCheckSeconds;
		RemoveEnemiesLeftBehind();
	}
	SpawnBudget += DeltaTime * Values.SpawnsPerSecond * SpawnShare;
	while (SpawnBudget >= 1.0f)
	{
		SpawnBudget -= 1.0f;
		if (CountThreat() < ThreatBudget)
		{
			SpawnOne();
		}
	}
}
