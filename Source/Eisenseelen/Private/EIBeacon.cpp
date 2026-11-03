#include "EIBeacon.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EIDataFile.h"
#include "EIVisuals.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float PillarHeightCm = 900.0f;
	constexpr float PillarWidthCm = 120.0f;
	constexpr float FireSizeCm = 180.0f;
	constexpr float ZoneDiscHeightCm = 3.0f;
	const FLinearColor PillarColor(0.1f, 0.1f, 0.12f);
	const FLinearColor ColdFireColor(0.05f, 0.05f, 0.06f);
	const FLinearColor BurningFireColor(1.0f, 0.4f, 0.05f);
	constexpr float BurningGlow = 12.0f;
	const FLinearColor ZoneColor(1.0f, 0.35f, 0.05f);
	constexpr float ZoneStrength = 0.12f;
	constexpr float LightIntensity = 200000.0f;
	constexpr float LightReachCm = 4000.0f;
}

AEIBeacon::AEIBeacon()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Pillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillar"));
	Pillar->SetupAttachment(Root);
	Pillar->SetStaticMesh(Cylinder.Object);
	Pillar->SetRelativeLocation(FVector(0.0f, 0.0f, PillarHeightCm / 2.0f));
	Pillar->SetRelativeScale3D(FVector(PillarWidthCm, PillarWidthCm, PillarHeightCm) / 100.0f);

	Fire = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fire"));
	Fire->SetupAttachment(Root);
	Fire->SetStaticMesh(Sphere.Object);
	Fire->SetRelativeLocation(FVector(0.0f, 0.0f, PillarHeightCm + FireSizeCm / 2.0f));
	Fire->SetRelativeScale3D(FVector(FireSizeCm / 100.0f));
	Fire->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneDisc"));
	ZoneDisc->SetupAttachment(Root);
	ZoneDisc->SetStaticMesh(Cylinder.Object);
	ZoneDisc->SetRelativeLocation(FVector(0.0f, 0.0f, ZoneDiscHeightCm));
	ZoneDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneDisc->SetCastShadow(false);
	ZoneDisc->SetVisibility(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(Fire->GetRelativeLocation());
	Light->SetLightColor(BurningFireColor);
	Light->SetIntensity(LightIntensity);
	Light->SetAttenuationRadius(LightReachCm);
	Light->SetCastShadows(false);
	Light->SetVisibility(false);
}

void AEIBeacon::BeginPlay()
{
	Super::BeginPlay();

	FString Error;
	if (!FEIRunValues::LoadFromProjectFile(Values, Error))
	{
		FEIDataFile::ReportError(TEXT("lauf.json"), Error);
	}
	EIVisuals::SetColor(Pillar, PillarColor);
	FireMaterial = EIVisuals::SetColor(Fire, ColdFireColor);
	EIVisuals::SetGlow(ZoneDisc, ZoneColor, ZoneStrength);
	// The engine's cylinder is 1 m wide, so its scale is its width in metres.
	const float ZoneWidth = Values.BeaconZoneMeters * 2.0f;
	ZoneDisc->SetRelativeScale3D(FVector(ZoneWidth, ZoneWidth, 0.02f));
}

void AEIBeacon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIBeacon, bLit);
}

void AEIBeacon::SetLit(bool bNewLit)
{
	bLit = bNewLit;
	OnRep_Lit();
}

void AEIBeacon::OnRep_Lit()
{
	ZoneDisc->SetVisibility(bLit);
	Light->SetVisibility(bLit);
	if (FireMaterial != nullptr)
	{
		FireMaterial->SetVectorParameterValue(TEXT("Farbe"), bLit ? BurningFireColor : ColdFireColor);
		FireMaterial->SetScalarParameterValue(TEXT("Leuchten"), bLit ? BurningGlow : 0.0f);
	}
	if (bLit)
	{
		EIVisuals::PlaySoundAt(this, TEXT("Feuerball"), GetFireLocation());
	}
}

bool AEIBeacon::IsInReach(const AActor* Actor) const
{
	return FVector::Dist2D(Actor->GetActorLocation(), GetActorLocation()) <= Values.BeaconReachMeters * CentimetersPerMeter;
}

bool AEIBeacon::IsInZone(const AActor* Actor) const
{
	return FVector::Dist2D(Actor->GetActorLocation(), GetActorLocation()) <= Values.BeaconZoneMeters * CentimetersPerMeter;
}

FVector AEIBeacon::GetFireLocation() const
{
	return Fire->GetComponentLocation();
}
