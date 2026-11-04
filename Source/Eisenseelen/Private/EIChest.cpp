#include "EIChest.h"

#include "Components/StaticMeshComponent.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIProjectile.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FVector BodySizeCm(85.0f, 130.0f, 60.0f);
	const FVector LidSizeCm(91.0f, 136.0f, 20.0f);
	const FVector SealSizeCm(99.0f, 24.0f, 88.0f);
	constexpr float GemSizeCm = 22.0f;
	constexpr float OpenLidDegrees = 75.0f;
	constexpr float LidDegreesPerSecond = 220.0f;
	constexpr float SealBurstRadiusCm = 170.0f;
	constexpr float ZoneDiscHeightCm = 3.0f;
	// Somebody diving past high above is not in the circle yet.
	constexpr float ZoneHeightCm = 400.0f;
	const FLinearColor BodyColor(0.16f, 0.09f, 0.04f);
	const FLinearColor LidColor(0.5f, 0.33f, 0.06f);
	const FLinearColor GemColor(1.0f, 0.75f, 0.2f);
	const FLinearColor SealColor(1.0f, 0.12f, 0.05f);
	constexpr float GemGlow = 8.0f;
	constexpr float SealGlow = 3.0f;
	constexpr float ZoneStrength = 0.14f;
}

AEIChest::AEIChest()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

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

	Seal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Seal"));
	Seal->SetupAttachment(Root);
	Seal->SetStaticMesh(Cube.Object);
	Seal->SetRelativeLocation(FVector(0.0f, 0.0f, SealSizeCm.Z / 2.0f));
	Seal->SetRelativeScale3D(SealSizeCm / 100.0f);
	Seal->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneDisc"));
	ZoneDisc->SetupAttachment(Root);
	ZoneDisc->SetStaticMesh(Cylinder.Object);
	ZoneDisc->SetRelativeLocation(FVector(0.0f, 0.0f, ZoneDiscHeightCm));
	ZoneDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneDisc->SetCastShadow(false);
	ZoneDisc->SetVisibility(false);
}

void AEIChest::BeginPlay()
{
	Super::BeginPlay();

	LoadValues();
	EIVisuals::SetColor(Body, BodyColor);
	EIVisuals::SetColor(Lid, LidColor);
	EIVisuals::SetColor(Seal, SealColor, SealGlow);
	GemMaterial = EIVisuals::SetColor(Gem, SealColor, GemGlow);
	EIVisuals::SetGlow(ZoneDisc, SealColor, ZoneStrength);
	OnRep_Look();
}

void AEIChest::LoadValues()
{
	FString Error;
	if (!FEIRunValues::LoadFromProjectFile(Values, Error))
	{
		FEIDataFile::ReportError(TEXT("lauf.json"), Error);
	}
	// The engine's cylinder is 1 m wide, so its scale is its width in metres.
	const float ZoneWidth = Values.ChestZoneMeters * 2.0f;
	ZoneDisc->SetRelativeScale3D(FVector(ZoneWidth, ZoneWidth, 0.02f));
}

void AEIChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIChest, bInUse);
	DOREPLIFETIME(AEIChest, State);
	DOREPLIFETIME(AEIChest, GuardsLeft);
	DOREPLIFETIME(AEIChest, bOpened);
}

bool AEIChest::IsInZone(const AActor* Hero) const
{
	const FVector Offset = Hero->GetActorLocation() - GetActorLocation();
	return Offset.Size2D() <= Values.ChestZoneMeters * CentimetersPerMeter && FMath::Abs(Offset.Z) <= ZoneHeightCm;
}

bool AEIChest::IsInReach(const AActor* Hero) const
{
	return FVector::Dist(Hero->GetActorLocation(), GetActorLocation()) <= Values.ChestReachMeters * CentimetersPerMeter;
}

FVector AEIChest::GetMarkLocation() const
{
	return GetActorLocation() + FVector(0.0f, 0.0f, BodySizeCm.Z + LidSizeCm.Z * 2.0f);
}

void AEIChest::PrepareForRun(bool bUse)
{
	LoadValues();
	bInUse = bUse;
	State = EEIChestState::Sealed;
	ShownState = State;
	Guards.Reset();
	GuardsLeft = 0;
	bOpened = false;
	OnRep_Look();
}

void AEIChest::SetGuards(const TArray<AEIEnemy*>& Enemies)
{
	Guards.Reset();
	for (AEIEnemy* Enemy : Enemies)
	{
		Guards.Add(Enemy);
	}
	State = EEIChestState::Guarded;
	OnRep_Look();
	UpdateGuards();
}

void AEIChest::UpdateGuards()
{
	GuardsLeft = 0;
	for (const TWeakObjectPtr<AEIEnemy>& Guard : Guards)
	{
		GuardsLeft += Guard.IsValid() && !Guard->IsDead() ? 1 : 0;
	}
	if (GuardsLeft == 0)
	{
		State = EEIChestState::Free;
		OnRep_Look();
	}
}

void AEIChest::Open()
{
	bOpened = true;
	OnRep_Look();
}

void AEIChest::OnRep_Look()
{
	SetActorHiddenInGame(!bInUse);
	SetActorEnableCollision(bInUse);

	LidTargetDegrees = bOpened ? OpenLidDegrees : 0.0f;
	if (!bOpened)
	{
		LidDegrees = 0.0f;
	}
	ShowLid();
	SetActorTickEnabled(LidDegrees != LidTargetDegrees);

	const bool bFree = State == EEIChestState::Free;
	Gem->SetVisibility(bInUse && !bOpened);
	Seal->SetVisibility(bInUse && !bFree);
	ZoneDisc->SetVisibility(bInUse && !bFree);
	if (GemMaterial != nullptr)
	{
		GemMaterial->SetVectorParameterValue(TEXT("Farbe"), bFree ? GemColor : SealColor);
	}

	if (State != ShownState)
	{
		ShownState = State;
		if (State == EEIChestState::Guarded)
		{
			EIVisuals::PlaySoundAt(this, TEXT("Schlachtruf"), GetActorLocation());
		}
		else if (bFree)
		{
			EIVisuals::PlaySoundAt(this, TEXT("TrefferSchwer"), GetActorLocation());
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AEIExplosionFX* FX = GetWorld()->SpawnActor<AEIExplosionFX>(GetMarkLocation(), FRotator::ZeroRotator, Params))
			{
				FX->Setup(SealBurstRadiusCm, GemColor);
			}
		}
	}
}

void AEIChest::ShowLid()
{
	// The lid turns around its back edge.
	const FVector Hinge(-LidSizeCm.X / 2.0f, 0.0f, BodySizeCm.Z);
	const FRotator Turn(LidDegrees, 0.0f, 0.0f);
	const FVector FromHinge = Turn.RotateVector(FVector(LidSizeCm.X / 2.0f, 0.0f, LidSizeCm.Z / 2.0f));
	Lid->SetRelativeLocationAndRotation(Hinge + FromHinge, Turn);
}

void AEIChest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	LidDegrees = FMath::FInterpConstantTo(LidDegrees, LidTargetDegrees, DeltaSeconds, LidDegreesPerSecond);
	ShowLid();
	SetActorTickEnabled(LidDegrees != LidTargetDegrees);
}
