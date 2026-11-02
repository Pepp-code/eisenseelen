#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EITrainingDummy.generated.h"

class UCapsuleComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTextRenderComponent;

/** A target that never dies. It shows every hit's damage, so swings can be judged at a glance. */
UCLASS()
class EISENSEELEN_API AEITrainingDummy : public AActor
{
	GENERATED_BODY()

public:
	AEITrainingDummy();

	float GetLastDamage() const { return LastDamage; }
	int32 GetHitCount() const { return HitCount; }
	void ResetHits();

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_HitCount();

	void ShowHit();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> DamageText;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Replicated)
	float LastDamage = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_HitCount)
	int32 HitCount = 0;

	float FlashRemaining = 0.0f;
};
