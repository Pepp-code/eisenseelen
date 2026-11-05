#include "EIBoss.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EIDataFile.h"
#include "EIGameMode.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIRunDirector.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float HomeReachCm = 300.0f;
	const FVector StompKnockback(900.0f, 0.0f, 500.0f);
	constexpr float WardSize = 1.7f;
	constexpr float WardHeightCm = 10.0f;
	constexpr float WardStrength = 0.5f;
	const FLinearColor WardColor(0.45f, 0.9f, 1.0f);
	constexpr float FirstSummonShare = 0.33f;
	constexpr float FirstVanishShare = 0.5f;
	constexpr int32 VanishTries = 8;
	constexpr float VanishLandAboveCm = 5.0f;
	constexpr float GroundSearchCm = 5000.0f;
}

AEIBoss::AEIBoss()
{
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Ward = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ward"));
	Ward->SetupAttachment(GetCapsuleComponent());
	Ward->SetStaticMesh(Sphere.Object);
	Ward->SetRelativeScale3D(FVector(WardSize));
	Ward->SetRelativeLocation(FVector(0.0f, 0.0f, WardHeightCm));
	Ward->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ward->SetCastShadow(false);
	Ward->SetVisibility(false);
}

void AEIBoss::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIBoss, BossIndex);
	DOREPLIFETIME(AEIBoss, bBound);
	DOREPLIFETIME(AEIBoss, bShielded);
}

void AEIBoss::BeginPlay()
{
	// The enemy loads its numbers here, and with them the table.
	Super::BeginPlay();
	EIVisuals::SetGlow(Ward, WardColor, WardStrength);
	ApplyGraveLook();
	OnRep_Shielded();
}

void AEIBoss::LoadTable()
{
	if (!Table.Bosses.IsEmpty())
	{
		return;
	}
	FString Error;
	if (!FEIBossTable::LoadFromProjectFile(Table, Error))
	{
		FEIDataFile::ReportError(TEXT("bosse.json"), Error);
	}
}

const FEIBossValues* AEIBoss::GetBossValues() const
{
	return Table.Bosses.IsValidIndex(BossIndex) ? &Table.Bosses[BossIndex] : nullptr;
}

bool AEIBoss::SleepsInGrave() const
{
	const FEIBossValues* Boss = GetBossValues();
	return bBound && Boss != nullptr && Boss->Bannbruch.Kind == EEIBannbruchKind::Kettle;
}

bool AEIBoss::LoadEnemyValues(FEIEnemyValues& OutValues, FString& OutError)
{
	LoadTable();
	if (const FEIBossValues* Boss = GetBossValues())
	{
		OutValues = Boss->Fight;
		return true;
	}
	return Super::LoadEnemyValues(OutValues, OutError);
}

FLinearColor AEIBoss::PickBaseColor() const
{
	const FEIBossValues* Boss = GetBossValues();
	return Boss != nullptr ? Boss->Color : Super::PickBaseColor();
}

