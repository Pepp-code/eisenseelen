#include "EIProjectile.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EICasterComponent.h"
#include "EIEffects.h"
#include "EIEnemy.h"
#include "EIHeroCharacter.h"
#include "EIRuneComponent.h"
#include "EIVisuals.h"
#include "Engine/CollisionProfile.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float LifeSeconds = 5.0f;
	constexpr float CollisionRadiusCm = 10.0f;
	constexpr float SphereMeshRadiusCm = 50.0f;
	constexpr float BoltVisualRadiusCm = 12.0f;
	constexpr float FireballVisualRadiusCm = 38.0f;
	constexpr float BallGlow = 6.0f;
	constexpr float BoltLight = 6000.0f;
	constexpr float FireballLight = 30000.0f;

	constexpr float MinBurstRadiusCm = 60.0f;
	const FVector LanceSizeCm(140.0f, 16.0f, 16.0f);
	const FVector PiercingBoltSizeCm(70.0f, 14.0f, 14.0f);
	constexpr float MinLightRadiusCm = 150.0f;
}

AEIProjectile::AEIProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true);
	InitialLifeSpan = LifeSeconds;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(CollisionRadiusCm);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	SetRootComponent(Collision);

	Ball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
	Ball->SetupAttachment(Collision);
	Ball->SetStaticMesh(Sphere.Object);
	Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ball->SetCastShadow(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Collision);
	Light->SetCastShadows(false);

	Flight = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Flight"));
	Flight->ProjectileGravityScale = 0.0f;
	Flight->bRotationFollowsVelocity = true;
	Flight->bShouldBounce = false;
}

void AEIProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIProjectile, RadiusCm);
	DOREPLIFETIME(AEIProjectile, bBig);
	DOREPLIFETIME(AEIProjectile, bPierces);
	DOREPLIFETIME(AEIProjectile, Element);
}

void AEIProjectile::Configure(const FEIProjectileSetup& Setup)
{
	Damage = Setup.Damage;
	RadiusCm = Setup.RadiusCm;
	bBig = Setup.bBig;
	bPierces = Setup.bPierces;
	bCarriesElement = Setup.bCarriesElement;
	Element = Setup.Element;
	SlowShare = Setup.SlowShare;
	SlowSeconds = Setup.SlowSeconds;
	FreezeSeconds = Setup.FreezeSeconds;
	Flight->InitialSpeed = Setup.SpeedCm;
	Flight->MaxSpeed = Setup.SpeedCm;
}

void AEIProjectile::BeginPlay()
{
	Super::BeginPlay();

	// What goes through people is long and thin, what bursts is round.
	const float VisualRadius = bBig ? FireballVisualRadiusCm : BoltVisualRadiusCm;
	const FVector RoundSizeCm(VisualRadius * 2.0f);
	Ball->SetRelativeScale3D((bPierces ? (bBig ? LanceSizeCm : PiercingBoltSizeCm) : RoundSizeCm) / (SphereMeshRadiusCm * 2.0f));
	const FLinearColor Color = EIElementColor(Element);
	EIVisuals::SetColor(Ball, Color, BallGlow);
	Light->SetLightColor(Color);
	Light->SetIntensity(bBig ? FireballLight : BoltLight);
	Light->SetAttenuationRadius(FMath::Max(RadiusCm, MinLightRadiusCm) * 2.0f);

	FEIEffectParams Look;
	Look.Location = GetActorLocation();
	Look.Rotation = GetActorRotation();
	Look.Color = Color;
	Look.AttachTo = Collision;
	Trail = AEIEffect::Play(this, bBig ? (bPierces ? TEXT("eislanze") : TEXT("feuerball")) : TEXT("bolzen"), Look);
	if (Trail.IsValid() && Trail->HidesPlaceholder())
	{
		Ball->SetVisibility(false);
	}

	// Heroes never get in the way of fire, not even their own.
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		Collision->IgnoreActorWhenMoving(*It, true);
	}
	if (HasAuthority())
	{
		Flight->OnProjectileStop.AddDynamic(this, &AEIProjectile::HandleStop);
	}
	if (bPierces)
	{
		Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Collision->SetGenerateOverlapEvents(true);
		if (HasAuthority())
		{
			Collision->OnComponentBeginOverlap.AddDynamic(this, &AEIProjectile::HandlePierce);
		}
	}
}

void AEIProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Trail.IsValid())
	{
		Trail->Stop();
	}
	Super::EndPlay(EndPlayReason);
}

void AEIProjectile::HandlePierce(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != nullptr && !OtherActor->IsA<AEIHeroCharacter>() && OtherActor->CanBeDamaged() && !Pierced.Contains(OtherActor))
	{
		Pierced.Add(OtherActor);
		HitTarget(OtherActor);
	}
}

void AEIProjectile::HitTarget(AActor* Target)
{
	AActor* Caster = GetOwner();
	const AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(Caster);
	AEIEnemy* Enemy = Cast<AEIEnemy>(Target);
	if (bCarriesElement && Hero != nullptr && Enemy != nullptr)
	{
		Hero->GetRunes()->HitWithElement(Enemy, Element);
	}
	// A big spell goes around a shield, a small bolt does not.
	UGameplayStatics::ApplyDamage(Target, Damage, GetInstigatorController(), Caster, bBig ? UEISpellDamage::StaticClass() : nullptr);
	if (Enemy != nullptr && SlowSeconds > 0.0f)
	{
		if (Enemy->IsSlowed() && FreezeSeconds > 0.0f)
		{
			Enemy->Freeze(FreezeSeconds);
		}
		Enemy->ApplySlow(SlowShare, SlowSeconds);
	}
}

void AEIProjectile::HandleStop(const FHitResult& Hit)
{
	Burst(Hit.ImpactPoint);
}

void AEIProjectile::LifeSpanExpired()
{
	if (HasAuthority())
	{
		Burst(GetActorLocation());
	}
	else
	{
		Super::LifeSpanExpired();
	}
}

void AEIProjectile::Burst(const FVector& Location)
{
	if (bBurst)
	{
		return;
	}
	bBurst = true;

	// What went through people has done its work on the way.
	if (!bPierces)
	{
		TArray<FOverlapResult> Overlaps;
		GetWorld()->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(RadiusCm));
		TSet<AActor*> Done;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Target = Overlap.GetActor();
			if (Target == nullptr || Done.Contains(Target) || Target->IsA<AEIHeroCharacter>() || !Target->CanBeDamaged())
			{
				continue;
			}
			Done.Add(Target);
			FCollisionQueryParams WallQuery(SCENE_QUERY_STAT(EIBurstWall), false, Target);
			if (!GetWorld()->LineTraceTestByObjectType(Location, Target->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), WallQuery))
			{
				HitTarget(Target);
			}
		}
	}
	MulticastBurst(Location);
	Destroy();
}

void AEIProjectile::MulticastBurst_Implementation(FVector_NetQuantize Location)
{
	EIVisuals::PlaySoundAt(this, bBig && !bPierces ? TEXT("ExplosionGross") : TEXT("ExplosionKlein"), Location);
	FEIEffectParams Look;
	Look.Location = Location;
	Look.Rotation = GetActorRotation();
	Look.Color = EIElementColor(Element);
	Look.RadiusCm = FMath::Max(RadiusCm, MinBurstRadiusCm);
	AEIEffect::Play(this, bBig ? (bPierces ? TEXT("eislanze_treffer") : TEXT("feuerball_explosion")) : TEXT("bolzen_treffer"), Look);
}
