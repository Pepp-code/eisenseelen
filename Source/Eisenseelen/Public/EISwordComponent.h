#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EISwordComponent.generated.h"

class AEIHeroCharacter;

UENUM()
enum class EEISwingKind : uint8
{
	Light1,
	Light2,
	Light3,
	Heavy
};

/** The numbers of one kind of swing from Data/waffen.json. */
struct FEISwingValues
{
	float DurationSeconds = 0.45f;
	float Damage = 35.0f;
	float ReachMeters = 2.8f;
	float BladeWidthMeters = 0.25f;
	float HitFrom = 0.2f;
	float HitTo = 0.65f;
	float MaxTargets = 5.0f;
	float TargetFalloff = 0.15f;
};

/** The numbers of the block from Data/waffen.json. */
struct FEIBlockValues
{
	float DamageReduction = 0.6f;
	float AngleDegrees = 120.0f;
	float ParryWindowSeconds = 0.2f;
	float ParryDamageReduction = 1.0f;
	float StaggerSeconds = 1.0f;
};

struct EISENSEELEN_API FEISwordValues
{
	FEISwingValues Light;
	FEISwingValues Heavy;
	FEIBlockValues Block;
	float ChargeSeconds = 0.6f;
	float ChainWindowSeconds = 0.5f;
	float HitStopSeconds = 0.05f;
	float CameraKickDegrees = 1.2f;
	float HeavyCameraKickDegrees = 3.0f;

	FEISwordValues();
	static bool LoadFromProjectFile(FEISwordValues& OutValues, FString& OutError);
};

/**
 * The sword. A press and release is a light swing in a chain of three, a long hold is a heavy swing. The owner sees
 * the swing at once. Hits only count on the server, along the blade's path and not through walls. Holding the second
 * key raises a block against hits from the front, whose first moment is a parry that staggers the attacker. An attack
 * goes first, the block comes back after it.
 */
UCLASS()
class EISENSEELEN_API UEISwordComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEISwordComponent();

	void LoadValues();
	const FEISwordValues& GetValues() const { return Values; }

	/** Owner only: the key went down or up. */
	void PressPrimary();
	void ReleasePrimary();
	void PressBlock();
	void ReleaseBlock();
	void Deactivate();

	/** Server only: what a block leaves of a hit from this causer. Without a fitting block that is all of it. */
	float BlockDamage(float Damage, AActor* Causer);

	/** Seconds of the note left after the block caught a hit, and whether that was a parry. For the HUD. */
	float GetBlockFlash() const { return BlockFlashRemaining; }
	bool WasParry() const { return bFlashIsParry; }

	bool IsSwinging() const { return bSwinging; }
	EEISwingKind GetSwingKind() const { return SwingKind; }
	bool IsHolding() const { return bHolding; }
	float GetChargeFraction() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerBeginSwing(EEISwingKind Kind);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHitFeedback(FVector_NetQuantize Location, bool bHeavy);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBlockFeedback(bool bParry);

	AEIHeroCharacter* GetHero() const;
	void StartOwnerSwing(EEISwingKind Kind);
	void BeginSwing(EEISwingKind Kind);
	void EndSwing();
	EEISwingKind NextLightSwing();
	void SweepBlade(float FromT, float ToT);
	void HitActor(AActor* Target, const FVector& Location);
	void UpdateBlockAsOwner(float DeltaTime);

	FEISwordValues Values;

	bool bHolding = false;
	double HoldStartedAt = 0.0;
	bool bBlockKeyHeld = false;
	float BlockLift = 0.0f;
	float BlockFlashRemaining = 0.0f;
	bool bFlashIsParry = false;
	bool bHasQueuedSwing = false;
	EEISwingKind QueuedSwing = EEISwingKind::Light1;
	bool bServerHasQueuedSwing = false;
	EEISwingKind ServerQueuedSwing = EEISwingKind::Light1;

	bool bSwinging = false;
	EEISwingKind SwingKind = EEISwingKind::Light1;
	float SwingElapsed = 0.0f;
	int32 NextChainIndex = 0;
	double LastSwingEndedAt = -100.0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> HitThisSwing;
};
