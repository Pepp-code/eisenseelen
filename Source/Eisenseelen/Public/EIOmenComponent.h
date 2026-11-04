#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIItems.h"
#include "EIOmenComponent.generated.h"

class AEIHeroCharacter;

/** One line of Data/omen.json. */
struct EISENSEELEN_API FEIOmen
{
	FString Name;
	EEIStat Effect = EEIStat::Speed;
	float Amount = 0.0f;
	bool bForBladewarden = true;
	bool bForArcanist = true;

	/** What it does, in words for the player. */
	FString Describe() const;
};

struct EISENSEELEN_API FEIOmenTable
{
	TArray<FEIOmen> Omens;

	/** OutTable only changes when the whole text is valid. */
	static bool LoadFromString(const FString& Json, FEIOmenTable& OutTable, FString& OutError);
	static bool LoadFromProjectFile(FEIOmenTable& OutTable, FString& OutError);
};

/**
 * The omens a hero has found in this run. They end with the run, because a new run brings a new hero. Each player has
 * their own: only the server hands them out, and only the owner learns about them.
 */
UCLASS()
class EISENSEELEN_API UEIOmenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIOmenComponent();

	void LoadValues();
	const FEIOmenTable& GetTable() const { return Table; }

	/** Server only: one that fits the hero's class, if possible one the hero does not have yet. */
	void GrantRandomOmen();

	/** Server only. */
	void AddOmen(int32 Index);

	int32 CountOf(int32 Index) const;
	int32 CountAll() const { return Held.Num(); }

	/** The sum of all held omens with this effect. */
	float GetBonus(EEIStat Effect) const;

	/** The same as a factor: a bonus of 0.25 is 1.25. */
	float GetFactor(EEIStat Effect) const { return 1.0f + GetBonus(Effect); }

	/** Seconds of the note left after finding an omen, and which one that was. For the HUD. */
	float GetFoundNote() const { return FoundNoteRemaining; }
	int32 GetLastFound() const { return Held.IsEmpty() ? INDEX_NONE : Held.Last(); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_Held();

	AEIHeroCharacter* GetHero() const;

	FEIOmenTable Table;

	UPROPERTY(ReplicatedUsing = OnRep_Held)
	TArray<int32> Held;

	int32 KnownCount = 0;
	float FoundNoteRemaining = 0.0f;
};
