#include "EIRunDirector.h"

#include "Dom/JsonObject.h"
#include "EIAltar.h"
#include "EIAnchorStone.h"
#include "EIBoss.h"
#include "EIChest.h"
#include "EIDataFile.h"
#include "EIEnemy.h"
#include "EIEnemyCamp.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHeroCharacter.h"
#include "EIHordeDirector.h"
#include "EIPlayerController.h"
#include "EIProfileComponent.h"
#include "EISignalFire.h"
#include "EISkyBarge.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float BargeHoverHeightCm = 140.0f;
	// Ahead and to the right of the team, so that everybody sees it leave.
	const FVector BargeAtArrivalCm(800.0f, 1400.0f, 300.0f);
	constexpr float GroundSearchCm = 5000.0f;
	constexpr float AboveGroundCm = 5.0f;
	constexpr float BossAboveGroundCm = 100.0f;

	bool LoadPlaces(const TCHAR* Key, TArray<FVector>& OutPlaces, FString& OutError)
	{
		const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("laufkarte.json"), OutError);
		const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
		if (!Root.IsValid())
		{
			return false;
		}
		if (!Root->TryGetArrayField(Key, Lines) || Lines->IsEmpty())
		{
			OutError = FString::Printf(TEXT("Die Liste %s fehlt oder ist leer."), Key);
			return false;
		}
		TArray<FVector> Loaded;
		for (const TSharedPtr<FJsonValue>& Line : *Lines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			float X = 0.0f;
			float Y = 0.0f;
			if (!Line->TryGetObject(Object)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("x_m"), -100000.0, 100000.0, X, OutError)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("y_m"), -100000.0, 100000.0, Y, OutError))
			{
				return false;
			}
			Loaded.Add(FVector(X, Y, 0.0f) * CentimetersPerMeter);
		}
		OutPlaces = Loaded;
		return true;
	}

	bool LoadArrivals(TArray<FEIArrivalPoint>& OutArrivals, FString& OutError)
	{
		const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("laufkarte.json"), OutError);
		const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
		if (!Root.IsValid())
		{
			return false;
		}
		if (!Root->TryGetArrayField(TEXT("ankunft"), Lines) || Lines->IsEmpty())
		{
			OutError = TEXT("Die Liste ankunft fehlt oder ist leer.");
			return false;
		}
		TArray<FEIArrivalPoint> Loaded;
		for (const TSharedPtr<FJsonValue>& Line : *Lines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			float X = 0.0f;
			float Y = 0.0f;
			FEIArrivalPoint Arrival;
			if (!Line->TryGetObject(Object)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("x_m"), -100000.0, 100000.0, X, OutError)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("y_m"), -100000.0, 100000.0, Y, OutError)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("blick_grad"), -360.0, 360.0, Arrival.YawDegrees, OutError))
			{
				return false;
			}
			Arrival.Location = FVector(X, Y, 0.0f) * CentimetersPerMeter;
			Loaded.Add(Arrival);
		}
		OutArrivals = Loaded;
		return true;
	}

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
		|| !FEIDataFile::ReadNumber(*Root, TEXT("ablegen_s"), 0.0, 600.0, Loaded.CastOffSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("ueberfahrt_s"), 0.0, 600.0, Loaded.CrossingSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("ankunft_abstand_m"), 0.5, 100.0, Loaded.ArrivalSpacingMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("ankunft_abflug_tempo_m_pro_s"), 0.1, 200.0, Loaded.ArrivalDepartMetersPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("signalfeuer_halten_s"), 0.1, 3600.0, Loaded.SignalHoldSeconds, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("signalfeuer_reichweite_m"), 0.5, 100.0, Loaded.SignalReachMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("signalfeuer_zone_m"), 1.0, 200.0, Loaded.SignalZoneMeters, OutError)
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
		FEIDataFile::ReportError(TEXT("nebenauftraege.json"), Error);
	}
	if (!LoadArrivals(Arrivals, Error) || !LoadPlaces(TEXT("bossplaetze"), BossPlaces, Error))
	{
		FEIDataFile::ReportError(TEXT("laufkarte.json"), Error);
	}
	if (!FEIBossTable::LoadFromProjectFile(BossTable, Error))
	{
		FEIDataFile::ReportError(TEXT("bosse.json"), Error);
	}
}

