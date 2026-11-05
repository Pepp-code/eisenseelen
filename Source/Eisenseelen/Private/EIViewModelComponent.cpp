#include "EIViewModelComponent.h"

#include "Components/StaticMeshComponent.h"
#include "EIHeroCharacter.h"
#include "EIHeroMovementComponent.h"
#include "EILookComponent.h"
#include "EIPoses.h"
#include "EIVisuals.h"
#include "Engine/StaticMesh.h"
#include "ProceduralMeshComponent.h"

namespace
{
	// The blade runs along the hand's forward axis. All sizes are centimetres.
	constexpr float BladeLength = 110.0f;
	constexpr float BladeStart = 6.0f;
	const FLinearColor BladeColor(0.42f, 0.45f, 0.5f);
	const FLinearColor GauntletColor(0.09f, 0.1f, 0.13f);
	const FLinearColor GuardColor(0.3f, 0.22f, 0.06f);
	const FLinearColor ShaftColor(0.12f, 0.07f, 0.03f);
	const FVector BladeSizeCm(BladeLength, 2.0f, 7.0f);
	const FVector BladeEdgeSizeCm(BladeLength * 0.96f, 2.4f, 1.4f);
	constexpr float BladeEdgeGlow = 1.2f;
	constexpr float StaffHeadGlow = 1.2f;
	constexpr float StaffLength = 120.0f;
	constexpr float StaffBack = 35.0f;
	constexpr float StaffHeadSize = 13.0f;

	constexpr int32 MaxTrailSamples = 24;
	constexpr float TrailLifeSeconds = 0.16f;
	constexpr float TrailBaseFraction = 0.45f;
	constexpr float TrailStrength = 1.2f;
	constexpr float CancelBlendSeconds = 0.08f;

	constexpr float MotionBlendSpeed = 9.0f;
	constexpr float LandingRecoverSpeed = 7.0f;
	constexpr float LookLagRecoverSpeed = 10.0f;
	constexpr float SprintFromRunShare = 1.15f;
	constexpr float MaxWalkShare = 1.5f;
	// While the hand does something on purpose, it should not also wobble.
	constexpr float BusyCalm = 0.85f;
}

UEIViewModelComponent::UEIViewModelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

UStaticMeshComponent* UEIViewModelComponent::AddPart(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FLinearColor& Color, float Glow)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(GetOwner());
	Part->SetStaticMesh(Mesh);
	Part->SetupAttachment(Parent);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeScale3D(Scale / 100.0f);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCastShadow(false);
	Part->SetOnlyOwnerSee(true);
	Part->RegisterComponent();
	EIVisuals::SetColor(Part, Color, Glow);
	return Part;
}

void UEIViewModelComponent::Build()
{
	if (Hand != nullptr)
	{
		return;
	}
	Hand = NewObject<USceneComponent>(GetOwner());
	Hand->SetupAttachment(this);
	Hand->RegisterComponent();

	UStaticMesh* Cube = EIVisuals::GetCube();
	// The hero is an empty suit of armour, so the gauntlet floats without an arm. It stays for every weapon.
	Gauntlet = AddPart(Hand, Cube, FVector(-3.0f, 0.0f, 0.0f), FVector(7.0f, 6.0f, 6.0f), GauntletColor);

	SwordParts = NewObject<USceneComponent>(GetOwner());
	SwordParts->SetupAttachment(Hand);
	SwordParts->RegisterComponent();
	Blade = AddPart(SwordParts, Cube, FVector(BladeStart + BladeLength / 2.0f, 0.0f, 0.0f), BladeSizeCm, BladeColor);
	BladeEdge = AddPart(SwordParts, Cube, FVector(BladeStart + BladeLength / 2.0f, 0.0f, 0.0f), BladeEdgeSizeCm, BladeColor);
	Guard = AddPart(SwordParts, Cube, FVector(BladeStart - 1.0f, 0.0f, 0.0f), FVector(2.5f, 4.0f, 16.0f), GuardColor);

	StaffParts = NewObject<USceneComponent>(GetOwner());
	StaffParts->SetupAttachment(Hand);
	StaffParts->RegisterComponent();
	Shaft = AddPart(StaffParts, Cube, FVector(StaffLength / 2.0f - StaffBack, 0.0f, 0.0f), FVector(StaffLength, 3.5f, 3.5f), ShaftColor);
	StaffHead = AddPart(StaffParts, EIVisuals::GetSphere(), FVector(StaffLength - StaffBack + StaffHeadSize / 2.0f, 0.0f, 0.0f), FVector(StaffHeadSize), BladeColor);
	StaffParts->SetVisibility(false, true);

	// The models from Data/aussehen.json hang on the same hand, so they follow the same poses.
	HandLook = NewObject<UEILookComponent>(GetOwner());
	HandLook->SetupAttachment(Hand);
	HandLook->SetViewOptions(true, false, false);
	HandLook->RegisterComponent();
	WeaponLook = NewObject<UEILookComponent>(GetOwner());
	WeaponLook->SetupAttachment(Hand);
	WeaponLook->SetViewOptions(true, false, false);
	WeaponLook->RegisterComponent();

	Trail = NewObject<UProceduralMeshComponent>(GetOwner());
	Trail->SetupAttachment(this);
	Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Trail->SetCastShadow(false);
	Trail->SetOnlyOwnerSee(true);
	Trail->RegisterComponent();

	Hand->SetRelativeTransform(CurrentPose());
	SetComponentTickEnabled(true);
}

