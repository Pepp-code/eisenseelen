#include "EIHollowArmor.h"

#include "Components/StaticMeshComponent.h"
#include "EILookComponent.h"
#include "EIVisuals.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FVector LegSizeCm(70.0f, 70.0f, 170.0f);
	const FVector TorsoSizeCm(180.0f, 110.0f, 150.0f);
	const FVector ArmSizeCm(40.0f, 40.0f, 150.0f);
	const FVector HeadSizeCm(65.0f, 65.0f, 60.0f);
	constexpr float CoreSizeCm = 55.0f;
	constexpr float ZoneDiscHeightCm = 3.0f;
	constexpr float ZoneStrength = 0.12f;
	const FLinearColor SteelColor(0.3f, 0.31f, 0.34f);
	const FLinearColor DarkSteelColor(0.24f, 0.25f, 0.27f);
	const FLinearColor CoreColor(0.55f, 0.75f, 1.0f);
	constexpr float MarkAboveHeadCm = 130.0f;
}

AEIHollowArmor::AEIHollowArmor()
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

	LeftLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLeg"));
	LeftLeg->SetupAttachment(Root);
	LeftLeg->SetStaticMesh(Cube.Object);
	LeftLeg->SetRelativeLocation(FVector(-45.0f, 40.0f, LegSizeCm.Z / 2.0f));
	LeftLeg->SetRelativeScale3D(LegSizeCm / 100.0f);
	LeftLeg->SetCanEverAffectNavigation(false);

	RightLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLeg"));
	RightLeg->SetupAttachment(Root);
	RightLeg->SetStaticMesh(Cube.Object);
	RightLeg->SetRelativeLocation(FVector(-45.0f, -40.0f, LegSizeCm.Z / 2.0f));
	RightLeg->SetRelativeScale3D(LegSizeCm / 100.0f);
	RightLeg->SetCanEverAffectNavigation(false);

	Torso = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Torso"));
	Torso->SetupAttachment(Root);
	Torso->SetStaticMesh(Cube.Object);
	Torso->SetRelativeLocation(FVector(0.0f, 0.0f, LegSizeCm.Z + TorsoSizeCm.Z / 2.0f));
	Torso->SetRelativeScale3D(TorsoSizeCm / 100.0f);
	Torso->SetCanEverAffectNavigation(false);

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(Root);
	Head->SetStaticMesh(Cube.Object);
	Head->SetRelativeLocation(FVector(0.0f, 0.0f, LegSizeCm.Z + TorsoSizeCm.Z + HeadSizeCm.Z / 2.0f));
	Head->SetRelativeScale3D(HeadSizeCm / 100.0f);
	Head->SetCanEverAffectNavigation(false);

	LeftArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftArm"));
	LeftArm->SetupAttachment(Root);
	LeftArm->SetStaticMesh(Cube.Object);
	LeftArm->SetRelativeLocation(FVector(0.0f, TorsoSizeCm.Y / 2.0f + ArmSizeCm.Y / 2.0f, LegSizeCm.Z + TorsoSizeCm.Z - ArmSizeCm.Z / 2.0f));
	LeftArm->SetRelativeScale3D(ArmSizeCm / 100.0f);
	LeftArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftArm->SetCanEverAffectNavigation(false);

	RightArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightArm"));
	RightArm->SetupAttachment(Root);
	RightArm->SetStaticMesh(Cube.Object);
	RightArm->SetRelativeLocation(FVector(0.0f, -TorsoSizeCm.Y / 2.0f - ArmSizeCm.Y / 2.0f, LegSizeCm.Z + TorsoSizeCm.Z - ArmSizeCm.Z / 2.0f));
	RightArm->SetRelativeScale3D(ArmSizeCm / 100.0f);
	RightArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightArm->SetCanEverAffectNavigation(false);

	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Core"));
	Core->SetupAttachment(Root);
	Core->SetStaticMesh(Sphere.Object);
	Core->SetRelativeLocation(FVector(TorsoSizeCm.X / 2.0f + CoreSizeCm / 4.0f, 0.0f, LegSizeCm.Z + TorsoSizeCm.Z / 2.0f));
	Core->SetRelativeScale3D(FVector(CoreSizeCm / 100.0f));
	Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ZoneDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneDisc"));
	ZoneDisc->SetupAttachment(Root);
	ZoneDisc->SetStaticMesh(Cylinder.Object);
	ZoneDisc->SetRelativeLocation(FVector(0.0f, 0.0f, ZoneDiscHeightCm));
	ZoneDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneDisc->SetCastShadow(false);
}

void AEIHollowArmor::Setup(float InReachMeters, int32 InTotal)
{
	ReachMeters = InReachMeters;
	Total = InTotal;
}

void AEIHollowArmor::BeginPlay()
{
	Super::BeginPlay();

	Look->Setup({TEXT("ort.leere_wacht")}, {Torso, Head, LeftLeg, RightLeg, LeftArm, RightArm}, {Core});
	EIVisuals::SetColor(Torso, SteelColor);
	EIVisuals::SetColor(Head, SteelColor);
	EIVisuals::SetColor(LeftLeg, DarkSteelColor);
	EIVisuals::SetColor(RightLeg, DarkSteelColor);
	EIVisuals::SetColor(LeftArm, DarkSteelColor);
	EIVisuals::SetColor(RightArm, DarkSteelColor);
	EIVisuals::SetGlow(ZoneDisc, CoreColor, ZoneStrength);
	// The engine's cylinder is 1 m wide, so its scale is its width in metres.
	const float ZoneWidth = ReachMeters * 2.0f;
	ZoneDisc->SetRelativeScale3D(FVector(ZoneWidth, ZoneWidth, 0.02f));
	ShowInserted();
}

void AEIHollowArmor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIHollowArmor, ReachMeters);
	DOREPLIFETIME(AEIHollowArmor, Total);
	DOREPLIFETIME(AEIHollowArmor, Inserted);
}

void AEIHollowArmor::InsertStone()
{
	Inserted++;
	ShowInserted();
}

void AEIHollowArmor::SetInserted(int32 NewInserted)
{
	Inserted = NewInserted;
	ShowInserted();
}

void AEIHollowArmor::OnRep_Inserted()
{
	ShowInserted();
}

void AEIHollowArmor::ShowInserted()
{
	// The hollow in the chest stays dark at first and glows brighter with every soul stone.
	const float Glow = Total > 0 ? 0.2f + 1.4f * static_cast<float>(Inserted) / static_cast<float>(Total) : 0.2f;
	EIVisuals::SetColor(Core, CoreColor, Glow);
	Look->ShowActive(Inserted > 0);
	if (Inserted > 0)
	{
		EIVisuals::PlaySoundAt(this, TEXT("TrefferSchwer"), GetMarkLocation());
	}
}

bool AEIHollowArmor::IsInReach(const AActor* Actor) const
{
	return FVector::Dist2D(Actor->GetActorLocation(), GetActorLocation()) <= ReachMeters * CentimetersPerMeter;
}

FVector AEIHollowArmor::GetMarkLocation() const
{
	return Head->GetComponentLocation() + FVector(0.0f, 0.0f, MarkAboveHeadCm);
}
