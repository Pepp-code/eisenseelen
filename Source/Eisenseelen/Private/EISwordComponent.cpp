#include "EISwordComponent.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EIGearComponent.h"
#include "EIRuneComponent.h"
#include "EIPoses.h"
#include "EIViewModelComponent.h"
#include "EIVisuals.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float DegreesPerSweepStep = 10.0f;
	constexpr float SlowestSwingFactor = 0.1f;
	constexpr float BlockLiftPerSecond = 10.0f;
	constexpr float BlockFlashSeconds = 0.5f;

	FName CurveName(EEISwingKind Kind)
	{
		switch (Kind)
		{
		case EEISwingKind::Light1: return TEXT("leicht_1");
		case EEISwingKind::Light2: return TEXT("leicht_2");
		default: return TEXT("schwer");
		}
	}

	bool ReadSwing(const FJsonObject& Sword, const TCHAR* Name, FEISwingValues& Out, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Swing = nullptr;
		if (!Sword.TryGetObjectField(Name, Swing))
		{
			OutError = FString::Printf(TEXT("Der Abschnitt %s fehlt."), Name);
			return false;
		}
		return FEIDataFile::ReadNumber(**Swing, TEXT("reichweite_m"), 0.5, 20.0, Out.ReachMeters, OutError)
			&& FEIDataFile::ReadNumber(**Swing, TEXT("klingenbreite_m"), 0.01, 5.0, Out.BladeWidthMeters, OutError)
			&& FEIDataFile::ReadNumber(**Swing, TEXT("treffer_von_0_bis_1"), 0.0, 1.0, Out.HitFrom, OutError)
			&& FEIDataFile::ReadNumber(**Swing, TEXT("treffer_bis_0_bis_1"), 0.0, 1.0, Out.HitTo, OutError)
			&& FEIDataFile::ReadNumber(**Swing, TEXT("ziele_max"), 1.0, 50.0, Out.MaxTargets, OutError)
			&& FEIDataFile::ReadNumber(**Swing, TEXT("abzug_je_ziel_0_bis_1"), 0.0, 1.0, Out.TargetFalloff, OutError);
	}

	bool ReadBlock(const FJsonObject& Sword, FEIBlockValues& Out, FString& OutError)
	{
		const TSharedPtr<FJsonObject>* Block = nullptr;
		if (!Sword.TryGetObjectField(TEXT("blocken"), Block))
		{
			OutError = TEXT("Der Abschnitt blocken fehlt.");
			return false;
		}
		return FEIDataFile::ReadNumber(**Block, TEXT("schaden_abzug_0_bis_1"), 0.0, 1.0, Out.DamageReduction, OutError)
			&& FEIDataFile::ReadNumber(**Block, TEXT("winkel_grad"), 0.0, 360.0, Out.AngleDegrees, OutError)
			&& FEIDataFile::ReadNumber(**Block, TEXT("parade_fenster_s"), 0.0, 5.0, Out.ParryWindowSeconds, OutError)
			&& FEIDataFile::ReadNumber(**Block, TEXT("parade_schaden_abzug_0_bis_1"), 0.0, 1.0, Out.ParryDamageReduction, OutError)
			&& FEIDataFile::ReadNumber(**Block, TEXT("taumeln_s"), 0.0, 30.0, Out.StaggerSeconds, OutError);
	}
}

FEISwordValues::FEISwordValues()
{
	Heavy.DurationSeconds = 0.55f;
	Heavy.Damage = 110.0f;
	Heavy.ReachMeters = 3.2f;
	Heavy.BladeWidthMeters = 0.6f;
	Heavy.HitFrom = 0.25f;
	Heavy.HitTo = 0.6f;
}