void UEIRunDirector::ResetRun()
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	TActorIterator<AEISkyBarge> FoundBarge(GetWorld());
	Barge = FoundBarge ? *FoundBarge : nullptr;
	HoldLeft = 0.0f;
	CastOffLeft = Values.CastOffSeconds;
	CrossingLeft = Values.CrossingSeconds;
	State->ResetRunClock();
	State->SetSignalHold(0.0f, false);
	State->SetCastingOff(false);
	SetCorruptionStage(0);
	WaveLeft = 0.0f;
	ReinforcementLeft = 0.0f;
	LastStage = 0;
	CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetEnemyStrength(1.0f);

	PrepareBoss();
	PrepareContracts();
	PrepareSignalFire();
	if (Barge == nullptr)
	{
		State->SetRunPhase(EEIRunPhase::None);
		return;
	}
	Barge->ReturnToStart();
	State->SetRunPhase(EEIRunPhase::AtDock);
}

void UEIRunDirector::PrepareBoss()
{
	for (AEIAnchorStone* Anchor : Anchors)
	{
		if (Anchor != nullptr)
		{
			Anchor->Destroy();
		}
	}
	Anchors.Reset();
	if (Boss != nullptr)
	{
		Boss->Destroy();
		Boss = nullptr;
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const bool bHasBoss = Barge != nullptr && !BossTable.Bosses.IsEmpty() && !BossPlaces.IsEmpty();
	BossIndex = bHasBoss ? FMath::RandRange(0, BossTable.Bosses.Num() - 1) : INDEX_NONE;
	BossPlace = bHasBoss ? BossPlaces[FMath::RandRange(0, BossPlaces.Num() - 1)] : FVector::ZeroVector;
	State->SetBossStage(bHasBoss ? EEIBossStage::Sealed : EEIBossStage::None);
	State->SetBannbruchProgress(0, bHasBoss ? BossTable.Bosses[BossIndex].Bannbruch.Count : 0);
}

void UEIRunDirector::SpawnBoss(int32 HeroCount)
{
	if (BossIndex == INDEX_NONE)
	{
		return;
	}
	FVector Ground = BossPlace;
	FindGround(BossPlace, Ground);
	const FEIBannbruch& Bannbruch = BossTable.Bosses[BossIndex].Bannbruch;
	const FTransform Spawn(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Ground + FVector(0.0f, 0.0f, BossAboveGroundCm));
	Boss = GetWorld()->SpawnActorDeferred<AEIBoss>(AEIBoss::StaticClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Boss == nullptr)
	{
		return;
	}
	Boss->SetKind(EEIEnemyKind::Colossus);
	Boss->SetBossIndex(BossIndex);
	Boss->FinishSpawning(Spawn);
	Boss->StandWatch(Boss->GetActorLocation());

	// The stones stand in a ring around the boss, each with a chain to it.
	const FVector ChainEnd = Boss->GetActorLocation();
	const float Turn = FMath::FRandRange(0.0f, 360.0f);
	for (int32 Index = 0; Index < Bannbruch.Count; ++Index)
	{
		const FVector Direction = FVector::ForwardVector.RotateAngleAxis(Turn + Index * 360.0f / Bannbruch.Count, FVector::UpVector);
		const FTransform StoneSpawn(Ground + Direction * Bannbruch.DistanceMeters * CentimetersPerMeter);
		if (AEIAnchorStone* Anchor = GetWorld()->SpawnActorDeferred<AEIAnchorStone>(AEIAnchorStone::StaticClass(), StoneSpawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Anchor->Setup(ChainEnd, Bannbruch.ZoneMeters, Bannbruch.HoldSeconds);
			Anchor->FinishSpawning(StoneSpawn);
			Anchors.Add(Anchor);
		}
	}
}

void UEIRunDirector::UpdateBannbruch(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (State->GetBossStage() != EEIBossStage::Sealed || Boss == nullptr)
	{
		return;
	}
	int32 Broken = 0;
	for (AEIAnchorStone* Anchor : Anchors)
	{
		Anchor->UpdateHold(LivingHeroes.ContainsByPredicate([Anchor](const AEIHeroCharacter* Hero) { return Anchor->IsInZone(Hero); }), DeltaTime);
		Broken += Anchor->IsBroken() ? 1 : 0;
	}
	State->SetBannbruchProgress(Broken, Anchors.Num());
	if (Broken == Anchors.Num())
	{
		BreakBossFree(LivingHeroes.Num());
	}
}

void UEIRunDirector::BreakBossFree(int32 HeroCount)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetBossStage(EEIBossStage::Awake);
	WaveLeft = 0.0f;
	Boss->BreakFree(1.0f + Values.CorruptionStrengthPerStage * State->GetCorruptionStage(), HeroCount);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (const AEIPlayerController* Player = Cast<AEIPlayerController>(It->Get()))
		{
			Player->GetProfile()->ClientShowNote(BossTable.Bosses[BossIndex].Entrance);
		}
	}
}

