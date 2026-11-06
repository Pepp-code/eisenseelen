#pragma once

#include "CoreMinimal.h"
#include "EIEnemy.h"
#include "GameFramework/Actor.h"
#include "EIRuneField.generated.h"

class AEIEffect;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
struct FEIRune;
struct FEIWhirlValues;

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
	void Configure(const FEIRune& Rune, float InDamage, float StatusFactor);

	/** A whirl of wind and the partner element. Between the deferred spawn and FinishSpawning. */
	void ConfigureWhirl(EEIElement Partner, const FEIWhirlValues& Whirl, float ReactionFactor, float StatusFactor);

	bool IsWall() const { return bWall; }
	bool IsWhirl() const { return bWhirl; }
	EEIElement GetElement() const { return Element; }
	float GetRadiusCm() const { return SizeCm / 2.0f; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Pulse();
	void CollectTargets(TArray<AActor*>& OutTargets) const;

	/** Which effect of Data/aussehen.json shows it. */
	FName GetEffectName() const;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStrikes(const TArray<FVector_NetQuantize>& Targets);

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

	/** Wind stands up as a column, the other elements lie flat on the floor. */
	UPROPERTY(Replicated)
	bool bTall = false;

	/** A whirl comes from a reaction, so it starts no further reactions. */
	UPROPERTY(Replicated)
	bool bWhirl = false;

	UPROPERTY(Replicated)
	EEIElement Element = EEIElement::Ice;

	/** What everyone sees of it, such as the frost of a nova or the flames of a wall. */
	TWeakObjectPtr<AEIEffect> Look;
	float PullCm = 0.0f;
	float EndFreezeSeconds = 0.0f;
	float Damage = 0.0f;
	float TickSeconds = 0.5f;
	float FreezeSeconds = 0.0f;
	float SlowShare = 0.0f;
	float BurnPerSecond = 0.0f;
	float StatusSeconds = 0.0f;
	float Age = 0.0f;
	float NextPulseIn = 0.0f;
};
