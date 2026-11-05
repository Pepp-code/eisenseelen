#include "EIEnemyCamp.h"

#include "Components/StaticMeshComponent.h"
#include "EIEnemy.h"
#include "EILookComponent.h"
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

	// The ashes are squashed flat, and the model keeps its own size.
	Look = CreateDefaultSubobject<UEILookComponent>(TEXT("Look"));
	Look->SetupAttachment(Ashes);
	Look->SetUsingAbsoluteScale(true);
}

void AEIEnemyCamp::BeginPlay()
{
	Super::BeginPlay();
	Look->Setup({TEXT("ort.lager")}, {Ashes});
	EIVisuals::SetColor(Ashes, AshesColor);
}

void AEIEnemyCamp::SetMembers(const TArray<AEIEnemy*>& Enemies)
{
	Members.Reset();
	for (AEIEnemy* Enemy : Enemies)
	{
		Members.Add(Enemy);
	}
}

int32 AEIEnemyCamp::CountMembers() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<AEIEnemy>& Member : Members)
	{
		Count += Member.IsValid() && !Member->IsDead() ? 1 : 0;
	}
	return Count;
}
