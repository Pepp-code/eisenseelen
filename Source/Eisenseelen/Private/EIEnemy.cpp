#include "EIEnemy.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIGameMode.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIRunDirector.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
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
	const FLinearColor ThrowerColor(0.2f, 0.22f, 0.34f);
	const FLinearColor ShieldBearerColor(0.33f, 0.22f, 0.14f);
	const FLinearColor ColossusColor(0.22f, 0.07f, 0.07f);
	const FLinearColor ShieldColor(0.5f, 0.45f, 0.35f);
	const FVector ShieldPlace(48.0f, 0.0f, 0.0f);
	const FVector ShieldSize(0.12f, 0.95f, 1.5f);
	const FVector HeldBonePlace(30.0f, 30.0f, 50.0f);
	const FVector HeldBoneSize(0.1f, 0.1f, 0.6f);
	const FLinearColor SlowedColor(0.35f, 0.65f, 1.0f);
	const FLinearColor FrozenColor(0.7f, 0.9f, 1.0f);
	const FLinearColor LeaderColor(0.75f, 0.55f, 0.1f);
	constexpr float FrozenGlow = 2.0f;
	const FLinearColor ChargeMarkColor(1.0f, 0.1f, 0.03f);
	constexpr float ChargeMarkStrength = 0.8f;
	constexpr float ChargeMarkWidthCm = 260.0f;
	constexpr float ChargeMarkAboveFloorCm = 8.0f;
	constexpr float ChargeBlockedAfterSeconds = 0.4f;
	constexpr float ChargeBlockedBelowCm = 150.0f;
	const FVector ChargeKnockback(700.0f, 0.0f, 350.0f);
	const FLinearColor BurningColor(1.0f, 0.4f, 0.08f);
	constexpr float BurningGlow = 1.5f;
	constexpr float BurnTickSeconds = 0.5f;
	constexpr float ThrowFromForwardCm = 60.0f;
	constexpr float ThrownBoneLifeSeconds = 3.0f;
	constexpr float ThrownBoneRadiusCm = 12.0f;
	const FVector ThrownBoneSize(0.45f, 0.1f, 0.1f);
	constexpr float ThrownBoneGlow = 1.5f;
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
	constexpr float NoticeCheckSeconds = 0.25f;
	constexpr float HomeReachCm = 200.0f;

	constexpr int32 DebrisPieces = 7;
	constexpr float DebrisLifeSeconds = 1.8f;
	constexpr float DebrisSpeedCm = 450.0f;

	// Walking.
	constexpr float WalkFullFromCm = 250.0f;
	constexpr float WalkBlendSpeed = 8.0f;
	constexpr float StrideCm = 110.0f;
	constexpr float WalkBobCm = 7.0f;
	constexpr float WalkWaddleDegrees = 7.0f;
	constexpr float WalkLeanDegrees = 9.0f;

	// Standing around.
	constexpr float IdlePhaseSpeed = 1.3f;
	constexpr float IdleSwayDegrees = 2.0f;

	// Winding up, striking and being hit.
	constexpr float WindupLeanDegrees = 16.0f;
	constexpr float StrikeLeanDegrees = 30.0f;
	constexpr float StrikeForwardCm = 30.0f;
	constexpr float StrikeRecoverSpeed = 5.0f;
	constexpr float RecoilLeanDegrees = 16.0f;
	constexpr float RecoilRecoverSpeed = 9.0f;
	constexpr float NoticeHeadCm = 16.0f;
	constexpr float NoticeRecoverSpeed = 4.0f;
	constexpr float StaggerWobbleDegrees = 8.0f;
	constexpr float StaggerWobbleSpeed = 14.0f;
	const FVector ThrowWindupOffset(-26.0f, 6.0f, 26.0f);
	const FVector ThrowFlingOffset(45.0f, -10.0f, 0.0f);
	const FVector ShieldBashOffset(32.0f, 0.0f, 4.0f);
	const FVector ShieldWindupOffset(-10.0f, 0.0f, 6.0f);
}

const TCHAR* EIEnemyKindKey(EEIEnemyKind Kind)
{
	switch (Kind)
	{
	case EEIEnemyKind::Thrower:
		return TEXT("knochenwerfer");
	case EEIEnemyKind::ShieldBearer:
		return TEXT("schildtraeger");
	case EEIEnemyKind::Colossus:
		return TEXT("kettenkoloss");
	default:
		return TEXT("knochenlaeufer");
	}
}

