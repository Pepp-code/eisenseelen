#pragma once

#include "CoreMinimal.h"
#include "EIItems.h"

/** What an engraving changes beyond the plain values: how a weapon plays. Each adds up like a value. */
enum class EEIEngravingRule : uint8
{
	Reach,
	ExtraTargets,
	LongerStagger,
	SpellDamage,
	SpellCooldown,
	ParryEmpowers,
	HeavyElement,
	BoltRadius,
	BoltSpeed,
	BoltPierces,
	BoltSlow,
	BoltSlowSeconds,
	BoltElement,
	StrongShotEvery,
	StrongShotBonus
};

constexpr int32 EIEngravingRuleCount = 15;

/** The name of a rule in Data/gravuren.json. */
EISENSEELEN_API const TCHAR* EIEngravingRuleKey(EEIEngravingRule Rule);
EISENSEELEN_API bool EIEngravingRuleFromKey(const FString& Key, EEIEngravingRule& OutRule);

struct FEIRuleLine
{
	EEIEngravingRule Rule = EEIEngravingRule::Reach;
	float Amount = 0.0f;
};

/** One choice of a tier. */
struct EISENSEELEN_API FEIEngraving
{
	FString Name;
	FString Text;
	TArray<FEIStatLine> Stats;
	TArray<FEIRuleLine> Rules;
};

/** A tier: one theme, its price, and the choices of which one can be active. */
struct EISENSEELEN_API FEIEngravingTier
{
	FString Theme;
	int32 Gold = 0;
	int32 Points = 0;
	TArray<FEIEngraving> Choices;
};

/** The engravings of a kind of weapon. Every weapon of this kind has the same ones, but each weapon buys its own. */
struct EISENSEELEN_API FEIEngravingForm
{
	FString FormKey;
	TArray<FEIEngravingTier> Tiers;

	/** Where an engraving of this name stands. False if there is none. */
	bool Find(const FString& Name, int32& OutTier, int32& OutChoice) const;
};

/** Data/gravuren.json. */
struct EISENSEELEN_API FEIEngravingTable
{
	TArray<FEIEngravingForm> Forms;

	/** Whose points pay for the engravings of a weapon of this element. */
	FString FactionNames[4];

	/** Null if this kind of weapon has no engravings yet. */
	const FEIEngravingForm* FindForm(const FString& FormKey) const;
	const FString& GetFaction(EEIElement Element) const { return FactionNames[static_cast<int32>(Element)]; }

	/** OutTable only changes when the text is valid. */
	static bool LoadFromString(const FString& Json, FEIEngravingTable& OutTable, FString& OutError);
	static bool LoadFromProjectFile(FEIEngravingTable& OutTable, FString& OutError);
};