bool FEISwordValues::LoadFromProjectFile(FEISwordValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("waffen.json"), OutError);
	const TSharedPtr<FJsonObject>* Sword = nullptr;
	if (!Root.IsValid())
	{
		return false;
	}
	if (!Root->TryGetObjectField(TEXT("schwert"), Sword))
	{
		OutError = TEXT("Der Abschnitt schwert fehlt.");
		return false;
	}
	FEISwordValues Loaded;
	if (!ReadSwing(**Sword, TEXT("leichter_hieb"), Loaded.Light, OutError)
		|| !ReadSwing(**Sword, TEXT("schwerer_hieb"), Loaded.Heavy, OutError)
		|| !ReadBlock(**Sword, Loaded.Block, OutError)
		|| !FEIDataFile::ReadNumber(**Sword, TEXT("ketten_fenster_s"), 0.0, 5.0, Loaded.ChainWindowSeconds, OutError)
		|| !FEIDataFile::ReadNumber(**Sword, TEXT("treff_stopp_s"), 0.0, 0.5, Loaded.HitStopSeconds, OutError)
		|| !FEIDataFile::ReadNumber(**Sword, TEXT("kamera_stoss_grad"), 0.0, 30.0, Loaded.CameraKickDegrees, OutError)
		|| !FEIDataFile::ReadNumber(**Sword, TEXT("schwerer_kamera_stoss_grad"), 0.0, 30.0, Loaded.HeavyCameraKickDegrees, OutError))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}

UEISwordComponent::UEISwordComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UEISwordComponent::LoadValues()
{
	FString Error;
	if (!FEISwordValues::LoadFromProjectFile(BaseValues, Error))
	{
		FEIDataFile::ReportError(TEXT("waffen.json"), Error);
	}
	Values = BaseValues;
}

void UEISwordComponent::ApplyGear(const UEIGearComponent& Gear)
{
	Values = BaseValues;
	const FEIWeaponForm* Form = Gear.GetWeaponForm();
	if (Form == nullptr || Form->bStaff)
	{
		return;
	}
	const float SwingFactor = FMath::Max(SlowestSwingFactor, 1.0f + Gear.GetStat(EEIStat::AttackSpeed));
	Values.Light.Damage = Form->LightDamage;
	Values.Light.DurationSeconds = Form->LightSeconds / SwingFactor;
	Values.Light.ReachMeters *= Form->LengthFactor;
	Values.Heavy.Damage = Form->HeavyDamage;
	Values.Heavy.DurationSeconds = Form->HeavySeconds / SwingFactor;
	Values.Heavy.ReachMeters *= Form->LengthFactor;
	Values.ChargeSeconds = Form->ChargeSeconds / FMath::Max(SlowestSwingFactor, 1.0f + Gear.GetStat(EEIStat::ChargeSpeed));
	Values.HeavyStaggerSeconds = Form->StaggerSeconds;
}

AEIHeroCharacter* UEISwordComponent::GetHero() const
{
	return CastChecked<AEIHeroCharacter>(GetOwner());
}

float UEISwordComponent::GetChargeFraction() const
{
	if (!bHolding)
	{
		return 0.0f;
	}
	return FMath::Clamp(static_cast<float>(GetWorld()->GetTimeSeconds() - HoldStartedAt) / Values.ChargeSeconds, 0.0f, 1.0f);
}

void UEISwordComponent::PressPrimary()
{
	bHolding = true;
	HoldStartedAt = GetWorld()->GetTimeSeconds();
}

void UEISwordComponent::ReleasePrimary()
{
	if (!bHolding)
	{
		return;
	}
	const bool bCharged = GetChargeFraction() >= 1.0f;
	bHolding = false;
	if (AEIHeroCharacter* Hero = GetHero(); Hero->GetViewModel() != nullptr)
	{
		Hero->GetViewModel()->ClearHold();
	}
	// A press during a swing plays right after it, so a fast chain never swallows a press. One waiting swing is enough.
	if (bHasQueuedSwing)
	{
		return;
	}
	const EEISwingKind Kind = bCharged ? EEISwingKind::Heavy : NextLightSwing();
	if (bSwinging)
	{
		bHasQueuedSwing = true;
		QueuedSwing = Kind;
	}
	else
	{
		StartOwnerSwing(Kind);
	}
}

void UEISwordComponent::PressBlock()
{
	bBlockKeyHeld = true;
	bHolding = false;
	bHasQueuedSwing = false;
	AEIHeroCharacter* Hero = GetHero();
	if (Hero->GetViewModel() != nullptr)
	{
		Hero->GetViewModel()->ClearHold();
	}
	if (bSwinging)
	{
		CancelSwing();
		if (!GetOwner()->HasAuthority())
		{
			ServerCancelSwing();
		}
	}
}

