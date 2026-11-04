#include "EIFlare.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EIDataFile.h"
#include "EIVisuals.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float RiseSeconds = 1.5f;
	constexpr float RiseHeightCm = 3500.0f;
	constexpr float RiseEasing = 2.0f;
	constexpr float SparkSizeCm = 160.0f;
	constexpr float TrailWidthCm = 22.0f;
	constexpr float ZoneDiscHeightCm = 3.0f;
	const FLinearColor FlareColor(1.0f, 0.4f, 0.05f);
	constexpr float SparkGlow = 12.0f;
	constexpr float TrailStrength = 0.5f;
	constexpr float ZoneStrength = 0.12f;
	constexpr float LightIntensity = 150000.0f;
	constexpr float LightReachCm = 6000.0f;
}

AEIFlare::AEIFlare()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	const auto AddPart = [this, Root](const TCHAR* Name, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(Mesh);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCastShadow(false);
		return Part;
	};
	Spark = AddPart(TEXT("Spark"), Sphere.Object);
	Spark->SetRelativeScale3D(FVector(SparkSizeCm / 100.0f));
	Trail = AddPart(TEXT("Trail"), Cylinder.Object);
	ZoneDisc = AddPart(TEXT("ZoneDisc"), Cylinder.Object);
	ZoneDisc->SetRelativeLocation(FVector(0.0f, 0.0f, ZoneDiscHeightCm));

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Spark);
	Light->SetLightColor(FlareColor);
	Light->SetIntensity(LightIntensity);
	Light->SetAttenuationRadius(LightReachCm);
	Light->SetCastShadows(false);
}

void AEIFlare::BeginPlay()
{
	Super::BeginPlay();

	FString Error;
	if (!FEIRunValues::LoadFromProjectFile(Values, Error))
	{
		FEIDataFile::ReportError(TEXT("lauf.json"), Error);
	}
	EIVisuals::SetColor(Spark, FlareColor, SparkGlow);
	EIVisuals::SetGlow(Trail, FlareColor, TrailStrength);
	EIVisuals::SetGlow(ZoneDisc, FlareColor, ZoneStrength);
	// The engine's cylinder is 1 m wide, so its scale is its width in metres.
	const float ZoneWidth = Values.FlareZoneMeters * 2.0f;
	ZoneDisc->SetRelativeScale3D(FVector(ZoneWidth, ZoneWidth, 0.02f));
	ShowRise(0.0f);
	EIVisuals::PlaySoundAt(this, TEXT("Sprungfeld"), GetActorLocation());
}

void AEIFlare::ShowRise(float Share)
{
	const float Height = FMath::InterpEaseOut(0.0f, RiseHeightCm, Share, RiseEasing);
	Spark->SetRelativeLocation(FVector(0.0f, 0.0f, Height));
	Trail->SetRelativeLocation(FVector(0.0f, 0.0f, Height / 2.0f));
	Trail->SetRelativeScale3D(FVector(TrailWidthCm / 100.0f, TrailWidthCm / 100.0f, Height / 100.0f));
}

void AEIFlare::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RisenSeconds = FMath::Min(RisenSeconds + DeltaSeconds, RiseSeconds);
	ShowRise(RisenSeconds / RiseSeconds);
	if (RisenSeconds >= RiseSeconds)
	{
		SetActorTickEnabled(false);
	}
}

bool AEIFlare::IsInZone(const AActor* Actor) const
{
	return FVector::Dist2D(Actor->GetActorLocation(), GetActorLocation()) <= Values.FlareZoneMeters * CentimetersPerMeter;
}

FVector AEIFlare::GetLightLocation() const
{
	return Spark->GetComponentLocation();
}
