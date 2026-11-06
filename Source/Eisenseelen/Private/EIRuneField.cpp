#include "EIRuneField.h"

#include "Components/StaticMeshComponent.h"
#include "EIEffects.h"
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
	// Wind stands up as a turning column, see-through, so that the player still sees what is in it.
	constexpr float TallHeightCm = 250.0f;
	constexpr float TallStrength = 0.18f;
	constexpr float TallTurnDegreesPerSecond = 240.0f;
	constexpr float WhirlPartnerShare = 0.6f;
	// A bolt of a storm field comes down from this high above whom it strikes.
	constexpr float StrikeFromCm = 700.0f;
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
	DOREPLIFETIME(AEIRuneField, bTall);
	DOREPLIFETIME(AEIRuneField, bWhirl);
	DOREPLIFETIME(AEIRuneField, Element);
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
	PullCm = Rune.PullMetersPerSecond * CentimetersPerMeter;
	bTall = !bWall && Rune.Element == EEIElement::Wind;
}

void AEIRuneField::ConfigureWhirl(EEIElement Partner, const FEIWhirlValues& Whirl, float ReactionFactor, float StatusFactor)
{
	bWhirl = true;
	bTall = true;
	SizeCm = Whirl.RadiusMeters * CentimetersPerMeter * 2.0f;
	DurationSeconds = Whirl.DurationSeconds;
	TickSeconds = Whirl.TickSeconds;
	PullCm = Whirl.PullMetersPerSecond * CentimetersPerMeter;
	Element = Partner;
	Color = FMath::Lerp(EIElementColor(EEIElement::Wind), EIElementColor(Partner), WhirlPartnerShare);
	switch (Partner)
	{
	case EEIElement::Fire:
		Damage = Whirl.FireDamage * ReactionFactor;
		BurnPerSecond = Whirl.FireBurnPerSecond;
		StatusSeconds = Whirl.FireBurnSeconds * StatusFactor;
		break;
	case EEIElement::Ice:
		Damage = Whirl.FrostDamage * ReactionFactor;
		SlowShare = Whirl.FrostSlowShare;
		// The slow lasts until the next turn of the whirl.
		StatusSeconds = Whirl.TickSeconds * 2.0f;
		EndFreezeSeconds = Whirl.FrostFreezeSeconds * StatusFactor;
		break;
	case EEIElement::Lightning:
		Damage = Whirl.StormDamage * ReactionFactor;
		break;
	default:
		break;
	}
}

void AEIRuneField::BeginPlay()
{
	Super::BeginPlay();

	FEIEffectParams Params;
	Params.Location = GetActorLocation();
	Params.Rotation = GetActorRotation();
	Params.Color = Color;
	Params.RadiusCm = SizeCm / 2.0f;
	Params.Seconds = DurationSeconds;

	if (bWall)
	{
		Shape->SetStaticMesh(EIVisuals::GetCube());
		Shape->SetWorldScale3D(FVector(WallThicknessCm, SizeCm, WallHeightCm) / 100.0f);
		AddActorWorldOffset(FVector(0.0f, 0.0f, WallHeightCm / 2.0f));
	}
	else if (bTall)
	{
		Shape->SetWorldScale3D(FVector(SizeCm, SizeCm, TallHeightCm) / 100.0f);
		AddActorWorldOffset(FVector(0.0f, 0.0f, TallHeightCm / 2.0f + LookAboveFloorCm));
	}
	else
	{
		Shape->SetWorldScale3D(FVector(SizeCm, SizeCm, CircleHeightCm) / 100.0f);
		AddActorWorldOffset(FVector(0.0f, 0.0f, CircleHeightCm / 2.0f + LookAboveFloorCm));
	}
	Material = EIVisuals::SetGlow(Shape, Color, bWall ? WallStrength : (bTall ? TallStrength : CircleStrength));
	Params.AttachTo = Shape;
	Look = AEIEffect::Play(this, GetEffectName(), Params);
	if (Look.IsValid() && Look->HidesPlaceholder())
	{
		Shape->SetVisibility(false);
	}
	EIVisuals::PlaySoundAt(this, bWhirl ? TEXT("ExplosionGross") : TEXT("ExplosionKlein"), GetActorLocation());
	SetLifeSpan(FMath::Max(DurationSeconds, BurstSeconds));
	if (HasAuthority())
	{
		Pulse();
		NextPulseIn = TickSeconds;
	}
}