void UEIRunDirector::SkipBannbruch()
{
	for (AEIAnchorStone* Anchor : Anchors)
	{
		Anchor->SetHoldLeft(0.0f);
		Anchor->UpdateHold(false, 0.0f);
	}
	if (Boss != nullptr && GetWorld()->GetGameState<AEIGameState>()->GetBossStage() == EEIBossStage::Sealed)
	{
		GetWorld()->GetGameState<AEIGameState>()->SetBannbruchProgress(Anchors.Num(), Anchors.Num());
		int32 Heroes = 0;
		for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
		{
			Heroes += It->IsDead() ? 0 : 1;
		}
		BreakBossFree(Heroes);
	}
}

bool UEIRunDirector::IsAnchorHeld() const
{
	return Anchors.ContainsByPredicate([](const AEIAnchorStone* Anchor) { return Anchor != nullptr && Anchor->IsHeld(); });
}

void UEIRunDirector::BossFell()
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetBossStage(EEIBossStage::Down);
	WaveLeft = 0.0f;
	const FEIBossValues& Fallen = BossTable.Bosses[BossIndex];
	const float Bonus = 1.0f + ContractTable.CorruptionBonusPerStage * State->GetCorruptionStage();

	// Everybody rolls for themselves, and whoever has fallen in the fight still has a share of it.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (const AEIPlayerController* Player = Cast<AEIPlayerController>(It->Get()))
		{
			Player->GetProfile()->GrantBossLoot(Fallen.Name, FMath::RoundToInt32(Fallen.RollGold() * Bonus), Fallen.RollLoot());
		}
	}
}

void UEIRunDirector::PrepareSignalFire()
{
	TArray<AEISignalFire*> Fires;
	TArray<AEISignalFire*> FarFires;
	const double MinDistance = BossTable.FireMinDistanceMeters * CentimetersPerMeter;
	for (TActorIterator<AEISignalFire> It(GetWorld()); It; ++It)
	{
		It->SetLit(false);
		It->SetInUse(false);
		Fires.Add(*It);
		if (BossIndex == INDEX_NONE || FVector::Dist2D(It->GetActorLocation(), BossPlace) >= MinDistance)
		{
			FarFires.Add(*It);
		}
	}
	// After the boss there should be a way to run.
	const TArray<AEISignalFire*>& Choice = !FarFires.IsEmpty() ? FarFires : Fires;
	ActiveFire = Barge != nullptr && !Choice.IsEmpty() ? Choice[FMath::RandRange(0, Choice.Num() - 1)] : nullptr;
	if (ActiveFire != nullptr)
	{
		ActiveFire->SetInUse(true);
	}
}

