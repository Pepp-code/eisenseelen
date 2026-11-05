#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIForge.generated.h"

class UEILookComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The forge at the dock: an anvil beside a glowing hearth. Whoever stands at it opens the forge page of the menu with the
 * use key, and there the level opens new gear and gold buys it. The server spawns it once.
 */
UCLASS()
class EISENSEELEN_API AEIForge : public AActor
{
	GENERATED_BODY()

public:
	AEIForge();

	/** Between the deferred spawn and FinishSpawning. */
	void Setup(float InReachMeters) { ReachMeters = InReachMeters; }

	bool IsInReach(const AActor* Actor) const;
	FVector GetMarkLocation() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UStaticMeshComponent* AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Embers;

	/** The model from Data/aussehen.json over the blocks, if there is one. Collision stays with the blocks. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEILookComponent> Look;

	UPROPERTY(Replicated)
	float ReachMeters = 4.0f;
};
