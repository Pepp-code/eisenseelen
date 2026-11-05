#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EILaunchPad.generated.h"

class UBoxComponent;
class UEILookComponent;
class UStaticMeshComponent;

/** Throws a hero straight up as high as Data/welt.json says. Horizontal speed stays, so a running hero flies on. */
UCLASS()
class EISENSEELEN_API AEILaunchPad : public AActor
{
	GENERATED_BODY()

public:
	AEILaunchPad();

	float GetLaunchHeightMeters() const { return LaunchHeightMeters; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* Trigger, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Plate;

	/** The model from Data/aussehen.json over the blocks, if there is one. Collision stays with the blocks. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEILookComponent> Look;

	float LaunchHeightMeters = 6.0f;
};