void UEIViewModelComponent::ShowStaff(bool bStaff)
{
	SwordParts->SetVisibility(!bStaff, true);
	StaffParts->SetVisibility(bStaff, true);
	RestCurve = bStaff ? TEXT("stab_ruhe") : TEXT("ruhe");
	PlayingCurve = NAME_None;
	HeldCurve = NAME_None;
}

void UEIViewModelComponent::SetWeaponLook(float LengthFactor, const FLinearColor& Color)
{
	if (Hand == nullptr || (LengthFactor == BladeLengthFactor && Color == WeaponColor))
	{
		return;
	}
	BladeLengthFactor = LengthFactor;
	WeaponColor = Color;
	const FVector Middle(BladeStart + BladeLength * LengthFactor / 2.0f, 0.0f, 0.0f);
	const FVector Stretch(LengthFactor, 1.0f, 1.0f);
	Blade->SetRelativeLocation(Middle);
	Blade->SetRelativeScale3D(BladeSizeCm * Stretch / 100.0f);
	BladeEdge->SetRelativeLocation(Middle);
	BladeEdge->SetRelativeScale3D(BladeEdgeSizeCm * Stretch / 100.0f);
	EIVisuals::SetColor(BladeEdge, Color, BladeEdgeGlow);
	EIVisuals::SetColor(StaffHead, Color, StaffHeadGlow);
	EIVisuals::SetGlow(Trail, Color, TrailStrength);
}

