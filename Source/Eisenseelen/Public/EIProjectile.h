#pragma once

#include "CoreMinimal.h"
#include "EIEnemy.h"
#include "GameFramework/Actor.h"
#include "EIProjectile.generated.h"

class UPointLightComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/** What a projectile does, from the deferred spawn on. */
struct FEIProjectileSetup
{
	float Damage = 0.0f;
	float RadiusCm = 150.0f;
	float SpeedCm = 6000.0f;
	bool bBig = false;

	/** It flies through whoever it meets and hits each of them, and only a wall stops it. */
	bool bPierces = false;

	/** Only a real spell leaves its element on the target, not the small bolts of the staff. */
	bool bCarriesElement = false;
	EEIElement Element = EEIElement::Fire;
	float SlowShare = 0.0f;
	float SlowSeconds = 0.0f;

	/** Whoever it slows that is slowed already freezes for this long. */
	float FreezeSeconds = 0.0f;
};

/** A bolt or a spell in flight. It flies straight and bursts on the first thing it hits, hurting everything but heroes. */
UCLASS()
class EISENSEELEN_API AEIProjectile : public AActor
{
	GENERATED_BODY()

public:
	AEIProjectile();

	/** Between the deferred spawn and FinishSpawning. */
	void Configure(const FEIProjectileSetup& Setup);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void LifeSpanExpired() override;

private:
	UFUNCTION()
	void HandleStop(const FHitResult& Hit);

	UFUNCTION()
	void HandlePierce(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBurst(FVector_NetQuantize Location);

	void Burst(const FVector& Location);
	void HitTarget(AActor* Target);

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

	UPROPERTY(Replicated)
	bool bPierces = false;

	UPROPERTY(Replicated)
	EEIElement Element = EEIElement::Fire;

	float Damage = 0.0f;
	bool bCarriesElement = false;
	float SlowShare = 0.0f;
	float SlowSeconds = 0.0f;
	float FreezeSeconds = 0.0f;
	bool bBurst = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> Pierced;
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

	/** Grows just as fast, but stays for a while and only then fades. */
	void SetupCloud(float InRadiusCm, const FLinearColor& Color, float Seconds, float Strength);

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
	float LifeSeconds = 0.0f;
};

/** A short flash between two points, for a lightning that jumps. Purely for the eye, every machine spawns its own. */
UCLASS()
class EISENSEELEN_API AEIBeamFX : public AActor
{
	GENERATED_BODY()

public:
	AEIBeamFX();

	void Setup(const FVector& From, const FVector& To, const FLinearColor& Color);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Beam;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	float Age = 0.0f;
};
