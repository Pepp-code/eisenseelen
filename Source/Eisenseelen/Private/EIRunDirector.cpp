#include "EIRunDirector.h"

#include "Dom/JsonObject.h"
#include "EIBeacon.h"
#include "EIDataFile.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHeroCharacter.h"
#include "EIHordeDirector.h"
#include "EISkyBarge.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	// Falling this far below the deck counts as having jumped.
	constexpr float LeftBargeBelowCm = 300.0f;
	constexpr float BargeHoverHeightCm = 140.0f;
}

bool FEIRunValues::LoadFromProjectFile(FEIRunValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("lauf.json"), OutError);
	FEIRunValues Loaded;
	if (!Root.IsValid()
		|| !FEIDataFile::ReadNumber(*Root, TEXT("leuchtfeuer_halten_s"), 0.1, 3600.0, Loaded.BeaconHoldSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("leuchtfeuer_reichweite_m"), 0.5, 100.0, Loaded.BeaconReachMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("leuchtfeuer_zone_m"), 1.0, 200.0, Loaded.BeaconZoneMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("barke_abstand_m"), 1.0, 100.0, Loaded.BargeDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("barke_abflug_tempo_m_pro_s"), 0.1, 100.0, Loaded.BargeDepartMetersPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_alle_s"), 0.1, 3600.0, Loaded.CorruptionEverySeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_stufen"), 1.0, 100.0, Loaded.CorruptionMaxStage, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_bedrohung_je_stufe"), 0.0, 10.0, Loaded.CorruptionThreatPerStage, OutError))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}

UEIRunDirector::UEIRunDirector()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEIRunDirector::LoadValues()
{
	FString Error;
	if (!FEIRunValues::LoadFromProjectFile(Values, Error))
	{
		FEIDataFile::ReportError(TEXT("lauf.json"), Error);
	}
}

void UEIRunDirector::SetHorde(bool bActive)
{
	AEIGameMode* GameMode = CastChecked<AEIGameMode>(GetOwner());
	GameMode->GetHorde()->SetHordeActive(bActive);
	GameMode->GetGameState<AEIGameState>()->SetHordeActive(bActive);
}

void UEIRunDirector::ResetRun()
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	TActorIterator<AEISkyBarge> FoundBarge(GetWorld());
	Barge = FoundBarge ? *FoundBarge : nullptr;
	LitBeacon = nullptr;
	HoldLeft = 0.0f;
	for (TActorIterator<AEIBeacon> It(GetWorld()); It; ++It)
	{
		It->SetLit(false);
	}
	State->ResetRunClock();
	State->SetBeaconHold(0.0f, false);
	SetCorruptionStage(0);
	if (Barge == nullptr)
	{
		State->SetRunPhase(EEIRunPhase::None);
		return;
	}
	Barge->ReturnToStart();
	State->SetRunPhase(EEIRunPhase::OnBarge);
}

void UEIRunDirector::Interact(AEIHeroCharacter* Hero)
{
	const EEIRunPhase Phase = GetWorld()->GetGameState<AEIGameState>()->GetRunPhase();
	if (Phase == EEIRunPhase::Exploring)
	{
		for (TActorIterator<AEIBeacon> It(GetWorld()); It; ++It)
		{
			if (It->IsInReach(Hero))
			{
				IgniteBeacon(*It);
				return;
			}
		}
	}
	else if (Phase == EEIRunPhase::BargeWaiting && !Hero->IsAboard() && Barge->IsInBoardingZone(Hero))
	{
		Hero->BoardBarge(Barge);
	}
}

void UEIRunDirector::IgniteBeacon(AEIBeacon* Beacon)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	LitBeacon = Beacon;
	Beacon->SetLit(true);
	HoldLeft = Values.BeaconHoldSeconds;
	State->SetBeaconHold(HoldLeft, true);
	State->SetRunPhase(EEIRunPhase::BeaconLit);

	// The barge comes down on the side towards the middle of the map, where it is open.
	const FVector BeaconLocation = Beacon->GetActorLocation();
	FVector Hover = BeaconLocation - BeaconLocation.GetSafeNormal2D() * Values.BargeDistanceMeters * CentimetersPerMeter;
	Hover.Z = BeaconLocation.Z + BargeHoverHeightCm;
	Barge->BeginApproach(Hover);
}

void UEIRunDirector::SetCorruptionStage(int32 Stage)
{
	const int32 MaxStage = FMath::Max(1, FMath::RoundToInt(Values.CorruptionMaxStage));
	GetWorld()->GetGameState<AEIGameState>()->SetCorruption(Stage, static_cast<float>(Stage) / MaxStage, Stage >= MaxStage);
}

