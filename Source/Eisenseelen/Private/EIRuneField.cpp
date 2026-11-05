#include "EIRuneField.h"

#include "Components/StaticMeshComponent.h"
#include "EIHeroCharacter.h"
#include "EIRuneComponent.h"
#include "EIVisuals.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float WallThicknessCm = 80.0f;
	constexpr float WallHeightCm = 300.0f;
	// Flat on the floor, because from inside a body the player would not see it.
	constexpr float CircleHeightCm = 30.0f;
	constexpr float BurstSeconds = 0.4f;
	constexpr float FadeOutSeconds = 0.4f;
	constexpr float CircleStrength = 0.3f;
	constexpr float WallStrength = 0.45f;
	constexpr float WallCheckHeightCm = 100.0f;
	constexpr float LookAboveFloorCm = 5.0f;
}

AEIRuneField::AEIRuneField()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shape"));
	SetRootComponent(Shape);
	Shape->SetStaticMesh(Cylinder.Object);
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetCanEverAffectNavigation(false);
	Shape->SetCastShadow(false);
}

void AEIRuneField::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIRuneField, bWall);
	DOREPLIFETIME(AEIRuneField, SizeCm);
	DOREPLIFETIME(AEIRuneField, DurationSeconds);
	DOREPLIFETIME(AEIRuneField, Color);
}

void AEIRuneField::Configure(const FEIRune& Rune, float InDamage, float StatusFactor)
{
	bWall = Rune.bWall;
	SizeCm = Rune.SizeMeters * CentimetersPerMeter * (Rune.bWall ? 1.0f : 2.0f);
	DurationSeconds = Rune.DurationSeconds;
	Color = EIElementColor(Rune.Element);
	Damage = InDamage;
	TickSeconds = Rune.TickSeconds;
	FreezeSeconds = Rune.FreezeSeconds * StatusFactor;
	SlowShare = Rune.SlowShare;
	BurnPerSecond = Rune.BurnPerSecond;
	StatusSeconds = Rune.StatusSeconds * StatusFactor;
	Element = Rune.Element;
}

void AEIRuneField::BeginPlay()
{
	Super::BeginPlay();

	if (bWall)
	{
		Shape->SetStaticMesh(EIVisuals::GetCube());
		Shape->SetWorldScale3D(FVector(WallThicknessCm, SizeCm, WallHeightCm) / 100.0f);
		AddActorWorldOffset(FVector(0.0f, 0.0f, WallHeightCm / 2.0f));
	}
	else
	{
		Shape->SetWorldScale3D(FVector(SizeCm, SizeCm, CircleHeightCm) / 100.0f);
		AddActorWorldOffset(FVector(0.0f, 0.0f, CircleHeightCm / 2.0f + LookAboveFloorCm));
	}
	Material = EIVisuals::SetGlow(Shape, Color, bWall ? WallStrength : CircleStrength);
	EIVisuals::PlaySoundAt(this, TEXT("ExplosionKlein"), GetActorLocation());
	SetLifeSpan(FMath::Max(DurationSeconds, BurstSeconds));
	if (HasAuthority())
	{
		Pulse();
		NextPulseIn = TickSeconds;
	}
}

void AEIRuneField::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	const float Life = FMath::Max(DurationSeconds, BurstSeconds);
	if (Material != nullptr)
	{
		Material->SetScalarParameterValue(TEXT("Deckkraft"), FMath::Clamp((Life - Age) / FadeOutSeconds, 0.0f, 1.0f));
	}
	// A burst spreads out quickly from the hero.
	if (DurationSeconds <= 0.0f && !bWall)
	{
		const float Width = SizeCm * FMath::Lerp(0.2f, 1.0f, 1.0f - FMath::Square(1.0f - FMath::Clamp(Age / BurstSeconds, 0.0f, 1.0f)));
		Shape->SetWorldScale3D(FVector(Width, Width, CircleHeightCm) / 100.0f);
	}
	if (!HasAuthority() || DurationSeconds <= 0.0f)
	{
		return;
	}
	NextPulseIn -= DeltaSeconds;
	if (NextPulseIn <= 0.0f)
	{
		NextPulseIn += TickSeconds;
		Pulse();
	}
}

void AEIRuneField::Pulse()
{
	AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(GetOwner());
	const FCollisionShape Area = bWall
		? FCollisionShape::MakeBox(FVector(WallThicknessCm, SizeCm, WallHeightCm) / 2.0f)
		: FCollisionShape::MakeSphere(SizeCm / 2.0f);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), GetActorQuat(), FCollisionObjectQueryParams(ECC_Pawn), Area);
	TSet<AActor*> Done;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (Target == nullptr || Done.Contains(Target) || Target->IsA<AEIHeroCharacter>() || !Target->CanBeDamaged())
		{
			continue;
		}
		Done.Add(Target);
		FCollisionQueryParams WallQuery(SCENE_QUERY_STAT(EIRuneWall), false, Target);
		if (GetWorld()->LineTraceTestByObjectType(GetActorLocation() + FVector(0.0f, 0.0f, WallCheckHeightCm), Target->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), WallQuery))
		{
			continue;
		}
		AEIEnemy* Enemy = Cast<AEIEnemy>(Target);
		if (Enemy != nullptr && Hero != nullptr)
		{
			Hero->GetRunes()->HitWithElement(Enemy, Element);
		}
		UGameplayStatics::ApplyDamage(Target, Damage, GetInstigatorController(), Hero, UEISpellDamage::StaticClass());
		if (Enemy != nullptr)
		{
			if (FreezeSeconds > 0.0f)
			{
				Enemy->Freeze(FreezeSeconds);
			}
			if (SlowShare > 0.0f)
			{
				Enemy->ApplySlow(SlowShare, StatusSeconds);
			}
			if (BurnPerSecond > 0.0f)
			{
				Enemy->ApplyBurn(BurnPerSecond, StatusSeconds);
			}
		}
	}
}
