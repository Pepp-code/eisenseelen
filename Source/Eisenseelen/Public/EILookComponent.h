#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EILookComponent.generated.h"

class UAnimInstance;
class UAnimSequenceBase;
class UMaterialInstanceDynamic;
class UMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
struct FEILookEntry;

/**
 * Puts the real model from Data/aussehen.json over the grey-box placeholder of one thing and lets it move.
 * Without a model in the file nothing changes, so every actor keeps working with its blocks.
 * Collision always stays with the placeholder: the model only changes what is seen.
 */
UCLASS()
class EISENSEELEN_API UEILookComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UEILookComponent();

	/** Who sees the model and whether it casts a shadow. Call before Setup. */
	void SetViewOptions(bool bInOnlyOwnerSee, bool bInOwnerNoSee, bool bInCastShadow);

	/**
	 * Uses the first of the keys that names a model. It hides the placeholders, and the glow parts too unless the
	 * entry keeps them. Calling it again first gives the old placeholders back, so it also works when the keys change.
	 */
	void Setup(const TArray<FString>& Keys, const TArray<USceneComponent*>& Placeholders, const TArray<USceneComponent*>& GlowParts = TArray<USceneComponent*>());

	/** Extra models worn over the main one, such as armour. Each key with a model adds one piece, the others add nothing. */
	void SetPieces(const TArray<FString>& Keys);

	bool HasModel() const { return Model != nullptr; }
	UMeshComponent* GetModel() const { return Model; }
	USkeletalMeshComponent* GetSkeletalModel() const;
	const FString& GetModelKey() const { return ModelKey; }

	/** The socket of the entry, where a figure carries this thing. waffe_r when the entry names none. */
	FName GetSocket() const { return Socket; }

	/** Whether this moment, or what stands in for it, has an animation. */
	bool HasAnimation(FName Event) const { return Resolve(Event) != nullptr; }

	/** Hangs Child on a socket of the skeleton. False, and nothing changes, without a skeleton or without that socket. */
	bool AttachToModelSocket(USceneComponent* Child, FName SocketName) const;

	/** Chooses stehen, laufen, rennen or springen. Speeds in centimetres per second. */
	void SetMotion(float SpeedCm, float RunFromCm, bool bInAir);

	/** Plays the animation of one moment, such as "schlag" or "treffer", and goes back to the loop. "tod" stays. */
	void PlayEvent(FName Event);

	/** For places: on plays "aktiv" once and keeps its last pose, off goes back to "stehen". */
	void ShowActive(bool bOn);

	/** Freezes the animation where it is, for example while the thing is frozen. */
	void SetPaused(bool bPaused);

	/** A coloured shimmer over the whole model, such as the red before a strike. Below 0.01 it is off. */
	void SetStatus(const FLinearColor& Color, float Strength);

	/**
	 * Leaves the model behind as a body that plays "tod" and vanishes after Seconds, for things that disappear when they die.
	 * False without a skeleton or without a "tod" animation, then the caller shows its own end.
	 */
	bool LeaveCorpse(float Seconds);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Clear();
	void ApplyPieces();
	UMeshComponent* CreateModel(const FEILookEntry& Entry, const FString& Key, USceneComponent* Parent, FName ParentSocket);
	void ApplyViewOptions(UMeshComponent* Mesh) const;
	void LoadAnimations(const FEILookEntry& Entry, const FString& Key);
	UAnimSequenceBase* Resolve(FName Event) const;
	void PlayLoop();
	void PlayOnce(UAnimSequenceBase* Animation, bool bHold);
	void UseAnimBlueprintAgain();
	void WriteAnimValues(float Speed, bool bInAir);
	void ApplyOverlay(UMeshComponent* Mesh) const;

	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> Model;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> Pieces;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> HiddenParts;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UAnimSequenceBase>> Animations;

	UPROPERTY(Transient)
	TObjectPtr<UClass> AnimClass;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Overlay;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> PlayingLoop;

	TArray<FString> PieceKeys;
	TArray<FString> SetupKeys;
	TArray<TWeakObjectPtr<USceneComponent>> SetupParts;
	bool bSetUp = false;
	FString ModelKey;
	FName Socket = TEXT("waffe_r");
	FName LoopEvent = TEXT("stehen");
	float OnceSecondsLeft = 0.0f;
	float LastSpeed = 0.0f;
	bool bLastInAir = false;
	bool bHolding = false;
	bool bWantActive = false;
	bool bDead = false;
	bool bOverlayOn = false;
	bool bOnlyOwnerSee = false;
	bool bOwnerNoSee = false;
	bool bCastShadow = true;
};

/** The body a model with a skeleton leaves behind: it plays its death, lies still and vanishes. Only seen on this machine. */
UCLASS()
class EISENSEELEN_API AEILookCorpse : public AActor
{
	GENERATED_BODY()

public:
	AEILookCorpse();

	void Setup(const USkeletalMeshComponent* From, UAnimSequenceBase* DeathAnimation, float Seconds);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh;
};
