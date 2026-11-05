#include "EISoulKettle.h"

#include "Components/StaticMeshComponent.h"
#include "EILookComponent.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FVector PotSizeCm(180.0f, 180.0f, 110.0f);
	const FVector FluidSizeCm(160.0f, 160.0f, 8.0f);
	constexpr float FluidLowestCm = 15.0f;
	constexpr float WispSizeCm = 45.0f;
	constexpr float WispAbovePotCm = 70.0f;
	constexpr float WispGrowth = 1.8f;
	constexpr float ZoneDiscHeightCm = 3.0f;
	constexpr float ZoneStrength = 0.14f;
	constexpr float FluidGlowEmpty = 0.3f;
	constexpr float FluidGlowFull = 2.5f;
	constexpr float MarkAbovePotCm = 260.0f;
	const FVector GraveSlabSizeCm(320.0f, 170.0f, 18.0f);
	const FVector HeadstoneSizeCm(30.0f, 150.0f, 160.0f);
	constexpr float HeadstoneBehindCm = 220.0f;
	const FVector OpenSlabOffsetCm(0.0f, 210.0f, 40.0f);
	constexpr float OpenSlabRollDegrees = 35.0f;
	const FLinearColor IronColor(0.07f, 0.07f, 0.08f);
	const FLinearColor GraveColor(0.16f, 0.16f, 0.17f);
	const FLinearColor SoulColor(0.45f, 0.9f, 1.0f);
	constexpr float WispFlightSeconds = 0.7f;
	constexpr float WispArcCm = 250.0f;
	constexpr float FlyingWispSizeCm = 30.0f;
	constexpr float FlyingWispGlow = 2.0f;
}

AEISoulKettle::AEISoulKettle()
{
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Pot = AddPart(TEXT("Pot"), Cylinder.Object, FVector(0.0f, 0.0f, PotSizeCm.Z / 2.0f), PotSizeCm);
	Fluid = AddPart(TEXT("Fluid"), Cylinder.Object, FVector(0.0f, 0.0f, FluidLowestCm), FluidSizeCm);
	Wisp = AddPart(TEXT("Wisp"), Sphere.Object, FVector(0.0f, 0.0f, PotSizeCm.Z + WispAbovePotCm), FVector(WispSizeCm));
	ZoneDisc = AddPart(TEXT("ZoneDisc"), Cylinder.Object, FVector(0.0f, 0.0f, ZoneDiscHeightCm), FVector(100.0f, 100.0f, 2.0f));
	GraveSlab = AddPart(TEXT("GraveSlab"), Cube.Object, FVector::ZeroVector, GraveSlabSizeCm);
	Headstone = AddPart(TEXT("Headstone"), Cube.Object, FVector::ZeroVector, HeadstoneSizeCm);
	for (UStaticMeshComponent* Part : {Fluid.Get(), Wisp.Get(), ZoneDisc.Get(), GraveSlab.Get(), Headstone.Get()})
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	ZoneDisc->SetCastShadow(false);
	Fluid->SetCastShadow(false);
	Wisp->SetCastShadow(false);
	// The grave lies where the boss sleeps, away from the kettle.
	GraveSlab->SetUsingAbsoluteLocation(true);
	GraveSlab->SetUsingAbsoluteRotation(true);
	Headstone->SetUsingAbsoluteLocation(true);
	Headstone->SetUsingAbsoluteRotation(true);

	Look = CreateDefaultSubobject<UEILookComponent>(TEXT("Look"));
	Look->SetupAttachment(Root);
	GraveLook = CreateDefaultSubobject<UEILookComponent>(TEXT("GraveLook"));
	GraveLook->SetupAttachment(Root);
	GraveLook->SetUsingAbsoluteLocation(true);
	GraveLook->SetUsingAbsoluteRotation(true);
}

UStaticMeshComponent* AEISoulKettle::AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(GetRootComponent());
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeScale3D(SizeCm / 100.0f);
	Part->SetCanEverAffectNavigation(false);
	return Part;
}

void AEISoulKettle::Setup(const FVector& InGraveLocation, float InZoneMeters, int32 InCount)
{
	GraveLocation = InGraveLocation;
	ZoneMeters = InZoneMeters;
	Count = FMath::Max(InCount, 1);
}

void AEISoulKettle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEISoulKettle, GraveLocation);
	DOREPLIFETIME(AEISoulKettle, ZoneMeters);
	DOREPLIFETIME(AEISoulKettle, Count);
	DOREPLIFETIME(AEISoulKettle, Souls);
}

