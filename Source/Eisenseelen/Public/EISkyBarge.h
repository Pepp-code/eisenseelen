#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EISkyBarge.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/**
 * The flying barge, grey boxes for now. A run starts on its deck high above the map. Once a beacon burns it flies down
 * to it, and whoever steps into its boarding zone rides it away. The run director on the server moves it.
 */
UCLASS()
class EISENSEELEN_API AEISkyBarge : public AActor
{
	GENERATED_BODY()

public:
	AEISkyBarge();

	/** Server only. */
	void ReturnToStart();
	void ArriveAt(const FVector& Location);
	void BeginApproach(const FVector& HoverLocation);
	void SetApproachFraction(float Fraction);
	void Depart(float SpeedCmPerSecond);

	bool IsInBoardingZone(const AActor* Actor) const;

	/** The place on the deck closest to this location, for a hero standing on it. */
	FVector GetDeckSpotNear(const FVector& Location, float HalfHeight) const;
	float GetDeckHeight() const;
	const FVector& GetStartLocation() const { return StartLocation; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Flying();

	UStaticMeshComponent* AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Deck;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Hull;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Prow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mast;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Sail;

	/** While it flies nobody collides with it, so it cannot land on anyone. */
	UPROPERTY(ReplicatedUsing = OnRep_Flying)
	bool bFlying = false;

	FVector StartLocation = FVector::ZeroVector;
	FVector ApproachFrom = FVector::ZeroVector;
	double DepartUpToHeight = 0.0;
	FVector ApproachTo = FVector::ZeroVector;
	float DepartSpeedCm = 0.0f;
};
