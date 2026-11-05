#include "EIForge.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EIVisuals.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FVector AnvilFootSizeCm(50.0f, 40.0f, 60.0f);
	const FVector AnvilTopSizeCm(90.0f, 36.0f, 26.0f);
	constexpr float AnvilYCm = -90.0f;
	const FVector HearthSizeCm(140.0f, 120.0f, 80.0f);
	constexpr float HearthYCm = 90.0f;
	const FVector EmbersSizeCm(110.0f, 90.0f, 8.0f);
	const FVector ChimneySizeCm(60.0f, 60.0f, 260.0f);
	constexpr float ChimneyBackCm = -40.0f;
	constexpr float MarkAboveCm = 300.0f;
	constexpr float EmberGlow = 3.0f;
	constexpr float LightIntensity = 4000.0f;
	constexpr float LightRadiusCm = 600.0f;
	const FLinearColor IronColor(0.06f, 0.06f, 0.07f);
	const FLinearColor StoneColor(0.22f, 0.2f, 0.19f);
	const FLinearColor EmberColor(1.0f, 0.35f, 0.05f);
}

AEIForge::AEIForge()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	AddPart(TEXT("AnvilFoot"), Cube.Object, FVector(0.0f, AnvilYCm, AnvilFootSizeCm.Z / 2.0f), AnvilFootSizeCm);
	AddPart(TEXT("AnvilTop"), Cube.Object, FVector(0.0f, AnvilYCm, AnvilFootSizeCm.Z + AnvilTopSizeCm.Z / 2.0f), AnvilTopSizeCm);
	AddPart(TEXT("Hearth"), Cube.Object, FVector(0.0f, HearthYCm, HearthSizeCm.Z / 2.0f), HearthSizeCm);
	AddPart(TEXT("Chimney"), Cube.Object, FVector(ChimneyBackCm, HearthYCm, HearthSizeCm.Z + ChimneySizeCm.Z / 2.0f), ChimneySizeCm);
	Embers = AddPart(TEXT("Embers"), Cube.Object, FVector(HearthSizeCm.X / 2.0f - EmbersSizeCm.X / 2.0f, HearthYCm, HearthSizeCm.Z + EmbersSizeCm.Z / 2.0f), EmbersSizeCm);
	Embers->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(HearthSizeCm.X / 2.0f, HearthYCm, HearthSizeCm.Z + 60.0f));
	Light->SetLightColor(EmberColor);
	Light->SetIntensity(LightIntensity);
	Light->SetAttenuationRadius(LightRadiusCm);
	Light->SetCastShadows(false);
}

UStaticMeshComponent* AEIForge::AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(GetRootComponent());
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeScale3D(SizeCm / 100.0f);
	Part->SetCanEverAffectNavigation(false);
	return Part;
}

void AEIForge::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIForge, ReachMeters);
}

void AEIForge::BeginPlay()
{
	Super::BeginPlay();

	for (UActorComponent* Component : GetComponents())
	{
		UStaticMeshComponent* Part = Cast<UStaticMeshComponent>(Component);
		if (Part == nullptr || Part == Embers)
		{
			continue;
		}
		const bool bIron = Part->GetName().StartsWith(TEXT("Anvil"));
		EIVisuals::SetColor(Part, bIron ? IronColor : StoneColor);
	}
	EIVisuals::SetColor(Embers, EmberColor, EmberGlow);
}

bool AEIForge::IsInReach(const AActor* Actor) const
{
	return Actor != nullptr && FVector::Dist2D(Actor->GetActorLocation(), GetActorLocation()) <= ReachMeters * CentimetersPerMeter;
}

FVector AEIForge::GetMarkLocation() const
{
	return GetActorLocation() + FVector(0.0f, 0.0f, MarkAboveCm);
}