bool EIElementFromKey(const FString& Key, EEIElement& OutElement)
{
	if (Key == TEXT("feuer"))
	{
		OutElement = EEIElement::Fire;
		return true;
	}
	if (Key == TEXT("eis"))
	{
		OutElement = EEIElement::Ice;
		return true;
	}
	if (Key == TEXT("blitz"))
	{
		OutElement = EEIElement::Lightning;
		return true;
	}
	return false;
}

FLinearColor EIElementColor(EEIElement Element)
{
	switch (Element)
	{
	case EEIElement::Ice: return FLinearColor(0.35f, 0.7f, 1.0f);
	case EEIElement::Lightning: return FLinearColor(0.95f, 0.9f, 0.35f);
	default: return FLinearColor(1.0f, 0.38f, 0.06f);
	}
}

bool FEIEnemyValues::LoadFromProjectFile(EEIEnemyKind Kind, FEIEnemyValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("gegner.json"), OutError);
	const TSharedPtr<FJsonObject>* Section = nullptr;
	if (!Root.IsValid())
	{
		return false;
	}
	if (!Root->TryGetObjectField(EIEnemyKindKey(Kind), Section))
	{
		OutError = FString::Printf(TEXT("Der Abschnitt %s fehlt."), EIEnemyKindKey(Kind));
		return false;
	}
	FEIEnemyValues Loaded;
	if (!FEIDataFile::ReadNumber(**Section, TEXT("leben"), 1.0, 100000.0, Loaded.Life, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("bedrohung"), 0.0, 1000.0, Loaded.Threat, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("tempo_m_pro_s"), 0.1, 30.0, Loaded.SpeedMetersPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("schaden"), 0.0, 10000.0, Loaded.Damage, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("angriff_ab_abstand_m"), 0.5, 20.0, Loaded.AttackFromMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("ausholen_s"), 0.0, 10.0, Loaded.WindupSeconds, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("trefferzone_m"), 0.5, 20.0, Loaded.HitZoneMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("erholung_s"), 0.0, 10.0, Loaded.RecoverSeconds, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("taumeln_faktor_schwerer_hieb_0_bis_1"), 0.0, 1.0, Loaded.HeavyHitStaggerFactor, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("sicht_m"), 1.0, 500.0, Loaded.SightMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("alarm_umkreis_m"), 0.0, 500.0, Loaded.AlarmMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("aufgeben_m"), 1.0, 1000.0, Loaded.GiveUpMeters, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("wach_mindestens_s"), 0.0, 600.0, Loaded.StayAwakeSeconds, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("groesse_faktor"), 0.2, 10.0, Loaded.Scale, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("drehtempo_grad_pro_s"), 1.0, 3600.0, Loaded.TurnDegreesPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("schild_schaden_faktor_0_bis_1"), 0.0, 1.0, Loaded.ShieldDamageFactor, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("schild_winkel_grad"), 0.0, 360.0, Loaded.ShieldAngleDegrees, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("erfahrung"), 0.0, 1000000.0, Loaded.Experience, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("item_chance_0_bis_1"), 0.0, 1.0, Loaded.ItemChance, OutError)
		|| !FEIDataFile::ReadNumber(**Section, TEXT("wurf_tempo_m_pro_s"), 0.0, 200.0, Loaded.ThrowSpeedMetersPerSecond, OutError))
	{
		return false;
	}
	// Only a kind with a charge has these numbers.
	if ((*Section)->HasField(TEXT("ansturm_tempo_m_pro_s"))
		&& (!FEIDataFile::ReadNumber(**Section, TEXT("ansturm_tempo_m_pro_s"), 0.0, 100.0, Loaded.ChargeSpeedMetersPerSecond, OutError)
			|| !FEIDataFile::ReadNumber(**Section, TEXT("ansturm_strecke_m"), 0.0, 200.0, Loaded.ChargeDistanceMeters, OutError)
			|| !FEIDataFile::ReadNumber(**Section, TEXT("ansturm_ab_abstand_m"), 0.0, 200.0, Loaded.ChargeFromMeters, OutError)
			|| !FEIDataFile::ReadNumber(**Section, TEXT("ansturm_bis_abstand_m"), 0.0, 200.0, Loaded.ChargeToMeters, OutError)
			|| !FEIDataFile::ReadNumber(**Section, TEXT("ansturm_ankuendigung_s"), 0.0, 20.0, Loaded.ChargeWindupSeconds, OutError)
			|| !FEIDataFile::ReadNumber(**Section, TEXT("ansturm_schaden"), 0.0, 100000.0, Loaded.ChargeDamage, OutError)
			|| !FEIDataFile::ReadNumber(**Section, TEXT("ansturm_pause_s"), 0.0, 600.0, Loaded.ChargePauseSeconds, OutError)))
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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

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

	Held = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Held"));
	Held->SetupAttachment(GetCapsuleComponent());
	Held->SetStaticMesh(Cube.Object);
	Held->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Held->SetVisibility(false);

	ChargeMark = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChargeMark"));
	ChargeMark->SetupAttachment(GetCapsuleComponent());
	ChargeMark->SetStaticMesh(Cube.Object);
	ChargeMark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChargeMark->SetCastShadow(false);
	ChargeMark->SetVisibility(false);

	Health = CreateDefaultSubobject<UEIHealthComponent>(TEXT("Health"));
}

