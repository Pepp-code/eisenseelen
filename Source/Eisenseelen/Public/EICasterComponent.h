#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EICasterComponent.generated.h"

class AEIHeroCharacter;

UENUM()
enum class EEIProjectileKind : uint8
{
	Bolt,
	Fireball
};

/** The numbers of one projectile from Data/waffen.json. */
struct FEIProjectileValues
{
	float Damage = 40.0f;
	float RadiusMeters = 1.5f;
	float SpeedMetersPerSecond = 60.0f;
	float CooldownSeconds = 0.5f;
};

struct EISENSEELEN_API FEICasterValues
{
	FEIProjectileValues Bolt;
	FEIProjectileValues Fireball;
	float HoldForFireballSeconds = 0.4f;

	FEICasterValues();
	static bool LoadFromProjectFile(FEICasterValues& OutValues, FString& OutError);
};

/**
 * Fire magic. The arcanist's staff fires a bolt on a tap and a fireball on a hold, and both classes cast the fireball
 * with the spell key. The owner sees it at once, the server spawns the real projectile and checks the cooldowns.
 */
UCLASS()
class EISENSEELEN_API UEICasterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEICasterComponent();

	void LoadValues();
	const FEICasterValues& GetValues() const { return Values; }

	/** Owner only. */
	void PressStaff();
	void ReleaseStaff();
	void CastFireball();
	void Deactivate();

	float GetChargeFraction() const;
	float GetFireballCooldownFraction() const;
	float GetFireballCooldownSeconds() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerFire(EEIProjectileKind Kind, FVector_NetQuantize Target);

	AEIHeroCharacter* GetHero() const;
	void FireAsOwner(EEIProjectileKind Kind);
	void SpawnProjectile(EEIProjectileKind Kind, const FVector& Target);
	FVector FindAimPoint() const;

	FEICasterValues Values;

	bool bHolding = false;
	double HoldStartedAt = 0.0;

	// Each side keeps its own cooldowns: the owner to answer at once, the server to have the last word.
	double BoltReadyAt = 0.0;
	double FireballReadyAt = 0.0;
};
