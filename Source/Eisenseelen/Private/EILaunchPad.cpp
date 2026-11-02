#include "EILaunchPad.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIHeroCharacter.h"
#include "EIVisuals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float PadSizeCm = 300.0f;
	constexpr float TriggerHeightCm = 40.0f;
	constexpr float PlateThicknessCm = 6.0f;
	const FLinearColor PadColor(1.0f, 0.45f, 0.05f);
	constexpr float PadGlow = 1.5f;
}

AEILaunchPad::AEILaunchPad()
{
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetBoxExtent(FVector(PadSizeCm / 2.0f, PadSizeCm / 2.0f, TriggerHeightCm));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	SetRootComponent(TriggerBox);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Plate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));
	Plate->SetupAttachment(TriggerBox);
	Plate->SetStaticMesh(Cube.Object);
	Plate->SetRelativeLocation(FVector(0.0f, 0.0f, -TriggerHeightCm));
	Plate->SetRelativeScale3D(FVector(PadSizeCm / 100.0f, PadSizeCm / 100.0f, PlateThicknessCm / 100.0f));
	Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEILaunchPad::BeginPlay()
{
	Super::BeginPlay();

	FString Error;
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("welt.json"), Error);
	if (!Root.IsValid() || !FEIDataFile::ReadNumber(*Root, TEXT("sprungfeld_hoehe_m"), 0.5, 50.0, LaunchHeightMeters, Error))
	{
		FEIDataFile::ReportError(TEXT("welt.json"), Error);
	}
	EIVisuals::SetColor(Plate, PadColor, PadGlow);
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEILaunchPad::HandleOverlap);
}

void AEILaunchPad::HandleOverlap(UPrimitiveComponent* Trigger, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(OtherActor);
	if (Hero == nullptr)
	{
		return;
	}
	const float Gravity = FMath::Abs(Hero->GetCharacterMovement()->GetGravityZ());
	const float LaunchSpeed = FMath::Sqrt(2.0f * Gravity * LaunchHeightMeters * CentimetersPerMeter);
	Hero->LaunchCharacter(FVector(0.0f, 0.0f, LaunchSpeed), false, true);
	if (Hero->IsLocallyControlled())
	{
		EIVisuals::PlaySound2D(this, TEXT("Sprungfeld"));
	}
}
