#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EIWorldTools.generated.h"

class ALandscape;
class UMaterialInterface;

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
};
