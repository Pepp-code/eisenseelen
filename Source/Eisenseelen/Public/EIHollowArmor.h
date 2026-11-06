#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIHollowArmor.generated.h"

class UEILookComponent;
class UStaticMeshComponent;

/**
 * The empty giant armor of a boss that is not there yet. Stone chests hold its soul stones, and whoever sets them all
 * into the armor wakes what waits inside. The server spawns it and counts the stones.
 */
UCLASS()
class EISENSEELEN_API AEIHollowArmor : public AActor
{
	GENERATED_BODY()

public:
	AEIHollowArmor();

	/** Between the deferred spawn and FinishSpawning. */
	void Setup(float InReachMeters, int32 InTotal);

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	int32 GetInserted() const { return Inserted; }

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	int32 GetTotal() const { return Total; }

	bool IsFull() const { return Inserted >= Total; }
	bool IsInReach(const AActor* Actor) const;
	FVector GetMarkLocation() const;

	/** Server only. A soul stone from a chest goes in. */
	void InsertStone();

	/** Server only. For tests and preview pictures: so many stones are in it at once. */
	void SetInserted(int32 NewInserted);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Inserted();

	void ShowInserted();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Torso;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> LeftLeg;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RightLeg;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> LeftArm;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RightArm;

	/** The hollow in the chest. It glows brighter with every soul stone. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ZoneDisc;

	/** The model from Data/aussehen.json over the blocks, if there is one. Collision stays with the blocks. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEILookComponent> Look;

	UPROPERTY(Replicated)
	float ReachMeters = 4.0f;

	UPROPERTY(Replicated)
	int32 Total = 3;

	UPROPERTY(ReplicatedUsing = OnRep_Inserted)
	int32 Inserted = 0;
};
