#pragma once

#include "CoreMinimal.h"
#include "EIRunDirector.h"
#include "GameFramework/Actor.h"
#include "EIAltar.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UENUM()
enum class EEIAltarState : uint8
{
	Dormant,
	Active,
	Broken
};

/** A stone with a glowing crystal at a place on the map. The ritual is disturbed by holding the zone of the active altars. */
UCLASS()
class EISENSEELEN_API AEIAltar : public AActor
{
	GENERATED_BODY()

public:
	AEIAltar();

	EEIAltarState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsActive() const { return State == EEIAltarState::Active; }

	/** Server only. */
	void SetState(EEIAltarState NewState);

	bool IsInZone(const AActor* Actor) const;
	FVector GetCrystalLocation() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_State();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Stone;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Crystal;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ZoneDisc;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CrystalMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	EEIAltarState State = EEIAltarState::Dormant;

	FEIRunValues Values;
};