void UEISwordComponent::ServerCancelSwing_Implementation()
{
	CancelSwing();
}

void UEISwordComponent::CancelSwing()
{
	bSwinging = false;
	bServerHasQueuedSwing = false;
	LastSwingEndedAt = GetWorld()->GetTimeSeconds();
	if (AEIHeroCharacter* Hero = GetHero(); Hero->GetViewModel() != nullptr)
	{
		Hero->GetViewModel()->CancelPose();
	}
}

void UEISwordComponent::ReleaseBlock()
{
	bBlockKeyHeld = false;
}

float UEISwordComponent::BlockDamage(float Damage, AActor* Causer)
{
	const AEIHeroCharacter* Hero = GetHero();
	const UEIHeroMovementComponent* Movement = Hero->GetHeroMovement();
	if (Damage <= 0.0f || Causer == nullptr || bSwinging || !Movement->IsBlocking() || Hero->GetHeroClass() != EEIHeroClass::Bladewarden)
	{
		return Damage;
	}
	const FVector ToCauser = (Causer->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D();
	const double MinDot = FMath::Cos(FMath::DegreesToRadians(Values.Block.AngleDegrees / 2.0f));
	if (FVector::DotProduct(Hero->GetActorForwardVector().GetSafeNormal2D(), ToCauser) < MinDot)
	{
		return Damage;
	}
	const bool bParry = Movement->GetBlockHeldSeconds() <= Values.Block.ParryWindowSeconds;
	if (AEIEnemy* Enemy = Cast<AEIEnemy>(Causer); Enemy != nullptr && bParry)
	{
		Enemy->Stagger(Values.Block.StaggerSeconds);
	}
	MulticastBlockFeedback(bParry);
	const float Blocked = bParry ? Values.Block.ParryDamageReduction : Values.Block.DamageReduction + Hero->GetGear()->GetStat(EEIStat::BlockStrength);
	return Damage * (1.0f - FMath::Min(Blocked, 1.0f));
}

void UEISwordComponent::MulticastBlockFeedback_Implementation(bool bParry)
{
	const TCHAR* Sound = bParry ? TEXT("Parade") : TEXT("Blocken");
	AEIHeroCharacter* Hero = GetHero();
	if (!Hero->IsLocallyControlled())
	{
		EIVisuals::PlaySoundAt(this, Sound, Hero->GetActorLocation());
		return;
	}
	EIVisuals::PlaySound2D(this, Sound);
	Hero->AddCameraKick(bParry ? Values.HeavyCameraKickDegrees : Values.CameraKickDegrees);
	BlockFlashRemaining = BlockFlashSeconds;
	bFlashIsParry = bParry;
}

void UEISwordComponent::UpdateBlockAsOwner(float DeltaTime)
{
	AEIHeroCharacter* Hero = GetHero();
	const bool bBlocking = bBlockKeyHeld && !bSwinging && !bHasQueuedSwing && !bHolding;
	// Set every frame, because replaying a corrected move overwrites the wish with an older one.
	Hero->GetHeroMovement()->SetWantsToBlock(bBlocking);

	const float PreviousLift = BlockLift;
	BlockLift = FMath::FInterpConstantTo(BlockLift, bBlocking ? 1.0f : 0.0f, DeltaTime, BlockLiftPerSecond);
	if (Hero->GetViewModel() == nullptr)
	{
		return;
	}
	if (BlockLift > 0.0f)
	{
		Hero->GetViewModel()->HoldPose(TEXT("blocken"), BlockLift);
	}
	else if (PreviousLift > 0.0f)
	{
		Hero->GetViewModel()->ClearHold();
	}
}

void UEISwordComponent::Deactivate()
{
	bHolding = false;
	bBlockKeyHeld = false;
	BlockLift = 0.0f;
	if (GetHero()->IsLocallyControlled())
	{
		GetHero()->GetHeroMovement()->SetWantsToBlock(false);
	}
	bHasQueuedSwing = false;
	bServerHasQueuedSwing = false;
	bSwinging = false;
	if (GetHero()->GetViewModel() != nullptr)
	{
		GetHero()->GetViewModel()->ClearHold();
	}
}

EEISwingKind UEISwordComponent::NextLightSwing()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (!bSwinging && !bHasQueuedSwing && Now - LastSwingEndedAt > Values.ChainWindowSeconds)
	{
		NextChainIndex = 0;
	}
	const int32 Index = NextChainIndex;
	NextChainIndex = (NextChainIndex + 1) % 2;
	return Index == 0 ? EEISwingKind::Light1 : EEISwingKind::Light2;
}

