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
	const FVector LanceSizeCm(140.0f, 16.0f, 16.0f);
	const FVector PiercingBoltSizeCm(70.0f, 14.0f, 14.0f);
	constexpr float MinLightRadiusCm = 150.0f;
	constexpr float BeamSeconds = 0.25f;
	constexpr float BeamThicknessCm = 7.0f;
	constexpr float BeamGlow = 6.0f;
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
	UGameplayStatics::ApplyDamage(Target, Damage, GetInstigatorController(), Caster, nullptr);
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
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AEIExplosionFX* FX = GetWorld()->SpawnActor<AEIExplosionFX>(Location, FRotator::ZeroRotator, Params))
	{
		FX->Setup(RadiusCm, EIElementColor(Element));
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
	Light->SetCastShadows(false);
}

void AEIExplosionFX::Setup(float InRadiusCm)
{
	Setup(InRadiusCm, EIElementColor(EEIElement::Fire));
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

AEIBeamFX::AEIBeamFX()
{
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = BeamSeconds;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Beam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beam"));
	SetRootComponent(Beam);
	Beam->SetStaticMesh(Cube.Object);
	Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beam->SetCastShadow(false);
}

void AEIBeamFX::Setup(const FVector& From, const FVector& To, const FLinearColor& Color)
{
	SetActorLocationAndRotation((From + To) / 2.0, (To - From).Rotation());
	Beam->SetWorldScale3D(FVector(FVector::Dist(From, To), BeamThicknessCm, BeamThicknessCm) / 100.0f);
	Material = EIVisuals::SetGlow(Beam, Color, BeamGlow);
}

void AEIBeamFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (Material != nullptr)
	{
		Material->SetScalarParameterValue(TEXT("Deckkraft"), FMath::Clamp(1.0f - Age / BeamSeconds, 0.0f, 1.0f));
	}
}
