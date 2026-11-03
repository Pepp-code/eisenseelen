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

	/** Seconds of the red flash left after taking a hit. For the HUD. */
	float GetHurtFlash() const { return HurtFlashRemaining; }
	UEISwordComponent* GetSword() const { return Sword; }
	UEICasterComponent* GetCaster() const { return Caster; }

	EEIHeroClass GetHeroClass() const { return HeroClass; }
	const FEIClassValues& GetClassValues() const { return ClassValues; }

	/** Server only. Keeps the share of life the hero has left. */
	void SetHeroClass(EEIHeroClass NewClass);

	/** Owner only: the attack key and the spell key, routed to the class's weapon. */
	void PressPrimary();
	void ReleasePrimary();
	void PressSpell();
	const FEIPoseLibrary& GetPoses() const { return Poses; }

	/** Null on everyone but the owner, and until the owner knows it is local. */
	UEIViewModelComponent* GetViewModel() const { return bViewModelBuilt ? ViewModel : nullptr; }

	void AddCameraKick(float Degrees);

	/** Owner only. The server plays it for everyone. */
	void PressBattleCry();
	bool IsBattleCryReady() const;

	/** A press shortly before landing counts as a jump on landing, so it does not get lost to bad luck. */
	void PressJump();

	/** Called by the movement component once per dodge, not again when a move is replayed. */
	void OnDodgePerformed();

	virtual void Landed(const FHitResult& Hit) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode = 0) override;

protected:
	virtual void BeginPlay() override;

	/** The one jump stays unused after a dodge or a fall, it is only spent by jumping. */
	virtual bool CanJumpWhileFalling() const override;

private:
	UFUNCTION()
	void OnRep_HeroClass();

	void ApplyClassToView();
	void LoadClassValues();

	UFUNCTION(Server, Reliable)
	void ServerBattleCry();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBattleCry();

	UFUNCTION(Client, Unreliable)
	void ClientHurt();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDied();

	void HandleDied();

	void UpdateCameraKick(float DeltaSeconds);

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

	UPROPERTY(ReplicatedUsing = OnRep_HeroClass)
	EEIHeroClass HeroClass = EEIHeroClass::Bladewarden;

	FEIClassValues ClassValues;

	FEIHeroValues Values;
	FEIPoseLibrary Poses;
	bool bViewModelBuilt = false;
	float CameraKickRemaining = 0.0f;
	float CameraKickDegrees = 0.0f;
	double NextBattleCryAt = 0.0;
	float HurtFlashRemaining = 0.0f;
	double BufferedJumpUntil = -1.0;
	bool bJumpOnGround = false;
};
