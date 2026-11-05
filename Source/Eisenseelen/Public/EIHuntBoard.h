#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIHuntBoard.generated.h"

class UEILookComponent;
class UStaticMeshComponent;

/**
 * The board at the dock with the hunt letters. Every letter names an area and the boss that waits there, and whoever
 * stands at the board takes another one with the use key. The server spawns it once; the letters stand in the game state.
 */
UCLASS()
class EISENSEELEN_API AEIHuntBoard : public AActor
{
	GENERATED_BODY()

public:
	AEIHuntBoard();

	/** Between the deferred spawn and FinishSpawning. */
	void Setup(float InReachMeters) { ReachMeters = InReachMeters; }

	bool IsInReach(const AActor* Actor) const;
	FVector GetMarkLocation() const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UStaticMeshComponent* AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Panel;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Sheets;

	/** The model from Data/aussehen.json over the blocks, if there is one. Collision stays with the blocks. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEILookComponent> Look;

	UPROPERTY(Replicated)
	float ReachMeters = 4.0f;

	int32 ShownChosen = INDEX_NONE - 1;
	int32 ShownCount = -1;
};
