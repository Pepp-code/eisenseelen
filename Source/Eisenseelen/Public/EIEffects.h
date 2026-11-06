#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIEffects.generated.h"

class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Where and how big an effect plays. Everything but the location may stay as it is. */
struct EISENSEELEN_API FEIEffectParams
{
	FVector Location = FVector::ZeroVector;

	/** Its forward is the way a gust blows or a spell flies, and a wall stands across it. */
	FRotator Rotation = FRotator::ZeroRotator;

	/** Most effects glow in this color, such as the color of the element. */
	FLinearColor Color = FLinearColor::White;

	/** The size it covers. Effects on an enemy take 100 times its scale. */
	float RadiusCm = 100.0f;

	/** How long a cloud or a field lasts. 0 keeps the length of the effect itself. */
	float Seconds = 0.0f;

	/** Where a bolt or a spark ends. */
	FVector Target = FVector::ZeroVector;

	/** The width of a cone, such as the gust. */
	float AngleDegrees = 0.0f;

	/** It moves along with this, such as a spell in flight or a burning enemy. */
	USceneComponent* AttachTo = nullptr;
};

/**
 * One visible effect of a spell, a reaction or a state, for the eye only: every machine plays its own and a dedicated
 * server none. Without art it is made of glowing placeholder particles. A Niagara system in Data/aussehen.json under
 * effekt.<name> plays instead, or on top while leuchtteile_behalten is true.
 */
UCLASS()
class EISENSEELEN_API AEIEffect : public AActor
{
	GENERATED_BODY()

public:
	AEIEffect();

	/** Plays it here. Null on a dedicated server and for a name that is not in GetNames. */
	static AEIEffect* Play(const UObject* WorldContext, FName Name, const FEIEffectParams& Params);

	/** Every effect there is. Each one has a line effekt.<name> in Data/aussehen.json. */
	static const TArray<FName>& GetNames();

	/** For tests: how often an effect of this name played since the last reset. */
	static int32 GetPlayCount(FName Name);
	static void ResetPlayCounts();

	FName GetEffectName() const { return EffectName; }

	/** Whether it runs until Stop, such as a field or a burning enemy. */
	bool IsLasting() const;

	/** Whether a Niagara system from Data/aussehen.json plays. */
	bool UsesModel() const { return Model != nullptr; }

	/** Whether the placeholder particles are gone for the model, so whatever stood for the effect may go too. */
	bool HidesPlaceholder() const { return UsesModel() && !bShowsPlaceholder; }

	/** How many placeholder parts show right now. */
	int32 CountShownParts() const;

	/** It makes nothing new and goes once what it made has faded. */
	void Stop();
	bool IsStopped() const { return bStopped; }

	virtual void Tick(float DeltaSeconds) override;

private:
	struct FParticle
	{
		FVector Position = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		/** How it moved in the last frame, so a streak points along its way even while it circles. */
		FVector Motion = FVector::ZeroVector;
		FRotator Turn = FRotator::ZeroRotator;
		float Age = 0.0f;
		float Life = 1.0f;
	};

	struct FEmitterState
	{
		int32 Index = 0;
		TArray<FParticle> Particles;
		TArray<UStaticMeshComponent*> Parts;
		UMaterialInstanceDynamic* Material = nullptr;
		float SpawnDebt = 0.0f;
	};

	struct FBolt
	{
		FVector From = FVector::ZeroVector;
		FVector To = FVector::ZeroVector;
		float Age = 0.0f;
		float NextJitterIn = 0.0f;
		TArray<UStaticMeshComponent*> Parts;
		UMaterialInstanceDynamic* Material = nullptr;
	};

	void Start(FName Name, const FEIEffectParams& InParams);
	void StartModel(UNiagaraSystem* System, const struct FEILookEntry& Entry);
	UStaticMeshComponent* MakePart(UStaticMesh* Mesh, const FLinearColor& Color, float Glow, UMaterialInstanceDynamic*& Material);
	void SpawnParticle(FEmitterState& State, int32 Slot = 0, int32 Slots = 0);
	void TickEmitter(FEmitterState& State, float DeltaSeconds);
	void StartBolt(const FVector& From, const FVector& To);
	void LayOutBolt(FBolt& Bolt) const;
	void TickBolts(float DeltaSeconds);
	void TickLayers(float DeltaSeconds);
	bool IsDone() const;
	FVector GetCenter() const;

	UFUNCTION()
	void HandleModelFinished(UNiagaraComponent* Finished);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> Model;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Flash;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Ring;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> Light;

	/** Keeps the parts and their materials that the plain lists below point to. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Kept;

	UMaterialInstanceDynamic* FlashMaterial = nullptr;
	UMaterialInstanceDynamic* RingMaterial = nullptr;
	TArray<FEmitterState> Emitters;
	TArray<FBolt> Bolts;
	FEIEffectParams Params;
	FName EffectName;
	int32 RecipeIndex = INDEX_NONE;
	TWeakObjectPtr<USceneComponent> Parent;
	float Age = 0.0f;
	float SpawnSeconds = 0.0f;
	float NextBoltIn = 0.0f;
	float LightShare = 0.0f;
	bool bAttached = false;
	bool bStopped = false;
	bool bShowsPlaceholder = true;
	bool bModelFinished = false;
	bool bModelStopped = false;
};