void AEIRuneField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The frost whirl freezes everyone it still holds when it ends.
	if (EndPlayReason == EEndPlayReason::Destroyed && HasAuthority() && EndFreezeSeconds > 0.0f)
	{
		TArray<AActor*> Targets;
		CollectTargets(Targets);
		for (AActor* Target : Targets)
		{
			if (AEIEnemy* Enemy = Cast<AEIEnemy>(Target))
			{
				Enemy->Freeze(EndFreezeSeconds);
			}
		}
	}
	if (Look.IsValid())
	{
		Look->Stop();
	}
	Super::EndPlay(EndPlayReason);
}

FName AEIRuneField::GetEffectName() const
{
	if (bWhirl)
	{
		switch (Element)
		{
		case EEIElement::Fire:
			return TEXT("feuerwirbel");
		case EEIElement::Ice:
			return TEXT("frostwirbel");
		case EEIElement::Lightning:
			return TEXT("gewitterwirbel");
		default:
			return TEXT("windhose");
		}
	}
	switch (Element)
	{
	case EEIElement::Fire:
		return TEXT("flammenwand");
	case EEIElement::Lightning:
		return TEXT("gewitterfeld");
	case EEIElement::Wind:
		return TEXT("windhose");
	default:
		return TEXT("frostnova");
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
		Shape->SetWorldScale3D(FVector(Width, Width, bTall ? TallHeightCm : CircleHeightCm) / 100.0f);
	}
	// Wind turns.
	if (bTall)
	{
		AddActorWorldRotation(FRotator(0.0f, TallTurnDegreesPerSecond * DeltaSeconds, 0.0f));
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

void AEIRuneField::CollectTargets(TArray<AActor*>& OutTargets) const
{
	const FCollisionShape Area = bWall
		? FCollisionShape::MakeBox(FVector(WallThicknessCm, SizeCm, WallHeightCm) / 2.0f)
		: FCollisionShape::MakeSphere(SizeCm / 2.0f);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), GetActorQuat(), FCollisionObjectQueryParams(ECC_Pawn), Area);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (Target == nullptr || OutTargets.Contains(Target) || Target->IsA<AEIHeroCharacter>() || !Target->CanBeDamaged())
		{
			continue;
		}
		FCollisionQueryParams WallQuery(SCENE_QUERY_STAT(EIRuneWall), false, Target);
		if (GetWorld()->LineTraceTestByObjectType(GetActorLocation() + FVector(0.0f, 0.0f, WallCheckHeightCm), Target->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), WallQuery))
		{
			continue;
		}
		OutTargets.Add(Target);
	}
}

void AEIRuneField::Pulse()
{
	AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(GetOwner());
	TArray<AActor*> Targets;
	CollectTargets(Targets);
	TArray<FVector_NetQuantize> Struck;
	for (AActor* Target : Targets)
	{
		AEIEnemy* Enemy = Cast<AEIEnemy>(Target);
		if (Enemy != nullptr && Hero != nullptr && !bWhirl)
		{
			Hero->GetRunes()->HitWithElement(Enemy, Element);
		}
		if (Damage > 0.0f)
		{
			UGameplayStatics::ApplyDamage(Target, Damage, GetInstigatorController(), Hero, UEISpellDamage::StaticClass());
		}
		if (Element == EEIElement::Lightning)
		{
			Struck.Add(Target->GetActorLocation());
		}
		if (Enemy == nullptr)
		{
			continue;
		}
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
		if (PullCm > 0.0f && !Enemy->IsFrozen())
		{
			Enemy->PullToward(GetActorLocation(), PullCm);
		}
	}
	if (Struck.Num() > 0)
	{
		MulticastStrikes(Struck);
	}
}

void AEIRuneField::MulticastStrikes_Implementation(const TArray<FVector_NetQuantize>& Targets)
{
	// The whirl throws its bolts out of the top of its column, a storm field calls them down from the sky.
	const FVector Top = GetActorLocation() + FVector(0.0f, 0.0f, TallHeightCm / 2.0f);
	for (const FVector_NetQuantize& Target : Targets)
	{
		FEIEffectParams Params;
		Params.Location = Target;
		Params.Target = bWhirl ? Top : Target + FVector(0.0f, 0.0f, StrikeFromCm);
		Params.Color = EIElementColor(EEIElement::Lightning);
		AEIEffect::Play(this, TEXT("blitzschlag"), Params);
	}
}
