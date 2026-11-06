#include "EIWorldTools.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInterface.h"

namespace
{
	// With this height scale one landscape unit is 100 / 128 cm, so heights from -256 m to +256 m fit.
	constexpr float HeightScale = 100.0f;
	constexpr float UnitsPerCentimeter = 128.0f / HeightScale;
	constexpr int32 ZeroHeight = 32768;
	constexpr int32 SectionsPerComponent = 1;
}

AEIInstancedMeshActor::AEIInstancedMeshActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Instances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Instances"));
	Instances->SetMobility(EComponentMobility::Static);
	SetRootComponent(Instances);
}

ALandscape* UEIWorldTools::BuildLandscape(UObject* WorldContext, const TArray<float>& HeightsCm, int32 ComponentsPerSide, int32 QuadsPerComponent, float QuadSizeCm, UMaterialInterface* Material, FName Tag)
{
#if WITH_EDITOR
	UWorld* World = GEngine != nullptr ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	const int32 Side = ComponentsPerSide * QuadsPerComponent + 1;
	if (World == nullptr || World->IsGameWorld() || ComponentsPerSide < 1 || QuadsPerComponent < 1 || QuadSizeCm <= 0.0f || HeightsCm.Num() != Side * Side)
	{
		return nullptr;
	}

	TArray<uint16> Heights;
	Heights.SetNumUninitialized(HeightsCm.Num());
	for (int32 Index = 0; Index < HeightsCm.Num(); ++Index)
	{
		Heights[Index] = static_cast<uint16>(FMath::Clamp(ZeroHeight + FMath::RoundToInt32(HeightsCm[Index] * UnitsPerCentimeter), 0, MAX_uint16));
	}

	const double Half = (Side - 1) * QuadSizeCm / 2.0;
	ALandscape* Landscape = World->SpawnActor<ALandscape>(FVector(-Half, -Half, 0.0), FRotator::ZeroRotator);
	if (Landscape == nullptr)
	{
		return nullptr;
	}
	Landscape->LandscapeMaterial = Material;
	Landscape->SetActorRelativeScale3D(FVector(QuadSizeCm, QuadSizeCm, HeightScale));

	TMap<FGuid, TArray<uint16>> HeightsPerLayer;
	HeightsPerLayer.Add(FGuid(), MoveTemp(Heights));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> PaintPerLayer;
	PaintPerLayer.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());
	Landscape->Import(FGuid::NewGuid(), 0, 0, Side - 1, Side - 1, SectionsPerComponent, QuadsPerComponent / SectionsPerComponent, HeightsPerLayer, TEXT(""), PaintPerLayer, ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());
	Landscape->Tags.Add(Tag);
	Landscape->SetActorLabel(TEXT("Gelaende"));
	return Landscape;
#else
	return nullptr;
#endif
}

AEIInstancedMeshActor* UEIWorldTools::BuildInstancedMesh(UObject* WorldContext, UStaticMesh* Mesh, UMaterialInterface* Material, const TArray<FTransform>& Transforms, bool bCollide, int32 EndCullDistanceCm, FName Label, FName Tag)
{
#if WITH_EDITOR
	UWorld* World = GEngine != nullptr ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (World == nullptr || World->IsGameWorld() || Mesh == nullptr || Transforms.IsEmpty())
	{
		return nullptr;
	}

	AEIInstancedMeshActor* Actor = World->SpawnActor<AEIInstancedMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (Actor == nullptr || Actor->Instances == nullptr)
	{
		return nullptr;
	}

	Actor->SetActorEnableCollision(bCollide);
	UHierarchicalInstancedStaticMeshComponent* Comp = Actor->Instances;
	Comp->SetStaticMesh(Mesh);
	if (Material != nullptr)
	{
		Comp->SetMaterial(0, Material);
	}
	Comp->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Comp->SetCollisionEnabled(bCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Comp->SetCanEverAffectNavigation(bCollide);
	Comp->SetCullDistances(0, FMath::Max(0, EndCullDistanceCm));
	Comp->AddInstances(Transforms, false, true);
	Comp->BuildTreeIfOutdated(false, true);

	Actor->Tags.Add(Tag);
	Actor->SetActorLabel(Label.IsNone() ? TEXT("WaldInstanzen") : Label.ToString());
	return Actor;
#else
	return nullptr;
#endif
}
