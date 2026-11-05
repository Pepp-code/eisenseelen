#include "EIBoss.h"

#include "EIDataFile.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float HomeReachCm = 300.0f;
	const FVector StompKnockback(900.0f, 0.0f, 500.0f);
}

AEIBoss::AEIBoss()
{
	bAlwaysRelevant = true;
}

void AEIBoss::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIBoss, BossIndex);
	DOREPLIFETIME(AEIBoss, bBound);
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

void AEIBoss::OnRep_Bound()
{
	if (!bBound)
	{
		EIVisuals::PlaySoundAt(this, TEXT("ExplosionGross"), GetActorLocation());
	}
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

void AEIBoss::TickServer(float DeltaSeconds)
{
	if (bBound)
	{
		return;
	}
	// A parry staggers it, which takes the ring away, and the stomp is off.
	if (IsStompMarked())
	{
		StompLeft -= DeltaSeconds;
		if (StompLeft <= 0.0f)
		{
			Stomp();
		}
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
	if (IsChasing() && CanStartStomp(FindNearestHero()))
	{
		const FEIBossValues* Boss = GetBossValues();
		StopWalking();
		StompLeft = Boss->Stomp.WindupSeconds;
		ShowGroundCircle(Boss->Stomp.RadiusMeters * CentimetersPerMeter, Boss->Stomp.WindupSeconds);
		SetTelegraphing(true);
		return;
	}
	Super::TickServer(DeltaSeconds);
}
