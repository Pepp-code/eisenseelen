#include "EIRunDirector.h"

#include "Dom/JsonObject.h"
#include "EIAltar.h"
#include "EIChest.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIEnemyCamp.h"
#include "EIFlare.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHeroCharacter.h"
#include "EIHordeDirector.h"
#include "EIPlayerController.h"
#include "EIProfileComponent.h"
#include "EISkyBarge.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	// Falling this far below the deck counts as having jumped.
	constexpr float LeftBargeBelowCm = 300.0f;
	constexpr float BargeHoverHeightCm = 140.0f;
	constexpr int32 BargeSpotTries = 8;
	constexpr float GroundSearchUpCm = 1000.0f;
	constexpr float GroundSearchDownCm = 3000.0f;
	// On a different level than the flare the barge could not be reached from its circle.
	constexpr float BargeSpotHeightDifferenceCm = 150.0f;
	constexpr float BargeRoomAboveGroundCm = 20.0f;

	/** The first place far enough from all others, or else simply the first one. It leaves the list. */
	template <typename TPlace>
	TPlace* TakePlace(TArray<TPlace*>& Places, const TArray<FVector>& Taken, double MinDistanceCm)
	{
		if (Places.IsEmpty())
		{
			return nullptr;
		}
		const int32 Found = Places.IndexOfByPredicate([&Taken, MinDistanceCm](const TPlace* Place)
		{
			return !Taken.ContainsByPredicate([Place, MinDistanceCm](const FVector& Other) { return FVector::Dist2D(Place->GetActorLocation(), Other) < MinDistanceCm; });
		});
		TPlace* Place = Places[FMath::Max(Found, 0)];
		Places.RemoveAt(FMath::Max(Found, 0));
		return Place;
	}
}

bool FEIRunValues::LoadFromProjectFile(FEIRunValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("lauf.json"), OutError);
	FEIRunValues Loaded;
	if (!Root.IsValid()
		|| !FEIDataFile::ReadNumber(*Root, TEXT("lager_faktor"), 0.0, 100.0, Loaded.CampFactor, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("lager_radius_m"), 0.5, 100.0, Loaded.CampRadiusMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("lager_nachschub_ab_abstand_m"), 0.0, 1000.0, Loaded.CampRefillFromMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("truhe_reichweite_m"), 0.5, 100.0, Loaded.ChestReachMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("truhe_zone_m"), 1.0, 100.0, Loaded.ChestZoneMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("truhe_waechter_abstand_m"), 1.0, 100.0, Loaded.ChestGuardDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("fackel_halten_s"), 0.1, 3600.0, Loaded.FlareHoldSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("fackel_zone_m"), 1.0, 200.0, Loaded.FlareZoneMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("barke_abstand_m"), 1.0, 100.0, Loaded.BargeDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("barke_abflug_tempo_m_pro_s"), 0.1, 100.0, Loaded.BargeDepartMetersPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("anfuehrer_leben_faktor"), 0.1, 100.0, Loaded.LeaderLifeFactor, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("anfuehrer_schaden_faktor"), 0.1, 100.0, Loaded.LeaderDamageFactor, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("anfuehrer_groesse_faktor"), 0.1, 10.0, Loaded.LeaderScaleFactor, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_alle_s"), 0.1, 3600.0, Loaded.CorruptionEverySeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_stufen"), 1.0, 100.0, Loaded.CorruptionMaxStage, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("verderbnis_gegner_staerke_je_stufe"), 0.0, 10.0, Loaded.CorruptionStrengthPerStage, OutError))
	{
		return false;
	}
	FString LeaderKind;
	Root->TryGetStringField(TEXT("anfuehrer_art"), LeaderKind);
	bool bKnownKind = false;
	for (int32 Index = 0; Index < EIEnemyKindCount; ++Index)
	{
		if (LeaderKind == EIEnemyKindKey(static_cast<EEIEnemyKind>(Index)))
		{
			Loaded.LeaderKind = static_cast<EEIEnemyKind>(Index);
			bKnownKind = true;
		}
	}
	if (!bKnownKind)
	{
		OutError = FString::Printf(TEXT("Die Gegnerart %s als anfuehrer_art gibt es nicht."), *LeaderKind);
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
	if (!FEIContractTable::LoadFromProjectFile(ContractTable, Error))
	{
		FEIDataFile::ReportError(TEXT("vertraege.json"), Error);
	}
}

