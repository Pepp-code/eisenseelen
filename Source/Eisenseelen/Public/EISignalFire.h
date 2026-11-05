#pragma once

#include "CoreMinimal.h"
#include "EIRunDirector.h"
#include "GameFramework/Actor.h"
#include "EISignalFire.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * A pillar with a fire on top, the only place where the barge picks anybody up. The map has several, and a run uses one
 * of them. Lighting it calls the barge, and a hero has to stay in its zone until the barge is there.
 */
UCLASS()
class EISENSEELEN_API AEISignalFire : public AActor
{
	GENERATED_BODY()

public:
	AEISignalFire();

	bool IsInUse() const { return bInUse; }
	bool IsLit() const { return bLit; }

	/** Server only. A fire not in use is gone for this run. */
	void SetInUse(bool bUse);

	/** Server only. */
	void SetLit(bool bNewLit);

	bool IsInReach(const AActor* Actor) const;
	bool IsInZone(const AActor* Actor) const;
	FVector GetFireLocation() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Look();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Pillar;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Fire;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ZoneDisc;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FireMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	bool bInUse = false;

	UPROPERTY(ReplicatedUsing = OnRep_Look)
	bool bLit = false;

	bool bShownLit = false;
	FEIRunValues Values;
};