void UEISwordComponent::StartOwnerSwing(EEISwingKind Kind)
{
	BeginSwing(Kind);
	if (!GetOwner()->HasAuthority())
	{
		ServerBeginSwing(Kind);
	}
}

void UEISwordComponent::ServerBeginSwing_Implementation(EEISwingKind Kind)
{
	if (bSwinging)
	{
		// The owner's clock runs a little ahead of the server's, so the next swing can arrive just before the end.
		bServerHasQueuedSwing = true;
		ServerQueuedSwing = Kind;
		return;
	}
	BeginSwing(Kind);
}

void UEISwordComponent::BeginSwing(EEISwingKind Kind)
{
	const FEISwingValues& Swing = Kind == EEISwingKind::Heavy ? Values.Heavy : Values.Light;
	bSwinging = true;
	SwingKind = Kind;
	SwingElapsed = 0.0f;
	HitThisSwing.Reset();

	AEIHeroCharacter* Hero = GetHero();
	if (Hero->IsLocallyControlled())
	{
		EIVisuals::PlaySound2D(this, Kind == EEISwingKind::Heavy ? TEXT("HiebSchwer") : TEXT("Hieb"));
		if (Hero->GetViewModel() != nullptr)
		{
			Hero->GetViewModel()->PlayPose(CurveName(Kind), Swing.DurationSeconds, Swing.HitFrom, Swing.HitTo);
		}
	}
}

void UEISwordComponent::EndSwing()
{
	bSwinging = false;
	LastSwingEndedAt = GetWorld()->GetTimeSeconds();
	if (GetHero()->IsLocallyControlled() && bHasQueuedSwing)
	{
		bHasQueuedSwing = false;
		StartOwnerSwing(QueuedSwing);
	}
	else if (bServerHasQueuedSwing)
	{
		bServerHasQueuedSwing = false;
		BeginSwing(ServerQueuedSwing);
	}
}

void UEISwordComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AEIHeroCharacter* Hero = GetHero();
	BlockFlashRemaining = FMath::Max(0.0f, BlockFlashRemaining - DeltaTime);
	if (Hero->IsLocallyControlled())
	{
		UpdateBlockAsOwner(DeltaTime);
	}
	if (bHolding && Hero->GetViewModel() != nullptr)
	{
		// A short tap shows nothing. Only a real hold lifts the sword.
		constexpr float ShowHoldFromFraction = 0.25f;
		const float Fraction = GetChargeFraction();
		if (Fraction > ShowHoldFromFraction && !bSwinging)
		{
			Hero->GetViewModel()->HoldPose(TEXT("schwer_laden"), (Fraction - ShowHoldFromFraction) / (1.0f - ShowHoldFromFraction));
		}
	}
	if (!bSwinging)
	{
		return;
	}

	const FEISwingValues& Swing = SwingKind == EEISwingKind::Heavy ? Values.Heavy : Values.Light;
	const float PreviousT = SwingElapsed / Swing.DurationSeconds;
	SwingElapsed += DeltaTime;
	const float T = SwingElapsed / Swing.DurationSeconds;
	if (GetOwner()->HasAuthority() && T > Swing.HitFrom && PreviousT < Swing.HitTo)
	{
		SweepBlade(FMath::Max(PreviousT, Swing.HitFrom), FMath::Min(T, Swing.HitTo));
	}
	if (T >= 1.0f)
	{
		EndSwing();
	}
}

