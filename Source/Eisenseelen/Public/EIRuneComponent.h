#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIEnemy.h"
#include "EIRuneComponent.generated.h"

class AEIHeroCharacter;
class AEIRuneField;

/** One line of Data/runen.json: an item that gives the hero a spell on the E key. */
struct EISENSEELEN_API FEIRune
{
	FString Name;
	FString SpellName;
	bool bWall = false;
	EEIElement Element = EEIElement::Ice;
	float Damage = 0.0f;
	float CooldownSeconds = 10.0f;

	/** The width of a circle's radius, or the whole width of a wall. */
	float SizeMeters = 5.0f;

	/** How far from the hero the spell lands. 0 means around the hero. */
	float RangeMeters = 0.0f;

	/** 0 means one single burst. */
	float DurationSeconds = 0.0f;
	float TickSeconds = 0.5f;
	float FreezeSeconds = 0.0f;
	float SlowShare = 0.0f;
	float BurnPerSecond = 0.0f;
	float StatusSeconds = 0.0f;

	/** How fast it draws enemies to its middle. 0 for all but wind. */
	float PullMetersPerSecond = 0.0f;
};

/** Wind on an enemy with another element, or the other way round: a whirl of that element. */
struct EISENSEELEN_API FEIWhirlValues
{
	float RadiusMeters = 5.0f;
	float DurationSeconds = 3.0f;
	float TickSeconds = 0.5f;
	float PullMetersPerSecond = 6.0f;
	float FireDamage = 8.0f;
	float FireBurnPerSecond = 10.0f;
	float FireBurnSeconds = 3.0f;
	float FrostDamage = 5.0f;
	float FrostSlowShare = 0.5f;
	float FrostFreezeSeconds = 2.0f;
	float StormDamage = 18.0f;
};

struct EISENSEELEN_API FEIRuneTable
{
	TArray<FEIRune> Runes;
	float ReactionWindowSeconds = 2.0f;
	float SteamDamage = 80.0f;
	float SteamRadiusMeters = 6.0f;
	float SteamCloudSeconds = 3.0f;
	float ShatterDamage = 100.0f;
	float ShatterRadiusMeters = 5.0f;
	float SparkDamage = 30.0f;
	float SparkRadiusMeters = 8.0f;
	float SparkTargets = 4.0f;
	float SparkBurnPerSecond = 10.0f;
	float SparkBurnSeconds = 3.0f;
	FEIWhirlValues Whirl;

	/** INDEX_NONE if there is none of that name. */
	int32 FindByName(const FString& Name) const;

	/** OutTable only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEIRuneTable& OutTable, FString& OutError);
	static bool LoadFromProjectFile(FEIRuneTable& OutTable, FString& OutError);
};

/**
 * The rune a hero has equipped, and with it the spell on the E key. Which one that is follows from what the hero wears.
 * The owner sees the cast at once, the server checks the cooldown and spawns the real spell.
 */
UCLASS()
class EISENSEELEN_API UEIRuneComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIRuneComponent();

	void LoadValues();

	/** Tests may change it. */
	FEIRuneTable& GetTable() { return Table; }
	const FEIRuneTable& GetTable() const { return Table; }

	/** Null if there is none. */
	const FEIRune* GetRune() const;
	int32 GetRuneIndex() const { return RuneIndex; }

	/** A new rune is ready at once. INDEX_NONE means none at all. */
	void SetRune(int32 Index);

	/** Owner only. */
	void CastRune();

	float GetCooldownSeconds() const;
	float GetCooldownFraction() const;

	/** Server only. Marks the enemy with the element, and with a fitting element already on it a reaction breaks loose. */
	void HitWithElement(AEIEnemy* Enemy, EEIElement Element);

	/** Server only. A frozen enemy breaks, and the splinters hit those around it. */
	void Shatter(AEIEnemy* Enemy);

private:
	UFUNCTION(Server, Reliable)
	void ServerCast(FVector_NetQuantize Location, float Yaw);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSteam(FVector_NetQuantize Location);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBurst(FVector_NetQuantize Location, float RadiusCm, FLinearColor Color);

	AEIHeroCharacter* GetHero() const;
	void FindPlace(const FEIRune& Rune, FVector& OutLocation, float& OutYaw) const;
	void SpawnField(const FEIRune& Rune, const FVector& Location, float Yaw);
	void HurtAround(const FVector& Location, float RadiusCm, float Damage);
	void SteamExplosion(const AEIEnemy* Enemy);
	void SparksFly(AEIEnemy* Enemy);
	void StartWhirl(const AEIEnemy* Enemy, EEIElement Partner);

	FEIRuneTable Table;

	/** Server only. A new whirl does not start where one already turns. */
	TArray<TWeakObjectPtr<AEIRuneField>> Whirls;

	int32 RuneIndex = INDEX_NONE;

	// Each side keeps its own cooldown: the owner to answer at once, the server to have the last word.
	double ReadyAt = 0.0;
};
