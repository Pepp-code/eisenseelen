#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EICasterComponent.generated.h"

class AEIHeroCharacter;

UENUM()
enum class EEICast : uint8
{
	Bolt,
	ChargedFireball,
	SpellFireball
};

/** The numbers of one projectile from Data/waffen.json. */
struct FEIProjectileValues
{
	float Damage = 40.0f;
	float RadiusMeters = 1.5f;
	float SpeedMetersPerSecond = 60.0f;
	float ManaCost = 10.0f;
};

struct EISENSEELEN_API FEICasterValues
{
	FEIProjectileValues Bolt;
	FEIProjectileValues Fireball;
	float HoldForFireballSeconds = 0.4f;
	float SpellCooldownSeconds = 6.0f;

	FEICasterValues();
	static bool LoadFromProjectFile(FEICasterValues& OutValues, FString& OutError);
};

/**
 * Fire magic. The arcanist's staff fires a bolt on every press of the attack key and a fireball after a full charge of
 * the second key, without any wait but for mana. Both classes also cast the fireball with the spell key, which needs
 * neither charge nor mana but has a cooldown. The owner sees it at once, the server spawns the real projectile and
 * checks cooldown and mana.
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
	void FireBolt();
	void PressCharge();
	void ReleaseCharge();
	void CastFireball();
	void Deactivate();

	float GetChargeFraction() const;
	float GetSpellCooldownFraction() const;
	float GetSpellCooldownSeconds() const;

	/** The mana of the hero's class. It refills over time, and a class without mana pays nothing. */
	float GetMana() const;
	float GetMaxMana() const;
	void SpendMana(float Amount);
	void RefillMana();
	bool CanAffordFireball() const;

	/** How fast the mana comes back compared to the class's normal rate. */
	void SetManaFactor(float Factor);

	/** Seconds of the flash left after a cast failed for lack of mana. For the HUD. */
	float GetManaDeniedFlash() const { return ManaDeniedRemaining; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerFire(EEICast Kind, FVector_NetQuantize Target);

	AEIHeroCharacter* GetHero() const;
	const FEIProjectileValues& GetProjectile(EEICast Kind) const;
	float GetManaCost(EEICast Kind) const;
	bool CanAfford(float Cost) const;
	void FireAsOwner(EEICast Kind);
	void SpawnProjectile(EEICast Kind, const FVector& Target);
	FVector FindAimPoint() const;

	FEICasterValues Values;

	bool bHolding = false;
	double HoldStartedAt = 0.0;

	// Each side keeps its own cooldown and mana: the owner to answer at once, the server to have the last word.
	double SpellReadyAt = 0.0;
	float ManaAtStamp = 0.0f;
	double ManaStampedAt = 0.0;
	float ManaFactor = 1.0f;

	float ManaDeniedRemaining = 0.0f;
};
