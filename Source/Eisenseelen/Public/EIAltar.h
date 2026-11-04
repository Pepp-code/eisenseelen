#pragma once

#include "CoreMinimal.h"
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

/** A stone with a crystal at a place on the map. A contract wakes it up, and holding its zone for long enough breaks it. */
UCLASS()
class EISENSEELEN_API AEIAltar : public AActor
{
	GENERATED_BODY()

public:
	AEIAltar();

	EEIAltarState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsActive() const { return State == EEIAltarState::Active; }

	/** Server only. Wakes it up for a contract, in the color of the faction. */
	void StartJob(float NewZoneMeters, const FLinearColor& Color);

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

	UPROPERTY(ReplicatedUsing = OnRep_State)
	float ZoneMeters = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_State)
	FLinearColor JobColor = FLinearColor::White;

	EEIAltarState ShownState = EEIAltarState::Dormant;
};
