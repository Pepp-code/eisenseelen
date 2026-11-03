#include "EISkyBarge.h"

#include "Components/StaticMeshComponent.h"
#include "EIVisuals.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float DeckLengthCm = 1400.0f;
	constexpr float DeckWidthCm = 700.0f;
	constexpr float DeckThicknessCm = 40.0f;
	constexpr float BoardingMarginCm = 150.0f;
	constexpr float BoardingHeightCm = 500.0f;
	constexpr float DeckEdgeKeepCm = 60.0f;
	constexpr float ApproachEasing = 2.0f;
	const FLinearColor DeckColor(0.2f, 0.13f, 0.07f);
	const FLinearColor HullColor(0.11f, 0.08f, 0.05f);
	const FLinearColor MastColor(0.15f, 0.1f, 0.06f);
	const FLinearColor SailColor(0.55f, 0.5f, 0.4f);
}

UStaticMeshComponent* AEISkyBarge::AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(Root);
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeScale3D(SizeCm / 100.0f);
	// Otherwise the paths on the ground would be rebuilt all along its flight.
	Part->SetCanEverAffectNavigation(false);
	return Part;
}

AEISkyBarge::AEISkyBarge()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Deck = AddPart(TEXT("Deck"), Cube.Object, FVector::ZeroVector, FVector(DeckLengthCm, DeckWidthCm, DeckThicknessCm));
	Hull = AddPart(TEXT("Hull"), Cube.Object, FVector(0.0f, 0.0f, -100.0f), FVector(DeckLengthCm * 0.85f, DeckWidthCm * 0.7f, 160.0f));
	Prow = AddPart(TEXT("Prow"), Cube.Object, FVector(DeckLengthCm * 0.57f, 0.0f, -50.0f), FVector(240.0f, DeckWidthCm * 0.4f, 120.0f));
	Mast = AddPart(TEXT("Mast"), Cylinder.Object, FVector(-DeckLengthCm * 0.3f, 0.0f, 475.0f), FVector(35.0f, 35.0f, 950.0f));
	Sail = AddPart(TEXT("Sail"), Cube.Object, FVector(-DeckLengthCm * 0.3f, 0.0f, 640.0f), FVector(25.0f, 560.0f, 420.0f));
	for (UStaticMeshComponent* Part : { Hull.Get(), Prow.Get(), Mast.Get(), Sail.Get() })
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AEISkyBarge::BeginPlay()
{
	Super::BeginPlay();
	StartLocation = GetActorLocation();
	EIVisuals::SetColor(Deck, DeckColor);
	EIVisuals::SetColor(Hull, HullColor);
	EIVisuals::SetColor(Prow, HullColor);
	EIVisuals::SetColor(Mast, MastColor);
	EIVisuals::SetColor(Sail, SailColor);
}

void AEISkyBarge::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEISkyBarge, bFlying);
}

void AEISkyBarge::OnRep_Flying()
{
	Deck->SetCollisionResponseToChannel(ECC_Pawn, bFlying ? ECR_Ignore : ECR_Block);
}

void AEISkyBarge::ReturnToStart()
{
	DepartSpeedCm = 0.0f;
	bFlying = false;
	OnRep_Flying();
	SetActorLocation(StartLocation);
}

void AEISkyBarge::BeginApproach(const FVector& HoverLocation)
{
	ApproachFrom = GetActorLocation();
	ApproachTo = HoverLocation;
	bFlying = true;
	OnRep_Flying();
}

void AEISkyBarge::SetApproachFraction(float Fraction)
{
	SetActorLocation(FMath::InterpEaseInOut(ApproachFrom, ApproachTo, FMath::Clamp(Fraction, 0.0f, 1.0f), ApproachEasing));
}

void AEISkyBarge::Depart(float SpeedCmPerSecond)
{
	DepartSpeedCm = SpeedCmPerSecond;
}

void AEISkyBarge::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DepartSpeedCm > 0.0f && HasAuthority())
	{
		AddActorWorldOffset(FVector(0.0f, 0.0f, DepartSpeedCm * DeltaSeconds));
	}
}

bool AEISkyBarge::IsInBoardingZone(const AActor* Actor) const
{
	const FVector Offset = Actor->GetActorLocation() - GetActorLocation();
	return FMath::Abs(Offset.X) <= DeckLengthCm / 2.0f + BoardingMarginCm
		&& FMath::Abs(Offset.Y) <= DeckWidthCm / 2.0f + BoardingMarginCm
		&& FMath::Abs(Offset.Z) <= BoardingHeightCm;
}

FVector AEISkyBarge::GetDeckSpotNear(const FVector& Location, float HalfHeight) const
{
	const FVector Center = GetActorLocation();
	const float MaxX = DeckLengthCm / 2.0f - DeckEdgeKeepCm;
	const float MaxY = DeckWidthCm / 2.0f - DeckEdgeKeepCm;
	return FVector(
		Center.X + FMath::Clamp(Location.X - Center.X, -MaxX, MaxX),
		Center.Y + FMath::Clamp(Location.Y - Center.Y, -MaxY, MaxY),
		GetDeckHeight() + HalfHeight);
}

float AEISkyBarge::GetDeckHeight() const
{
	return GetActorLocation().Z + DeckThicknessCm / 2.0f;
}
