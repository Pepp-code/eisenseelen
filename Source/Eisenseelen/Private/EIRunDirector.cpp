#include "EIRunDirector.h"

#include "Dom/JsonObject.h"
#include "EIAltar.h"
#include "EIBeacon.h"
#include "EIChest.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIEnemyCamp.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHeroCharacter.h"
#include "EIHordeDirector.h"
#include "EIOmenComponent.h"
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
		|| !FEIDataFile::ReadNumber(*Root, TEXT("lager_faktor"), 0.0, 100.0, Loaded.CampFactor, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("lager_radius_m"), 0.5, 100.0, Loaded.CampRadiusMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("lager_nachschub_ab_abstand_m"), 0.0, 1000.0, Loaded.CampRefillFromMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("truhen_pro_lauf"), 0.0, 1000.0, Loaded.ChestsPerRun, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("truhe_reichweite_m"), 0.5, 100.0, Loaded.ChestReachMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("truhe_zone_m"), 1.0, 100.0, Loaded.ChestZoneMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("truhe_waechter_abstand_m"), 1.0, 100.0, Loaded.ChestGuardDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("ritual_altare"), 0.0, 100.0, Loaded.RitualAltars, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("altar_halten_s"), 0.1, 3600.0, Loaded.AltarHoldSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("altar_zone_m"), 1.0, 100.0, Loaded.AltarZoneMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("leuchtfeuer_halten_s"), 0.1, 3600.0, Loaded.BeaconHoldSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("leuchtfeuer_reichweite_m"), 0.5, 100.0, Loaded.BeaconReachMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("leuchtfeuer_zone_m"), 1.0, 200.0, Loaded.BeaconZoneMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("barke_abstand_m"), 1.0, 100.0, Loaded.BargeDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("barke_abflug_tempo_m_pro_s"), 0.1, 100.0, Loaded.BargeDepartMetersPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_alle_s"), 0.1, 3600.0, Loaded.CorruptionEverySeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_stufen"), 1.0, 100.0, Loaded.CorruptionMaxStage, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_gegner_staerke_je_stufe"), 0.0, 10.0, Loaded.CorruptionStrengthPerStage, OutError))
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
	if (!FEIEnemyGroups::LoadFromProjectFile(Groups, Error))
	{
		FEIDataFile::ReportError(TEXT("gruppen.json"), Error);
	}
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
	WaveLeft = 0.0f;
	ReinforcementLeft = 0.0f;
	LastStage = 0;
	CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetEnemyStrength(1.0f);

	PrepareChests();

	AltarOrder.Reset();
	AltarsBroken = 0;
	for (TActorIterator<AEIAltar> It(GetWorld()); It; ++It)
	{
		It->SetState(EEIAltarState::Dormant);
		AltarOrder.Insert(*It, FMath::RandRange(0, AltarOrder.Num()));
	}
	const int32 AltarsThisRun = FMath::Min(FMath::RoundToInt(Values.RitualAltars), AltarOrder.Num());
	AltarOrder.SetNum(AltarsThisRun);
	State->SetAltarProgress(0, AltarsThisRun);
	AltarHoldLeft = Values.AltarHoldSeconds;
	State->SetAltarHold(AltarHoldLeft, false);
	if (AltarOrder.Num() > 0)
	{
		AltarOrder[0]->SetState(EEIAltarState::Active);
	}
	if (Barge == nullptr)
	{
		State->SetRunPhase(EEIRunPhase::None);
		return;
	}
	Barge->ReturnToStart();
	State->SetRunPhase(EEIRunPhase::OnBarge);
}

void UEIRunDirector::PrepareChests()
{
	TArray<AEIChest*> Chests;
	for (TActorIterator<AEIChest> It(GetWorld()); It; ++It)
	{
		Chests.Insert(*It, FMath::RandRange(0, Chests.Num()));
	}
	const int32 InUse = FMath::RoundToInt(Values.ChestsPerRun);
	ChestsInUse.Reset();
	for (int32 Index = 0; Index < Chests.Num(); ++Index)
	{
		Chests[Index]->PrepareForRun(Index < InUse);
		if (Index < InUse)
		{
			ChestsInUse.Add(Chests[Index]);
		}
	}
}

float UEIRunDirector::GetPlayerFactor(int32 HeroCount) const
{
	// All numbers in the files are for one player.
	const FEIDirectorValues& Director = CastChecked<AEIGameMode>(GetOwner())->GetHorde()->GetValues().Director;
	return Director.PlayerShare[FMath::Clamp(HeroCount, 1, 4) - 1] / FMath::Max(Director.PlayerShare[0], KINDA_SMALL_NUMBER);
}

