#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIEnemyCamp.generated.h"

class UStaticMeshComponent;

/**
 * A fixed place on the map where enemies stand watch from the start of a run. Later in the run reinforcements come out
 * of these places. The camp itself only marks the spot, the run director fills it.
 */
UCLASS()
class EISENSEELEN_API AEIEnemyCamp : public AActor
{
	GENERATED_BODY()

public:
	AEIEnemyCamp();

	int32 GetEnemies() const { return Enemies; }

protected:
	virtual void BeginPlay() override;

private:
	/** Set by tools/make_runmap.py from Data/laufkarte.json. */
	UPROPERTY(EditAnywhere, Category = "Eisenseelen")
	int32 Enemies = 4;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Ashes;
};
