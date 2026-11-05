#include "EITrainingDummy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EILookComponent.h"
#include "EIVisuals.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float HalfHeightCm = 88.0f;
	constexpr float RadiusCm = 34.0f;
	constexpr float FlashSeconds = 0.12f;
	constexpr float WobbleDegrees = 12.0f;
	const FLinearColor DummyColor(0.55f, 0.42f, 0.25f);
	const FLinearColor FlashColor(1.0f, 0.85f, 0.6f);
	constexpr float FlashGlow = 2.0f;
}

AEITrainingDummy::AEITrainingDummy()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetCanBeDamaged(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(RadiusCm, HalfHeightCm);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	SetRootComponent(Capsule);

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Capsule);
	Body->SetStaticMesh(Cylinder.Object);
	Body->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.35f));
	Body->SetRelativeLocation(FVector(0.0f, 0.0f, -18.0f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(Capsule);
	Head->SetStaticMesh(Sphere.Object);
	Head->SetRelativeScale3D(FVector(0.42f));
	Head->SetRelativeLocation(FVector(0.0f, 0.0f, 68.0f));
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Look = CreateDefaultSubobject<UEILookComponent>(TEXT("Look"));
	Look->SetupAttachment(Capsule);
	Look->SetRelativeLocation(FVector(0.0f, 0.0f, -HalfHeightCm));

	DamageText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DamageText"));
	DamageText->SetupAttachment(Capsule);
	DamageText->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	DamageText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	DamageText->SetHorizontalAlignment(EHTA_Center);
	DamageText->SetWorldSize(50.0f);
	DamageText->SetTextRenderColor(FColor::White);
	DamageText->SetText(FText::GetEmpty());
}

void AEITrainingDummy::BeginPlay()
{
	Super::BeginPlay();
	BodyMaterial = EIVisuals::SetColor(Body, DummyColor);
	EIVisuals::SetColor(Head, DummyColor);
	Look->Setup({TEXT("ort.trainingspuppe")}, {Body, Head});
}

void AEITrainingDummy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEITrainingDummy, LastDamage);
	DOREPLIFETIME(AEITrainingDummy, HitCount);
}

float AEITrainingDummy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	LastDamage = Applied;
	++HitCount;
	ShowHit();
	return Applied;
}

void AEITrainingDummy::OnRep_HitCount()
{
	ShowHit();
}

void AEITrainingDummy::ResetHits()
{
	LastDamage = 0.0f;
	HitCount = 0;
}

void AEITrainingDummy::ShowHit()
{
	DamageText->SetText(FText::AsNumber(FMath::RoundToInt(LastDamage)));
	FlashRemaining = FlashSeconds;
	Look->PlayEvent(TEXT("treffer"));
}

void AEITrainingDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Flash = FMath::Max(0.0f, FlashRemaining / FlashSeconds);
	FlashRemaining = FMath::Max(0.0f, FlashRemaining - DeltaSeconds);
	if (BodyMaterial != nullptr)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Farbe"), FMath::Lerp(DummyColor, FlashColor, Flash));
		BodyMaterial->SetScalarParameterValue(TEXT("Leuchten"), Flash * FlashGlow);
	}
	Body->SetRelativeRotation(FRotator(Flash * WobbleDegrees, 0.0f, 0.0f));
	Head->SetRelativeRotation(FRotator(Flash * WobbleDegrees, 0.0f, 0.0f));
	if (Look->HasModel())
	{
		// A model without its own hit animation wobbles from its feet like the blocks.
		Look->SetRelativeRotation(FRotator(Look->HasAnimation(TEXT("treffer")) ? 0.0f : Flash * WobbleDegrees, 0.0f, 0.0f));
		Look->SetStatus(FlashColor, Flash);
	}
}
