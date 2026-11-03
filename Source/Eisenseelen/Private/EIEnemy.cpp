#include "EIEnemy.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FLinearColor BoneColor(0.32f, 0.31f, 0.28f);
	const FLinearColor WarningColor(1.0f, 0.12f, 0.04f);
	const FLinearColor HurtColor(1.0f, 0.9f, 0.7f);
	const FLinearColor StaggerColor(0.75f, 0.7f, 0.2f);
	constexpr float WarningGlow = 3.0f;
	constexpr float HurtFlashSeconds = 0.1f;
	constexpr float HurtGlow = 2.5f;
	constexpr float TelegraphBlendPerSecond = 8.0f;
	constexpr float TelegraphStretch = 0.22f;
	constexpr float StaggerBlendPerSecond = 10.0f;
	constexpr float StaggerTiltDegrees = 28.0f;
	constexpr float BodyHeight = -20.0f;
	constexpr float HeadAboveBody = 86.0f;

	constexpr float StuckBelowSpeedCm = 80.0f;
	constexpr float StuckAfterSeconds = 0.6f;
	constexpr float SidestepSeconds = 0.9f;
	constexpr float PathGoalReachCm = 50.0f;
	constexpr float PathRetrySeconds = 0.5f;

	constexpr int32 DebrisPieces = 7;
	constexpr float DebrisLifeSeconds = 1.8f;
	constexpr float DebrisSpeedCm = 450.0f;
}

bool FEIEnemyValues::LoadFromProjectFile(FEIEnemyValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("gegner.json"), OutError);
	const TSharedPtr<FJsonObject>* Runner = nullptr;
	if (!Root.IsValid())
	{
		return false;
	}
	if (!Root->TryGetObjectField(TEXT("knochenlaeufer"), Runner))
	{
		OutError = TEXT("Der Abschnitt knochenlaeufer fehlt.");
		return false;
	}
	FEIEnemyValues Loaded;
	if (!FEIDataFile::ReadNumber(**Runner, TEXT("leben"), 1.0, 100000.0, Loaded.Life, OutError)
		|| !FEIDataFile::ReadNumber(**Runner, TEXT("tempo_m_pro_s"), 0.1, 30.0, Loaded.SpeedMetersPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(**Runner, TEXT("schaden"), 0.0, 10000.0, Loaded.Damage, OutError)
		|| !FEIDataFile::ReadNumber(**Runner, TEXT("angriff_ab_abstand_m"), 0.5, 20.0, Loaded.AttackFromMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Runner, TEXT("ausholen_s"), 0.0, 10.0, Loaded.WindupSeconds, OutError)
		|| !FEIDataFile::ReadNumber(**Runner, TEXT("trefferzone_m"), 0.5, 20.0, Loaded.HitZoneMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Runner, TEXT("erholung_s"), 0.0, 10.0, Loaded.RecoverSeconds, OutError)
		|| !FEIDataFile::ReadNumber(**Runner, TEXT("taumeln_faktor_schwerer_hieb_0_bis_1"), 0.0, 1.0, Loaded.HeavyHitStaggerFactor, OutError))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}

AEIEnemy::AEIEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetCanBeDamaged(true);
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	Movement->bUseRVOAvoidance = true;
	Movement->AvoidanceConsiderationRadius = 150.0f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetStaticMesh(Cylinder.Object);
	Body->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.3f));
	Body->SetRelativeLocation(FVector(0.0f, 0.0f, BodyHeight));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(GetCapsuleComponent());
	Head->SetStaticMesh(Sphere.Object);
	Head->SetRelativeScale3D(FVector(0.42f));
	Head->SetRelativeLocation(FVector(0.0f, 0.0f, BodyHeight + HeadAboveBody));
	Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Health = CreateDefaultSubobject<UEIHealthComponent>(TEXT("Health"));
}

void AEIEnemy::BeginPlay()
{
	Super::BeginPlay();

	BodyMaterial = EIVisuals::SetColor(Body, BoneColor);
	HeadMaterial = EIVisuals::SetColor(Head, BoneColor);
	if (!HasAuthority())
	{
		return;
	}
	FString Error;
	FEIEnemyValues Loaded;
	if (!FEIEnemyValues::LoadFromProjectFile(Loaded, Error))
	{
		FEIDataFile::ReportError(TEXT("gegner.json"), Error);
	}
	ApplyValues(Loaded);
	Health->Reset(Values.Life);
	Health->OnDied.AddUObject(this, &AEIEnemy::HandleDied);
}

void AEIEnemy::ApplyValues(const FEIEnemyValues& NewValues)
{
	Values = NewValues;
	GetCharacterMovement()->MaxWalkSpeed = Values.SpeedMetersPerSecond * CentimetersPerMeter;
}

void AEIEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIEnemy, bTelegraphing);
	DOREPLIFETIME(AEIEnemy, bStaggering);
}

bool AEIEnemy::IsDead() const
{
	return Health->IsDead();
}

void AEIEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && !IsDead())
	{
		TickServer(DeltaSeconds);
	}
	UpdateLook(DeltaSeconds);
}

AActor* AEIEnemy::FindNearestHero() const
{
	AActor* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		const double Distance = FVector::DistSquared(It->GetActorLocation(), GetActorLocation());
		if (!It->IsDead() && Distance < NearestDistance)
		{
			Nearest = *It;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

void AEIEnemy::TickServer(float DeltaSeconds)
{
	StateTime += DeltaSeconds;
	AActor* Target = FindNearestHero();
	switch (State)
	{
	case EState::Chase:
		if (Target == nullptr)
		{
			return;
		}
		if (FVector::Dist(Target->GetActorLocation(), GetActorLocation()) <= Values.AttackFromMeters * CentimetersPerMeter)
		{
			StopWalking();
			State = EState::Windup;
			StateTime = 0.0f;
			bTelegraphing = true;
			OnRep_Telegraphing();
			return;
		}
		Steer(Target, DeltaSeconds);
		return;

	case EState::Windup:
		if (StateTime >= Values.WindupSeconds)
		{
			bTelegraphing = false;
			OnRep_Telegraphing();
			State = EState::Recover;
			StateTime = 0.0f;
			// Last, because a parry during the strike changes the state.
			Strike();
		}
		return;

	case EState::Recover:
		if (StateTime >= Values.RecoverSeconds)
		{
			State = EState::Chase;
			StateTime = 0.0f;
		}
		return;

	case EState::Stagger:
		if (StateTime >= StaggerSeconds)
		{
			bStaggering = false;
			State = EState::Chase;
			StateTime = 0.0f;
		}
		return;
	}
}

void AEIEnemy::Stagger(float Seconds)
{
	if (!HasAuthority() || IsDead())
	{
		return;
	}
	StopWalking();
	State = EState::Stagger;
	StateTime = 0.0f;
	StaggerSeconds = Seconds;
	bStaggering = true;
	bTelegraphing = false;
	OnRep_Telegraphing();
}

void AEIEnemy::StaggerFromHeavyHit(float Seconds)
{
	const float Arriving = Seconds * Values.HeavyHitStaggerFactor;
	if (Arriving > 0.0f)
	{
		Stagger(Arriving);
	}
}

void AEIEnemy::StopWalking()
{
	if (AAIController* AI = GetController<AAIController>())
	{
		AI->StopMovement();
	}
}

void AEIEnemy::Steer(AActor* Target, float DeltaSeconds)
{
	AAIController* AI = GetController<AAIController>();
	if (AI == nullptr)
	{
		return;
	}
	if (AI->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}
	// A failed request stops the walker on the spot, so it is not tried again on every frame.
	PathRetryRemaining -= DeltaSeconds;
	if (PathRetryRemaining <= 0.0f && StandsOnPaths())
	{
		const bool bStopOnOverlap = false;
		const bool bUsePathfinding = true;
		const bool bCanStrafe = false;
		const bool bAllowPartialPath = true;
		if (AI->MoveToActor(Target, PathGoalReachCm, bStopOnOverlap, bUsePathfinding, bCanStrafe, nullptr, bAllowPartialPath) != EPathFollowingRequestResult::Failed)
		{
			return;
		}
		PathRetryRemaining = PathRetrySeconds;
	}
	SteerStraight(Target, DeltaSeconds);
}

bool AEIEnemy::StandsOnPaths() const
{
	const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation OnPaths;
	return Navigation != nullptr && Navigation->GetDefaultNavDataInstance() != nullptr && Navigation->ProjectPointToNavigation(GetActorLocation(), OnPaths);
}

void AEIEnemy::SteerStraight(const AActor* Target, float DeltaSeconds)
{
	// Without paths, as in the test arena or while they are still being built: if something blocks the way, sidestep for a moment.
	FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (SidestepRemaining > 0.0f)
	{
		SidestepRemaining -= DeltaSeconds;
		Direction = FVector(-Direction.Y, Direction.X, 0.0f) * SidestepSign;
	}
	else if (GetVelocity().Size2D() < StuckBelowSpeedCm)
	{
		StuckSeconds += DeltaSeconds;
		if (StuckSeconds >= StuckAfterSeconds)
		{
			StuckSeconds = 0.0f;
			SidestepRemaining = SidestepSeconds;
			SidestepSign = FMath::RandBool() ? 1.0f : -1.0f;
		}
	}
	else
	{
		StuckSeconds = 0.0f;
	}
	AddMovementInput(Direction);
}

void AEIEnemy::Strike()
{
	MulticastStrike();
	const float HitZone = Values.HitZoneMeters * CentimetersPerMeter;
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), GetActorLocation()) <= HitZone)
		{
			UGameplayStatics::ApplyDamage(*It, Values.Damage, GetController(), this, nullptr);
		}
	}
}

float AEIEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Applied = Health->ApplyDamage(Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser));
	if (Applied > 0.0f && !IsDead())
	{
		MulticastHurt();
	}
	return Applied;
}

void AEIEnemy::HandleDied()
{
	MulticastDie();
	GetCharacterMovement()->DisableMovement();
	SetLifeSpan(0.2f);
}

void AEIEnemy::OnRep_Telegraphing()
{
	if (bTelegraphing)
	{
		EIVisuals::PlaySoundAt(this, TEXT("GegnerAusholen"), GetActorLocation());
	}
}

void AEIEnemy::MulticastHurt_Implementation()
{
	HurtFlashRemaining = HurtFlashSeconds;
}

void AEIEnemy::MulticastStrike_Implementation()
{
	TelegraphBlend = 0.0f;
}

void AEIEnemy::MulticastDie_Implementation()
{
	EIVisuals::PlaySoundAt(this, TEXT("GegnerTod"), GetActorLocation());
	Body->SetVisibility(false);
	Head->SetVisibility(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActor<AEIBoneDebris>(GetActorLocation(), FRotator::ZeroRotator, Params);
}

void AEIEnemy::UpdateLook(float DeltaSeconds)
{
	if (BodyMaterial == nullptr || HeadMaterial == nullptr)
	{
		return;
	}
	TelegraphBlend = FMath::FInterpConstantTo(TelegraphBlend, bTelegraphing ? 1.0f : 0.0f, DeltaSeconds, TelegraphBlendPerSecond);
	HurtFlashRemaining = FMath::Max(0.0f, HurtFlashRemaining - DeltaSeconds);
	const float Hurt = HurtFlashRemaining / HurtFlashSeconds;
	StaggerBlend = FMath::FInterpConstantTo(StaggerBlend, bStaggering ? 1.0f : 0.0f, DeltaSeconds, StaggerBlendPerSecond);
	const FLinearColor Calm = FMath::Lerp(FMath::Lerp(BoneColor, WarningColor, TelegraphBlend), StaggerColor, StaggerBlend);
	const FLinearColor Color = FMath::Lerp(Calm, HurtColor, Hurt);
	const float Glow = FMath::Max(TelegraphBlend * WarningGlow, Hurt * HurtGlow);
	for (UMaterialInstanceDynamic* Material : { BodyMaterial.Get(), HeadMaterial.Get() })
	{
		Material->SetVectorParameterValue(TEXT("Farbe"), Color);
		Material->SetScalarParameterValue(TEXT("Leuchten"), Glow);
	}
	// It rears up while it winds up, readable even without the color.
	const float HeadLength = HeadAboveBody + TelegraphBlend * TelegraphStretch * 100.0f;
	// A staggered runner leans back, body and head together.
	const float Tilt = StaggerBlend * StaggerTiltDegrees;
	Body->SetRelativeRotation(FRotator(Tilt, 0.0f, 0.0f));
	Head->SetRelativeLocation(FVector(-FMath::Sin(FMath::DegreesToRadians(Tilt)) * HeadLength, 0.0f, BodyHeight + FMath::Cos(FMath::DegreesToRadians(Tilt)) * HeadLength));
}

AEIBoneDebris::AEIBoneDebris()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = DebrisLifeSeconds;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AEIBoneDebris::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Cube = EIVisuals::GetCube();
	for (int32 Index = 0; Index < DebrisPieces; ++Index)
	{
		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
		Piece->SetStaticMesh(Cube);
		Piece->SetupAttachment(GetRootComponent());
		Piece->SetWorldScale3D(FVector(FMath::FRandRange(0.08f, 0.14f), FMath::FRandRange(0.08f, 0.14f), FMath::FRandRange(0.2f, 0.5f)));
		Piece->SetRelativeLocation(FVector(FMath::FRandRange(-20.0f, 20.0f), FMath::FRandRange(-20.0f, 20.0f), FMath::FRandRange(-60.0f, 60.0f)));
		Piece->SetCollisionProfileName(TEXT("PhysicsActor"));
		Piece->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Piece->RegisterComponent();
		EIVisuals::SetColor(Piece, BoneColor);
		Piece->SetSimulatePhysics(true);
		Piece->SetPhysicsLinearVelocity(FMath::VRand() * DebrisSpeedCm + FVector(0.0f, 0.0f, DebrisSpeedCm * 0.5f));
		Piece->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 720.0f);
	}
}
