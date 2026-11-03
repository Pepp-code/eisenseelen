#include "EIViewModelComponent.h"

#include "Components/StaticMeshComponent.h"
#include "EIHeroCharacter.h"
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
	const FLinearColor StaffHeadColor(1.0f, 0.38f, 0.06f);
	constexpr float StaffHeadGlow = 5.0f;
	constexpr float StaffLength = 120.0f;
	constexpr float StaffBack = 35.0f;
	constexpr float StaffHeadSize = 13.0f;

	constexpr int32 MaxTrailSamples = 24;
	constexpr float TrailLifeSeconds = 0.16f;
	constexpr float TrailBaseFraction = 0.45f;
	const FLinearColor TrailColor(0.6f, 0.75f, 1.0f);
	constexpr float TrailStrength = 1.2f;
	constexpr float CancelBlendSeconds = 0.08f;
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
	AddPart(Hand, Cube, FVector(-3.0f, 0.0f, 0.0f), FVector(7.0f, 6.0f, 6.0f), GauntletColor);

	SwordParts = NewObject<USceneComponent>(GetOwner());
	SwordParts->SetupAttachment(Hand);
	SwordParts->RegisterComponent();
	AddPart(SwordParts, Cube, FVector(BladeStart + BladeLength / 2.0f, 0.0f, 0.0f), FVector(BladeLength, 2.0f, 7.0f), BladeColor);
	AddPart(SwordParts, Cube, FVector(BladeStart - 1.0f, 0.0f, 0.0f), FVector(2.5f, 4.0f, 16.0f), GuardColor);

	StaffParts = NewObject<USceneComponent>(GetOwner());
	StaffParts->SetupAttachment(Hand);
	StaffParts->RegisterComponent();
	AddPart(StaffParts, Cube, FVector(StaffLength / 2.0f - StaffBack, 0.0f, 0.0f), FVector(StaffLength, 3.5f, 3.5f), ShaftColor);
	AddPart(StaffParts, EIVisuals::GetSphere(), FVector(StaffLength - StaffBack + StaffHeadSize / 2.0f, 0.0f, 0.0f), FVector(StaffHeadSize), StaffHeadColor, StaffHeadGlow);
	StaffParts->SetVisibility(false, true);

	Trail = NewObject<UProceduralMeshComponent>(GetOwner());
	Trail->SetupAttachment(this);
	Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Trail->SetCastShadow(false);
	Trail->SetOnlyOwnerSee(true);
	Trail->RegisterComponent();
	EIVisuals::SetGlow(Trail, TrailColor, TrailStrength);

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

void UEIViewModelComponent::PlayPose(FName Curve, float Seconds, float TrailFrom, float TrailTo)
{
	PlayingCurve = Curve;
	PlayingT = 0.0f;
	PlayingSeconds = FMath::Max(Seconds, KINDA_SMALL_NUMBER);
	TrailFromT = TrailFrom;
	TrailToT = TrailTo;
	HitStopRemaining = 0.0f;
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
	Hand->SetRelativeTransform(Pose);
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
		const float TipX = BladeStart + BladeLength;
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
