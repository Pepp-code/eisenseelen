#pragma once

#include "CoreMinimal.h"
#include "EIRunDirector.h"
#include "GameFramework/Actor.h"
#include "EIChest.generated.h"

class AEIEnemy;
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
 * A chest with an omen in it, which has to be earned: whoever steps into its circle calls its guards, and only when they
 * have fallen does the seal break. The map has more chests than a run uses, so every run finds them somewhere else. Each
 * player opens it for themselves, so nobody takes anything away from anyone.
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
	bool IsOpenFor(const AActor* Hero) const;
	FVector GetMarkLocation() const;

	/** Server only. A chest not in use is gone for this run. Either way it starts sealed and closed. */
	void PrepareForRun(bool bUse);

	/** Server only. The seal holds until all of them are gone, and without any it breaks at once. */
	void SetGuards(const TArray<AEIEnemy*>& Enemies);
	void UpdateGuards();

	/** Server only. */
	void OpenFor(AActor* Hero);

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

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GemMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	bool bInUse = false;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	EEIChestState State = EEIChestState::Sealed;

	UPROPERTY(Replicated)
	int32 GuardsLeft = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	TArray<TObjectPtr<AActor>> OpenedBy;

	TArray<TWeakObjectPtr<AEIEnemy>> Guards;
	EEIChestState ShownState = EEIChestState::Sealed;
	float LidDegrees = 0.0f;
	float LidTargetDegrees = 0.0f;
	FEIRunValues Values;
};
