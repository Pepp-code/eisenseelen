#include "EIBoss.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EIDataFile.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FLinearColor StompRingColor(1.0f, 0.1f, 0.03f);
	constexpr float StompRingStrength = 0.5f;
	constexpr float StompRingAboveFloorCm = 8.0f;
	constexpr float HomeReachCm = 300.0f;
	const FVector StompKnockback(900.0f, 0.0f, 500.0f);
}

AEIBoss::AEIBoss()
{
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	StompRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StompRing"));
	StompRing->SetupAttachment(GetCapsuleComponent());
	StompRing->SetStaticMesh(Cylinder.Object);
	StompRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StompRing->SetCastShadow(false);
	StompRing->SetVisibility(false);
}

void AEIBoss::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIBoss, BossIndex);
	DOREPLIFETIME(AEIBoss, bBound);
	DOREPLIFETIME(AEIBoss, bStompMarked);
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

void AEIBoss::BeginPlay()
{
	Super::BeginPlay();

	if (const FEIBossValues* Boss = GetBossValues())
	{
		// The sizes count within the boss, so they are divided by its own size. The engine's cylinder is 1 m wide.
		const float Width = Boss->Stomp.RadiusMeters * 2.0f / GetValues().Scale;
		const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		StompRing->SetRelativeLocation(FVector(0.0f, 0.0f, StompRingAboveFloorCm / GetValues().Scale - HalfHeight));
		StompRing->SetRelativeScale3D(FVector(Width, Width, 0.02f));
		EIVisuals::SetGlow(StompRing, StompRingColor, StompRingStrength);
	}
}

void AEIBoss::OnRep_Bound()
{
	if (!bBound)
	{
		EIVisuals::PlaySoundAt(this, TEXT("ExplosionGross"), GetActorLocation());
	}
}

void AEIBoss::OnRep_StompMarked()
{
	StompRing->SetVisibility(bStompMarked);
	if (bStompMarked)
	{
		EIVisuals::PlaySoundAt(this, TEXT("GegnerAusholen"), GetActorLocation());
	}
}

void AEIBoss::BreakFree(float StrengthFactor, int32 PlayerCount)
{
	if (!bBound)
	{
		return;
	}
	bBound = false;
	OnRep_Bound();
	DamageFactor = StrengthFactor;
	const float PlayerFactor = 1.0f + Table.LifePerExtraPlayer * FMath::Max(PlayerCount - 1, 0);
	ApplyStrength(StrengthFactor);
	GetHealth()->Reset(GetHealth()->GetMaxHealth() * PlayerFactor);
	NextStompAt = GetWorld()->GetTimeSeconds() + (GetBossValues() != nullptr ? GetBossValues()->Stomp.PauseSeconds / 2.0f : 0.0f);
	WakeUp(false);
}

bool AEIBoss::CanStartStomp(const AActor* Target) const
{
	const FEIBossValues* Boss = GetBossValues();
	return Boss != nullptr && Target != nullptr && GetWorld()->GetTimeSeconds() >= NextStompAt
		&& FVector::Dist2D(Target->GetActorLocation(), GetActorLocation()) <= Boss->Stomp.FromMeters * CentimetersPerMeter;
}

void AEIBoss::Stomp()
{
	const FEIBossValues* Boss = GetBossValues();
	bStompMarked = false;
	OnRep_StompMarked();
	SetTelegraphing(false);
	MulticastStrike();
	MulticastStompSound();
	NextStompAt = GetWorld()->GetTimeSeconds() + (Boss != nullptr ? Boss->Stomp.PauseSeconds : 0.0f);
	if (Boss == nullptr)
	{
		return;
	}
	const double Radius = Boss->Stomp.RadiusMeters * CentimetersPerMeter;
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		const FVector Away = It->GetActorLocation() - GetActorLocation();
		if (It->IsDead() || Away.Size2D() > Radius)
		{
			continue;
		}
		UGameplayStatics::ApplyDamage(*It, Boss->Stomp.Damage * DamageFactor, GetController(), this, nullptr);
		if (It->CanBeDamaged())
		{
			It->LaunchCharacter(Away.GetSafeNormal2D() * StompKnockback.X + FVector(0.0f, 0.0f, StompKnockback.Z), true, true);
		}
	}
}

void AEIBoss::TickServer(float DeltaSeconds)
{
	if (bBound)
	{
		return;
	}
	if (bStompMarked)
	{
		// A parry staggers it, and the stomp is off.
		if (IsStaggering())
		{
			bStompMarked = false;
			OnRep_StompMarked();
		}
		else
		{
			StompLeft -= DeltaSeconds;
			if (StompLeft <= 0.0f)
			{
				Stomp();
			}
			return;
		}
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
	if (IsChasing() && CanStartStomp(FindNearestHero()))
	{
		const FEIBossValues* Boss = GetBossValues();
		StopWalking();
		StompLeft = Boss->Stomp.WindupSeconds;
		bStompMarked = true;
		OnRep_StompMarked();
		SetTelegraphing(true);
		return;
	}
	Super::TickServer(DeltaSeconds);
}
