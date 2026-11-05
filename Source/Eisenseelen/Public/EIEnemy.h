#pragma once

#include "CoreMinimal.h"
#include "EIGroundMark.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "EIEnemy.generated.h"

class FJsonObject;
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
	Colossus,
	EmberSkull
};

constexpr int32 EIEnemyKindCount = 5;

/** What a spell is made of. Two elements on one target can react. */
UENUM()
enum class EEIElement : uint8
{
	Fire,
	Ice,
	Lightning
};

/** What two elements on one target set off. */
enum class EEIReaction : uint8
{
	None,
	Steam,
	Sparks
};

/** The name of a kind in the data files. */
EISENSEELEN_API const TCHAR* EIEnemyKindKey(EEIEnemyKind Kind);

/** The name of an element in the data files. False if the text is none. */
EISENSEELEN_API bool EIElementFromKey(const FString& Key, EEIElement& OutElement);

/** The color everything of this element glows in. */
EISENSEELEN_API FLinearColor EIElementColor(EEIElement Element);

/** The numbers of one kind of enemy from Data/gegner.json. */
struct EISENSEELEN_API FEIEnemyValues
{
	FString Name;
	float Life = 40.0f;
	float Threat = 1.0f;
	float SpeedMetersPerSecond = 4.5f;
	float Damage = 10.0f;
	float AttackFromMeters = 1.7f;
	float WindupSeconds = 0.5f;
	float WindupFollowSeconds = 0.0f;
	float HitZoneMeters = 2.2f;
	float HitWidthMeters = 2.4f;
	bool bMarksStrike = false;
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
	float Experience = 1.0f;
	float ChargeSpeedMetersPerSecond = 0.0f;
	float ChargeDistanceMeters = 0.0f;
	float ChargeWidthMeters = 0.0f;
	float ChargeFromMeters = 0.0f;
	float ChargeToMeters = 0.0f;
	float ChargeWindupSeconds = 0.0f;
	float ChargeDamage = 0.0f;
	float ChargeRecoverSeconds = 0.0f;
	float ChargePauseSeconds = 0.0f;
	bool bExplodes = false;
	float ExplosionEnemyDamage = 0.0f;
	float RetreatBelowMeters = 0.0f;

	static bool LoadFromProjectFile(EEIEnemyKind Kind, FEIEnemyValues& OutValues, FString& OutError);

	/** From a section with the keys of Data/gegner.json. OutValues only changes when all of it is valid. */
	static bool ReadFrom(const FJsonObject& Section, FEIEnemyValues& OutValues, FString& OutError);
};

/** Damage from a spell or a blast. It comes from everywhere at once, so a shield does not hold it. */
UCLASS()
class EISENSEELEN_API UEISpellDamage : public UDamageType
{
	GENERATED_BODY()
};

/**
 * An enemy, a grey capsule for now. It stands watch or hunts, finds its way to the nearest hero, winds up visibly and audibly, and only then
 * strikes what lies in front of it, so a hero who gets out of the way in time takes no damage. A parry or a heavy hit staggers it, which interrupts
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

	/** A shield bearer lowers its shield for a moment after its blow. */
	bool IsShieldDown() const { return bShieldDown; }

	/** Server only. Callable for preview pictures. */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen")
	void Stagger(float Seconds);

	/** Server only. How much of this stagger arrives depends on the kind of enemy, down to nothing at all. */
	void StaggerFromHeavyHit(float Seconds);

	bool IsDead() const;

	/** Bosses have their own place in the counting and the rewards. */
	virtual bool IsBoss() const { return false; }

	/** Server only. It gets stronger and bigger, stands out by its color, and brings a reward when it falls. */
	void MakeLeader(float LifeFactor, float DamageFactor, float ScaleFactor);

	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsLeader() const { return LeaderScale > 0.0f; }

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

	/** Server only. Stops it on the spot for a while. Whoever hits it hard then, breaks it. */
	void Freeze(float Seconds);
	void Unfreeze();
	bool IsFrozen() const { return bFrozen; }

	/** What lies on the floor right now. It decides who gets hit. */
	const FEIGroundMarkData& GetGroundMark() const { return GroundMark; }

	/** Whether the strip of a strike or a charge lies on the floor right now. */
	UFUNCTION(BlueprintPure, Category = "Eisenseelen")
	bool IsStripMarked() const { return GroundMark.bVisible && GroundMark.Shape == EEIMarkShape::Strip; }

	/** Who did this to a hero, for the note on the screen. Empty if nobody can be named. */
	static FString DescribeAttackOf(const AActor* Causer);

	/** Server only. Leaves the element on the enemy for a while, or sets off what it does with the one already there. */
	EEIReaction MarkElement(EEIElement Element, float WindowSeconds);

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Server only. The floor shows where the next attack lands. */
	void ShowGroundCircle(float RadiusCm, float FillSeconds);
	void ShowGroundCircleAt(const FVector& Location, float RadiusCm, float FillSeconds);
	void ShowGroundStrip(const FVector& Start, const FVector& Direction, float LengthCm, float WidthCm, float FillSeconds);
	void HideGroundMark();

	/** Where it stands on the floor, whatever its size. */
	FVector GetFloorSpot() const;

	/** Its feet and its head are at about the same height as the other one's, so that a blow can reach. */
	bool ReachesHeightOf(const AActor* Other) const;

	/** What the screen calls the attack that comes next. */
	void NameAttack(const TCHAR* Name) { AttackName = Name; }

	/** Where its numbers come from. */
	virtual bool LoadEnemyValues(FEIEnemyValues& OutValues, FString& OutError);
	virtual FLinearColor PickBaseColor() const;

	/** As long as this is true nothing can hurt it, slow it or wake it. */
	virtual bool IsUntouchable() const { return false; }
	virtual bool CanBeFrozen() const { return true; }

	virtual void TickServer(float DeltaSeconds);
	AActor* FindNearestHero() const;
	void StopWalking();
	void WakeUp(bool bTellNeighbours);
	void WalkHome(float DeltaSeconds);
	void SetTelegraphing(bool bNow);
	const FVector& GetHome() const { return Home; }
	bool IsChasing() const { return State == EState::Chase; }
	bool IsGoingHome() const { return State == EState::GoHome; }
	void GoHomeNow();
	void ArriveHome();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStrike();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastStompSound();

