#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "EIHeroBodyComponent.generated.h"

class UEIGearComponent;
enum class EEIHeroClass : uint8;
class UEILookComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** How a class is called in the keys of Data/aussehen.json: klingenwacht or arkanist. */
EISENSEELEN_API FString EIHeroClassLookName(EEIHeroClass HeroClass);

/** The keys of the weapon a hero holds in Data/aussehen.json: the item itself first, then its form. */
EISENSEELEN_API TArray<FString> EIWeaponLookKeys(const UEIGearComponent* Gear, bool bStaff);

/**
 * The hero as the others see them: an empty suit of armour from blocks, with the weapon in the right hand, until
 * Data/aussehen.json names a model for held.<class>. The owner never sees it, they have their hand in front of the camera.
 */
UCLASS()
class EISENSEELEN_API UEIHeroBodyComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UEIHeroBodyComponent();

	/** Builds the blocks once and puts on what the hero is and wears. Again whenever the class or the gear change. */
	void Refresh();

	/** A moment such as "hieb" or "treffer": the model plays it, the blocks swing the right arm. */
	void PlayEvent(FName Event);

	/** The hero falls. The model plays "tod", and without one the blocks topple over. */
	void ShowDeath();

	void SetCarryingVisualState(bool bCarrying);

	UEILookComponent* GetLook() const { return Look; }
	UEILookComponent* GetWeaponLook() const { return WeaponLook; }
	bool IsBuilt() const { return Frame != nullptr; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Build();
	USceneComponent* AddPivot(USceneComponent* Parent, const FVector& Location);
	UStaticMeshComponent* AddPart(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UEILookComponent> Look;

	UPROPERTY(Transient)
	TObjectPtr<UEILookComponent> WeaponLook;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ArmourParts;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Visor;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> LeftHip;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RightHip;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> LeftShoulder;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RightShoulder;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> RightHand;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> SwordParts;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> StaffParts;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Blade;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BladeEdge;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Guard;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Shaft;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> StaffHead;

	float StridePhase = 0.0f;
	float WalkBlend = 0.0f;
	float AirBlend = 0.0f;
	float ActionBlend = 0.0f;
	float FallBlend = 0.0f;
	bool bDead = false;
	bool bToppleModel = false;
};