void AEIBoss::ApplyGraveLook()
{
	// Whoever sleeps in a grave is not there yet: nobody sees it, hits it or bumps into it.
	const bool bInGrave = SleepsInGrave();
	SetActorHiddenInGame(bInGrave);
	SetActorEnableCollision(!bInGrave);
	if (!HasAuthority())
	{
		return;
	}
	if (bInGrave)
	{
		GetCharacterMovement()->DisableMovement();
	}
	else if (GetCharacterMovement()->MovementMode == MOVE_None && !IsDead())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AEIBoss::OnRep_Bound()
{
	ApplyGraveLook();
	if (!bBound)
	{
		EIVisuals::PlaySoundAt(this, TEXT("ExplosionGross"), GetActorLocation());
	}
}

void AEIBoss::OnRep_Shielded()
{
	Ward->SetVisibility(bShielded);
}

void AEIBoss::BreakFree(int32 PlayerCount)
{
	if (!bBound)
	{
		return;
	}
	bBound = false;
	OnRep_Bound();
	GetHealth()->Reset(GetValues().Life * (1.0f + Table.LifePerExtraPlayer * FMath::Max(PlayerCount - 1, 0)));
	const double Now = GetWorld()->GetTimeSeconds();
	if (const FEIBossValues* Boss = GetBossValues())
	{
		NextStompAt = Now + Boss->Stomp.PauseSeconds / 2.0f;
		NextSummonAt = Now + Boss->Summon.EverySeconds * FirstSummonShare;
		NextVanishAt = Now + Boss->Vanish.EverySeconds * FirstVanishShare;
	}
	WakeUp(false);
}

void AEIBoss::HurryMoves()
{
	NextStompAt = 0.0;
	NextSummonAt = 0.0;
	NextVanishAt = 0.0;
}

bool AEIBoss::CanStartStomp(const AActor* Target) const
{
	const FEIBossValues* Boss = GetBossValues();
	return Boss != nullptr && Boss->Stomp.bHas && Target != nullptr && GetWorld()->GetTimeSeconds() >= NextStompAt
		&& FVector::Dist2D(Target->GetActorLocation(), GetActorLocation()) <= Boss->Stomp.FromMeters * CentimetersPerMeter;
}

void AEIBoss::StartStomp()
{
	const FEIBossValues* Boss = GetBossValues();
	StopWalking();
	Move = EMove::Stomp;
	MoveLeft = Boss->Stomp.WindupSeconds;
	ShowGroundCircle(Boss->Stomp.RadiusMeters * CentimetersPerMeter, Boss->Stomp.WindupSeconds);
	SetTelegraphing(true);
}

void AEIBoss::Stomp()
{
	const FEIBossValues* Boss = GetBossValues();
	const FEIGroundMarkData Ring = GetGroundMark();
	HideGroundMark();
	SetTelegraphing(false);
	MulticastStrike();
	MulticastStompSound();
	NextStompAt = GetWorld()->GetTimeSeconds() + (Boss != nullptr ? Boss->Stomp.PauseSeconds : 0.0f);
	if (Boss == nullptr)
	{
		return;
	}
	NameAttack(TEXT("Stampfen"));
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || !Ring.Contains(It->GetActorLocation()))
		{
			continue;
		}
		UGameplayStatics::ApplyDamage(*It, Boss->Stomp.Damage, GetController(), this, nullptr);
		if (It->CanBeDamaged())
		{
			const FVector Away = (It->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			It->LaunchCharacter(Away * StompKnockback.X + FVector(0.0f, 0.0f, StompKnockback.Z), true, true);
		}
	}
}

void AEIBoss::StartSummon()
{
	const FEIBossValues* Boss = GetBossValues();
	StopWalking();
	Move = EMove::Summon;
	MoveLeft = Boss->Summon.WindupSeconds;
	// The ring shows where the group will stand.
	ShowGroundCircle(Boss->Summon.DistanceMeters * CentimetersPerMeter, Boss->Summon.WindupSeconds);
	SetTelegraphing(true);
	NameAttack(TEXT("Beschwören"));
}

void AEIBoss::Summon()
{
	const FEIBossValues* Boss = GetBossValues();
	HideGroundMark();
	SetTelegraphing(false);
	const double Now = GetWorld()->GetTimeSeconds();
	NextSummonAt = Now + (Boss != nullptr ? Boss->Summon.EverySeconds : 0.0f);
	const AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>();
	if (Boss == nullptr || GameMode == nullptr)
	{
		return;
	}
	MulticastStompSound();
	Summons.Reset();
	for (AEIEnemy* Called : GameMode->GetRun()->SummonGroup(Boss->Summon.GroupSize, GetFloorSpot(), Boss->Summon.DistanceMeters * CentimetersPerMeter))
	{
		Summons.Add(Called);
	}
	if (!Summons.IsEmpty())
	{
		ShieldStartedAt = Now;
		bShielded = true;
		OnRep_Shielded();
	}
}

void AEIBoss::UpdateShield()
{
	if (!bShielded)
	{
		return;
	}
	const FEIBossValues* Boss = GetBossValues();
	const bool bAnyAlive = Summons.ContainsByPredicate([](const TWeakObjectPtr<AEIEnemy>& Called) { return Called.IsValid() && !Called->IsDead(); });
	const double Now = GetWorld()->GetTimeSeconds();
	if (bAnyAlive && Boss != nullptr && Now - ShieldStartedAt < Boss->Summon.MaxShieldSeconds)
	{
		return;
	}
	// Once its group has fallen, or after a while anyway, it is open to hits again for a good stretch.
	bShielded = false;
	OnRep_Shielded();
	Summons.Reset();
	NextSummonAt = Now + (Boss != nullptr ? Boss->Summon.EverySeconds : 0.0f);
	EIVisuals::PlaySoundAt(this, TEXT("Blocken"), GetActorLocation());
}