void AEIEnemy::BeginPlay()
{
	Super::BeginPlay();

	FString Error;
	FEIEnemyValues Loaded;
	if (!FEIEnemyValues::LoadFromProjectFile(Kind, Loaded, Error))
	{
		FEIDataFile::ReportError(TEXT("gegner.json"), Error);
	}
	ApplyValues(Loaded);
	switch (Kind)
	{
	case EEIEnemyKind::Thrower:
		BaseColor = ThrowerColor;
		break;
	case EEIEnemyKind::ShieldBearer:
		BaseColor = ShieldBearerColor;
		break;
	case EEIEnemyKind::Colossus:
		BaseColor = ColossusColor;
		break;
	default:
		BaseColor = BoneColor;
		break;
	}
	BodyMaterial = EIVisuals::SetColor(Body, BaseColor);
	HeadMaterial = EIVisuals::SetColor(Head, BaseColor);
	ShowHeldPart();
	SetActorScale3D(FVector(Values.Scale));
	if (IsLeader())
	{
		OnRep_LeaderScale();
	}
	if (Values.ChargeSpeedMetersPerSecond > 0.0f)
	{
		// The sizes count within the enemy, so they are divided by its own size.
		const float Length = Values.ChargeDistanceMeters * CentimetersPerMeter / Values.Scale;
		const float HalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		ChargeMark->SetRelativeLocation(FVector(Length / 2.0f, 0.0f, ChargeMarkAboveFloorCm / Values.Scale - HalfHeight));
		ChargeMark->SetRelativeScale3D(FVector(Length, ChargeMarkWidthCm / Values.Scale, 1.0f) / 100.0f);
		EIVisuals::SetGlow(ChargeMark, ChargeMarkColor, ChargeMarkStrength);
	}
	if (!HasAuthority())
	{
		return;
	}
	// A bigger body must not start in the floor.
	AddActorWorldOffset(FVector(0.0f, 0.0f, (Values.Scale - 1.0f) * GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
	Health->Reset(Values.Life);
	Health->OnDied.AddUObject(this, &AEIEnemy::HandleDied);
}

void AEIEnemy::ApplyValues(const FEIEnemyValues& NewValues)
{
	Values = NewValues;
	GetCharacterMovement()->MaxWalkSpeed = Values.SpeedMetersPerSecond * CentimetersPerMeter;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, Values.TurnDegreesPerSecond, 0.0f);
}

void AEIEnemy::ShowHeldPart()
{
	if (Values.ShieldDamageFactor < 1.0f)
	{
		Held->SetRelativeLocation(ShieldPlace);
		Held->SetRelativeScale3D(ShieldSize);
		EIVisuals::SetColor(Held, ShieldColor);
		Held->SetVisibility(true);
	}
	else if (Values.ThrowSpeedMetersPerSecond > 0.0f)
	{
		Held->SetRelativeLocation(HeldBonePlace);
		Held->SetRelativeScale3D(HeldBoneSize);
		EIVisuals::SetColor(Held, BoneColor);
		Held->SetVisibility(true);
	}
}

void AEIEnemy::ApplyStrength(float Factor)
{
	Values.Damage *= Factor;
	Health->Reset(Values.Life * Factor);
}

void AEIEnemy::ApplySlow(float Share, float Seconds)
{
	SlowRemaining = Seconds;
	bSlowed = true;
	GetCharacterMovement()->MaxWalkSpeed = Values.SpeedMetersPerSecond * CentimetersPerMeter * (1.0f - Share);
}

void AEIEnemy::ApplyBurn(float PerSecond, float Seconds)
{
	if (!bBurning)
	{
		BurnTickRemaining = BurnTickSeconds;
	}
	BurnPerSecond = PerSecond;
	BurnRemaining = Seconds;
	bBurning = true;
}

EEIReaction AEIEnemy::MarkElement(EEIElement Element, float WindowSeconds)
{
	EEIReaction Reaction = EEIReaction::None;
	if (ElementMarkRemaining > 0.0f)
	{
		const auto Between = [Element, this](EEIElement A, EEIElement B) { return (Element == A && ElementMark == B) || (Element == B && ElementMark == A); };
		Reaction = Between(EEIElement::Fire, EEIElement::Ice) ? EEIReaction::Steam : (Between(EEIElement::Fire, EEIElement::Lightning) ? EEIReaction::Sparks : EEIReaction::None);
	}
	if (Reaction != EEIReaction::None)
	{
		ElementMarkRemaining = 0.0f;
		return Reaction;
	}
	ElementMark = Element;
	ElementMarkRemaining = WindowSeconds;
	return EEIReaction::None;
}

void AEIEnemy::Freeze(float Seconds)
{
	if (IsDead())
	{
		return;
	}
	if (State == EState::Charging)
	{
		EndCharge();
	}
	StopWalking();
	State = EState::Frozen;
	StateTime = 0.0f;
	FrozenSeconds = Seconds;
	bFrozen = true;
	bStaggering = false;
	bChargeMarked = false;
	OnRep_ChargeMarked();
	bTelegraphing = false;
	OnRep_Telegraphing();
}

void AEIEnemy::Unfreeze()
{
	if (bFrozen)
	{
		bFrozen = false;
		State = EState::Chase;
		StateTime = 0.0f;
	}
}

bool AEIEnemy::CanStartCharge(const AActor* Target) const
{
	if (Values.ChargeSpeedMetersPerSecond <= 0.0f || GetWorld()->GetTimeSeconds() < NextChargeAt)
	{
		return false;
	}
	const double Distance = FVector::Dist2D(Target->GetActorLocation(), GetActorLocation());
	if (Distance < Values.ChargeFromMeters * CentimetersPerMeter || Distance > Values.ChargeToMeters * CentimetersPerMeter)
	{
		return false;
	}
	FCollisionQueryParams Query(SCENE_QUERY_STAT(EIEnemyCharge), false, this);
	return !GetWorld()->LineTraceTestByObjectType(GetPawnViewLocation(), Target->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Query);
}

void AEIEnemy::StartChargeWindup(const AActor* Target)
{
	StopWalking();
	ChargeDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	SetActorRotation(ChargeDirection.Rotation());
	State = EState::ChargeWindup;
	StateTime = 0.0f;
	bChargeMarked = true;
	OnRep_ChargeMarked();
	bTelegraphing = true;
	OnRep_Telegraphing();
}

void AEIEnemy::UpdateCharge(float DeltaSeconds)
{
	AddMovementInput(ChargeDirection);
	ChargeTravelledCm += static_cast<float>(GetVelocity().Size2D()) * DeltaSeconds;
	const double HitZone = Values.HitZoneMeters * CentimetersPerMeter;
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && !ChargeHit.Contains(*It) && FVector::Dist(It->GetActorLocation(), GetActorLocation()) <= HitZone)
		{
			ChargeHit.Add(*It);
			UGameplayStatics::ApplyDamage(*It, Values.ChargeDamage, GetController(), this, nullptr);
			if (It->CanBeDamaged())
			{
				It->LaunchCharacter(ChargeDirection * ChargeKnockback.X + FVector(0.0f, 0.0f, ChargeKnockback.Z), true, true);
			}
		}
	}
	const bool bBlocked = StateTime > ChargeBlockedAfterSeconds && GetVelocity().Size2D() < ChargeBlockedBelowCm;
	if (bBlocked || ChargeTravelledCm >= Values.ChargeDistanceMeters * CentimetersPerMeter)
	{
		EndCharge();
	}
}

