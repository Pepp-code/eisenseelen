#include "EIVisuals.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Eisenseelen.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"

namespace
{
	/** How loud every effect is in total, set from the player's settings. */
	float EffectsVolume = 1.0f;

	USoundBase* FindSound(const TCHAR* Name)
	{
		// Null entries remember missing sounds, so each one only warns once.
		static TMap<FString, TWeakObjectPtr<USoundBase>> Loaded;
		static TSet<FString> Missing;
		const FString Key(Name);
		if (const TWeakObjectPtr<USoundBase>* Found = Loaded.Find(Key))
		{
			if (Found->IsValid())
			{
				return Found->Get();
			}
		}
		if (Missing.Contains(Key))
		{
			return nullptr;
		}
		const FString Path = FString::Printf(TEXT("/Game/Audio/S_%s.S_%s"), Name, Name);
		USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path);
		if (Sound == nullptr)
		{
			UE_LOG(LogEisenseelen, Warning, TEXT("The sound %s is missing"), *Path);
			Missing.Add(Key);
			return nullptr;
		}
		Loaded.Add(Key, Sound);
		return Sound;
	}
}

UStaticMesh* EIVisuals::GetCube()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
}

UStaticMesh* EIVisuals::GetSphere()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
}

UStaticMesh* EIVisuals::GetCylinder()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
}

UMaterialInstanceDynamic* EIVisuals::SetColor(UMeshComponent* Mesh, const FLinearColor& Color, float Glow)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Farbe.M_Farbe"));
	UMaterialInstanceDynamic* Instance = Base != nullptr ? UMaterialInstanceDynamic::Create(Base, Mesh) : nullptr;
	if (Instance != nullptr)
	{
		Instance->SetVectorParameterValue(TEXT("Farbe"), Color);
		Instance->SetScalarParameterValue(TEXT("Leuchten"), Glow);
		Mesh->SetMaterial(0, Instance);
	}
	return Instance;
}

UMaterialInstanceDynamic* EIVisuals::SetGlow(UMeshComponent* Mesh, const FLinearColor& Color, float Strength)
{
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Glut.M_Glut"));
	UMaterialInstanceDynamic* Instance = Base != nullptr ? UMaterialInstanceDynamic::Create(Base, Mesh) : nullptr;
	if (Instance != nullptr)
	{
		Instance->SetVectorParameterValue(TEXT("Farbe"), Color);
		Instance->SetScalarParameterValue(TEXT("Leuchten"), Strength);
		Mesh->SetMaterial(0, Instance);
	}
	return Instance;
}

void EIVisuals::ApplyBloodMoonLook(UWorld* World, float Share)
{
	const FLinearColor ClearSky(0.175287f, 0.409607f, 1.0f);
	const FLinearColor BloodSky(1.0f, 0.2f, 0.12f);
	const FLinearColor ClearSun(FLinearColor::White);
	const FLinearColor BloodSun(1.0f, 0.3f, 0.2f);
	const FLinearColor ClearFog(0.45f, 0.55f, 0.75f);
	const FLinearColor BloodFog(0.5f, 0.06f, 0.03f);
	constexpr float ClearSunLight = 10.0f;
	constexpr float BloodSunLight = 4.0f;
	constexpr float ClearFogDensity = 0.02f;
	constexpr float BloodFogDensity = 0.08f;

	for (TActorIterator<ASkyAtmosphere> It(World); It; ++It)
	{
		It->GetComponent()->SetRayleighScattering(FMath::Lerp(ClearSky, BloodSky, Share));
	}
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		It->GetLightComponent()->SetLightColor(FMath::Lerp(ClearSun, BloodSun, Share));
		It->GetLightComponent()->SetIntensity(FMath::Lerp(ClearSunLight, BloodSunLight, Share));
	}
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		It->GetComponent()->SetFogInscatteringColor(FMath::Lerp(ClearFog, BloodFog, Share));
		It->GetComponent()->SetFogDensity(FMath::Lerp(ClearFogDensity, BloodFogDensity, Share));
	}
}

void EIVisuals::SetWorldAtmosphereLightFactor(UWorld* World, float LightFactor)
{
	if (!World)
	{
		return;
	}
	constexpr float BaseSunLight = 10.0f;
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		It->GetLightComponent()->SetIntensity(BaseSunLight * LightFactor);
	}
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		It->GetComponent()->SetFogDensity(0.02f * (1.0f / FMath::Max(0.1f, LightFactor)));
	}
}

void EIVisuals::SetEffectsVolume(float Volume)
{
	EffectsVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
}

void EIVisuals::PlaySound2D(const UObject* WorldContext, const TCHAR* Name, float Volume)
{
	if (USoundBase* Sound = FindSound(Name))
	{
		UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume * EffectsVolume);
	}
}

void EIVisuals::PlaySoundAt(const UObject* WorldContext, const TCHAR* Name, const FVector& Location, float Volume)
{
	if (USoundBase* Sound = FindSound(Name))
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, Volume * EffectsVolume);
	}
}
