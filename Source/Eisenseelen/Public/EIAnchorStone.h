#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIAnchorStone.generated.h"

class UEILookComponent;
class UStaticMeshComponent;

/**
 * A stone that holds one chain of a bound boss. Whoever holds the zone around it for long enough breaks it, and the chain
 * falls. The server spawns the stones of a run.
 */
UCLASS()
class EISENSEELEN_API AEIAnchorStone : public AActor
{
	GENERATED_BODY()

public:
	AEIAnchorStone();

	/** Between the deferred spawn and FinishSpawning. */
	void Setup(const FVector& InChainEnd, float InZoneMeters, float HoldSeconds);

	bool IsBroken() const { return bBroken; }
	bool IsHeld() const { return bHeld; }
	float GetHoldLeft() const { return HoldLeft; }
	float GetZoneMeters() const { return ZoneMeters; }
	bool IsInZone(const AActor* Actor) const;
	FVector GetMarkLocation() const;

	/** Server only. Runs the time while somebody stands in the zone, and shortens it for tests and preview pictures. */
	void UpdateHold(bool bNowHeld, float DeltaTime);
	void SetHoldLeft(float Seconds) { HoldLeft = Seconds; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Broken();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Stone;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Crystal;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Chain;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ZoneDisc;

	/** The model from Data/aussehen.json over the blocks, if there is one. Collision stays with the blocks. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEILookComponent> Look;

	UPROPERTY(Replicated)
	FVector ChainEnd = FVector::ZeroVector;

	UPROPERTY(Replicated)
	float ZoneMeters = 7.0f;

	UPROPERTY(Replicated)
	float HoldLeft = 0.0f;

	UPROPERTY(Replicated)
	bool bHeld = false;

	UPROPERTY(ReplicatedUsing = OnRep_Broken)
	bool bBroken = false;
};
