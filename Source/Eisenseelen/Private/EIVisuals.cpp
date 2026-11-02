#include "EIVisuals.h"

#include "Components/MeshComponent.h"
#include "Eisenseelen.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"

namespace
{
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

void EIVisuals::PlaySound2D(const UObject* WorldContext, const TCHAR* Name, float Volume)
{
	if (USoundBase* Sound = FindSound(Name))
	{
		UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume);
	}
}

void EIVisuals::PlaySoundAt(const UObject* WorldContext, const TCHAR* Name, const FVector& Location, float Volume)
{
	if (USoundBase* Sound = FindSound(Name))
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, Volume);
	}
}
