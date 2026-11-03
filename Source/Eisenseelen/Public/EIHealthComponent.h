#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIHealthComponent.generated.h"

/** Life points. Only the server changes them, and the numbers replicate to everyone. */
UCLASS()
class EISENSEELEN_API UEIHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIHealthComponent();

	void Reset(float NewMaxHealth);

	/** Server only. More life at once, and more room for it. */
	void AddMaxHealth(float Amount);
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	bool IsDead() const { return Health <= 0.0f; }

	/** Server only. Returns the damage that really counted. OnDied fires once, on the hit that kills. */
	float ApplyDamage(float Amount);

	FSimpleMulticastDelegate OnDied;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	float Health = 100.0f;

	UPROPERTY(Replicated)
	float MaxHealth = 100.0f;
};