void UEIRunDirector::FillCamp(AEIEnemyCamp* Camp, const FString& Size, int32 HeroCount)
{
	const FEIEnemyGroup* Group = Groups.Pick(Size, GetWorld()->GetGameState<AEIGameState>()->GetCorruptionStage());
	if (Group == nullptr)
	{
		return;
	}
	UEIHordeDirector* Horde = CastChecked<AEIGameMode>(GetOwner())->GetHorde();
	const float Factor = Values.CampFactor * GetPlayerFactor(HeroCount);
	const TArray<AEIEnemy*> Members = Horde->SpawnGroup(*Group, Factor, Camp->GetActorLocation(), Values.CampRadiusMeters * CentimetersPerMeter);
	for (AEIEnemy* Member : Members)
	{
		Member->StandWatch(Member->GetActorLocation());
	}
	Camp->SetMembers(Members);
}

void UEIRunDirector::RefillEmptyCamps(const TArray<AEIHeroCharacter*>& LivingHeroes, const FString& Size)
{
	const double NotCloserThan = Values.CampRefillFromMeters * CentimetersPerMeter;
	for (TActorIterator<AEIEnemyCamp> It(GetWorld()); It; ++It)
	{
		const AEIEnemyCamp* Camp = *It;
		const bool bHeroNear = LivingHeroes.ContainsByPredicate([Camp, NotCloserThan](const AEIHeroCharacter* Hero)
		{
			return FVector::Dist2D(Hero->GetActorLocation(), Camp->GetActorLocation()) < NotCloserThan;
		});
		if (Camp->CountMembers() == 0 && !bHeroNear)
		{
			FillCamp(*It, Size, LivingHeroes.Num());
		}
	}
}

void UEIRunDirector::UpdateChests(const TArray<AEIHeroCharacter*>& LivingHeroes)
{
	UEIHordeDirector* Horde = CastChecked<AEIGameMode>(GetOwner())->GetHorde();
	const int32 Stage = GetWorld()->GetGameState<AEIGameState>()->GetCorruptionStage();
	for (AEIChest* Chest : ChestsInUse)
	{
		if (Chest->GetState() == EEIChestState::Guarded)
		{
			Chest->UpdateGuards();
		}
		else if (Chest->GetState() == EEIChestState::Sealed
			&& LivingHeroes.ContainsByPredicate([Chest](const AEIHeroCharacter* Hero) { return Chest->IsInZone(Hero); }))
		{
			TArray<AEIEnemy*> Guards;
			if (const FEIEnemyGroup* Group = Groups.Pick(Groups.GetStage(Stage).Chest, Stage))
			{
				Guards = Horde->SpawnGroup(*Group, GetPlayerFactor(LivingHeroes.Num()), Chest->GetActorLocation(), Values.ChestGuardDistanceMeters * CentimetersPerMeter);
			}
			for (AEIEnemy* Guard : Guards)
			{
				Guard->MarkAsGuard();
			}
			Chest->SetGuards(Guards);
		}
	}
}

bool UEIRunDirector::OpenChest(AEIHeroCharacter* Hero)
{
	for (AEIChest* Chest : ChestsInUse)
	{
		if (Chest->GetState() == EEIChestState::Free && Chest->IsInReach(Hero) && !Chest->IsOpenFor(Hero))
		{
			Chest->OpenFor(Hero);
			Hero->GetOmens()->GrantRandomOmen();
			return true;
		}
	}
	return false;
}

AEIAltar* UEIRunDirector::GetActiveAltar() const
{
	return AltarOrder.IsValidIndex(AltarsBroken) ? AltarOrder[AltarsBroken] : nullptr;
}

