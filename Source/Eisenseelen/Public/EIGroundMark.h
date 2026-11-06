#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIGroundMark.generated.h"

class UStaticMeshComponent;

UENUM()
enum class EEIMarkShape : uint8
{
	Circle,
	Strip
};

/**
 * The place an attack lands on. The same numbers decide who gets hit and what the floor shows, so that the two can never
 * differ. A circle has its middle at Location, and LengthCm is its width. A strip starts at Location and runs along YawDegrees.
 */
USTRUCT()
struct EISENSEELEN_API FEIGroundMarkData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bVisible = false;

	UPROPERTY()
	EEIMarkShape Shape = EEIMarkShape::Circle;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	float YawDegrees = 0.0f;

	UPROPERTY()
	float LengthCm = 0.0f;

	UPROPERTY()
	float WidthCm = 0.0f;

	/** How long it takes to fill up. The attack lands when it is full. */
	UPROPERTY()
	float FillSeconds = 0.0f;

	/** Whether this spot lies on the mark, seen from above. */
	bool Contains(const FVector& Spot) const;

	/** The direction a strip runs in. */
	FVector GetDirection() const;
};

/** A red shape on the floor that shows where an attack lands and, by filling up, when. Purely for the eye, every machine spawns its own. */
UCLASS()
class EISENSEELEN_API AEIGroundMark : public AActor
{
	GENERATED_BODY()

public:
	AEIGroundMark();

	static AEIGroundMark* SpawnGroundMark(UWorld* World, const FVector& Location, float RadiusCm, float DurationSeconds);

	void Show(const FEIGroundMarkData& Data);

	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyFill(float Share);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Area;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Fill;

	FEIGroundMarkData Mark;
	float Age = 0.0f;
};
