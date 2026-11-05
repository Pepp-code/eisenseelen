#include "EIHuntBoard.h"

#include "Components/StaticMeshComponent.h"
#include "EIGameState.h"
#include "EILookComponent.h"
#include "EIVisuals.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr int32 MaxSheets = 4;
	constexpr float RefreshSeconds = 0.1f;
	const FVector PostSizeCm(16.0f, 16.0f, 230.0f);
	const FVector PanelSizeCm(8.0f, 260.0f, 150.0f);
	constexpr float PanelBottomCm = 70.0f;
	const FVector SheetSizeCm(2.0f, 46.0f, 64.0f);
	constexpr float SheetGapCm = 14.0f;
	constexpr float ChosenSheetForwardCm = 6.0f;
	constexpr float ChosenSheetGrowth = 1.15f;
	constexpr float ChosenGlow = 1.6f;
	constexpr float MarkAboveCm = 280.0f;
	const FLinearColor WoodColor(0.2f, 0.13f, 0.08f);
	const FLinearColor PaperColor(0.82f, 0.76f, 0.6f);
}

AEIHuntBoard::AEIHuntBoard()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = RefreshSeconds;
	bReplicates = true;
	bAlwaysRelevant = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Look = CreateDefaultSubobject<UEILookComponent>(TEXT("Look"));
	Look->SetupAttachment(Root);

	const float PostY = PanelSizeCm.Y / 2.0f - PostSizeCm.Y;
	AddPart(TEXT("PostLeft"), Cube.Object, FVector(0.0f, -PostY, PostSizeCm.Z / 2.0f), PostSizeCm);
	AddPart(TEXT("PostRight"), Cube.Object, FVector(0.0f, PostY, PostSizeCm.Z / 2.0f), PostSizeCm);
	Panel = AddPart(TEXT("Panel"), Cube.Object, FVector(PostSizeCm.X / 2.0f, 0.0f, PanelBottomCm + PanelSizeCm.Z / 2.0f), PanelSizeCm);
	for (int32 Index = 0; Index < MaxSheets; ++Index)
	{
		UStaticMeshComponent* Sheet = AddPart(*FString::Printf(TEXT("Sheet%d"), Index), Cube.Object, FVector::ZeroVector, SheetSizeCm);
		Sheet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sheet->SetVisibility(false);
		Sheets.Add(Sheet);
	}
}

UStaticMeshComponent* AEIHuntBoard::AddPart(const TCHAR* Name, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(GetRootComponent());
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeScale3D(SizeCm / 100.0f);
	Part->SetCanEverAffectNavigation(false);
	return Part;
}

void AEIHuntBoard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIHuntBoard, ReachMeters);
}

void AEIHuntBoard::BeginPlay()
{
	Super::BeginPlay();

	TArray<USceneComponent*> Blocks;
	for (UActorComponent* Component : GetComponents())
	{
		UStaticMeshComponent* Part = Cast<UStaticMeshComponent>(Component);
		if (Part != nullptr && !Sheets.Contains(Part) && Part->CreationMethod == EComponentCreationMethod::Native)
		{
			EIVisuals::SetColor(Part, WoodColor);
			Blocks.Add(Part);
		}
	}
	// The letters stay as they are: they show which hunts are on offer.
	Look->Setup({TEXT("ort.jagdbrett")}, Blocks);
}

void AEIHuntBoard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (State == nullptr)
	{
		return;
	}
	const TArray<FEIHuntLetter>& Letters = State->GetHuntLetters();
	int32 Signature = Letters.Num();
	for (const FEIHuntLetter& Letter : Letters)
	{
		Signature = Signature * 7 + Letter.BossIndex + 1;
	}
	if (Signature == ShownCount && State->GetChosenLetter() == ShownChosen)
	{
		return;
	}
	ShownCount = Signature;
	ShownChosen = State->GetChosenLetter();

	// The letters hang side by side on the front, the chosen one a little bigger, a little further out and glowing in the color of its boss.
	const int32 Count = FMath::Min(Letters.Num(), Sheets.Num());
	const float Row = Count * SheetSizeCm.Y + FMath::Max(Count - 1, 0) * SheetGapCm;
	const float FrontX = PostSizeCm.X / 2.0f + PanelSizeCm.X / 2.0f + SheetSizeCm.X / 2.0f;
	for (int32 Index = 0; Index < Sheets.Num(); ++Index)
	{
		UStaticMeshComponent* Sheet = Sheets[Index];
		Sheet->SetVisibility(Index < Count);
		if (Index >= Count)
		{
			continue;
		}
		const bool bChosen = Index == ShownChosen;
		const float Y = -Row / 2.0f + SheetSizeCm.Y / 2.0f + Index * (SheetSizeCm.Y + SheetGapCm);
		Sheet->SetRelativeLocation(FVector(FrontX + (bChosen ? ChosenSheetForwardCm : 0.0f), Y, PanelBottomCm + PanelSizeCm.Z / 2.0f));
		Sheet->SetRelativeScale3D(SheetSizeCm * (bChosen ? ChosenSheetGrowth : 1.0f) / 100.0f);
		const FLinearColor Color = FMath::Lerp(PaperColor, Letters[Index].Color, bChosen ? 0.7f : 0.25f);
		EIVisuals::SetColor(Sheet, Color, bChosen ? ChosenGlow : 0.0f);
	}
}

bool AEIHuntBoard::IsInReach(const AActor* Actor) const
{
	return Actor != nullptr && FVector::Dist2D(Actor->GetActorLocation(), GetActorLocation()) <= ReachMeters * CentimetersPerMeter;
}

FVector AEIHuntBoard::GetMarkLocation() const
{
	return GetActorLocation() + FVector(0.0f, 0.0f, MarkAboveCm);
}
