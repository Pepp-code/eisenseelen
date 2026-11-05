#include "EIAnchorStone.h"

#include "Components/StaticMeshComponent.h"
#include "EILookComponent.h"
#include "EIVisuals.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FVector StoneSizeCm(260.0f, 260.0f, 170.0f);
	constexpr float RubbleHeightShare = 0.2f;
	constexpr float CrystalSizeCm = 90.0f;
	constexpr float ChainThicknessCm = 22.0f;
	constexpr float ZoneDiscHeightCm = 3.0f;
	constexpr float ZoneStrength = 0.14f;
	constexpr float CrystalGlow = 0.8f;
	constexpr float MarkAboveCrystalCm = 120.0f;
	const FLinearColor StoneColor(0.1f, 0.09f, 0.11f);
	const FLinearColor ChainColor(0.16f, 0.16f, 0.18f);
	const FLinearColor BoundColor(0.65f, 0.2f, 1.0f);
}

AEIAnchorStone::AEIAnchorStone()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Look = CreateDefaultSubobject<UEILookComponent>(TEXT("Look"));
	Look->SetupAttachment(Root);

	Stone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stone"));
	Stone->SetupAttachment(Root);
	Stone->SetStaticMesh(Cube.Object);
	Stone->SetRelativeLocation(FVector(0.0f, 0.0f, StoneSizeCm.Z / 2.0f));
	Stone->SetRelativeScale3D(StoneSizeCm / 100.0f);
	Stone->SetCanEverAffectNavigation(false);

	Crystal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crystal"));
	Crystal->SetupAttachment(Root);
	Crystal->SetStaticMesh(Sphere.Object);
	Crystal->SetRelativeLocation(FVector(0.0f, 0.0f, StoneSizeCm.Z + CrystalSizeCm / 2.0f));
	Crystal->SetRelativeScale3D(FVector(CrystalSizeCm / 100.0f));
	Crystal->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Chain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chain"));
	Chain->SetupAttachment(Root);
	Chain->SetStaticMesh(Cube.Object);
	Chain->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Chain->SetCastShadow(false);

	ZoneDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneDisc"));
	ZoneDisc->SetupAttachment(Root);
	ZoneDisc->SetStaticMesh(Cylinder.Object);
	ZoneDisc->SetRelativeLocation(FVector(0.0f, 0.0f, ZoneDiscHeightCm));
	ZoneDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneDisc->SetCastShadow(false);
}

void AEIAnchorStone::Setup(const FVector& InChainEnd, float InZoneMeters, float HoldSeconds)
{
	ChainEnd = InChainEnd;
	ZoneMeters = InZoneMeters;
	HoldLeft = HoldSeconds;
}

void AEIAnchorStone::BeginPlay()
{
	Super::BeginPlay();

	// The chain stays a block: it runs to wherever the boss stands.
	Look->Setup({TEXT("ort.ankerstein")}, {Stone}, {Crystal});
	EIVisuals::SetColor(Stone, StoneColor);
	EIVisuals::SetColor(Chain, ChainColor);
	EIVisuals::SetColor(Crystal, BoundColor, CrystalGlow);
	EIVisuals::SetGlow(ZoneDisc, BoundColor, ZoneStrength);
	// The engine's cylinder is 1 m wide, so its scale is its width in metres.
	const float ZoneWidth = ZoneMeters * 2.0f;
	ZoneDisc->SetRelativeScale3D(FVector(ZoneWidth, ZoneWidth, 0.02f));

	// The chain runs from the top of the stone to the bound boss.
	const FVector Start = Crystal->GetComponentLocation();
	const FVector ToEnd = ChainEnd - Start;
	Chain->SetWorldLocationAndRotation(Start + ToEnd / 2.0f, ToEnd.Rotation());
	Chain->SetWorldScale3D(FVector(ToEnd.Size(), ChainThicknessCm, ChainThicknessCm) / 100.0f);
	OnRep_Broken();
}

void AEIAnchorStone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIAnchorStone, ChainEnd);
	DOREPLIFETIME(AEIAnchorStone, ZoneMeters);
	DOREPLIFETIME(AEIAnchorStone, HoldLeft);
	DOREPLIFETIME(AEIAnchorStone, bHeld);
	DOREPLIFETIME(AEIAnchorStone, bBroken);
}

void AEIAnchorStone::UpdateHold(bool bNowHeld, float DeltaTime)
{
	if (bBroken)
	{
		return;
	}
	bHeld = bNowHeld;
	HoldLeft -= bNowHeld ? DeltaTime : 0.0f;
	if (HoldLeft <= 0.0f)
	{
		bHeld = false;
		bBroken = true;
		OnRep_Broken();
	}
}

void AEIAnchorStone::OnRep_Broken()
{
	Chain->SetVisibility(!bBroken);
	ZoneDisc->SetVisibility(!bBroken);
	Crystal->SetVisibility(!bBroken);
	Look->ShowActive(bBroken);
	if (bBroken)
	{
		// It falls to rubble, which blocks nobody, so that the fight around the boss does not get stuck on it.
		SetActorEnableCollision(false);
		Stone->SetRelativeLocation(FVector(0.0f, 0.0f, StoneSizeCm.Z * RubbleHeightShare / 2.0f));
		Stone->SetRelativeScale3D(FVector(StoneSizeCm.X, StoneSizeCm.Y, StoneSizeCm.Z * RubbleHeightShare) / 100.0f);
		EIVisuals::PlaySoundAt(this, TEXT("ExplosionGross"), GetMarkLocation());
	}
}

bool AEIAnchorStone::IsInZone(const AActor* Actor) const
{
	return FVector::Dist2D(Actor->GetActorLocation(), GetActorLocation()) <= ZoneMeters * CentimetersPerMeter;
}

FVector AEIAnchorStone::GetMarkLocation() const
{
	return Crystal->GetComponentLocation() + FVector(0.0f, 0.0f, MarkAboveCrystalCm);
}
