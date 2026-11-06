#include "EIGroundMark.h"

#include "Components/StaticMeshComponent.h"
#include "EIVisuals.h"

namespace
{
	const FLinearColor MarkColor(1.0f, 0.1f, 0.03f);
	constexpr float AreaStrength = 0.3f;
	constexpr float FillStrength = 0.7f;
	constexpr float AboveFloorCm = 8.0f;
	constexpr float FillAboveAreaCm = 2.0f;
	constexpr float ThicknessScale = 0.01f;
	// The engine's basic shapes are 1 m wide.
	constexpr float ShapeSizeCm = 100.0f;
}

bool FEIGroundMarkData::Contains(const FVector& Spot) const
{
	if (Shape == EEIMarkShape::Circle)
	{
		return FVector::Dist2D(Spot, Location) <= LengthCm / 2.0f;
	}
	const FVector Direction = GetDirection();
	const FVector Offset = Spot - Location;
	const double Along = Offset.X * Direction.X + Offset.Y * Direction.Y;
	const double Across = Offset.Y * Direction.X - Offset.X * Direction.Y;
	return Along >= 0.0 && Along <= LengthCm && FMath::Abs(Across) <= WidthCm / 2.0f;
}

FVector FEIGroundMarkData::GetDirection() const
{
	return FRotator(0.0f, YawDegrees, 0.0f).Vector();
}

AEIGroundMark::AEIGroundMark()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	const auto MakePart = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(GetRootComponent());
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetCanEverAffectNavigation(false);
		return Part;
	};
	Area = MakePart(TEXT("Area"));
	Fill = MakePart(TEXT("Fill"));
}

AEIGroundMark* AEIGroundMark::SpawnGroundMark(UWorld* World, const FVector& Location, float RadiusCm, float DurationSeconds)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AEIGroundMark* MarkActor = World->SpawnActor<AEIGroundMark>(AEIGroundMark::StaticClass(), FTransform::Identity, Params))
	{
		FEIGroundMarkData Data;
		Data.bVisible = true;
		Data.Shape = EEIMarkShape::Circle;
		Data.Location = Location;
		Data.LengthCm = RadiusCm * 2.0f;
		Data.WidthCm = RadiusCm * 2.0f;
		Data.FillSeconds = DurationSeconds;
		MarkActor->Show(Data);
		MarkActor->SetLifeSpan(DurationSeconds + 0.5f);
		return MarkActor;
	}
	return nullptr;
}

void AEIGroundMark::Show(const FEIGroundMarkData& Data)
{
	Mark = Data;
	Age = 0.0f;
	const bool bCircle = Mark.Shape == EEIMarkShape::Circle;
	for (UStaticMeshComponent* Part : { Area.Get(), Fill.Get() })
	{
		Part->SetStaticMesh(bCircle ? EIVisuals::GetCylinder() : EIVisuals::GetCube());
	}
	EIVisuals::SetGlow(Area, MarkColor, AreaStrength);
	EIVisuals::SetGlow(Fill, MarkColor, FillStrength);
	SetActorLocationAndRotation(Mark.Location + FVector(0.0f, 0.0f, AboveFloorCm), FRotator(0.0f, Mark.YawDegrees, 0.0f));
	const float Length = Mark.LengthCm / ShapeSizeCm;
	Area->SetRelativeLocation(FVector(bCircle ? 0.0f : Mark.LengthCm / 2.0f, 0.0f, 0.0f));
	Area->SetRelativeScale3D(FVector(Length, bCircle ? Length : Mark.WidthCm / ShapeSizeCm, ThicknessScale));
	ApplyFill(Mark.FillSeconds > 0.0f ? 0.0f : 1.0f);
}

void AEIGroundMark::ApplyFill(float Share)
{
	const float Length = Mark.LengthCm / ShapeSizeCm * Share;
	if (Mark.Shape == EEIMarkShape::Circle)
	{
		Fill->SetRelativeLocation(FVector(0.0f, 0.0f, FillAboveAreaCm));
		Fill->SetRelativeScale3D(FVector(Length, Length, ThicknessScale));
	}
	else
	{
		Fill->SetRelativeLocation(FVector(Mark.LengthCm * Share / 2.0f, 0.0f, FillAboveAreaCm));
		Fill->SetRelativeScale3D(FVector(Length, Mark.WidthCm / ShapeSizeCm, ThicknessScale));
	}
	Fill->SetVisibility(Share > 0.0f);
}

void AEIGroundMark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Mark.FillSeconds > 0.0f && Age < Mark.FillSeconds)
	{
		Age += DeltaSeconds;
		ApplyFill(FMath::Min(Age / Mark.FillSeconds, 1.0f));
	}
}