void UEIRunDirector::HoldAltar(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	AEIAltar* Altar = GetActiveAltar();
	const bool bHeld = LivingHeroes.ContainsByPredicate([Altar](const AEIHeroCharacter* Hero) { return Altar->IsInZone(Hero); });
	if (bHeld)
	{
		AltarHoldLeft -= DeltaTime;
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetAltarHold(AltarHoldLeft, bHeld);
	if (AltarHoldLeft > 0.0f)
	{
		return;
	}

	// Broken. The ritual moves on to the next place, or it is over.
	Altar->SetState(EEIAltarState::Broken);
	++AltarsBroken;
	AltarHoldLeft = Values.AltarHoldSeconds;
	State->SetAltarProgress(AltarsBroken, AltarOrder.Num());
	State->SetAltarHold(AltarHoldLeft, false);
	if (AEIAltar* NextAltar = GetActiveAltar())
	{
		NextAltar->SetState(EEIAltarState::Active);
	}
	else
	{
		State->SetRunPhase(EEIRunPhase::RitualDone);
	}
}

void UEIRunDirector::Interact(AEIHeroCharacter* Hero)
{
	const EEIRunPhase Phase = GetWorld()->GetGameState<AEIGameState>()->GetRunPhase();
	const bool bRunning = Phase != EEIRunPhase::None && Phase != EEIRunPhase::OnBarge && Phase != EEIRunPhase::Escaped && Phase != EEIRunPhase::Failed;
	if (bRunning && OpenChest(Hero))
	{
		return;
	}
	if (Phase == EEIRunPhase::RitualDone)
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

bool UEIRunDirector::SendGroup(const FString& Size, int32 HeroCount)
{
	const int32 Stage = GetWorld()->GetGameState<AEIGameState>()->GetCorruptionStage();
	UEIHordeDirector* Horde = CastChecked<AEIGameMode>(GetOwner())->GetHorde();
	const FEIEnemyGroup* Group = Groups.Pick(Size, Stage);
	if (Group == nullptr || Horde->CountThreat() >= Groups.GetStage(Stage).ThreatCap * GetPlayerFactor(HeroCount))
	{
		return true;
	}
	return !Horde->SpawnReinforcement(*Group, GetPlayerFactor(HeroCount)).IsEmpty();
}

void UEIRunDirector::UpdateEnemyGroups(EEIRunPhase Phase, const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const int32 Stage = State->GetCorruptionStage();
	const FEIGroupStage& Row = Groups.GetStage(Stage);
	const int32 HeroCount = LivingHeroes.Num();
	CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetEnemyStrength(1.0f + Values.CorruptionStrengthPerStage * Stage);

	// A new stage brings life back to empty camps and sends its first group at once.
	if (Stage != LastStage)
	{
		LastStage = Stage;
		ReinforcementLeft = 0.0f;
		RefillEmptyCamps(LivingHeroes, Row.Camp);
	}
	if (Row.ReinforcementEverySeconds > 0.0f)
	{
		ReinforcementLeft -= DeltaTime;
		if (ReinforcementLeft <= 0.0f && SendGroup(Row.Reinforcement, HeroCount))
		{
			ReinforcementLeft = Row.ReinforcementEverySeconds;
		}
	}

	// Waves attack whoever holds an altar or the fire. Whoever comes back gets the next one at once.
	const bool bAtAltar = Phase == EEIRunPhase::Ritual && State->IsAltarHeld();
	const bool bAtFire = Phase == EEIRunPhase::BeaconLit && State->IsBeaconHeld();
	if (!bAtAltar && !bAtFire)
	{
		WaveLeft = 0.0f;
		return;
	}
	WaveLeft -= DeltaTime;
	if (WaveLeft <= 0.0f && SendGroup(bAtFire ? Row.BeaconWave : Row.AltarWave, HeroCount))
	{
		WaveLeft = bAtFire ? Groups.BeaconWaveEverySeconds : Groups.AltarWaveEverySeconds;
	}
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
		CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetRemovingLeftBehind(false);
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
			CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetRemovingLeftBehind(false);
		}
		return;
	}

	if (Phase != EEIRunPhase::OnBarge)
	{
		UpdateCorruption();
		UpdateEnemyGroups(Phase, LivingHeroes, DeltaTime);
		UpdateChests(LivingHeroes);
	}

	switch (Phase)
	{
	case EEIRunPhase::OnBarge:
		for (const AEIHeroCharacter* Hero : LivingHeroes)
		{
			if (Hero->GetActorLocation().Z < Barge->GetDeckHeight() - LeftBargeBelowCm)
			{
				State->StartRunClock();
				State->SetRunPhase(AltarOrder.IsEmpty() ? EEIRunPhase::RitualDone : EEIRunPhase::Exploring);
				for (TActorIterator<AEIEnemyCamp> Camp(GetWorld()); Camp; ++Camp)
				{
					FillCamp(*Camp, Camp->GetStartSize(), LivingHeroes.Num());
				}
				CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetRemovingLeftBehind(true);
				break;
			}
		}
		break;

	case EEIRunPhase::Exploring:
		if (LivingHeroes.ContainsByPredicate([this](const AEIHeroCharacter* Hero) { return GetActiveAltar()->IsInZone(Hero); }))
		{
			State->SetRunPhase(EEIRunPhase::Ritual);
		}
		break;

	case EEIRunPhase::Ritual:
		HoldAltar(LivingHeroes, DeltaTime);
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
