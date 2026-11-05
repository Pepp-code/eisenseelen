#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EISoulKettle.generated.h"

class UStaticMeshComponent;

/**
 * The kettle of a boss that sleeps in its grave. Every enemy that falls in the circle around it gives it a soul, and once
 * it is full the grave breaks open. The server spawns it and counts the souls.
 */
UCLASS()
class EISENSEELEN_API AEISoulKettle : public AActor
{
	GENERATED_BODY()

public:
	AEISoulKettle();

	/** Between the deferred spawn and FinishSpawning. */
	void Setup(const FVector& InGraveLocation, float InZoneMeters, int32 InCount);

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	int32 GetSouls() const { return Souls; }

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	int32 GetCount() const { return Count; }

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsFull() const { return Souls >= Count; }

	float GetZoneMeters() const { return ZoneMeters; }
	bool IsInZone(const AActor* Actor) const;
	bool IsInZone(const FVector& Location) const;
	FVector GetMarkLocation() const;

	/** Server only. A soul from an enemy that fell here flies into the kettle. */
	void AddSoul(const FVector& From);

	/** Server only. For tests and preview pictures: so many souls are in it at once. */
	void SetSouls(int32 NewSouls);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Souls();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSoulFlies(FVector_NetQuantize From);

	UStaticMeshComponent* AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Pot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Fluid;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Wisp;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ZoneDisc;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GraveSlab;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Headstone;

	UPROPERTY(Replicated)
	FVector GraveLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	float ZoneMeters = 12.0f;

	UPROPERTY(Replicated)
	int32 Count = 25;

	UPROPERTY(ReplicatedUsing = OnRep_Souls)
	int32 Souls = 0;

	bool bShownOpen = false;
};

/** A soul flying from a fallen enemy into the kettle. Purely for the eye, every machine spawns its own. */
UCLASS()
class EISENSEELEN_API AEISoulWisp : public AActor
{
	GENERATED_BODY()

public:
	AEISoulWisp();

	/** Between the deferred spawn and FinishSpawning. */
	void Setup(const FVector& InTarget) { Target = InTarget; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Glow;

	FVector Start = FVector::ZeroVector;
	FVector Target = FVector::ZeroVector;
	float Age = 0.0f;
};
