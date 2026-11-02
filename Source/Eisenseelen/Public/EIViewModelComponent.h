#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "EIViewModelComponent.generated.h"

class UProceduralMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * What the player sees of their own hand: rigid parts, moved along the curves from Data/posen.json. Only the owner
 * builds and sees it. The blade leaves a glowing trail while a swing can hit.
 */
UCLASS()
class EISENSEELEN_API UEIViewModelComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UEIViewModelComponent();

	/** Builds the parts. Called once the owner is known to be local. */
	void Build();

	void PlayPose(FName Curve, float Seconds, float TrailFrom = 2.0f, float TrailTo = 2.0f);
	void HoldPose(FName Curve, float T);
	void ClearHold();
	void HitStop(float Seconds);

	/** For preview pictures: freezes the hand in a pose. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void PreviewPose(FName Curve, float T);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FTrailSample
	{
		FVector Base = FVector::ZeroVector;
		FVector Tip = FVector::ZeroVector;
		float Age = 0.0f;
	};

	UStaticMeshComponent* AddPart(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FLinearColor& Color, float Glow = 0.0f);
	FTransform CurrentPose() const;
	void UpdateTrail(float DeltaTime, bool bAddSample);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Hand;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> SwordParts;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Trail;

	TArray<FTrailSample> TrailSamples;

	FName PlayingCurve = NAME_None;
	float PlayingT = 0.0f;
	float PlayingSeconds = 1.0f;
	float TrailFromT = 2.0f;
	float TrailToT = 2.0f;

	FName HeldCurve = NAME_None;
	float HeldT = 0.0f;

	FName PreviewCurve = NAME_None;
	float PreviewT = 0.0f;

	float HitStopRemaining = 0.0f;
};
