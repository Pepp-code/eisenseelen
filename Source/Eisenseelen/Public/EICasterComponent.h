#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EICasterComponent.generated.h"

class AEIHeroCharacter;
class UEIGearComponent;
enum class EEIElement : uint8;

UENUM()
enum class EEICast : uint8
{
	Bolt,
	ChargedSpell,
	Spell
};

/** What flies. */
struct FEIProjectileValues
{
	float Damage = 40.0f;
	float RadiusMeters = 1.5f;
	float SpeedMetersPerSecond = 60.0f;
	float ManaCost = 10.0f;
};

/** The numbers of one spell from the section zauber of Data/waffen.json. */
struct FEISpellValues : public FEIProjectileValues
{
	FString Name;
	float CooldownSeconds = 6.0f;
	float SlowShare = 0.0f;
	float StatusSeconds = 0.0f;
	float FreezeSeconds = 0.0f;
	float RangeMeters = 0.0f;
	float Jumps = 0.0f;
	float JumpMeters = 0.0f;
	float JumpFalloff = 0.0f;
	float AngleDegrees = 0.0f;
	float PushMetersPerSecond = 0.0f;
	float StaggerSeconds = 0.0f;
};

struct EISENSEELEN_API FEICasterValues
{
	FEIProjectileValues Bolt;
	bool bBoltPierces = false;
	FEISpellValues Fireball;
	FEISpellValues IceLance;
	FEISpellValues ChainLightning;
	FEISpellValues Gust;
	float HoldForSpellSeconds = 0.4f;

	FEICasterValues();
	const FEISpellValues& GetSpell(EEIElement Element) const;
	static bool LoadFromProjectFile(FEICasterValues& OutValues, FString& OutError);
};

/**
 * Magic. The arcanist's staff fires a bolt on every press of the attack key and its spell after a full charge of the
 * second key, without any wait but for mana. Both classes also cast the spell of their weapon with the spell key, which
 * needs neither charge nor mana but has a cooldown. The element of the weapon decides which spell that is. The owner
 * sees it at once, the server spawns the real thing and checks cooldown and mana.
 */
UCLASS()
class EISENSEELEN_API UEICasterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEICasterComponent();

	void LoadValues();
	const FEICasterValues& GetValues() const { return Values; }

	/** Takes over how the weapon in the hand plays. */
	void ApplyGear(const UEIGearComponent& Gear);

	/** The spell of the weapon in the hand. */
	const FEISpellValues& GetSpell() const;

	/** Owner only. */
	void FireBolt();
	void PressCharge();
	void ReleaseCharge();
	void CastSpell();
	void Deactivate();

	float GetChargeFraction() const;
	float GetSpellCooldownFraction() const;
	float GetSpellCooldownSeconds() const;

	/** The mana of the hero's class. It refills over time, and a class without mana pays nothing. */
	float GetMana() const;
	float GetMaxMana() const;
	void SpendMana(float Amount);
	void RefillMana();
	bool CanAffordSpell() const;

	/** How fast the mana comes back compared to the class's normal rate. */
	void SetManaFactor(float Factor);

	/** Seconds of the flash left after a cast failed for lack of mana. For the HUD. */
	float GetManaDeniedFlash() const { return ManaDeniedRemaining; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerFire(EEICast Kind, FVector_NetQuantize Target);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastChain(const TArray<FVector_NetQuantize>& Points);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastGust(FVector_NetQuantize From, FVector_NetQuantizeNormal Direction);

	AEIHeroCharacter* GetHero() const;
	float GetManaCost(EEICast Kind) const;
	bool CanAfford(float Cost) const;
	void FireAsOwner(EEICast Kind);
	void SpawnProjectile(EEICast Kind, const FVector& Target);
	void CastChain(const FVector& Target, float Damage);
	void CastGust(const FVector& Target, float Damage);
	AActor* FindNextChainTarget(const FVector& From, const TArray<AActor*>& Struck) const;
	FVector FindAimPoint() const;

	FEICasterValues BaseValues;
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
