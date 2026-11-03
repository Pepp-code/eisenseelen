#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIEnemy.h"
#include "EIRuneComponent.generated.h"

class AEIHeroCharacter;

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
	float SlowShare = 0.0f;
	float BurnPerSecond = 0.0f;
	float StatusSeconds = 0.0f;
};

struct EISENSEELEN_API FEIRuneTable
{
	TArray<FEIRune> Runes;
	float ReactionWindowSeconds = 2.0f;
	float SteamDamage = 80.0f;
	float SteamRadiusMeters = 6.0f;

	/** OutTable only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEIRuneTable& OutTable, FString& OutError);
	static bool LoadFromProjectFile(FEIRuneTable& OutTable, FString& OutError);
};

/**
 * The rune a hero has equipped, and with it the spell on the E key. It is chosen before a run. The owner sees the cast at
 * once, the server checks the cooldown and spawns the real spell.
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

	/** Server only. A new rune is ready at once. */
	void SetRune(int32 Index);

	/** Owner only. */
	void CastRune();

	float GetCooldownSeconds() const;
	float GetCooldownFraction() const;

	/** Server only. Marks the enemy with the element, and with fire on ice or ice on fire the steam explodes. */
	void HitWithElement(AEIEnemy* Enemy, EEIElement Element);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION(Server, Reliable)
	void ServerCast(FVector_NetQuantize Location, float Yaw);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSteam(FVector_NetQuantize Location);

	UFUNCTION()
	void OnRep_RuneIndex();

	AEIHeroCharacter* GetHero() const;
	void FindPlace(const FEIRune& Rune, FVector& OutLocation, float& OutYaw) const;
	void SpawnField(const FEIRune& Rune, const FVector& Location, float Yaw);

	FEIRuneTable Table;

	UPROPERTY(ReplicatedUsing = OnRep_RuneIndex)
	int32 RuneIndex = 0;

	// Each side keeps its own cooldown: the owner to answer at once, the server to have the last word.
	double ReadyAt = 0.0;
};
