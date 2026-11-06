#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIEngravings.h"
#include "EIItems.h"
#include "EIGearComponent.generated.h"

class AEIHeroCharacter;

/**
 * What a hero wears, and the sum of all bonuses: items, armour sets, attributes and omens land in the same numbers. Only the server
 * changes what is worn. Everybody who needs a number asks here, so that all of them agree.
 */
UCLASS()
class EISENSEELEN_API UEIGearComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIGearComponent();

	void LoadValues();
	const FEIItemTable& GetTable() const { return Table; }

	/** Server only. */
	void SetSetup(const FEIHeroSetup& NewSetup);
	const FEIHeroSetup& GetSetup() const { return Setup; }

	/** Null for an empty slot. */
	const FEIItem* GetWorn(EEISlot Slot) const;

	float GetStat(EEIStat Stat) const { return Stats[static_cast<int32>(Stat)]; }

	const FEIWeaponForm* GetWeaponForm() const;
	EEIElement GetWeaponElement() const;

	/** What the active engravings of the weapon add up to in this rule. */
	float GetRule(EEIEngravingRule Rule) const { return Rules[static_cast<int32>(Rule)]; }

	/** The names of the engravings that are active on the weapon. */
	TArray<FString> GetActiveEngravings() const;

	/** What the numbers mean together. Each is a factor on the base damage of what hits. */
	float GetSwordFactor() const;
	float GetShotFactor() const;
	float GetSpellFactor() const;
	float GetRuneFactor(EEIElement Element) const;
	float GetReactionFactor() const { return 1.0f + GetStat(EEIStat::ReactionDamage); }

	float GetStatusFactor() const { return 1.0f + GetStat(EEIStat::StatusDuration); }
	float GetCooldownFactor() const;
	float GetSpeedFactor() const;
	float GetManaCostFactor() const;

	/** The share of a hit that the armour swallows. */
	float GetArmorShare() const;

	/** Adds everything up again, and the hero passes it on. Whenever items or omens change. */
	void Recalculate();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_Setup();

	AEIHeroCharacter* GetHero() const;
	float GetElementBonus(EEIElement Element) const;

	const FEIEngravingForm* GetEngravingForm() const;

	FEIItemTable Table;
	FEIEngravingTable Engravings;

	UPROPERTY(ReplicatedUsing = OnRep_Setup)
	FEIHeroSetup Setup;

	float Stats[EIStatCount] = {};
	float Rules[EIEngravingRuleCount] = {};
};
