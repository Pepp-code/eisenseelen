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
}

bool FEIHordeValues::LoadFromProjectFile(FEIHordeValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("gegner.json"), OutError);
	const TSharedPtr<FJsonObject>* Horde = nullptr;
	const TSharedPtr<FJsonObject>* Hero = nullptr;
	if (!Root.IsValid())
	{
		return false;
	}
	if (!Root->TryGetObjectField(TEXT("horde"), Horde) || !Root->TryGetObjectField(TEXT("held"), Hero))
	{
		OutError = TEXT("Der Abschnitt horde oder held fehlt.");
		return false;
	}
	FEIHordeValues Loaded;
	if (!FEIDataFile::ReadNumber(**Horde, TEXT("gleichzeitig_max"), 0.0, 200.0, Loaded.MaxAlive, OutError)
		|| !FEIDataFile::ReadNumber(**Horde, TEXT("neue_pro_s"), 0.1, 100.0, Loaded.SpawnsPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(**Horde, TEXT("abstand_min_m"), 1.0, 500.0, Loaded.MinDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Horde, TEXT("abstand_max_m"), 1.0, 500.0, Loaded.MaxDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Hero, TEXT("leben"), 1.0, 100000.0, Loaded.HeroLife, OutError))
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
	const FVector Center = Heroes[FMath::RandRange(0, Heroes.Num() - 1)]->GetActorLocation();
	const float Distance = FMath::FRandRange(Values.MinDistanceMeters, Values.MaxDistanceMeters) * CentimetersPerMeter;
	const float Angle = FMath::FRandRange(0.0f, UE_TWO_PI);
	const FVector Ground(Center.X + FMath::Cos(Angle) * Distance, Center.Y + FMath::Sin(Angle) * Distance, 0.0f);

	FHitResult Floor;
	const FVector TraceStart = Ground + FVector(0.0f, 0.0f, TraceStartHeightCm);
	const FVector TraceEnd = Ground - FVector(0.0f, 0.0f, TraceDepthCm);
	if (!GetWorld()->LineTraceSingleByObjectType(Floor, TraceStart, TraceEnd, FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	const FVector Location = Floor.ImpactPoint + FVector(0.0f, 0.0f, StandHeightCm);
	return GetWorld()->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Location, FRotator::ZeroRotator, Params);
}

void UEIHordeDirector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bHordeActive)
	{
		return;
	}
	SpawnBudget += DeltaTime * Values.SpawnsPerSecond;
	while (SpawnBudget >= 1.0f)
	{
		SpawnBudget -= 1.0f;
		if (CountEnemies() < FMath::RoundToInt(Values.MaxAlive))
		{
			SpawnOne();
		}
	}
}