void UEIRunDirector::UpdateDock(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	// The team stays together: the barge only leaves with everybody aboard. Whoever steps off in time brings it back.
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (LivingHeroes.ContainsByPredicate([](const AEIHeroCharacter* Hero) { return !Hero->IsAboard(); }))
	{
		if (State->IsCastingOff())
		{
			State->SetCastingOff(false);
			CastOffLeft = Values.CastOffSeconds;
			Barge->ReturnToStart();
		}
		return;
	}
	if (!State->IsCastingOff())
	{
		State->SetCastingOff(true);
		Barge->Depart(Values.BargeDepartMetersPerSecond * CentimetersPerMeter);
	}
	CastOffLeft -= DeltaTime;
	if (CastOffLeft <= 0.0f)
	{
		State->SetRunPhase(EEIRunPhase::Crossing);
	}
}

void UEIRunDirector::BeginRunNow()
{
	if (Barge == nullptr || Arrivals.IsEmpty())
	{
		return;
	}
	TArray<AEIHeroCharacter*> LivingHeroes;
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead())
		{
			LivingHeroes.Add(*It);
		}
	}
	const FEIArrivalPoint& Arrival = Arrivals[FMath::RandRange(0, Arrivals.Num() - 1)];
	FVector Ground = Arrival.Location;
	FindGround(Arrival.Location, Ground);
	const FRotator Facing(0.0f, Arrival.YawDegrees, 0.0f);
	const FVector Side = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
	const float Spacing = Values.ArrivalSpacingMeters * CentimetersPerMeter;
	for (int32 Index = 0; Index < LivingHeroes.Num(); ++Index)
	{
		const float FromMiddle = (Index - (LivingHeroes.Num() - 1) / 2.0f) * Spacing;
		const float HalfHeight = LivingHeroes[Index]->GetSimpleCollisionHalfHeight();
		LivingHeroes[Index]->ArriveAt(Ground + Side * FromMiddle + FVector(0.0f, 0.0f, HalfHeight + AboveGroundCm), Arrival.YawDegrees);
	}
	Barge->ArriveAt(Ground + Facing.RotateVector(BargeAtArrivalCm));
	Barge->Depart(Values.ArrivalDepartMetersPerSecond * CentimetersPerMeter);

	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetCastingOff(false);
	State->StartRunClock();
	State->SetRunPhase(EEIRunPhase::Exploring);
	for (TActorIterator<AEIEnemyCamp> Camp(GetWorld()); Camp; ++Camp)
	{
		FillCamp(*Camp, Camp->GetStartSize(), LivingHeroes.Num());
	}
	SpawnLeaders();
	SpawnBoss(LivingHeroes.Num());
	CastChecked<AEIGameMode>(GetOwner())->GetHorde()->SetRemovingLeftBehind(true);
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
	if (BossIndex != INDEX_NONE)
	{
		Taken.Add(BossPlace);
	}
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
	GetWorld()->GetGameState<AEIGameState>()->SetContracts(Contracts);
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
	if (Boss != nullptr && &Enemy == Boss.Get())
	{
		BossFell();
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
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const EEIRunPhase Phase = State->GetRunPhase();
	if (Phase == EEIRunPhase::AtDock)
	{
		if (!Hero->IsAboard() && Barge->IsInBoardingZone(Hero))
		{
			Hero->BoardBarge(Barge);
		}
		else if (Hero->IsAboard())
		{
			Hero->ArriveAt(Hero->GetLocationBeforeBoarding(), static_cast<float>(Hero->GetActorRotation().Yaw));
		}
		return;
	}
	const bool bRunning = Phase == EEIRunPhase::Exploring || Phase == EEIRunPhase::SignalLit || Phase == EEIRunPhase::BargeWaiting;
	if (bRunning && OpenChest(Hero))
	{
		return;
	}
	if (Phase == EEIRunPhase::Exploring && ActiveFire != nullptr && ActiveFire->IsInReach(Hero))
	{
		if (State->CanLightSignal())
		{
			LightSignal();
		}
		else if (const AEIPlayerController* Player = Hero->GetController<AEIPlayerController>())
		{
			Player->GetProfile()->ClientShowNote(TEXT("Das Signalfeuer brennt erst, wenn der Boss gefallen ist."));
		}
	}
	else if (Phase == EEIRunPhase::BargeWaiting && !Hero->IsAboard() && Barge->IsInBoardingZone(Hero))
	{
		Hero->BoardBarge(Barge);
	}
}