void AEIEnemy::EndCharge()
{
	GetCharacterMovement()->MaxWalkSpeed = Values.SpeedMetersPerSecond * CentimetersPerMeter;
	NextChargeAt = GetWorld()->GetTimeSeconds() + Values.ChargePauseSeconds;
	State = EState::Recover;
	StateTime = 0.0f;
}

void AEIEnemy::MakeLeader(float LifeFactor, float DamageFactor, float ScaleFactor)
{
	Values.Damage *= DamageFactor;
	Health->Reset(Health->GetMaxHealth() * LifeFactor);
	LeaderScale = ScaleFactor;
	OnRep_LeaderScale();
}

void AEIEnemy::OnRep_LeaderScale()
{
	BaseColor = LeaderColor;
	SetActorScale3D(FVector(Values.Scale * LeaderScale));
}

void AEIEnemy::OnRep_ChargeMarked()
{
	ChargeMark->SetVisibility(bChargeMarked);
}

void AEIEnemy::UpdateStatus(float DeltaSeconds)
{
	ElementMarkRemaining = FMath::Max(0.0f, ElementMarkRemaining - DeltaSeconds);
	if (bSlowed)
	{
		SlowRemaining -= DeltaSeconds;
		if (SlowRemaining <= 0.0f)
		{
			bSlowed = false;
			GetCharacterMovement()->MaxWalkSpeed = Values.SpeedMetersPerSecond * CentimetersPerMeter;
		}
	}
	if (bBurning)
	{
		BurnRemaining -= DeltaSeconds;
		BurnTickRemaining -= DeltaSeconds;
		if (BurnTickRemaining <= 0.0f)
		{
			BurnTickRemaining += BurnTickSeconds;
			Health->ApplyDamage(BurnPerSecond * BurnTickSeconds);
		}
		bBurning = BurnRemaining > 0.0f;
	}
}

