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

	/** Whether its summoned group still shields it, so that nothing gets through. */
	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsShielded() const { return bShielded; }

	/** Whether it sleeps in a grave and nobody sees it until the kettle is full. */
	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool SleepsInGrave() const;

	/** Whether the ring of its stomp lies on the floor right now. */
	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsStompMarked() const { return Move == EMove::Stomp && GetGroundMark().bVisible && GetGroundMark().Shape == EEIMarkShape::Circle; }

	/** Whether it is calling its group right now. */
	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsSummoning() const { return Move == EMove::Summon; }

	/** Whether it is about to vanish and come up where the ring lies. */
	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsVanishing() const { return Move == EMove::Vanish; }

	/** Server only. The Bannbruch is done: it fights from now on, with more life for more players. */
	void BreakFree(int32 PlayerCount);

	/** For tests and preview pictures: its next summon or vanish starts as soon as it can. */
	void HurryMoves();

	virtual bool IsBoss() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual bool LoadEnemyValues(FEIEnemyValues& OutValues, FString& OutError) override;
	virtual FLinearColor PickBaseColor() const override;
	virtual bool IsUntouchable() const override { return bBound || bShielded; }
	virtual bool CanBeFrozen() const override { return false; }
	virtual void TickServer(float DeltaSeconds) override;

private:
	enum class EMove : uint8
	{
		None,
		Stomp,
		Summon,
		Vanish
	};

	UFUNCTION()
	void OnRep_Bound();

	UFUNCTION()
	void OnRep_Shielded();

	void LoadTable();
	void ApplyGraveLook();
	bool UpdateMove(float DeltaSeconds);
	void FinishMove();
	bool CanStartStomp(const AActor* Target) const;
	void StartStomp();
	void Stomp();
	void StartSummon();
	void Summon();
	void UpdateShield();
	bool StartVanish();
	void Vanish();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Ward;

	UPROPERTY(Replicated)
	int32 BossIndex = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Bound)
	bool bBound = true;

	UPROPERTY(ReplicatedUsing = OnRep_Shielded)
	bool bShielded = false;

	FEIBossTable Table;
	EMove Move = EMove::None;
	float MoveLeft = 0.0f;
	double NextStompAt = 0.0;
	double NextSummonAt = 0.0;
	double NextVanishAt = 0.0;
	double ShieldStartedAt = 0.0;
	FVector VanishTarget = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AEIEnemy>> Summons;
};
