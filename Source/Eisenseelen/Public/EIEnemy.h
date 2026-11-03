#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EIEnemy.generated.h"

class UEIHealthComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** The numbers of the bone runner from Data/gegner.json. */
struct EISENSEELEN_API FEIEnemyValues
{
	float Life = 40.0f;
	float Threat = 1.0f;
	float SpeedMetersPerSecond = 4.5f;
	float Damage = 10.0f;
	float AttackFromMeters = 1.7f;
	float WindupSeconds = 0.5f;
	float HitZoneMeters = 2.2f;
	float RecoverSeconds = 0.6f;
	float HeavyHitStaggerFactor = 1.0f;

	static bool LoadFromProjectFile(FEIEnemyValues& OutValues, FString& OutError);
};

/**
 * The bone runner, a grey capsule for now. It finds its way to the nearest hero, winds up visibly and audibly, and only then
 * strikes, so a hero who moves away in time takes no damage. A parry or a heavy hit staggers it, which interrupts
 * everything it does. All of its decisions happen on the server.
 */
UCLASS()
class EISENSEELEN_API AEIEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AEIEnemy();

	bool IsTelegraphing() const { return bTelegraphing; }
	bool IsStaggering() const { return bStaggering; }

	/** Server only. Callable for preview pictures. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void Stagger(float Seconds);

	/** Server only. How much of this stagger arrives depends on the kind of enemy, down to nothing at all. */
	void StaggerFromHeavyHit(float Seconds);

	bool IsDead() const;

	/** Server only. A guard stays on the heroes' heels, however far they run. */
	void MarkAsGuard() { bGuard = true; }
	bool IsGuard() const { return bGuard; }

	UEIHealthComponent* GetHealth() const { return Health; }
	const FEIEnemyValues& GetValues() const { return Values; }
	void ApplyValues(const FEIEnemyValues& NewValues);

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	enum class EState : uint8
	{
		Chase,
		Windup,
		Recover,
		Stagger
	};

	UFUNCTION()
	void OnRep_Telegraphing();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHurt();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStrike();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie();

	void TickServer(float DeltaSeconds);
	AActor* FindNearestHero() const;
	void Steer(AActor* Target, float DeltaSeconds);
	void SteerStraight(const AActor* Target, float DeltaSeconds);
	bool StandsOnPaths() const;
	void StopWalking();
	void Strike();
	void HandleDied();
	void UpdateLook(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIHealthComponent> Health;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeadMaterial;

	UPROPERTY(ReplicatedUsing = OnRep_Telegraphing)
	bool bTelegraphing = false;

	UPROPERTY(Replicated)
	bool bStaggering = false;

	FEIEnemyValues Values;
	bool bGuard = false;
	EState State = EState::Chase;
	float StateTime = 0.0f;
	float StaggerSeconds = 0.0f;
	float StuckSeconds = 0.0f;
	float SidestepRemaining = 0.0f;
	float SidestepSign = 1.0f;
	float PathRetryRemaining = 0.0f;
	float HurtFlashRemaining = 0.0f;
	float TelegraphBlend = 0.0f;
	float StaggerBlend = 0.0f;
};

/** Bones flying apart when a runner dies. Purely for the eye, every machine spawns its own. */
UCLASS()
class EISENSEELEN_API AEIBoneDebris : public AActor
{
	GENERATED_BODY()

public:
	AEIBoneDebris();

protected:
	virtual void BeginPlay() override;
};
