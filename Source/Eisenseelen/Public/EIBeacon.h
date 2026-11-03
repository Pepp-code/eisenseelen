#pragma once

#include "CoreMinimal.h"
#include "EIRunDirector.h"
#include "GameFramework/Actor.h"
#include "EIBeacon.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/** A pillar with a fire on top. Lighting one calls the barge, and a hero has to stay in its zone until the barge is there. */
UCLASS()
class EISENSEELEN_API AEIBeacon : public AActor
{
	GENERATED_BODY()

public:
	AEIBeacon();

	bool IsLit() const { return bLit; }

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
	void OnRep_Lit();

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

	UPROPERTY(ReplicatedUsing = OnRep_Lit)
	bool bLit = false;

	FEIRunValues Values;
};
