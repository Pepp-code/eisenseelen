#include "EIEnemyCamp.h"

#include "Components/StaticMeshComponent.h"
#include "EIVisuals.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float AshesWidthCm = 260.0f;
	constexpr float AshesHeightCm = 8.0f;
	const FLinearColor AshesColor(0.04f, 0.035f, 0.04f);
}

AEIEnemyCamp::AEIEnemyCamp()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Ashes = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ashes"));
	SetRootComponent(Ashes);
	Ashes->SetStaticMesh(Cylinder.Object);
	Ashes->SetRelativeScale3D(FVector(AshesWidthCm, AshesWidthCm, AshesHeightCm) / 100.0f);
	Ashes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ashes->SetCanEverAffectNavigation(false);
	Ashes->SetCastShadow(false);
}

void AEIEnemyCamp::BeginPlay()
{
	Super::BeginPlay();
	EIVisuals::SetColor(Ashes, AshesColor);
}
