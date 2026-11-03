#pragma once

#include "CoreMinimal.h"
#include "EIRunDirector.h"
#include "GameFramework/Actor.h"
#include "EIChest.generated.h"

class UStaticMeshComponent;

/**
 * A chest with an omen in it. The map has more of them than a run uses, so every run finds them somewhere else. Each
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

	bool IsInReach(const AActor* Hero) const;
	bool IsOpenFor(const AActor* Hero) const;
	FVector GetMarkLocation() const;

	/** Server only. A chest not in use is gone for this run. Either way it starts closed. */
	void PrepareForRun(bool bUse);
	void OpenFor(AActor* Hero);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Look();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Lid;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Gem;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	bool bInUse = false;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	TArray<TObjectPtr<AActor>> OpenedBy;

	FEIRunValues Values;
};
