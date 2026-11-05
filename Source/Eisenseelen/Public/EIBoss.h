#pragma once

#include "CoreMinimal.h"
#include "EIBosses.h"
#include "EIEnemy.h"
#include "EIBoss.generated.h"

/**
 * A boss, a big grey body for now. It stands bound at its place until its Bannbruch is done, and nothing can touch it in
 * that time. Then it fights around that place: it does not follow anybody far away, and whoever leaves it alone for long
 * gives it time to recover. Its numbers stand in Data/bosse.json.
 */
UCLASS()
class EISENSEELEN_API AEIBoss : public AEIEnemy
{
	GENERATED_BODY()

public:
	AEIBoss();

	/** Between the deferred spawn and FinishSpawning: which boss of the table this one is. */
	void SetBossIndex(int32 Index) { BossIndex = Index; }

	const FEIBossValues* GetBossValues() const;

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsBound() const { return bBound; }

	/** Whether the ring of its stomp lies on the floor right now. */
	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsStompMarked() const { return GetGroundMark().bVisible && GetGroundMark().Shape == EEIMarkShape::Circle; }

	/** Server only. The Bannbruch is done: it fights from now on, with more life for more players. */
	void BreakFree(int32 PlayerCount);

	virtual bool IsBoss() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual bool LoadEnemyValues(FEIEnemyValues& OutValues, FString& OutError) override;
	virtual FLinearColor PickBaseColor() const override;
	virtual bool IsUntouchable() const override { return bBound; }
	virtual bool CanBeFrozen() const override { return false; }
	virtual void TickServer(float DeltaSeconds) override;

private:
	UFUNCTION()
	void OnRep_Bound();

	void LoadTable();
	bool CanStartStomp(const AActor* Target) const;
	void Stomp();

	UPROPERTY(Replicated)
	int32 BossIndex = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Bound)
	bool bBound = true;

	FEIBossTable Table;
	float StompLeft = 0.0f;
	double NextStompAt = 0.0;
};