void UEIViewModelComponent::SetModels(const FString& HandKey, const TArray<FString>& WeaponKeys, bool bStaff)
{
	if (Hand == nullptr)
	{
		return;
	}
	// The weapon goes back into the hand first, so a hand model that changes does not take it along.
	WeaponLook->AttachToComponent(Hand, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	HandLook->Setup({HandKey}, {Gauntlet});
	if (bStaff)
	{
		WeaponLook->Setup(WeaponKeys, {Shaft}, {StaffHead});
	}
	else
	{
		WeaponLook->Setup(WeaponKeys, {Blade, Guard}, {BladeEdge});
	}
	HandLook->AttachToModelSocket(WeaponLook, WeaponLook->GetSocket());
}

void UEIViewModelComponent::PlayLookEvent(FName Event)
{
	if (HandLook != nullptr)
	{
		HandLook->PlayEvent(Event);
	}
}

void UEIViewModelComponent::PlayPose(FName Curve, float Seconds, float TrailFrom, float TrailTo)
{
	PlayingCurve = Curve;
	PlayingT = 0.0f;
	PlayingSeconds = FMath::Max(Seconds, KINDA_SMALL_NUMBER);
	TrailFromT = TrailFrom;
	TrailToT = TrailTo;
	HitStopRemaining = 0.0f;
}

void UEIViewModelComponent::PlayPoseIfIdle(FName Curve, float Seconds)
{
	if (PlayingCurve == NAME_None && HeldCurve == NAME_None)
	{
		PlayPose(Curve, Seconds);
	}
}

void UEIViewModelComponent::NoteLanding(float Strength)
{
	LandingDip = FMath::Max(LandingDip, Strength * CastChecked<AEIHeroCharacter>(GetOwner())->GetHandMotion().LandingDipCm);
}

void UEIViewModelComponent::CancelPose()
{
	if (Hand != nullptr)
	{
		BlendFrom = Hand->GetRelativeTransform();
		BlendRemaining = CancelBlendSeconds;
	}
	PlayingCurve = NAME_None;
	HitStopRemaining = 0.0f;
}

void UEIViewModelComponent::HoldPose(FName Curve, float T)
{
	HeldCurve = Curve;
	HeldT = T;
}

void UEIViewModelComponent::ClearHold()
{
	HeldCurve = NAME_None;
}

void UEIViewModelComponent::HitStop(float Seconds)
{
	HitStopRemaining = Seconds;
}

void UEIViewModelComponent::PreviewPose(FName Curve, float T)
{
	PreviewCurve = Curve;
	PreviewT = T;
}

FTransform UEIViewModelComponent::CurrentPose() const
{
	const FEIPoseLibrary& Poses = CastChecked<AEIHeroCharacter>(GetOwner())->GetPoses();
	const FEIPoseCurve* Curve = nullptr;
	float T = 0.0f;
	if (PreviewCurve != NAME_None)
	{
		Curve = Poses.Find(PreviewCurve);
		T = PreviewT;
	}
	else if (PlayingCurve != NAME_None)
	{
		Curve = Poses.Find(PlayingCurve);
		T = PlayingT;
	}
	else if (HeldCurve != NAME_None)
	{
		Curve = Poses.Find(HeldCurve);
		T = HeldT;
	}
	if (Curve == nullptr)
	{
		Curve = Poses.Find(RestCurve);
	}
	return Curve != nullptr ? Curve->Evaluate(T) : FTransform::Identity;
}

FTransform UEIViewModelComponent::AddMotion(const FTransform& Pose, float DeltaTime)
{
	const AEIHeroCharacter* Hero = CastChecked<AEIHeroCharacter>(GetOwner());
	const FEIHandMotion& Motion = Hero->GetHandMotion();
	const UEIHeroMovementComponent* Movement = Hero->GetHeroMovement();
	const float Speed = static_cast<float>(Hero->GetVelocity().Size2D());
	const bool bOnGround = Movement->IsMovingOnGround();
	const float RunSpeed = FMath::Max(Movement->MaxWalkSpeed, 1.0f);
	const auto BlendTowards = [DeltaTime](float& Value, float Target) { Value = FMath::FInterpTo(Value, Target, DeltaTime, MotionBlendSpeed); };

	BlendTowards(WalkBlend, bOnGround ? FMath::Min(Speed / RunSpeed, MaxWalkShare) : 0.0f);
	BlendTowards(SprintBlend, bOnGround && Speed > RunSpeed * SprintFromRunShare ? 1.0f : 0.0f);
	BlendTowards(AirBlend, bOnGround ? 0.0f : 1.0f);
	BlendTowards(BusyBlend, PlayingCurve != NAME_None || HeldCurve != NAME_None ? 1.0f : 0.0f);
	LandingDip = FMath::FInterpTo(LandingDip, 0.0f, DeltaTime, LandingRecoverSpeed);
	BreathPhase = FMath::Fmod(BreathPhase + DeltaTime * Motion.BreathsPerSecond * UE_TWO_PI, UE_TWO_PI);
	if (bOnGround)
	{
		// One whole swing from side to side takes two steps.
		BobPhase = FMath::Fmod(BobPhase + DeltaTime * Speed / (Motion.StrideMeters * 100.0f) * UE_PI, UE_TWO_PI);
	}

	// The hand lags behind when the view turns.
	const FRotator View = Hero->GetViewRotation();
	if (bHasLastView && DeltaTime > 0.0f)
	{
		const FRotator Turned = (View - LastView).GetNormalized();
		const float TargetYaw = FMath::Clamp(static_cast<float>(-Turned.Yaw) / DeltaTime * Motion.LookLag, -Motion.LookLagMaxDegrees, Motion.LookLagMaxDegrees);
		const float TargetPitch = FMath::Clamp(static_cast<float>(-Turned.Pitch) / DeltaTime * Motion.LookLag, -Motion.LookLagMaxDegrees, Motion.LookLagMaxDegrees);
		LookLagYaw = FMath::FInterpTo(LookLagYaw, TargetYaw, DeltaTime, LookLagRecoverSpeed);
		LookLagPitch = FMath::FInterpTo(LookLagPitch, TargetPitch, DeltaTime, LookLagRecoverSpeed);
	}
	LastView = View;
	bHasLastView = true;

	const float Calm = 1.0f - BusyBlend * BusyCalm;
	const float Side = FMath::Sin(BobPhase) * Motion.BobSideCm * WalkBlend;
	const float Up = FMath::Cos(BobPhase * 2.0f) * Motion.BobUpCm * WalkBlend
		+ FMath::Sin(BreathPhase) * Motion.BreathCm * (1.0f - FMath::Min(WalkBlend, 1.0f))
		+ Motion.AirUpCm * AirBlend - LandingDip;
	const FVector Offset = FVector(Motion.SprintForwardCm * SprintBlend, Side + Motion.SprintRightCm * SprintBlend, Up + Motion.SprintUpCm * SprintBlend) * Calm;
	const FRotator Turn = FRotator(Motion.SprintPitchDegrees * SprintBlend + LookLagPitch, Motion.SprintYawDegrees * SprintBlend + LookLagYaw, Motion.SprintRollDegrees * SprintBlend) * Calm;

	FTransform Result = Pose;
	Result.SetTranslation(Pose.GetTranslation() + Offset);
	Result.SetRotation(FQuat(Turn) * Pose.GetRotation());
	return Result;
}

void UEIViewModelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Hand == nullptr)
	{
		return;
	}

	bool bTrailActive = false;
	if (PlayingCurve != NAME_None)
	{
		if (HitStopRemaining > 0.0f)
		{
			HitStopRemaining -= DeltaTime;
		}
		else
		{
			PlayingT += DeltaTime / PlayingSeconds;
		}
		bTrailActive = PlayingT >= TrailFromT && PlayingT <= TrailToT;
		if (PlayingT >= 1.0f)
		{
			PlayingCurve = NAME_None;
		}
	}
	FTransform Pose = CurrentPose();
	if (BlendRemaining > 0.0f)
	{
		BlendRemaining = FMath::Max(0.0f, BlendRemaining - DeltaTime);
		FTransform Blended;
		Blended.Blend(BlendFrom, Pose, 1.0f - BlendRemaining / CancelBlendSeconds);
		Pose = Blended;
	}
	Hand->SetRelativeTransform(PreviewCurve != NAME_None ? Pose : AddMotion(Pose, DeltaTime));
	UpdateTrail(DeltaTime, bTrailActive || PreviewCurve != NAME_None);
}

