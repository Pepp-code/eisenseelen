#include "EIAltar.h"

#include "Components/StaticMeshComponent.h"
#include "EIVisuals.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float StoneHeightCm = 110.0f;
	constexpr float StoneWidthCm = 220.0f;
	constexpr float CrystalSizeCm = 90.0f;
	constexpr float CrystalAboveStoneCm = 90.0f;
	constexpr float ZoneDiscHeightCm = 3.0f;
	const FLinearColor StoneColor(0.08f, 0.07f, 0.09f);
	const FLinearColor DormantColor(0.1f, 0.08f, 0.12f);
	const FLinearColor BrokenColor(0.03f, 0.03f, 0.03f);
	constexpr float ActiveGlow = 1.5f;
	constexpr float ZoneStrength = 0.14f;
}

AEIAltar::AEIAltar()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Stone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stone"));
	Stone->SetupAttachment(Root);
	Stone->SetStaticMesh(Cube.Object);
	Stone->SetRelativeLocation(FVector(0.0f, 0.0f, StoneHeightCm / 2.0f));
	Stone->SetRelativeScale3D(FVector(StoneWidthCm, StoneWidthCm, StoneHeightCm) / 100.0f);

	Crystal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crystal"));
	Crystal->SetupAttachment(Root);
	Crystal->SetStaticMesh(Sphere.Object);
	Crystal->SetRelativeLocation(FVector(0.0f, 0.0f, StoneHeightCm + CrystalAboveStoneCm));
	Crystal->SetRelativeScale3D(FVector(CrystalSizeCm / 100.0f));
	Crystal->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneDisc"));
	ZoneDisc->SetupAttachment(Root);
	ZoneDisc->SetStaticMesh(Cylinder.Object);
	ZoneDisc->SetRelativeLocation(FVector(0.0f, 0.0f, ZoneDiscHeightCm));
	ZoneDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneDisc->SetCastShadow(false);
	ZoneDisc->SetVisibility(false);
}

void AEIAltar::BeginPlay()
{
	Super::BeginPlay();

	EIVisuals::SetColor(Stone, StoneColor);
	CrystalMaterial = EIVisuals::SetColor(Crystal, DormantColor);
	OnRep_State();
}

void AEIAltar::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIAltar, State);
	DOREPLIFETIME(AEIAltar, ZoneMeters);
	DOREPLIFETIME(AEIAltar, JobColor);
}

void AEIAltar::SetState(EEIAltarState NewState)
{
	State = NewState;
	OnRep_State();
}

void AEIAltar::StartJob(float NewZoneMeters, const FLinearColor& Color)
{
	ZoneMeters = NewZoneMeters;
	JobColor = Color;
	SetState(EEIAltarState::Active);
}

void AEIAltar::OnRep_State()
{
	const bool bActive = State == EEIAltarState::Active;
	ZoneDisc->SetVisibility(bActive);
	if (bActive)
	{
		EIVisuals::SetGlow(ZoneDisc, JobColor, ZoneStrength);
		// The engine's cylinder is 1 m wide, so its scale is its width in metres.
		const float ZoneWidth = ZoneMeters * 2.0f;
		ZoneDisc->SetRelativeScale3D(FVector(ZoneWidth, ZoneWidth, 0.02f));
	}
	if (CrystalMaterial != nullptr)
	{
		const FLinearColor Color = bActive ? JobColor : (State == EEIAltarState::Broken ? BrokenColor : DormantColor);
		CrystalMaterial->SetVectorParameterValue(TEXT("Farbe"), Color);
		CrystalMaterial->SetScalarParameterValue(TEXT("Leuchten"), bActive ? ActiveGlow : 0.0f);
	}
	if (State == EEIAltarState::Broken && ShownState != EEIAltarState::Broken)
	{
		EIVisuals::PlaySoundAt(this, TEXT("ExplosionGross"), GetCrystalLocation());
	}
	ShownState = State;
}

bool AEIAltar::IsInZone(const AActor* Actor) const
{
	return FVector::Dist2D(Actor->GetActorLocation(), GetActorLocation()) <= ZoneMeters * CentimetersPerMeter;
}

FVector AEIAltar::GetCrystalLocation() const
{
	return Crystal->GetComponentLocation();
}
