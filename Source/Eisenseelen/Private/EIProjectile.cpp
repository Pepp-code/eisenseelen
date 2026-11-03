#include "EIProjectile.h"

#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EICasterComponent.h"
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
	const FLinearColor FireColor(1.0f, 0.38f, 0.06f);
	constexpr float SphereMeshRadiusCm = 50.0f;
	constexpr float BoltVisualRadiusCm = 12.0f;
	constexpr float FireballVisualRadiusCm = 38.0f;
	constexpr float BallGlow = 6.0f;
	constexpr float BoltLight = 6000.0f;
	constexpr float FireballLight = 30000.0f;

	constexpr float BurstSeconds = 0.28f;
	constexpr float BurstGlow = 5.0f;
	constexpr float BurstLightPerRadius = 200.0f;
	constexpr float CloudFadeSeconds = 0.8f;
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
	Light->SetLightColor(FireColor);
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
}

void AEIProjectile::Configure(const FEIProjectileValues& Values, bool bInBig)
{
	Damage = Values.Damage;
	RadiusCm = Values.RadiusMeters * CentimetersPerMeter;
	bBig = bInBig;
	Flight->InitialSpeed = Values.SpeedMetersPerSecond * CentimetersPerMeter;
	Flight->MaxSpeed = Flight->InitialSpeed;
}

void AEIProjectile::BeginPlay()
{
	Super::BeginPlay();

	const float VisualRadius = bBig ? FireballVisualRadiusCm : BoltVisualRadiusCm;
	Ball->SetRelativeScale3D(FVector(VisualRadius / SphereMeshRadiusCm));
	EIVisuals::SetColor(Ball, FireColor, BallGlow);
	Light->SetIntensity(bBig ? FireballLight : BoltLight);
	Light->SetAttenuationRadius(RadiusCm * 2.0f);

	// Heroes never get in the way of fire, not even their own.
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		Collision->IgnoreActorWhenMoving(*It, true);
	}
	if (HasAuthority())
	{
		Flight->OnProjectileStop.AddDynamic(this, &AEIProjectile::HandleStop);
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

	AActor* Caster = GetOwner();
	AController* CasterController = GetInstigatorController();
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
			// Only a real fireball carries the element, not the small bolts of the staff.
			const AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(Caster);
			AEIEnemy* Enemy = Cast<AEIEnemy>(Target);
			if (bBig && Hero != nullptr && Enemy != nullptr)
			{
				Hero->GetRunes()->HitWithElement(Enemy, EEIElement::Fire);
			}
			UGameplayStatics::ApplyDamage(Target, Damage, CasterController, Caster, nullptr);
		}
	}
	MulticastBurst(Location);
	Destroy();
}

void AEIProjectile::MulticastBurst_Implementation(FVector_NetQuantize Location)
{
	EIVisuals::PlaySoundAt(this, bBig ? TEXT("ExplosionGross") : TEXT("ExplosionKlein"), Location);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AEIExplosionFX* FX = GetWorld()->SpawnActor<AEIExplosionFX>(Location, FRotator::ZeroRotator, Params))
	{
		FX->Setup(RadiusCm);
	}
}

AEIExplosionFX::AEIExplosionFX()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = BurstSeconds;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Ball = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ball"));
	SetRootComponent(Ball);
	Ball->SetStaticMesh(Sphere.Object);
	Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ball->SetCastShadow(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Ball);
	Light->SetLightColor(FireColor);
	Light->SetCastShadows(false);
}

void AEIExplosionFX::Setup(float InRadiusCm)
{
	Setup(InRadiusCm, FireColor);
}

void AEIExplosionFX::Setup(float InRadiusCm, const FLinearColor& Color)
{
	RadiusCm = InRadiusCm;
	Material = EIVisuals::SetGlow(Ball, Color, BurstGlow);
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(RadiusCm * 3.0f);
	Light->SetIntensity(RadiusCm * BurstLightPerRadius);
	Ball->SetWorldScale3D(FVector(RadiusCm * 0.2f / SphereMeshRadiusCm));
}

void AEIExplosionFX::SetupCloud(float InRadiusCm, const FLinearColor& Color, float Seconds, float Strength)
{
	RadiusCm = InRadiusCm;
	LifeSeconds = Seconds;
	SetLifeSpan(Seconds);
	Material = EIVisuals::SetGlow(Ball, Color, Strength);
	Light->SetIntensity(0.0f);
	Ball->SetWorldScale3D(FVector(RadiusCm * 0.2f / SphereMeshRadiusCm));
}

void AEIExplosionFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / BurstSeconds, 0.0f, 1.0f);
	if (LifeSeconds > 0.0f)
	{
		Ball->SetWorldScale3D(FVector(RadiusCm * FMath::Lerp(0.2f, 1.0f, 1.0f - FMath::Square(1.0f - T)) / SphereMeshRadiusCm));
		if (Material != nullptr)
		{
			Material->SetScalarParameterValue(TEXT("Deckkraft"), FMath::Clamp((LifeSeconds - Age) / CloudFadeSeconds, 0.0f, 1.0f));
		}
		return;
	}
	// Fast at first and slowing down, so it reads as a blast and not as a growing ball.
	const float Growth = 1.0f - FMath::Square(1.0f - T);
	Ball->SetWorldScale3D(FVector(RadiusCm * FMath::Lerp(0.2f, 1.0f, Growth) / SphereMeshRadiusCm));
	if (Material != nullptr)
	{
		Material->SetScalarParameterValue(TEXT("Deckkraft"), 1.0f - T);
	}
	Light->SetIntensity(RadiusCm * BurstLightPerRadius * (1.0f - T));
}
