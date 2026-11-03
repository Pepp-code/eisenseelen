#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EIEnemy.generated.h"

class UEIHealthComponent;
class UMaterialInstanceDynamic;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

/** The kinds of enemies. Their numbers stand in Data/gegner.json, and the groups they walk in in Data/gruppen.json. */
UENUM()
enum class EEIEnemyKind : uint8
{
	Runner,
	Thrower,
	ShieldBearer,
	Colossus
};

constexpr int32 EIEnemyKindCount = 4;

/** What a spell is made of. Two elements on one target can react. */
enum class EEIElement : uint8
{
	Fire,
	Ice,
	Lightning
};

/** The name of a kind in the data files. */
EISENSEELEN_API const TCHAR* EIEnemyKindKey(EEIEnemyKind Kind);

/** The numbers of one kind of enemy from Data/gegner.json. */
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
	float SightMeters = 25.0f;
	float AlarmMeters = 12.0f;
	float GiveUpMeters = 50.0f;
	float StayAwakeSeconds = 6.0f;
	float Scale = 1.0f;
	float TurnDegreesPerSecond = 720.0f;
	float ShieldDamageFactor = 1.0f;
	float ShieldAngleDegrees = 120.0f;
	float ThrowSpeedMetersPerSecond = 0.0f;

	static bool LoadFromProjectFile(EEIEnemyKind Kind, FEIEnemyValues& OutValues, FString& OutError);
};

/**
 * An enemy, a grey capsule for now. It stands watch or hunts, finds its way to the nearest hero, winds up visibly and audibly, and only then
 * strikes, so a hero who moves away in time takes no damage. A parry or a heavy hit staggers it, which interrupts
 * everything it does. All of its decisions happen on the server.
 */
UCLASS()
class EISENSEELEN_API AEIEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AEIEnemy();

	/** Between a deferred spawn and FinishSpawning. An enemy spawned the plain way is a bone runner. */
	void SetKind(EEIEnemyKind NewKind) { Kind = NewKind; }
	EEIEnemyKind GetKind() const { return Kind; }

	/** Server only. The next hit goes through a shield. */
	void BreakShieldWithNextHit() { bNextHitBreaksShield = true; }

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

	/** Server only. It stands here and only comes when it notices a hero. An enemy without this call hunts right away. */
	void StandWatch(const FVector& NewHome);

	/** Whether it is not after a hero at the moment. */
	bool IsWatching() const { return State == EState::Watch || State == EState::GoHome; }
	bool HasHome() const { return bHasHome; }

	UEIHealthComponent* GetHealth() const { return Health; }
	const FEIEnemyValues& GetValues() const { return Values; }
	void ApplyValues(const FEIEnemyValues& NewValues);

	/** Server only. What a spell does on top of its damage. */
	void ApplySlow(float Share, float Seconds);
	void ApplyBurn(float PerSecond, float Seconds);
	bool IsSlowed() const { return bSlowed; }
	bool IsBurning() const { return bBurning; }

	/** Server only. Leaves the element on the enemy for a while. True if it reacts with fire on ice or ice on fire. */
	bool MarkElement(EEIElement Element, float WindowSeconds);

	/** Server only. More life and more damage, for enemies that come later in a run. */
	void ApplyStrength(float Factor);

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
		Stagger,
		Watch,
		GoHome
	};

	UFUNCTION()
	void OnRep_Telegraphing();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastNoticed(bool bCallOut);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShieldBlocked();

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
	bool NoticesHero(float DeltaSeconds);
	void WakeUp(bool bTellNeighbours);
	void WalkHome(float DeltaSeconds);
	bool CanAttack(const AActor* Target) const;
	void ThrowBone();
	float ReduceByShield(float Amount, const AActor* DamageCauser);
	void ShowHeldPart();
	void UpdateStatus(float DeltaSeconds);
	bool StandsOnPaths() const;
	void StopWalking();
	void Strike();
	void HandleDied();
	void UpdateLook(float DeltaSeconds);
	void UpdatePose(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Head;

	/** The shield of a shield bearer, the bone of a thrower. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Held;

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

	UPROPERTY(Replicated)
	EEIEnemyKind Kind = EEIEnemyKind::Runner;

	UPROPERTY(Replicated)
	bool bSlowed = false;

	UPROPERTY(Replicated)
	bool bBurning = false;

	FEIEnemyValues Values;
	FLinearColor BaseColor = FLinearColor::Gray;
	bool bNextHitBreaksShield = false;
	float SlowRemaining = 0.0f;
	float BurnRemaining = 0.0f;
	float BurnPerSecond = 0.0f;
	float BurnTickRemaining = 0.0f;
	EEIElement ElementMark = EEIElement::Fire;
	float ElementMarkRemaining = 0.0f;
	bool bGuard = false;
	bool bHasHome = false;
	FVector Home = FVector::ZeroVector;
	double AwakeUntil = 0.0;
	float NoticeCheckRemaining = 0.0f;
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
	float WalkBlend = 0.0f;
	float WalkPhase = 0.0f;
	float IdlePhase = 0.0f;
	float StrikeBlend = 0.0f;
	float RecoilBlend = 0.0f;
	float NoticeBlend = 0.0f;
};

/** Bones flying apart when a runner dies. Purely for the eye, every machine spawns its own. */
UCLASS()
class EISENSEELEN_API AEIBoneDebris : public AActor
{
	GENERATED_BODY()

public:
	AEIBoneDebris();

	/** Between the deferred spawn and FinishSpawning. */
	void Setup(const FLinearColor& InColor, float InScale);

protected:
	virtual void BeginPlay() override;

private:
	FLinearColor Color = FLinearColor::Gray;
	float Size = 1.0f;
};

/** A bone a thrower throws. It flies straight, and whoever does not step aside in time gets hit. */
UCLASS()
class EISENSEELEN_API AEIThrownBone : public AActor
{
	GENERATED_BODY()

public:
	AEIThrownBone();

	/** Between the deferred spawn and FinishSpawning. */
	void Launch(float InDamage, float SpeedCm);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleStop(const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Bone;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Flight;

	float Damage = 0.0f;
};