bool UEIRunDirector::FindGround(const FVector& Location, FVector& OutGround) const
{
	FHitResult Hit;
	const FVector Start = Location + FVector(0.0f, 0.0f, GroundSearchCm);
	const FVector End = Location - FVector(0.0f, 0.0f, GroundSearchCm);
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	return true;
}

void UEIRunDirector::LightSignal()
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	ActiveFire->SetLit(true);
	HoldLeft = Values.SignalHoldSeconds;
	State->SetSignalHold(HoldLeft, true);
	State->SetRunPhase(EEIRunPhase::SignalLit);

	// The barge comes down on the side towards the middle of the map, where it is open.
	const FVector FireLocation = ActiveFire->GetActorLocation();
	FVector Hover = FireLocation - FireLocation.GetSafeNormal2D() * Values.BargeDistanceMeters * CentimetersPerMeter;
	Hover.Z = FireLocation.Z + BargeHoverHeightCm;
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

	// Waves attack whoever holds something, whoever fights the boss, and everybody after it has fallen. Whoever comes back gets the next one at once.
	const bool bAtAltar = State->IsContractHeld() || IsAnchorHeld();
	const bool bAtFire = Phase == EEIRunPhase::SignalLit && State->IsSignalHeld();
	const bool bPursuit = State->GetBossStage() == EEIBossStage::Down;
	const bool bFight = State->GetBossStage() == EEIBossStage::Awake;
	if (!bAtAltar && !bAtFire && !bPursuit && !bFight)
	{
		WaveLeft = 0.0f;
		return;
	}
	const bool bBigWave = bAtFire || bPursuit;
	const float EverySeconds = bAtFire ? Groups.SignalWaveEverySeconds : (bPursuit ? BossTable.PursuitWaveEverySeconds : (bFight ? BossTable.FightWaveEverySeconds : Groups.AltarWaveEverySeconds));
	WaveLeft -= DeltaTime;
	if (WaveLeft <= 0.0f && SendGroup(bBigWave ? Row.SignalWave : Row.AltarWave, HeroCount))
	{
		WaveLeft = EverySeconds;
	}
}

void UEIRunDirector::HoldSignal(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	const bool bHeld = LivingHeroes.ContainsByPredicate([this](const AEIHeroCharacter* Hero) { return ActiveFire->IsInZone(Hero); });
	if (bHeld)
	{
		HoldLeft = FMath::Max(0.0f, HoldLeft - DeltaTime);
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetSignalHold(HoldLeft, bHeld);
	Barge->SetApproachFraction(1.0f - HoldLeft / FMath::Max(Values.SignalHoldSeconds, KINDA_SMALL_NUMBER));
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

	if (Phase != EEIRunPhase::AtDock && Phase != EEIRunPhase::Crossing)
	{
		UpdateCorruption();
		UpdateEnemyGroups(Phase, LivingHeroes, DeltaTime);
		UpdateContracts(LivingHeroes, DeltaTime);
		UpdateBannbruch(LivingHeroes, DeltaTime);
	}

	switch (Phase)
	{
	case EEIRunPhase::AtDock:
		UpdateDock(LivingHeroes, DeltaTime);
		break;

	case EEIRunPhase::Crossing:
		CrossingLeft -= DeltaTime;
		if (CrossingLeft <= 0.0f)
		{
			BeginRunNow();
		}
		break;

	case EEIRunPhase::SignalLit:
		HoldSignal(LivingHeroes, DeltaTime);
		break;

	case EEIRunPhase::BargeWaiting:
		LeaveOnceAllAboard(LivingHeroes);
		break;

	default:
		break;
	}
}
