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
	Broken,
	Shrine
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

	/** A small job on the side: whoever holds its zone for a short while gets a reward. */
	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsShrine() const { return State == EEIAltarState::Shrine; }

	float GetShrineLeft() const { return ShrineLeft; }
	bool IsShrineHeld() const { return bShrineHeld; }

	/** Server only. */
	void SetShrineHold(float SecondsLeft, bool bHeld);

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

	UPROPERTY(Replicated)
	float ShrineLeft = 0.0f;

	UPROPERTY(Replicated)
	bool bShrineHeld = false;

	FEIRunValues Values;
};