void UEISwordComponent::SweepBlade(float FromT, float ToT)
{
	AEIHeroCharacter* Hero = GetHero();
	const FEIPoseCurve* Curve = Hero->GetPoses().Find(CurveName(SwingKind));
	if (Curve == nullptr)
	{
		return;
	}
	const FEISwingValues& Swing = SwingKind == EEISwingKind::Heavy ? Values.Heavy : Values.Light;
	const auto ArcFraction = [&Swing](float T) { return (T - Swing.HitFrom) / FMath::Max(Swing.HitTo - Swing.HitFrom, KINDA_SMALL_NUMBER); };
	const float From = ArcFraction(FromT);
	const float To = ArcFraction(ToT);

	const float ArcDegrees = FMath::Max(FMath::Abs(Curve->ArcYawTo - Curve->ArcYawFrom), FMath::Abs(Curve->ArcPitchTo - Curve->ArcPitchFrom));
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(ArcDegrees * (To - From) / DegreesPerSweepStep));
	const FVector Eye = Hero->GetPawnViewLocation();
	const FRotator View = Hero->GetViewRotation();
	const float Reach = Swing.ReachMeters * CentimetersPerMeter;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Swing.BladeWidthMeters * CentimetersPerMeter);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(EISwordSweep), false, Hero);

	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Fraction = FMath::Lerp(From, To, static_cast<float>(Step) / Steps);
		const FRotator Offset(FMath::Lerp(Curve->ArcPitchFrom, Curve->ArcPitchTo, Fraction), FMath::Lerp(Curve->ArcYawFrom, Curve->ArcYawTo, Fraction), 0.0f);
		const FVector Direction = View.RotateVector(Offset.Vector());
		TArray<FHitResult> Hits;
		GetWorld()->SweepMultiByObjectType(Hits, Eye, Eye + Direction * Reach, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), Shape, Query);
		for (const FHitResult& Hit : Hits)
		{
			if (AActor* Target = Hit.GetActor())
			{
				HitActor(Target, Hit.ImpactPoint);
			}
		}
	}
}

void UEISwordComponent::HitActor(AActor* Target, const FVector& Location)
{
	const FEISwingValues& Swing = SwingKind == EEISwingKind::Heavy ? Values.Heavy : Values.Light;
	AEIHeroCharacter* Hero = GetHero();
	if (Target->IsA<AEIHeroCharacter>() || !Target->CanBeDamaged() || HitThisSwing.Contains(Target) || HitThisSwing.Num() >= FMath::RoundToInt(Swing.MaxTargets))
	{
		return;
	}
	FCollisionQueryParams WallQuery(SCENE_QUERY_STAT(EISwordWall), false, Hero);
	WallQuery.AddIgnoredActor(Target);
	if (GetWorld()->LineTraceTestByObjectType(Hero->GetPawnViewLocation(), Target->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), WallQuery))
	{
		return;
	}

	const float Damage = Swing.Damage * FMath::Max(0.0f, 1.0f - Swing.TargetFalloff * HitThisSwing.Num()) * Hero->GetGear()->GetSwordFactor();
	HitThisSwing.Add(Target);
	AEIEnemy* Enemy = SwingKind == EEISwingKind::Heavy ? Cast<AEIEnemy>(Target) : nullptr;
	if (Enemy != nullptr)
	{
		Enemy->BreakShieldWithNextHit();
		if (Enemy->IsFrozen())
		{
			Hero->GetRunes()->Shatter(Enemy);
		}
	}
	UGameplayStatics::ApplyDamage(Target, Damage, Hero->GetController(), Hero, nullptr);
	if (Enemy != nullptr)
	{
		Enemy->StaggerFromHeavyHit(Values.HeavyStaggerSeconds);
	}
	MulticastHitFeedback(Location, SwingKind == EEISwingKind::Heavy);
}

void UEISwordComponent::MulticastHitFeedback_Implementation(FVector_NetQuantize Location, bool bHeavy)
{
	const TCHAR* Sound = bHeavy ? TEXT("TrefferSchwer") : TEXT("Treffer");
	AEIHeroCharacter* Hero = GetHero();
	if (!Hero->IsLocallyControlled())
	{
		EIVisuals::PlaySoundAt(this, Sound, Location);
		return;
	}
	EIVisuals::PlaySound2D(this, Sound);
	Hero->AddCameraKick(bHeavy ? Values.HeavyCameraKickDegrees : Values.CameraKickDegrees);
	if (Hero->GetViewModel() != nullptr)
	{
		Hero->GetViewModel()->HitStop(Values.HitStopSeconds);
	}
}
