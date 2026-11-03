#pragma once

#include "CoreMinimal.h"
#include "EIHeroValues.h"
#include "EIPoses.h"
#include "GameFramework/Character.h"
#include "EIHeroCharacter.generated.h"

class UCameraComponent;
class UEICasterComponent;
class UEIHealthComponent;
class UEIHeroMovementComponent;
class UEIOmenComponent;
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

/** The first-person hero. There is no model yet: a capsule and a camera. */
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
	UEISwordComponent* GetSword() const { return Sword; }
	UEICasterComponent* GetCaster() const { return Caster; }
	UEIOmenComponent* GetOmens() const { return Omens; }
	UEIRuneComponent* GetRunes() const { return Runes; }

	/** Called on the server and the owner when the set of omens changed. */
	void OnOmensChanged();

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

	/** Server only: from now on this hero stands on the barge and rides it. */
	void BoardBarge(AEISkyBarge* Barge);
	bool IsAboard() const { return bAboard; }
	void PressSpell();
	void PressRuneSpell();
	const FEIPoseLibrary& GetPoses() const { return Poses; }

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
	void LoadClassValues();

	UFUNCTION(Client, Unreliable)
	void ClientHurt();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDied();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBoardBarge(AEISkyBarge* Barge);

	void HandleDied();

	void UpdateCameraKick(float DeltaSeconds);
	void UpdateRecovery(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIViewModelComponent> ViewModel;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIHealthComponent> Health;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEISwordComponent> Sword;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEICasterComponent> Caster;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIOmenComponent> Omens;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIRuneComponent> Runes;

	UPROPERTY(ReplicatedUsing = OnRep_HeroClass)
	EEIHeroClass HeroClass = EEIHeroClass::Bladewarden;

	FEIClassValues ClassValues;

	FEIHeroValues Values;
	FEIPoseLibrary Poses;
	bool bViewModelBuilt = false;
	float CameraKickRemaining = 0.0f;
	float CameraKickDegrees = 0.0f;
	float HurtFlashRemaining = 0.0f;
	double LastCombatAt = -TNumericLimits<float>::Max();
	double BufferedJumpUntil = -1.0;
	bool bJumpOnGround = false;
	bool bSprintKeyHeld = false;
	bool bAboard = false;
};
