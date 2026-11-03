#include "EIChest.h"

#include "Components/StaticMeshComponent.h"
#include "EIDataFile.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FVector BodySizeCm(85.0f, 130.0f, 60.0f);
	const FVector LidSizeCm(91.0f, 136.0f, 20.0f);
	constexpr float GemSizeCm = 22.0f;
	constexpr float OpenLidDegrees = 75.0f;
	const FLinearColor BodyColor(0.16f, 0.09f, 0.04f);
	const FLinearColor LidColor(0.5f, 0.33f, 0.06f);
	const FLinearColor GemColor(1.0f, 0.75f, 0.2f);
	constexpr float GemGlow = 8.0f;
}

AEIChest::AEIChest()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetStaticMesh(Cube.Object);
	Body->SetRelativeLocation(FVector(0.0f, 0.0f, BodySizeCm.Z / 2.0f));
	Body->SetRelativeScale3D(BodySizeCm / 100.0f);
	// Most chests stay away in a run, and the paths on the ground should not change with that.
	Body->SetCanEverAffectNavigation(false);

	Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lid"));
	Lid->SetupAttachment(Root);
	Lid->SetStaticMesh(Cube.Object);
	Lid->SetRelativeScale3D(LidSizeCm / 100.0f);
	Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Gem = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gem"));
	Gem->SetupAttachment(Root);
	Gem->SetStaticMesh(Sphere.Object);
	Gem->SetRelativeLocation(FVector(BodySizeCm.X / 2.0f, 0.0f, BodySizeCm.Z));
	Gem->SetRelativeScale3D(FVector(GemSizeCm / 100.0f));
	Gem->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEIChest::BeginPlay()
{
	Super::BeginPlay();

	FString Error;
	if (!FEIRunValues::LoadFromProjectFile(Values, Error))
	{
		FEIDataFile::ReportError(TEXT("lauf.json"), Error);
	}
	EIVisuals::SetColor(Body, BodyColor);
	EIVisuals::SetColor(Lid, LidColor);
	EIVisuals::SetColor(Gem, GemColor, GemGlow);
	OnRep_Look();
}

void AEIChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIChest, bInUse);
	DOREPLIFETIME(AEIChest, OpenedBy);
}

bool AEIChest::IsInReach(const AActor* Hero) const
{
	return FVector::Dist(Hero->GetActorLocation(), GetActorLocation()) <= Values.ChestReachMeters * CentimetersPerMeter;
}

bool AEIChest::IsOpenFor(const AActor* Hero) const
{
	return OpenedBy.Contains(Hero);
}

FVector AEIChest::GetMarkLocation() const
{
	return GetActorLocation() + FVector(0.0f, 0.0f, BodySizeCm.Z + LidSizeCm.Z * 2.0f);
}

void AEIChest::PrepareForRun(bool bUse)
{
	bInUse = bUse;
	OpenedBy.Reset();
	OnRep_Look();
}

void AEIChest::OpenFor(AActor* Hero)
{
	OpenedBy.AddUnique(Hero);
	OnRep_Look();
}

void AEIChest::OnRep_Look()
{
	SetActorHiddenInGame(!bInUse);
	SetActorEnableCollision(bInUse);

	const APlayerController* LocalPlayer = GetWorld()->GetFirstPlayerController();
	const bool bOpen = LocalPlayer != nullptr && IsOpenFor(LocalPlayer->GetPawn());
	// The lid turns around its back edge.
	const float Angle = bOpen ? OpenLidDegrees : 0.0f;
	const FVector Hinge(-LidSizeCm.X / 2.0f, 0.0f, BodySizeCm.Z);
	const FRotator Turn(Angle, 0.0f, 0.0f);
	const FVector FromHinge = Turn.RotateVector(FVector(LidSizeCm.X / 2.0f, 0.0f, LidSizeCm.Z / 2.0f));
	Lid->SetRelativeLocationAndRotation(Hinge + FromHinge, Turn);
	Gem->SetVisibility(bInUse && !bOpen);
}
