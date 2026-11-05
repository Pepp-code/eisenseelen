#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "EIViewModelComponent.generated.h"

class UEILookComponent;
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

	void ShowStaff(bool bStaff);

	/** How long the blade is, compared to the long sword, and what its edge and the head of the staff glow in. */
	void SetWeaponLook(float LengthFactor, const FLinearColor& Color);

	/** Puts the models from Data/aussehen.json over the gauntlet and the weapon: HandKey, then the weapon's keys in order. */
	void SetModels(const FString& HandKey, const TArray<FString>& WeaponKeys, bool bStaff);

	/** A moment such as "hieb" for the hand's own model. The hand still moves along Data/posen.json. */
	void PlayLookEvent(FName Event);

	UEILookComponent* GetHandLook() const { return HandLook; }
	UEILookComponent* GetWeaponLook() const { return WeaponLook; }
	void PlayPose(FName Curve, float Seconds, float TrailFrom = 2.0f, float TrailTo = 2.0f);

	/** For a gesture nothing depends on: it does not interrupt a swing or a block. */
	void PlayPoseIfIdle(FName Curve, float Seconds);

	/** The hand dips when the hero lands. Strength runs from 0 to 1. */
	void NoteLanding(float Strength);

	/** Breaks off the pose that is playing. The hand blends over from where it is. */
	void CancelPose();
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
	FTransform AddMotion(const FTransform& Pose, float DeltaTime);
	void UpdateTrail(float DeltaTime, bool bAddSample);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Hand;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> SwordParts;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> StaffParts;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Blade;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BladeEdge;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> StaffHead;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Gauntlet;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Guard;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Shaft;

	UPROPERTY(Transient)
	TObjectPtr<UEILookComponent> HandLook;

	UPROPERTY(Transient)
	TObjectPtr<UEILookComponent> WeaponLook;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Trail;

	TArray<FTrailSample> TrailSamples;

	FName RestCurve = TEXT("ruhe");
	FName PlayingCurve = NAME_None;
	float PlayingT = 0.0f;
	float PlayingSeconds = 1.0f;
	float TrailFromT = 2.0f;
	float TrailToT = 2.0f;
	float BladeLengthFactor = 1.0f;
	FLinearColor WeaponColor = FLinearColor::Black;

	FName HeldCurve = NAME_None;
	float HeldT = 0.0f;

	FName PreviewCurve = NAME_None;
	float PreviewT = 0.0f;

	float HitStopRemaining = 0.0f;

	FTransform BlendFrom = FTransform::Identity;
	float BlendRemaining = 0.0f;

	float BobPhase = 0.0f;
	float BreathPhase = 0.0f;
	float WalkBlend = 0.0f;
	float SprintBlend = 0.0f;
	float AirBlend = 0.0f;
	float BusyBlend = 0.0f;
	float LandingDip = 0.0f;
	float LookLagYaw = 0.0f;
	float LookLagPitch = 0.0f;
	FRotator LastView = FRotator::ZeroRotator;
	bool bHasLastView = false;
};
