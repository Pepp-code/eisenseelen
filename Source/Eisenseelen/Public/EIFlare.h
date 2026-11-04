#pragma once

#include "CoreMinimal.h"
#include "EIRunDirector.h"
#include "GameFramework/Actor.h"
#include "EIFlare.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/**
 * The signal that calls the barge: a light that climbs up and stays in the sky, with a circle on the ground around the
 * place it was fired from. Somebody has to stay in that circle until the barge is there.
 */
UCLASS()
class EISENSEELEN_API AEIFlare : public AActor
{
	GENERATED_BODY()

public:
	AEIFlare();

	bool IsInZone(const AActor* Actor) const;

	/** Where the light is right now. */
	FVector GetLightLocation() const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void ShowRise(float Share);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Spark;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Trail;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ZoneDisc;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> Light;

	FEIRunValues Values;
	float RisenSeconds = 0.0f;
};
