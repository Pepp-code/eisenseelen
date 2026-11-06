#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EIWorldTools.generated.h"

class ALandscape;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/** An actor holding many copies of one static mesh (such as tree trunks, crowns, or bushes) in a single instanced component. */
UCLASS()
class EISENSEELEN_API AEIInstancedMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AEIInstancedMeshActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eisenseelen|Welt")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Instances;
};

/** Helpers for the map scripts in tools/. Python cannot make a landscape by itself, so it asks here. */
UCLASS()
class EISENSEELEN_API UEIWorldTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Editor only. Builds a landscape centred on the origin from heights in centimetres, row after row with X running fastest.
	 * There are ComponentsPerSide * QuadsPerComponent + 1 heights on each side. Returns null in a game or on wrong sizes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen|Welt", meta = (WorldContext = "WorldContext"))
	static ALandscape* BuildLandscape(UObject* WorldContext, const TArray<float>& HeightsCm, int32 ComponentsPerSide, int32 QuadsPerComponent, float QuadSizeCm, UMaterialInterface* Material, FName Tag);

	/**
	 * Editor only. Spawns an AEIInstancedMeshActor containing all Transforms in a single HierarchicalInstancedStaticMeshComponent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Eisenseelen|Welt", meta = (WorldContext = "WorldContext"))
	static AEIInstancedMeshActor* BuildInstancedMesh(UObject* WorldContext, UStaticMesh* Mesh, UMaterialInterface* Material, const TArray<FTransform>& Transforms, bool bCollide, int32 EndCullDistanceCm, FName Label, FName Tag);
};