void UEIRunDirector::UpdateCorruption()
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const int32 MaxStage = FMath::Max(1, FMath::RoundToInt(Values.CorruptionMaxStage));
	const int32 Stage = FMath::Min(MaxStage, FMath::FloorToInt(State->GetRunSeconds() / FMath::Max(Values.CorruptionEverySeconds, KINDA_SMALL_NUMBER)));
	if (Stage > State->GetCorruptionStage())
	{
		SetCorruptionStage(Stage);
	}
}

void UEIRunDirector::UpdateThreatBudget(EEIRunPhase Phase, int32 HeroCount)
{
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	UEIHordeDirector* Horde = CastChecked<AEIGameMode>(GetOwner())->GetHorde();
	const FEIDirectorValues& Director = Horde->GetValues().Director;
	// The blood moon knows no quiet phases.
	const bool bFullShare = State->IsBloodMoon() || Phase != EEIRunPhase::Exploring;
	const float PhaseShare = bFullShare ? Director.BeaconShare : Director.ExploringShare;
	const float PlayerShare = Director.PlayerShare[FMath::Clamp(HeroCount, 1, 4) - 1];
	const float CorruptionFactor = 1.0f + Values.CorruptionThreatPerStage * State->GetCorruptionStage();
	Horde->SetThreatBudget(FMath::Min(Director.FullThreat * PlayerShare * PhaseShare * CorruptionFactor, Director.PeakThreat), PhaseShare);
}

void UEIRunDirector::HoldBeacon(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	const bool bHeld = LivingHeroes.ContainsByPredicate([this](const AEIHeroCharacter* Hero) { return LitBeacon->IsInZone(Hero); });
	if (bHeld)
	{
		HoldLeft = FMath::Max(0.0f, HoldLeft - DeltaTime);
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetBeaconHold(HoldLeft, bHeld);
	Barge->SetApproachFraction(1.0f - HoldLeft / FMath::Max(Values.BeaconHoldSeconds, KINDA_SMALL_NUMBER));
	if (HoldLeft <= 0.0f)
	{
		State->SetRunPhase(EEIRunPhase::BargeWaiting);
	}
}

void UEIRunDirector::LeaveOnceAllAboard(const TArray<AEIHeroCharacter*>& LivingHeroes)
{
	const bool bAllAboard = !LivingHeroes.ContainsByPredicate([](const AEIHeroCharacter* Hero) { return !Hero->IsAboard(); });
	if (bAllAboard)
	{
		AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
		State->StopRunClock();
		State->SetRunPhase(EEIRunPhase::Escaped);
		Barge->Depart(Values.BargeDepartMetersPerSecond * CentimetersPerMeter);
		SetHorde(false);
	}
}

void UEIRunDirector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Not in BeginPlay: only now has every actor of the map begun play.
	if (!bPrepared)
	{
		bPrepared = true;
		ResetRun();
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const EEIRunPhase Phase = State->GetRunPhase();
	if (Phase == EEIRunPhase::None || Phase == EEIRunPhase::Escaped || Phase == EEIRunPhase::Failed)
	{
		return;
	}

	bool bAnyHero = false;
	TArray<AEIHeroCharacter*> LivingHeroes;
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		bAnyHero = true;
		if (!It->IsDead())
		{
			LivingHeroes.Add(*It);
		}
	}
	if (LivingHeroes.IsEmpty())
	{
		if (bAnyHero)
		{
			State->StopRunClock();
			State->SetRunPhase(EEIRunPhase::Failed);
			SetHorde(false);
		}
		return;
	}

	if (Phase != EEIRunPhase::OnBarge)
	{
		UpdateCorruption();
		UpdateThreatBudget(Phase, LivingHeroes.Num());
	}

	switch (Phase)
	{
	case EEIRunPhase::OnBarge:
		for (const AEIHeroCharacter* Hero : LivingHeroes)
		{
			if (Hero->GetActorLocation().Z < Barge->GetDeckHeight() - LeftBargeBelowCm)
			{
				State->StartRunClock();
				State->SetRunPhase(EEIRunPhase::Exploring);
				SetHorde(true);
				break;
			}
		}
		break;

	case EEIRunPhase::BeaconLit:
		HoldBeacon(LivingHeroes, DeltaTime);
		break;

	case EEIRunPhase::BargeWaiting:
		LeaveOnceAllAboard(LivingHeroes);
		break;

	default:
		break;
	}
}