void AEISoulKettle::BeginPlay()
{
	Super::BeginPlay();

	EIVisuals::SetColor(Pot, IronColor);
	EIVisuals::SetColor(GraveSlab, GraveColor);
	EIVisuals::SetColor(Headstone, GraveColor);
	EIVisuals::SetGlow(ZoneDisc, SoulColor, ZoneStrength);
	// The engine's cylinder is 1 m wide, so its scale is its width in metres.
	const float ZoneWidth = ZoneMeters * 2.0f;
	ZoneDisc->SetRelativeScale3D(FVector(ZoneWidth, ZoneWidth, 0.02f));

	const FVector Away = (GraveLocation - GetActorLocation()).GetSafeNormal2D();
	const FRotator GraveFacing = Away.IsNearlyZero() ? FRotator::ZeroRotator : Away.Rotation();
	GraveSlab->SetWorldLocationAndRotation(GraveLocation + FVector(0.0f, 0.0f, GraveSlabSizeCm.Z / 2.0f), GraveFacing);
	Headstone->SetWorldLocationAndRotation(GraveLocation + GraveFacing.RotateVector(FVector(HeadstoneBehindCm, 0.0f, HeadstoneSizeCm.Z / 2.0f)), GraveFacing);
	GraveLook->SetWorldLocationAndRotation(GraveLocation, GraveFacing);
	// Fluid and wisp stay: they show how full the kettle is.
	Look->Setup({TEXT("ort.seelenkessel")}, {Pot}, {Fluid, Wisp});
	GraveLook->Setup({TEXT("ort.grab")}, {GraveSlab, Headstone});
	OnRep_Souls();
}

void AEISoulKettle::OnRep_Souls()
{
	const float Fill = FMath::Clamp(static_cast<float>(Souls) / FMath::Max(Count, 1), 0.0f, 1.0f);
	Fluid->SetRelativeLocation(FVector(0.0f, 0.0f, FMath::Lerp(FluidLowestCm, PotSizeCm.Z - FluidSizeCm.Z, Fill)));
	EIVisuals::SetGlow(Fluid, SoulColor, FMath::Lerp(FluidGlowEmpty, FluidGlowFull, Fill));
	EIVisuals::SetGlow(Wisp, SoulColor, FMath::Lerp(FluidGlowEmpty, FluidGlowFull, Fill));
	Wisp->SetRelativeScale3D(FVector(WispSizeCm * FMath::Lerp(1.0f, WispGrowth, Fill) / 100.0f));
	if (IsFull() && !bShownOpen)
	{
		// The grave breaks open: the slab slides aside and the circle is no longer needed.
		bShownOpen = true;
		ZoneDisc->SetVisibility(false);
		Look->ShowActive(true);
		GraveLook->ShowActive(true);
		const FRotator Facing = GraveSlab->GetComponentRotation();
		GraveSlab->SetWorldLocationAndRotation(GraveLocation + Facing.RotateVector(OpenSlabOffsetCm), Facing + FRotator(0.0f, 0.0f, OpenSlabRollDegrees));
		EIVisuals::PlaySoundAt(this, TEXT("ExplosionGross"), GraveLocation);
	}
}

void AEISoulKettle::AddSoul(const FVector& From)
{
	if (IsFull())
	{
		return;
	}
	++Souls;
	OnRep_Souls();
	MulticastSoulFlies(From);
}

void AEISoulKettle::SetSouls(int32 NewSouls)
{
	Souls = FMath::Clamp(NewSouls, 0, Count);
	OnRep_Souls();
}

void AEISoulKettle::MulticastSoulFlies_Implementation(FVector_NetQuantize From)
{
	EIVisuals::PlaySoundAt(this, TEXT("Sprungfeld"), From, 0.5f);
	const FTransform Spawn(From);
	if (AEISoulWisp* Flying = GetWorld()->SpawnActorDeferred<AEISoulWisp>(AEISoulWisp::StaticClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Flying->Setup(Wisp->GetComponentLocation());
		Flying->FinishSpawning(Spawn);
	}
}

bool AEISoulKettle::IsInZone(const AActor* Actor) const
{
	return Actor != nullptr && IsInZone(Actor->GetActorLocation());
}

bool AEISoulKettle::IsInZone(const FVector& Location) const
{
	return FVector::Dist2D(Location, GetActorLocation()) <= ZoneMeters * CentimetersPerMeter;
}

FVector AEISoulKettle::GetMarkLocation() const
{
	return GetActorLocation() + FVector(0.0f, 0.0f, MarkAbovePotCm);
}

AEISoulWisp::AEISoulWisp()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = WispFlightSeconds + 0.1f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Glow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Glow"));
	SetRootComponent(Glow);
	Glow->SetStaticMesh(Sphere.Object);
	Glow->SetRelativeScale3D(FVector(FlyingWispSizeCm / 100.0f));
	Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glow->SetCastShadow(false);
}

void AEISoulWisp::BeginPlay()
{
	Super::BeginPlay();
	Start = GetActorLocation();
	EIVisuals::SetGlow(Glow, SoulColor, FlyingWispGlow);
}

void AEISoulWisp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float Share = FMath::Clamp(Age / WispFlightSeconds, 0.0f, 1.0f);
	// Up first and then down into the kettle.
	const FVector Along = FMath::Lerp(Start, Target, FMath::InterpEaseIn(0.0f, 1.0f, Share, 2.0f));
	SetActorLocation(Along + FVector(0.0f, 0.0f, FMath::Sin(Share * UE_PI) * WispArcCm));
	if (Share >= 1.0f)
	{
		Destroy();
	}
}
