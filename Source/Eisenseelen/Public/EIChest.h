#pragma once

#include "CoreMinimal.h"
#include "EIRunDirector.h"
#include "GameFramework/Actor.h"
#include "EIChest.generated.h"

class AEIEnemy;
class UEILookComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UENUM()
enum class EEIChestState : uint8
{
	Sealed,
	Guarded,
	Free
};

/**
 * A chest that a contract asks for: whoever steps into its circle calls its guards, and only when they have fallen does
 * the seal break. The map has more chests than a run uses, so every run finds them somewhere else.
 */
UCLASS()
class EISENSEELEN_API AEIChest : public AActor
{
	GENERATED_BODY()

public:
	AEIChest();

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsInUse() const { return bInUse; }

	EEIChestState GetState() const { return State; }
	int32 GetGuardsLeft() const { return GuardsLeft; }

	bool IsInZone(const AActor* Hero) const;
	bool IsInReach(const AActor* Hero) const;
	bool IsOpen() const { return bOpened; }
	FVector GetMarkLocation() const;

	/** Server only. A chest not in use is gone for this run. Either way it starts sealed and closed. */
	void PrepareForRun(bool bUse);

	/** Server only. The seal holds until all of them are gone, and without any it breaks at once. */
	void SetGuards(const TArray<AEIEnemy*>& Enemies);
	void UpdateGuards();

	/** Server only. */
	void Open();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void LoadValues();
	void ShowLid();

	UFUNCTION()
	void OnRep_Look();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Lid;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Gem;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Seal;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ZoneDisc;

	/** The model from Data/aussehen.json over the blocks, if there is one. Collision stays with the blocks. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEILookComponent> Look;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GemMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	bool bInUse = false;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	EEIChestState State = EEIChestState::Sealed;

	UPROPERTY(Replicated)
	int32 GuardsLeft = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	bool bOpened = false;

	TArray<TWeakObjectPtr<AEIEnemy>> Guards;
	EEIChestState ShownState = EEIChestState::Sealed;
	float LidDegrees = 0.0f;
	float LidTargetDegrees = 0.0f;
	FEIRunValues Values;
};
