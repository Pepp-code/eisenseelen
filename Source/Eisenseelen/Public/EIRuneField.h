#pragma once

#include "CoreMinimal.h"
#include "EIEnemy.h"
#include "GameFramework/Actor.h"
#include "EIRuneField.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;
struct FEIRune;

/**
 * What a rune spell leaves in the world: a circle or a wall of its element. It hits everything in it, once for a burst
 * and again and again for a lasting spell. Only the server hurts anyone, everyone sees it.
 */
UCLASS()
class EISENSEELEN_API AEIRuneField : public AActor
{
	GENERATED_BODY()

public:
	AEIRuneField();

	/** Between the deferred spawn and FinishSpawning. */
	void Configure(const FEIRune& Rune, float InDamage);

	bool IsWall() const { return bWall; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	void Pulse();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Shape;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(Replicated)
	bool bWall = false;

	UPROPERTY(Replicated)
	float SizeCm = 500.0f;

	UPROPERTY(Replicated)
	float DurationSeconds = 0.0f;

	UPROPERTY(Replicated)
	FLinearColor Color = FLinearColor::White;

	float Damage = 0.0f;
	float TickSeconds = 0.5f;
	float FreezeSeconds = 0.0f;
	float SlowShare = 0.0f;
	float BurnPerSecond = 0.0f;
	float StatusSeconds = 0.0f;
	EEIElement Element = EEIElement::Ice;
	float Age = 0.0f;
	float NextPulseIn = 0.0f;
};
