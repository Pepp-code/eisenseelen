#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UMeshComponent;
class USoundBase;
class UStaticMesh;
class UStaticMeshComponent;
class UWorld;

/** Small helpers for the grey-box look: parts from the engine's basic shapes, colors and sounds by name. */
namespace EIVisuals
{
	EISENSEELEN_API UStaticMesh* GetCube();
	EISENSEELEN_API UStaticMesh* GetSphere();
	EISENSEELEN_API UStaticMesh* GetCylinder();

	/** Gives a mesh the material M_Farbe with its own color. */
	EISENSEELEN_API UMaterialInstanceDynamic* SetColor(UMeshComponent* Mesh, const FLinearColor& Color, float Glow = 0.0f);

	/** Gives a mesh the additive material M_Glut. */
	EISENSEELEN_API UMaterialInstanceDynamic* SetGlow(UMeshComponent* Mesh, const FLinearColor& Color, float Strength);

	/** Turns the sky, the sun and the fog of the map towards the blood moon, from 0 for a clear day to 1. */
	EISENSEELEN_API void ApplyBloodMoonLook(UWorld* World, float Share);

	/** Plays S_<Name> from /Game/Audio. A missing sound stays silent and logs a warning once. */
	EISENSEELEN_API void PlaySound2D(const UObject* WorldContext, const TCHAR* Name, float Volume = 1.0f);
	EISENSEELEN_API void PlaySoundAt(const UObject* WorldContext, const TCHAR* Name, const FVector& Location, float Volume = 1.0f);
}