void UEIViewModelComponent::UpdateTrail(float DeltaTime, bool bAddSample)
{
	for (FTrailSample& Sample : TrailSamples)
	{
		Sample.Age += DeltaTime;
	}
	TrailSamples.RemoveAll([](const FTrailSample& Sample) { return Sample.Age > TrailLifeSeconds; });
	if (bAddSample)
	{
		const FTransform Pose = Hand->GetRelativeTransform();
		FTrailSample Sample;
		const float TipX = BladeStart + BladeLength * BladeLengthFactor;
		Sample.Tip = Pose.TransformPosition(FVector(TipX, 0.0f, 0.0f));
		Sample.Base = Pose.TransformPosition(FVector(TipX * TrailBaseFraction, 0.0f, 0.0f));
		TrailSamples.Add(Sample);
		if (TrailSamples.Num() > MaxTrailSamples)
		{
			TrailSamples.RemoveAt(0);
		}
	}

	if (TrailSamples.Num() < 2)
	{
		Trail->ClearAllMeshSections();
		return;
	}
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FLinearColor> Colors;
	for (int32 Index = 0; Index < TrailSamples.Num(); ++Index)
	{
		const FTrailSample& Sample = TrailSamples[Index];
		const float Alpha = 1.0f - Sample.Age / TrailLifeSeconds;
		Vertices.Add(Sample.Base);
		Vertices.Add(Sample.Tip);
		Colors.Add(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
		Colors.Add(FLinearColor(1.0f, 1.0f, 1.0f, Alpha));
		if (Index > 0)
		{
			const int32 V = Index * 2;
			Triangles.Append({ V - 2, V - 1, V, V, V - 1, V + 1 });
		}
	}
	Trail->CreateMeshSection_LinearColor(0, Vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(), Colors, TArray<FProcMeshTangent>(), false);
}
