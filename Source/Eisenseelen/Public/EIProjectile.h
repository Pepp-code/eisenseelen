#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIProjectile.generated.h"

class UPointLightComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
struct FEIProjectileValues;

/** A fire bolt or fireball. It flies straight and bursts on the first thing it hits, hurting everything but heroes. */
UCLASS()
class EISENSEELEN_API AEIProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEIProjectile();

	/** Between the deferred spawn and FinishSpawning. */
	void Configure(const FEIProjectileValues& Values, bool bInBig);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void LifeSpanExpired() override;

private:
	UFUNCTION()
	void HandleStop(const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBurst(FVector_NetQuantize Location);

	void Burst(const FVector& Location);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Ball;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Flight;

	UPROPERTY(Replicated)
	float RadiusCm = 150.0f;

	UPROPERTY(Replicated)
	bool bBig = false;

	float Damage = 0.0f;
	bool bBurst = false;
};

/** The expanding flash of a burst. Purely for the eye, every machine spawns its own. */
UCLASS()
class EISENSEELEN_API AEIExplosionFX : public AActor
{
	GENERATED_BODY()

public:
	AEIExplosionFX();

	void Setup(float InRadiusCm);
	void Setup(float InRadiusCm, const FLinearColor& Color);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Ball;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	float RadiusCm = 150.0f;
	float Age = 0.0f;
};