void AEIEnemy::StandWatch(const FVector& NewHome)
{
	Home = NewHome;
	bHasHome = true;
	State = EState::Watch;
	StateTime = 0.0f;
	// Not all at the same moment.
	NoticeCheckRemaining = FMath::FRandRange(0.0f, NoticeCheckSeconds);
	IdlePhase = FMath::FRandRange(0.0f, UE_TWO_PI);
	SetActorRotation(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f));
}

bool AEIEnemy::NoticesHero(float DeltaSeconds)
{
	NoticeCheckRemaining -= DeltaSeconds;
	if (NoticeCheckRemaining > 0.0f)
	{
		return false;
	}
	NoticeCheckRemaining = NoticeCheckSeconds;
	const double Sight = Values.SightMeters * CentimetersPerMeter;
	const FVector Eye = GetPawnViewLocation();
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || FVector::Dist(It->GetActorLocation(), GetActorLocation()) > Sight)
		{
			continue;
		}
		FCollisionQueryParams Query(SCENE_QUERY_STAT(EIEnemySight), false, this);
		if (!GetWorld()->LineTraceTestByObjectType(Eye, It->GetPawnViewLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Query))
		{
			return true;
		}
	}
	return false;
}

void AEIEnemy::WakeUp(bool bTellNeighbours)
{
	AwakeUntil = GetWorld()->GetTimeSeconds() + Values.StayAwakeSeconds;
	if (!IsWatching())
	{
		return;
	}
	StopWalking();
	State = EState::Chase;
	StateTime = 0.0f;
	MulticastNoticed(bTellNeighbours);
	if (!bTellNeighbours)
	{
		return;
	}
	const double Alarm = Values.AlarmMeters * CentimetersPerMeter;
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		if (*It != this && FVector::Dist(It->GetActorLocation(), GetActorLocation()) <= Alarm)
		{
			It->WakeUp(false);
		}
	}
}

void AEIEnemy::WalkHome(float DeltaSeconds)
{
	AAIController* AI = GetController<AAIController>();
	if (AI == nullptr || AI->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}
	PathRetryRemaining -= DeltaSeconds;
	if (PathRetryRemaining <= 0.0f && StandsOnPaths())
	{
		if (AI->MoveToLocation(Home, PathGoalReachCm) != EPathFollowingRequestResult::Failed)
		{
			return;
		}
		PathRetryRemaining = PathRetrySeconds;
	}
	AddMovementInput((Home - GetActorLocation()).GetSafeNormal2D());
}

void AEIEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIEnemy, bTelegraphing);
	DOREPLIFETIME(AEIEnemy, bStaggering);
	DOREPLIFETIME(AEIEnemy, Kind);
	DOREPLIFETIME(AEIEnemy, bSlowed);
	DOREPLIFETIME(AEIEnemy, bBurning);
	DOREPLIFETIME(AEIEnemy, bFrozen);
	DOREPLIFETIME(AEIEnemy, bChargeMarked);
	DOREPLIFETIME(AEIEnemy, LeaderScale);
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
	UpdateStatus(DeltaSeconds);
	if (IsDead())
	{
		return;
	}
	StateTime += DeltaSeconds;
	AActor* Target = FindNearestHero();
	switch (State)
	{
	case EState::Chase:
		if (Target == nullptr)
		{
			return;
		}
		if (bHasHome && GetWorld()->GetTimeSeconds() >= AwakeUntil && FVector::Dist(Target->GetActorLocation(), GetActorLocation()) > Values.GiveUpMeters * CentimetersPerMeter)
		{
			StopWalking();
			State = EState::GoHome;
			StateTime = 0.0f;
			return;
		}
		if (CanStartCharge(Target))
		{
			StartChargeWindup(Target);
			return;
		}
		if (CanAttack(Target))
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
		// A throw goes where the thrower looks. A shield bearer does not turn now, which is the moment to get around him.
		if (Values.ThrowSpeedMetersPerSecond > 0.0f && Target != nullptr)
		{
			const FRotator Facing = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();
			SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Facing, DeltaSeconds, Values.TurnDegreesPerSecond));
		}
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

	case EState::Frozen:
		if (StateTime >= FrozenSeconds)
		{
			Unfreeze();
		}
		return;

	case EState::ChargeWindup:
		if (StateTime >= Values.ChargeWindupSeconds)
		{
			bChargeMarked = false;
			OnRep_ChargeMarked();
			bTelegraphing = false;
			OnRep_Telegraphing();
			MulticastStrike();
			State = EState::Charging;
			StateTime = 0.0f;
			ChargeTravelledCm = 0.0f;
			ChargeHit.Reset();
			GetCharacterMovement()->MaxWalkSpeed = Values.ChargeSpeedMetersPerSecond * CentimetersPerMeter;
		}
		return;

	case EState::Charging:
		UpdateCharge(DeltaSeconds);
		return;

	case EState::Watch:
		if (NoticesHero(DeltaSeconds))
		{
			WakeUp(true);
		}
		return;

	case EState::GoHome:
		if (NoticesHero(DeltaSeconds))
		{
			WakeUp(true);
		}
		else if (FVector::Dist2D(GetActorLocation(), Home) <= HomeReachCm)
		{
			StopWalking();
			State = EState::Watch;
		}
		else
		{
			WalkHome(DeltaSeconds);
		}
		return;
	}
}