bool AEIBoss::StartVanish()
{
	const FEIBossValues* Boss = GetBossValues();
	const double Now = GetWorld()->GetTimeSeconds();
	NextVanishAt = Now + Boss->Vanish.EverySeconds;
	const float Range = Boss->Vanish.RangeMeters * CentimetersPerMeter;
	const float Min = Boss->Vanish.MinMeters * CentimetersPerMeter;
	bool bFound = false;
	const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	for (int32 Try = 0; Try < VanishTries && !bFound; ++Try)
	{
		FNavLocation Point;
		if (Navigation != nullptr && Navigation->GetDefaultNavDataInstance() != nullptr)
		{
			bFound = Navigation->GetRandomReachablePointInRadius(GetHome(), Range, Point) && FVector::Dist2D(Point.Location, GetActorLocation()) >= Min;
			VanishTarget = Point.Location;
			continue;
		}
		// Without paths on the map it looks for open floor on its own.
		const FVector Direction = FVector::ForwardVector.RotateAngleAxis(FMath::FRandRange(0.0f, 360.0f), FVector::UpVector);
		const FVector Wanted = GetHome() + Direction * FMath::FRandRange(0.0f, Range);
		FHitResult Hit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(EIBossVanish), false, this);
		bFound = GetWorld()->LineTraceSingleByObjectType(Hit, Wanted + FVector(0.0f, 0.0f, GroundSearchCm), Wanted - FVector(0.0f, 0.0f, GroundSearchCm), FCollisionObjectQueryParams(ECC_WorldStatic), Query)
			&& FVector::Dist2D(Hit.ImpactPoint, GetActorLocation()) >= Min;
		VanishTarget = Hit.ImpactPoint;
	}
	if (!bFound)
	{
		return false;
	}
	StopWalking();
	Move = EMove::Vanish;
	MoveLeft = Boss->Vanish.WindupSeconds;
	// The ring lies where it will come up, so that nobody gets surprised from behind.
	ShowGroundCircleAt(VanishTarget, GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.0f, Boss->Vanish.WindupSeconds);
	SetTelegraphing(true);
	NameAttack(TEXT("Verschwinden"));
	return true;
}

void AEIBoss::Vanish()
{
	HideGroundMark();
	SetTelegraphing(false);
	EIVisuals::PlaySoundAt(this, TEXT("Sprungfeld"), GetActorLocation());
	if (AAIController* AI = GetController<AAIController>())
	{
		AI->StopMovement();
	}
	SetActorLocation(VanishTarget + FVector(0.0f, 0.0f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + VanishLandAboveCm), false, nullptr, ETeleportType::TeleportPhysics);
	EIVisuals::PlaySoundAt(this, TEXT("Sprungfeld"), GetActorLocation());
}

bool AEIBoss::UpdateMove(float DeltaSeconds)
{
	if (Move == EMove::None)
	{
		return false;
	}
	// A parry or a heavy hit staggers it, which takes the ring away, and the move is off.
	if (!GetGroundMark().bVisible)
	{
		Move = EMove::None;
		SetTelegraphing(false);
		return false;
	}
	MoveLeft -= DeltaSeconds;
	if (MoveLeft <= 0.0f)
	{
		FinishMove();
	}
	return true;
}

void AEIBoss::FinishMove()
{
	const EMove Finished = Move;
	Move = EMove::None;
	switch (Finished)
	{
	case EMove::Stomp:
		Stomp();
		break;
	case EMove::Summon:
		Summon();
		break;
	case EMove::Vanish:
		Vanish();
		break;
	default:
		break;
	}
}

void AEIBoss::TickServer(float DeltaSeconds)
{
	if (bBound)
	{
		return;
	}
	UpdateShield();
	if (UpdateMove(DeltaSeconds))
	{
		return;
	}
	if (IsWatching())
	{
		GetHealth()->Heal(GetHealth()->GetMaxHealth() * Table.RecoverySharePerSecond * DeltaSeconds);
	}
	const double Leash = Table.LeashMeters * CentimetersPerMeter;
	if (IsChasing() && FVector::Dist2D(GetActorLocation(), GetHome()) > Leash)
	{
		GoHomeNow();
		return;
	}
	if (IsGoingHome())
	{
		// On the way back it does not turn around for anybody, or it would be stuck at the end of its leash.
		if (FVector::Dist2D(GetActorLocation(), GetHome()) <= HomeReachCm)
		{
			ArriveHome();
		}
		else
		{
			WalkHome(DeltaSeconds);
		}
		return;
	}
	const FEIBossValues* Boss = GetBossValues();
	const AActor* Target = IsChasing() ? FindNearestHero() : nullptr;
	if (Boss != nullptr && Target != nullptr)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (Boss->Summon.bHas && !bShielded && Now >= NextSummonAt)
		{
			StartSummon();
			return;
		}
		if (Boss->Vanish.bHas && Now >= NextVanishAt && StartVanish())
		{
			return;
		}
		if (CanStartStomp(Target))
		{
			StartStomp();
			return;
		}
	}
	Super::TickServer(DeltaSeconds);
}
