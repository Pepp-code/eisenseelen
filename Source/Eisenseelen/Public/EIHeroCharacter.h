#pragma once

#include "CoreMinimal.h"
#include "EIHeroValues.h"
#include "EIPoses.h"
#include "GameFramework/Character.h"
#include "EIHeroCharacter.generated.h"

class UCameraComponent;
class UEICasterComponent;
class UEIGearComponent;
class UEIHealthComponent;
class UEIHeroBodyComponent;
class UEIHeroMovementComponent;
class UEIRuneComponent;
class AEISkyBarge;
class UEISwordComponent;
class UEIViewModelComponent;

UENUM()
enum class EEIHeroClass : uint8
{
	Bladewarden,
	Arcanist
};

/** What separates the classes, from Data/klassen.json. */
struct EISENSEELEN_API FEIClassValues
{
	float Life = 150.0f;
	float SpellCooldownFactor = 1.0f;
	float Mana = 0.0f;
	float ManaPerSecond = 0.0f;

	static bool LoadFromProjectFile(EEIHeroClass HeroClass, FEIClassValues& OutValues, FString& OutError);
};

/** The first-person hero: a capsule and a camera, the own hand in front of it, and a body the others see. */
UCLASS()
class EISENSEELEN_API AEIHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	explicit AEIHeroCharacter(const FObjectInitializer& ObjectInitializer);

	void ApplyValues(const FEIHeroValues& NewValues);
	const FEIHeroValues& GetValues() const { return Values; }
	UEIHeroMovementComponent* GetHeroMovement() const;

	UEIHealthComponent* GetHealth() const { return Health; }
	bool IsDead() const;

	/** Server only. A hit taken or dealt holds up the recovery of life for a while. */
	void NoteCombat();

	/** Seconds of the red flash left after taking a hit. For the HUD. */
	float GetHurtFlash() const { return HurtFlashRemaining; }

	/** Where the last hit came from, what it was, and how long the screen still shows it. For the HUD. */
	const FVector& GetLastHitFrom() const { return LastHitFrom; }
	const FString& GetLastHitNote() const { return LastHitNote; }
	float GetHitNoteLeft() const { return HitNoteRemaining; }

	UEISwordComponent* GetSword() const { return Sword; }
	UEICasterComponent* GetCaster() const { return Caster; }
	UEIRuneComponent* GetRunes() const { return Runes; }
	UEIGearComponent* GetGear() const { return Gear; }
	UEIHeroBodyComponent* GetBody() const { return Body; }

	/** Server only: everyone sees the hero do this, such as "hieb" or "treffer". The names are those of Data/aussehen.json. */
	void ShowLookEvent(FName Event);

	/** Passes the sum of all bonuses on to whoever needs it. */
	void OnGearChanged();

	/** Server only. Fits the life to the class and the gear, and keeps the share the hero has left. */
	void RefreshLife();

	EEIHeroClass GetHeroClass() const { return HeroClass; }
	const FEIClassValues& GetClassValues() const { return ClassValues; }

	/** Server only. Keeps the share of life the hero has left. */
	void SetHeroClass(EEIHeroClass NewClass);

	/** Owner only: the attack key, the second key and the spell key, routed to the class's weapon. */
	void PressPrimary();
	void ReleasePrimary();
	void PressSecondary();
	void ReleaseSecondary();
	void SetSprintKey(bool bHeld) { bSprintKeyHeld = bHeld; }

	bool PickUpSoulCargo(FName CargoId);
	void DropSoulCargo(bool bThrowForward);
	bool IsCarryingSoulCargo() const { return !CarriedCargoId.IsNone(); }
	FName GetCarriedCargoId() const { return CarriedCargoId; }

	/** Server only: from now on this hero stands on the barge and rides it. */
	void BoardBarge(AEISkyBarge* Barge);
	bool IsAboard() const { return bAboard; }

	/** A soul stone from a chest of a hollow armor. The HUD says so, and the boss work wants to know it. */
	bool CarriesSoulStone() const { return bCarriesSoulStone; }

	/** Server only. */
	void SetCarriesSoulStone(bool bCarries) { bCarriesSoulStone = bCarries; }
	const FVector& GetLocationBeforeBoarding() const { return LocationBeforeBoarding; }

	/** Server only: this hero stands here now, off the barge if it was on it, and looks that way. */
	void ArriveAt(const FVector& Location, float YawDegrees);
	void PressSpell();
	void PressRuneSpell();
	const FEIPoseLibrary& GetPoses() const { return Poses; }
	const FEIHandMotion& GetHandMotion() const { return HandMotion; }

	/** Null on everyone but the owner, and until the owner knows it is local. */
	UEIViewModelComponent* GetViewModel() const { return bViewModelBuilt ? ViewModel : nullptr; }

	void AddCameraKick(float Degrees);

	/** A press shortly before landing counts as a jump on landing, so it does not get lost to bad luck. */
	void PressJump();

	virtual void Landed(const FHitResult& Hit) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode = 0) override;

protected:
	virtual void BeginPlay() override;

	/** The one jump stays unused after a fall, it is only spent by jumping. Only the dive has none to offer. */
	virtual bool CanJumpWhileFalling() const override;

private:
	UFUNCTION()
	void OnRep_HeroClass();

	void ApplyClassToView();
	void ApplyWeaponToView();
	void LoadClassValues();

	UFUNCTION(Client, Unreliable)
	void ClientHurt(FVector_NetQuantize From, const FString& What, float Amount);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDied();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastLookEvent(FName Event);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBoardBarge(AEISkyBarge* Barge);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastArriveAt(const FVector& Location, float YawDegrees);

	void HandleDied();

	void UpdateCameraKick(float DeltaSeconds);
	void UpdateRecovery(float DeltaSeconds);
	void UpdateFieldOfView(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIViewModelComponent> ViewModel;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIHeroBodyComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIHealthComponent> Health;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEISwordComponent> Sword;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEICasterComponent> Caster;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIRuneComponent> Runes;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIGearComponent> Gear;

	UPROPERTY(ReplicatedUsing = OnRep_HeroClass)
	EEIHeroClass HeroClass = EEIHeroClass::Bladewarden;

	FEIClassValues ClassValues;

	FEIHeroValues Values;
	FEIPoseLibrary Poses;
	FEIHandMotion HandMotion;
	bool bViewModelBuilt = false;
	float CameraKickRemaining = 0.0f;
	float CameraKickDegrees = 0.0f;
	float HurtFlashRemaining = 0.0f;
	FVector LastHitFrom = FVector::ZeroVector;
	FString LastHitNote;
	float HitNoteRemaining = 0.0f;
	double LastCombatAt = -TNumericLimits<float>::Max();
	double BufferedJumpUntil = -1.0;
	bool bJumpOnGround = false;
	bool bSprintKeyHeld = false;
	bool bDamageableBeforeBoarding = true;
	FVector LocationBeforeBoarding = FVector::ZeroVector;

	UPROPERTY(Replicated)
	bool bAboard = false;

	UPROPERTY(Replicated)
	bool bCarriesSoulStone = false;

	UPROPERTY(Replicated)
	FName CarriedCargoId = NAME_None;

	FEISoulCargoDef CachedCargoDef;
};