void AEIEnemy::Stagger(float Seconds)
{
	if (!HasAuthority() || IsDead() || bFrozen)
	{
		return;
	}
	if (State == EState::Charging)
	{
		EndCharge();
	}
	bChargeMarked = false;
	OnRep_ChargeMarked();
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

bool AEIEnemy::CanAttack(const AActor* Target) const
{
	if (FVector::Dist(Target->GetActorLocation(), GetActorLocation()) > Values.AttackFromMeters * CentimetersPerMeter)
	{
		return false;
	}
	if (Values.ThrowSpeedMetersPerSecond <= 0.0f)
	{
		return true;
	}
	// A throw needs a free line.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(EIEnemyThrow), false, this);
	return !GetWorld()->LineTraceTestByObjectType(GetPawnViewLocation(), Target->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Query);
}

void AEIEnemy::ThrowBone()
{
	const AActor* Target = FindNearestHero();
	if (Target == nullptr)
	{
		return;
	}
	const FVector Start = GetPawnViewLocation() + GetActorForwardVector() * ThrowFromForwardCm;
	const FTransform Spawn((Target->GetActorLocation() - Start).Rotation(), Start);
	AEIThrownBone* Bone = GetWorld()->SpawnActorDeferred<AEIThrownBone>(AEIThrownBone::StaticClass(), Spawn, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Bone != nullptr)
	{
		Bone->Launch(Values.Damage, Values.ThrowSpeedMetersPerSecond * CentimetersPerMeter);
		Bone->FinishSpawning(Spawn);
	}
}

float AEIEnemy::ReduceByShield(float Amount, const AActor* DamageCauser)
{
	const bool bBroken = bNextHitBreaksShield;
	bNextHitBreaksShield = false;
	if (Values.ShieldDamageFactor >= 1.0f || bBroken || bStaggering || bFrozen || DamageCauser == nullptr || Amount <= 0.0f)
	{
		return Amount;
	}
	const FVector ToCauser = (DamageCauser->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(Values.ShieldAngleDegrees / 2.0f));
	if (FVector::DotProduct(GetActorForwardVector(), ToCauser) < MinDot)
	{
		return Amount;
	}
	MulticastShieldBlocked();
	return Amount * Values.ShieldDamageFactor;
}

void AEIEnemy::Strike()
{
	MulticastStrike();
	if (Values.ThrowSpeedMetersPerSecond > 0.0f)
	{
		ThrowBone();
		return;
	}
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
	const float Incoming = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	const float Applied = Health->ApplyDamage(ReduceByShield(Incoming, DamageCauser));
	if (AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(DamageCauser); Hero != nullptr && Applied > 0.0f)
	{
		Hero->NoteCombat();
	}
	if (Applied > 0.0f)
	{
		WakeUp(true);
	}
	if (Applied > 0.0f && !IsDead())
	{
		MulticastHurt();
	}
	return Applied;
}

void AEIEnemy::HandleDied()
{
	if (const AEIGameMode* GameMode = GetWorld()->GetAuthGameMode<AEIGameMode>())
	{
		GameMode->GetRun()->NoteEnemyDied(*this);
	}
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

void AEIEnemy::MulticastNoticed_Implementation(bool bCallOut)
{
	NoticeBlend = 1.0f;
	if (bCallOut)
	{
		EIVisuals::PlaySoundAt(this, TEXT("GegnerAusholen"), GetActorLocation());
	}
}

void AEIEnemy::MulticastShieldBlocked_Implementation()
{
	EIVisuals::PlaySoundAt(this, TEXT("Blocken"), GetActorLocation());
}

void AEIEnemy::MulticastHurt_Implementation()
{
	HurtFlashRemaining = HurtFlashSeconds;
	RecoilBlend = 1.0f;
}

void AEIEnemy::MulticastStrike_Implementation()
{
	TelegraphBlend = 0.0f;
	StrikeBlend = 1.0f;
}

void AEIEnemy::MulticastDie_Implementation()
{
	EIVisuals::PlaySoundAt(this, TEXT("GegnerTod"), GetActorLocation());
	Body->SetVisibility(false);
	Head->SetVisibility(false);
	Held->SetVisibility(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	const FTransform Spawn(GetActorLocation());
	if (AEIBoneDebris* Debris = GetWorld()->SpawnActorDeferred<AEIBoneDebris>(AEIBoneDebris::StaticClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Debris->Setup(BaseColor, Values.Scale);
		Debris->FinishSpawning(Spawn);
	}
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
	const FLinearColor StatusColor = bFrozen ? FrozenColor : (bBurning ? BurningColor : (bSlowed ? SlowedColor : BaseColor));
	const FLinearColor Calm = FMath::Lerp(FMath::Lerp(StatusColor, WarningColor, TelegraphBlend), StaggerColor, StaggerBlend);
	const FLinearColor Color = FMath::Lerp(Calm, HurtColor, Hurt);
	const float Glow = FMath::Max3(TelegraphBlend * WarningGlow, Hurt * HurtGlow, bFrozen ? FrozenGlow : (bBurning ? BurningGlow : 0.0f));
	for (UMaterialInstanceDynamic* Material : { BodyMaterial.Get(), HeadMaterial.Get() })
	{
		Material->SetVectorParameterValue(TEXT("Farbe"), Color);
		Material->SetScalarParameterValue(TEXT("Leuchten"), Glow);
	}
	UpdatePose(DeltaSeconds);
}

void AEIEnemy::UpdatePose(float DeltaSeconds)
{
	// A body of simple shapes that still walks, winds up, strikes and flinches, all from what it does anyway.
	const float Speed = static_cast<float>(GetVelocity().Size2D());
	WalkBlend = FMath::FInterpTo(WalkBlend, FMath::Min(Speed / WalkFullFromCm, 1.0f), DeltaSeconds, WalkBlendSpeed);
	WalkPhase = FMath::Fmod(WalkPhase + DeltaSeconds * Speed / StrideCm * UE_PI, UE_TWO_PI);
	IdlePhase = FMath::Fmod(IdlePhase + DeltaSeconds * IdlePhaseSpeed, UE_TWO_PI);
	StrikeBlend = FMath::FInterpTo(StrikeBlend, 0.0f, DeltaSeconds, StrikeRecoverSpeed);
	RecoilBlend = FMath::FInterpTo(RecoilBlend, 0.0f, DeltaSeconds, RecoilRecoverSpeed);
	NoticeBlend = FMath::FInterpTo(NoticeBlend, 0.0f, DeltaSeconds, NoticeRecoverSpeed);

	const float Step = FMath::Sin(WalkPhase);
	const float Bob = FMath::Abs(Step) * WalkBobCm * WalkBlend;
	const float Stagger = StaggerBlend * (StaggerTiltDegrees + FMath::Sin(GetWorld()->GetTimeSeconds() * StaggerWobbleSpeed) * StaggerWobbleDegrees);
	// Positive leans back.
	const float Lean = Stagger + TelegraphBlend * WindupLeanDegrees + RecoilBlend * RecoilLeanDegrees - WalkBlend * WalkLeanDegrees - StrikeBlend * StrikeLeanDegrees;
	// Whoever is frozen does not even sway.
	const float Waddle = bFrozen ? 0.0f : Step * WalkWaddleDegrees * WalkBlend + FMath::Sin(IdlePhase) * IdleSwayDegrees * (1.0f - WalkBlend);
	const float Forward = StrikeBlend * StrikeForwardCm;

	Body->SetRelativeRotation(FRotator(Lean, 0.0f, Waddle));
	Body->SetRelativeLocation(FVector(Forward, 0.0f, BodyHeight + Bob));

	// It rears up while it winds up, readable even without the color. The head stays on top of the leaning body.
	const float HeadLength = HeadAboveBody + TelegraphBlend * TelegraphStretch * 100.0f + NoticeBlend * NoticeHeadCm;
	const float LeanRadians = FMath::DegreesToRadians(Lean);
	const float WaddleRadians = FMath::DegreesToRadians(Waddle);
	Head->SetRelativeLocation(FVector(Forward - FMath::Sin(LeanRadians) * HeadLength, FMath::Sin(WaddleRadians) * HeadLength, BodyHeight + Bob + FMath::Cos(LeanRadians) * HeadLength));

	if (Values.ShieldDamageFactor < 1.0f)
	{
		Held->SetRelativeLocation(ShieldPlace + ShieldWindupOffset * TelegraphBlend + ShieldBashOffset * StrikeBlend + FVector(0.0f, 0.0f, Bob));
	}
	else if (Values.ThrowSpeedMetersPerSecond > 0.0f)
	{
		Held->SetRelativeLocation(HeldBonePlace + ThrowWindupOffset * TelegraphBlend + ThrowFlingOffset * StrikeBlend + FVector(0.0f, 0.0f, Bob));
		// The bone has just left the hand, and the next one is not there yet.
		Held->SetVisibility(!IsDead() && StrikeBlend < 0.15f);
	}
}

AEIBoneDebris::AEIBoneDebris()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = DebrisLifeSeconds;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AEIBoneDebris::Setup(const FLinearColor& InColor, float InScale)
{
	Color = InColor;
	Size = InScale;
}

void AEIBoneDebris::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Cube = EIVisuals::GetCube();
	const int32 Pieces = FMath::RoundToInt(DebrisPieces * Size * Size);
	for (int32 Index = 0; Index < Pieces; ++Index)
	{
		UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
		Piece->SetStaticMesh(Cube);
		Piece->SetupAttachment(GetRootComponent());
		Piece->SetWorldScale3D(FVector(FMath::FRandRange(0.08f, 0.14f), FMath::FRandRange(0.08f, 0.14f), FMath::FRandRange(0.2f, 0.5f)) * Size);
		Piece->SetRelativeLocation(FVector(FMath::FRandRange(-20.0f, 20.0f), FMath::FRandRange(-20.0f, 20.0f), FMath::FRandRange(-60.0f, 60.0f)) * Size);
		Piece->SetCollisionProfileName(TEXT("PhysicsActor"));
		Piece->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Piece->RegisterComponent();
		EIVisuals::SetColor(Piece, Color);
		Piece->SetSimulatePhysics(true);
		Piece->SetPhysicsLinearVelocity(FMath::VRand() * DebrisSpeedCm + FVector(0.0f, 0.0f, DebrisSpeedCm * 0.5f));
		Piece->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * 720.0f);
	}
}

AEIThrownBone::AEIThrownBone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true);
	InitialLifeSpan = ThrownBoneLifeSeconds;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(ThrownBoneRadiusCm);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	SetRootComponent(Collision);

	Bone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bone"));
	Bone->SetupAttachment(Collision);
	Bone->SetStaticMesh(Cube.Object);
	Bone->SetRelativeScale3D(ThrownBoneSize);
	Bone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Flight = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Flight"));
	Flight->ProjectileGravityScale = 0.0f;
	Flight->bRotationFollowsVelocity = true;
	Flight->bShouldBounce = false;
}

void AEIThrownBone::Launch(float InDamage, float SpeedCm)
{
	Damage = InDamage;
	Flight->InitialSpeed = SpeedCm;
	Flight->MaxSpeed = SpeedCm;
}

void AEIThrownBone::BeginPlay()
{
	Super::BeginPlay();

	EIVisuals::SetColor(Bone, BoneColor, ThrownBoneGlow);
	// Enemies do not hit each other.
	for (TActorIterator<AEIEnemy> It(GetWorld()); It; ++It)
	{
		Collision->IgnoreActorWhenMoving(*It, true);
	}
	if (HasAuthority())
	{
		Flight->OnProjectileStop.AddDynamic(this, &AEIThrownBone::HandleStop);
	}
}

void AEIThrownBone::HandleStop(const FHitResult& Hit)
{
	if (AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(Hit.GetActor()))
	{
		UGameplayStatics::ApplyDamage(Hero, Damage, GetInstigatorController(), this, nullptr);
	}
	Destroy();
}