void UEIRunDirector::ResetRun()
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	TActorIterator<AEISkyBarge> FoundBarge(GetWorld());
	Barge = FoundBarge ? *FoundBarge : nullptr;
	if (Flare != nullptr)
	{
		Flare->Destroy();
		Flare = nullptr;
	}
	HoldLeft = 0.0f;
	State->ResetRunClock();
	State->SetFlareHold(0.0f, false);
	SetCorruptionStage(0);
	WaveLeft = 0.0f;
	ReinforcementLeft = 0.0f;
	LastStage = 0;
	CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetEnemyStrength(1.0f);

	PrepareContracts();
	if (Barge == nullptr)
	{
		State->SetRunPhase(EEIRunPhase::None);
		return;
	}
	Barge->ReturnToStart();
	State->SetRunPhase(EEIRunPhase::OnBarge);
}

void UEIRunDirector::PrepareContracts()
{
	// All places are free again, and every run finds its jobs somewhere else.
	TArray<AEIAltar*> Altars;
	for (TActorIterator<AEIAltar> It(GetWorld()); It; ++It)
	{
		It->SetState(EEIAltarState::Dormant);
		Altars.Insert(*It, FMath::RandRange(0, Altars.Num()));
	}
	TArray<AEIChest*> Chests;
	for (TActorIterator<AEIChest> It(GetWorld()); It; ++It)
	{
		It->PrepareForRun(false);
		Chests.Insert(*It, FMath::RandRange(0, Chests.Num()));
	}
	TArray<AEIEnemyCamp*> Camps;
	for (TActorIterator<AEIEnemyCamp> It(GetWorld()); It; ++It)
	{
		Camps.Insert(*It, FMath::RandRange(0, Camps.Num()));
	}

	TArray<FEIContract> Contracts = Barge != nullptr ? ContractTable.RollContracts() : TArray<FEIContract>();
	const double MinDistance = ContractTable.MinDistanceMeters * CentimetersPerMeter;
	TArray<FVector> Taken;
	for (FEIContract& Contract : Contracts)
	{
		if (Contract.Kind == EEIContractKind::Leader)
		{
			Contract.Target = TakePlace(Camps, Taken, MinDistance);
		}
		else if (Contract.Kind == EEIContractKind::Chest)
		{
			AEIChest* Chest = TakePlace(Chests, Taken, MinDistance);
			Contract.Target = Chest;
			if (Chest != nullptr)
			{
				Chest->PrepareForRun(true);
			}
		}
		else
		{
			AEIAltar* Altar = TakePlace(Altars, Taken, MinDistance);
			Contract.Target = Altar;
			if (Altar != nullptr)
			{
				Altar->StartJob(ContractTable.FindTask(Contract.Kind)->ZoneMeters, ContractTable.FindFaction(Contract.Faction)->Color);
			}
		}
		if (Contract.Target != nullptr)
		{
			Contract.Location = Contract.Target->GetActorLocation();
			Taken.Add(Contract.Location);
		}
	}
	Contracts.RemoveAll([](const FEIContract& Contract) { return Contract.Target == nullptr; });
	GetWorld()->GetGameState<AEIGameState>()->SetContracts(Contracts, FMath::RoundToInt32(ContractTable.ContractsToFlare));
}