private:
	enum class EState : uint8
	{
		Chase,
		Windup,
		Recover,
		Stagger,
		Watch,
		GoHome,
		Frozen,
		ChargeWindup,
		Charging
	};

	UFUNCTION()
	void OnRep_Telegraphing();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastNoticed(bool bCallOut);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShieldBlocked();

	UFUNCTION()
	void OnRep_GroundMark();

	UFUNCTION()
	void OnRep_LeaderScale();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHurt();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplode(FVector_NetQuantize Location, float RadiusCm);

	void Steer(AActor* Target, float DeltaSeconds);
	void SteerStraight(const AActor* Target, float DeltaSeconds);
	bool NoticesHero(float DeltaSeconds);
	bool CanAttack(const AActor* Target) const;
	void ThrowBone();
	float ReduceByShield(float Amount, const AActor* DamageCauser, bool bGoesAround);
	void Explode(bool bByItself);
	void BackAway(const AActor* Target);
	void ShowHeldPart();
	void UpdateStatus(float DeltaSeconds);
	bool CanStartCharge(const AActor* Target) const;
	void StartChargeWindup(const AActor* Target);
	void UpdateCharge(float DeltaSeconds);
	void EndCharge();
	bool StandsOnPaths() const;
	FEIGroundMarkData MakeStrikeZone() const;
	void StartStrikeWindup(const AActor* Target);
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

	UPROPERTY(Replicated)
	bool bFrozen = false;

	UPROPERTY(Replicated)
	bool bShieldDown = false;

	UPROPERTY(ReplicatedUsing = OnRep_GroundMark)
	FEIGroundMarkData GroundMark;

	/** Every machine has its own, because it only is for the eye. */
	UPROPERTY(Transient)
	TObjectPtr<AEIGroundMark> ShownMark;

	/** Above 0 only for a leader. */
	UPROPERTY(ReplicatedUsing = OnRep_LeaderScale)
	float LeaderScale = 0.0f;

	FEIEnemyValues Values;
	const TCHAR* AttackName = TEXT("");
	FLinearColor BaseColor = FLinearColor::Gray;
	bool bNextHitBreaksShield = false;
	bool bExploded = false;
	float SlowRemaining = 0.0f;
	float BurnRemaining = 0.0f;
	float BurnPerSecond = 0.0f;
	float BurnTickRemaining = 0.0f;
	EEIElement ElementMark = EEIElement::Fire;
	float ElementMarkRemaining = 0.0f;
	float FrozenSeconds = 0.0f;
	double NextChargeAt = 0.0;
	FVector ChargeDirection = FVector::ForwardVector;
	float ChargeTravelledCm = 0.0f;
	double ChargeAlongBeforeCm = 0.0;
	TArray<TWeakObjectPtr<AActor>> ChargeHit;
	float RecoverFor = 0.0f;
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
	float ShieldDownBlend = 0.0f;
	float EmberPhase = 0.0f;
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