void UEIRunDirector::SpawnLeaders()
{
	UEIHordeDirector* Horde = CastChecked<AEIGameMode>(GetOwner())->GetHorde();
	for (FEIContract& Contract : GetWorld()->GetGameState<AEIGameState>()->EditContracts())
	{
		AEIEnemyCamp* Camp = Cast<AEIEnemyCamp>(Contract.Target);
		if (Contract.Kind != EEIContractKind::Leader || Camp == nullptr)
		{
			continue;
		}
		if (AEIEnemy* Leader = Horde->SpawnKind(Values.LeaderKind, Camp->GetActorLocation()))
		{
			Leader->MakeLeader(Values.LeaderLifeFactor, Values.LeaderDamageFactor, Values.LeaderScaleFactor);
			Leader->StandWatch(Leader->GetActorLocation());
			Camp->AddMember(Leader);
			Contract.Target = Leader;
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

void UEIRunDirector::UpdateChest(AEIChest* Chest, const TArray<AEIHeroCharacter*>& LivingHeroes)
{
	if (Chest->GetState() == EEIChestState::Guarded)
	{
		Chest->UpdateGuards();
	}
	else if (Chest->GetState() == EEIChestState::Sealed
		&& LivingHeroes.ContainsByPredicate([Chest](const AEIHeroCharacter* Hero) { return Chest->IsInZone(Hero); }))
	{
		UEIHordeDirector* Horde = CastChecked<AEIGameMode>(GetOwner())->GetHorde();
		const int32 Stage = GetWorld()->GetGameState<AEIGameState>()->GetCorruptionStage();
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

void UEIRunDirector::UpdateContracts(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	TArray<FEIContract>& Contracts = GetWorld()->GetGameState<AEIGameState>()->EditContracts();
	for (int32 Index = 0; Index < Contracts.Num(); ++Index)
	{
		FEIContract& Contract = Contracts[Index];
		if (Contract.bDone)
		{
			continue;
		}
		if (const AEIEnemy* Leader = Cast<AEIEnemy>(Contract.Target))
		{
			Contract.Location = Leader->GetActorLocation();
		}
		else if (AEIChest* Chest = Cast<AEIChest>(Contract.Target))
		{
			UpdateChest(Chest, LivingHeroes);
		}
		else if (const AEIAltar* Altar = Cast<AEIAltar>(Contract.Target))
		{
			// The time runs for whoever stands in the zone, however many enemies stand in it.
			Contract.bHeld = LivingHeroes.ContainsByPredicate([Altar](const AEIHeroCharacter* Hero) { return Altar->IsInZone(Hero); });
			Contract.HoldLeft -= Contract.bHeld ? DeltaTime : 0.0f;
			if (Contract.HoldLeft <= 0.0f)
			{
				CompleteContract(Index);
			}
		}
	}
}

void UEIRunDirector::CompleteContract(int32 Index)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	FEIContract& Contract = State->EditContracts()[Index];
	Contract.bDone = true;
	Contract.bHeld = false;
	if (AEIAltar* Altar = Cast<AEIAltar>(Contract.Target))
	{
		Altar->SetState(EEIAltarState::Broken);
	}

	// Everybody near gets their own pay, so nobody has to fight over the last hit.
	const int32 Stage = State->GetCorruptionStage();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const AEIPlayerController* Player = Cast<AEIPlayerController>(It->Get());
		const AEIHeroCharacter* Hero = Player != nullptr ? Player->GetPawn<AEIHeroCharacter>() : nullptr;
		if (Hero != nullptr && !Hero->IsDead() && FVector::Dist2D(Hero->GetActorLocation(), Contract.Location) <= Player->GetProfile()->GetTable().Rewards.RadiusMeters * CentimetersPerMeter)
		{
			Player->GetProfile()->GrantContract(Contract.Faction, ContractTable.GetGold(Stage), ContractTable.GetPoints(Stage));
		}
	}
}

void UEIRunDirector::SkipContracts(int32 Count)
{
	const TArray<FEIContract>& Contracts = GetWorld()->GetGameState<AEIGameState>()->GetContracts();
	for (int32 Index = 0; Index < Contracts.Num() && Count > 0; ++Index)
	{
		if (!Contracts[Index].bDone)
		{
			CompleteContract(Index);
			--Count;
		}
	}
}

bool UEIRunDirector::OpenChest(AEIHeroCharacter* Hero)
{
	const TArray<FEIContract>& Contracts = GetWorld()->GetGameState<AEIGameState>()->GetContracts();
	for (int32 Index = 0; Index < Contracts.Num(); ++Index)
	{
		AEIChest* Chest = Cast<AEIChest>(Contracts[Index].Target);
		if (Chest != nullptr && !Contracts[Index].bDone && Chest->GetState() == EEIChestState::Free && Chest->IsInReach(Hero))
		{
			Chest->Open();
			CompleteContract(Index);
			return true;
		}
	}
	return false;
}

void UEIRunDirector::NoteEnemyDied(const AEIEnemy& Enemy)
{
	// In the test arena nothing counts.
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (State->GetRunPhase() == EEIRunPhase::None)
	{
		return;
	}
	TArray<FEIContract>& Contracts = State->EditContracts();
	for (int32 Index = 0; Index < Contracts.Num(); ++Index)
	{
		if (!Contracts[Index].bDone && Contracts[Index].Target == &Enemy)
		{
			Contracts[Index].Location = Enemy.GetActorLocation();
			CompleteContract(Index);
		}
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const AEIPlayerController* Player = Cast<AEIPlayerController>(It->Get());
		const AEIHeroCharacter* Hero = Player != nullptr ? Player->GetPawn<AEIHeroCharacter>() : nullptr;
		if (Hero != nullptr && !Hero->IsDead() && FVector::Dist2D(Hero->GetActorLocation(), Enemy.GetActorLocation()) <= Player->GetProfile()->GetTable().Rewards.RadiusMeters * CentimetersPerMeter)
		{
			Player->GetProfile()->GrantEnemyReward(Enemy.GetValues().Experience, Enemy.GetValues().ItemChance);
		}
	}
}

void UEIRunDirector::EndRunForAll(bool bEscaped)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (const AEIPlayerController* Player = Cast<AEIPlayerController>(It->Get()))
		{
			Player->GetProfile()->EndRun(bEscaped);
		}
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
	if (Phase == EEIRunPhase::BargeWaiting && !Hero->IsAboard() && Barge->IsInBoardingZone(Hero))
	{
		Hero->BoardBarge(Barge);
	}
}

bool UEIRunDirector::FindGround(const FVector& Location, FVector& OutGround) const
{
	FHitResult Hit;
	const FVector Start = Location + FVector(0.0f, 0.0f, GroundSearchUpCm);
	const FVector End = Location - FVector(0.0f, 0.0f, GroundSearchDownCm);
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	return true;
}

bool UEIRunDirector::FindBargeSpot(const FVector& FlareGround, FVector& OutHover) const
{
	// Towards the middle of the map first, where it is usually open.
	const FVector Inwards = (-FlareGround).GetSafeNormal2D();
	const FVector First = Inwards.IsNearlyZero() ? FVector::ForwardVector : Inwards;
	const FVector Room = Barge->GetRoomNeeded();
	const FCollisionShape Box = FCollisionShape::MakeBox(Room);
	for (int32 Try = 0; Try < BargeSpotTries; ++Try)
	{
		const FVector Direction = First.RotateAngleAxis(Try * 360.0f / BargeSpotTries, FVector::UpVector);
		FVector Ground;
		if (!FindGround(FlareGround + Direction * Values.BargeDistanceMeters * CentimetersPerMeter, Ground)
			|| FMath::Abs(Ground.Z - FlareGround.Z) > BargeSpotHeightDifferenceCm)
		{
			continue;
		}
		const FVector BoxCenter = Ground + FVector(0.0f, 0.0f, BargeRoomAboveGroundCm + Room.Z);
		if (!GetWorld()->OverlapAnyTestByObjectType(BoxCenter, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic), Box))
		{
			OutHover = Ground + FVector(0.0f, 0.0f, BargeHoverHeightCm);
			return true;
		}
	}
	return false;
}

void UEIRunDirector::FireFlare(AEIHeroCharacter* Hero)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const AEIPlayerController* Player = Hero->GetController<AEIPlayerController>();
	if (State->GetRunPhase() != EEIRunPhase::Contracts || Player == nullptr)
	{
		return;
	}
	if (!State->CanFireFlare())
	{
		Player->GetProfile()->ClientShowNote(FString::Printf(TEXT("Die Leuchtfackel geht erst nach %d erf\u00FCllten Vertr\u00E4gen."), State->GetContractsToFlare()));
		return;
	}
	FVector Ground;
	FVector Hover;
	if (!FindGround(Hero->GetActorLocation(), Ground) || !FindBargeSpot(Ground, Hover))
	{
		Player->GetProfile()->ClientShowNote(TEXT("Hier hat die Barke keinen Platz. Geh ins Freie."));
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Flare = GetWorld()->SpawnActor<AEIFlare>(Ground, FRotator::ZeroRotator, Params);
	HoldLeft = Values.FlareHoldSeconds;
	State->SetFlareHold(HoldLeft, true);
	State->SetRunPhase(EEIRunPhase::FlareLit);
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

	// Waves attack whoever holds an altar or the circle of the flare. Whoever comes back gets the next one at once.
	const bool bAtAltar = State->IsContractHeld();
	const bool bAtFlare = Phase == EEIRunPhase::FlareLit && State->IsFlareHeld();
	if (!bAtAltar && !bAtFlare)
	{
		WaveLeft = 0.0f;
		return;
	}
	WaveLeft -= DeltaTime;
	if (WaveLeft <= 0.0f && SendGroup(bAtFlare ? Row.FlareWave : Row.AltarWave, HeroCount))
	{
		WaveLeft = bAtFlare ? Groups.FlareWaveEverySeconds : Groups.AltarWaveEverySeconds;
	}
}

void UEIRunDirector::HoldFlare(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	const bool bHeld = LivingHeroes.ContainsByPredicate([this](const AEIHeroCharacter* Hero) { return Flare->IsInZone(Hero); });
	if (bHeld)
	{
		HoldLeft = FMath::Max(0.0f, HoldLeft - DeltaTime);
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetFlareHold(HoldLeft, bHeld);
	Barge->SetApproachFraction(1.0f - HoldLeft / FMath::Max(Values.FlareHoldSeconds, KINDA_SMALL_NUMBER));
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
		EndRunForAll(true);
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
			EndRunForAll(false);
			CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetRemovingLeftBehind(false);
		}
		return;
	}

	if (Phase != EEIRunPhase::OnBarge)
	{
		UpdateCorruption();
		UpdateEnemyGroups(Phase, LivingHeroes, DeltaTime);
		UpdateContracts(LivingHeroes, DeltaTime);
	}

	switch (Phase)
	{
	case EEIRunPhase::OnBarge:
		for (const AEIHeroCharacter* Hero : LivingHeroes)
		{
			if (Hero->GetActorLocation().Z < Barge->GetDeckHeight() - LeftBargeBelowCm)
			{
				State->StartRunClock();
				State->SetRunPhase(EEIRunPhase::Contracts);
				for (TActorIterator<AEIEnemyCamp> Camp(GetWorld()); Camp; ++Camp)
				{
					FillCamp(*Camp, Camp->GetStartSize(), LivingHeroes.Num());
				}
				SpawnLeaders();
				CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetRemovingLeftBehind(true);
				break;
			}
		}
		break;

	case EEIRunPhase::FlareLit:
		HoldFlare(LivingHeroes, DeltaTime);
		break;

	case EEIRunPhase::BargeWaiting:
		LeaveOnceAllAboard(LivingHeroes);
		break;

	default:
		break;
	}
}
