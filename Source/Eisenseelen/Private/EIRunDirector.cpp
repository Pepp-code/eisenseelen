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
#include "EIHeroMovementComponent.h"
#include "EIHollowArmor.h"
#include "EIHordeDirector.h"
#include "EIForge.h"
#include "EIHuntBoard.h"
#include "EIPlayerController.h"
#include "EIProfileComponent.h"
#include "EISignalFire.h"
#include "EISkyBarge.h"
#include "EISoulKettle.h"
#include "EISoulStoneChest.h"
#include "EIVisuals.h"
#include "Engine/DamageEvents.h"
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

	bool LoadAreas(TArray<FEIPlaceArea>& OutAreas, FString& OutError)
	{
		const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("laufkarte.json"), OutError);
		const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
		if (!Root.IsValid())
		{
			return false;
		}
		// Without the list every place belongs to the whole map, so old data keeps working.
		if (!Root->TryGetArrayField(TEXT("gebiete"), Lines))
		{
			return true;
		}
		for (const TSharedPtr<FJsonValue>& Line : *Lines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			float X = 0.0f;
			float Y = 0.0f;
			float Radius = 0.0f;
			FEIPlaceArea Area;
			if (!Line->TryGetObject(Object)
				|| !(*Object)->TryGetStringField(TEXT("name"), Area.Name) || Area.Name.IsEmpty()
				|| !FEIDataFile::ReadNumber(**Object, TEXT("mitte_x_m"), -100000.0, 100000.0, X, OutError)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("mitte_y_m"), -100000.0, 100000.0, Y, OutError)
				|| !FEIDataFile::ReadNumber(**Object, TEXT("radius_m"), 1.0, 100000.0, Radius, OutError))
			{
				if (OutError.IsEmpty())
				{
					OutError = TEXT("Einem Gebiet fehlt name, mitte oder radius.");
				}
				return false;
			}
			Area.Center = FVector(X, Y, 0.0) * CentimetersPerMeter;
			Area.Radius = Radius * CentimetersPerMeter;
			OutAreas.Add(Area);
		}
		return true;
	}

	/** The name of the area and where the dock, the hunt board and the forge stand. */
	bool LoadDock(FString& OutArea, FVector& OutCenter, FVector& OutBoardPlace, FVector& OutForgePlace, FString& OutError)
	{
		const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("laufkarte.json"), OutError);
		const TSharedPtr<FJsonObject>* Dock = nullptr;
		if (!Root.IsValid())
		{
			return false;
		}
		FString Area;
		if (!Root->TryGetStringField(TEXT("gebiet"), Area) || Area.IsEmpty())
		{
			OutError = TEXT("Der Name gebiet fehlt.");
			return false;
		}
		if (!Root->TryGetObjectField(TEXT("anlegestelle"), Dock))
		{
			OutError = TEXT("Die anlegestelle fehlt.");
			return false;
		}
		float X = 0.0f;
		float Y = 0.0f;
		float Height = 0.0f;
		float BoardX = 0.0f;
		float BoardY = 0.0f;
		float ForgeX = 0.0f;
		float ForgeY = 0.0f;
		if (!FEIDataFile::ReadNumber(**Dock, TEXT("x_m"), -100000.0, 100000.0, X, OutError)
			|| !FEIDataFile::ReadNumber(**Dock, TEXT("y_m"), -100000.0, 100000.0, Y, OutError)
			|| !FEIDataFile::ReadNumber(**Dock, TEXT("hoehe_m"), -10000.0, 10000.0, Height, OutError)
			|| !FEIDataFile::ReadNumber(**Dock, TEXT("jagdbrett_x_m"), -1000.0, 1000.0, BoardX, OutError)
			|| !FEIDataFile::ReadNumber(**Dock, TEXT("jagdbrett_y_m"), -1000.0, 1000.0, BoardY, OutError)
			|| !FEIDataFile::ReadNumber(**Dock, TEXT("schmiede_x_m"), -1000.0, 1000.0, ForgeX, OutError)
			|| !FEIDataFile::ReadNumber(**Dock, TEXT("schmiede_y_m"), -1000.0, 1000.0, ForgeY, OutError))
		{
			return false;
		}
		OutArea = Area;
		OutCenter = FVector(X, Y, Height) * CentimetersPerMeter;
		OutBoardPlace = FVector(X + BoardX, Y + BoardY, Height) * CentimetersPerMeter;
		OutForgePlace = FVector(X + ForgeX, Y + ForgeY, Height) * CentimetersPerMeter;
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
		|| !FEIDataFile::ReadNumber(*Root, TEXT("signalfeuer_koop_tempo_bonus"), 0.0, 10.0, Loaded.SignalCoopSpeedBonus, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("barke_abstand_m"), 1.0, 100.0, Loaded.BargeDistanceMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("barke_abflug_tempo_m_pro_s"), 0.1, 100.0, Loaded.BargeDepartMetersPerSecond, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("jagdbrett_reichweite_m"), 0.5, 100.0, Loaded.HuntBoardReachMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("schmiede_reichweite_m"), 0.5, 100.0, Loaded.ForgeReachMeters, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("anfuehrer_leben_faktor"), 0.1, 100.0, Loaded.LeaderLifeFactor, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("anfuehrer_schaden_faktor"), 0.1, 100.0, Loaded.LeaderDamageFactor, OutError)
		|| !FEIDataFile::ReadNumber(*Root, TEXT("anfuehrer_groesse_faktor"), 0.1, 10.0, Loaded.LeaderScaleFactor, OutError))
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
	if (!LoadArrivals(Arrivals, Error) || !LoadPlaces(TEXT("bossplaetze"), BossPlaces, Error) || !LoadAreas(Areas, Error))
	{
		FEIDataFile::ReportError(TEXT("laufkarte.json"), Error);
	}
	bHasDock = LoadDock(AreaName, DockCenter, HuntBoardPlace, ForgePlace, Error);
	if (!bHasDock)
	{
		FEIDataFile::ReportError(TEXT("laufkarte.json"), Error);
	}
	if (!FEIBossTable::LoadFromProjectFile(BossTable, Error))
	{
		FEIDataFile::ReportError(TEXT("bosse.json"), Error);
	}
	// A boss that names an area must also find it on the map.
	for (const FEIBossValues& Named : BossTable.Bosses)
	{
		if (!Named.Area.IsEmpty() && !Areas.ContainsByPredicate([&Named](const FEIPlaceArea& Area) { return Area.Name == Named.Area; }))
		{
			FEIDataFile::ReportError(TEXT("bosse.json"), FString::Printf(TEXT("Das Gebiet %s von %s steht nicht in laufkarte.json."), *Named.Area, *Named.Name));
		}
	}
	if (AEIGameState* State = GetWorld() ? GetWorld()->GetGameState<AEIGameState>() : nullptr)
	{
		State->SetSignalCoopBonus(Values.SignalCoopSpeedBonus);
	}
}

void UEIRunDirector::ApplyValues(const FEIRunValues& NewValues)
{
	Values = NewValues;
	if (AEIGameState* State = GetWorld() ? GetWorld()->GetGameState<AEIGameState>() : nullptr)
	{
		State->SetSignalCoopBonus(Values.SignalCoopSpeedBonus);
	}
}

UEIRunDirector* UEIRunDirector::Get(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}
	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return nullptr;
	}
	if (AEIGameMode* GM = Cast<AEIGameMode>(World->GetAuthGameMode()))
	{
		return GM->GetRun();
	}
	for (TActorIterator<AEIGameMode> It(World); It; ++It)
	{
		return It->GetRun();
	}
	return nullptr;
}

void UEIRunDirector::StartHuntFromSkyBarge()
{
	BeginRunNow();
}

void UEIRunDirector::AddBossTrackProgress(int32 Amount)
{
	CurrentBossTracks += Amount;
}

void UEIRunDirector::AddUnsecuredLoot(int32 Souls)
{
	UnsecuredSouls += FMath::RoundToInt(Souls * EscalationLootMultiplier);
}

void UEIRunDirector::IncreaseEscalationTier()
{
	EscalationTier++;
	EscalationLootMultiplier = 1.0f + (EscalationTier - 1) * 0.5f;
	EscalationNoteSeconds = 5.0f;
}

void UEIRunDirector::ActivateWorldMutator(FName MutatorId)
{
	if (const FEIWorldMutatorDef* Def = FEIDataFile::FindWorldMutator(MutatorId))
	{
		ActiveMutator = *Def;
	}
	else
	{
		ActiveMutator = FEIWorldMutatorDef();
		ActiveMutator.Id = MutatorId;
	}
	MutatorTimer = 0.0f;
	EIVisuals::SetWorldAtmosphereLightFactor(GetWorld(), ActiveMutator.LightFactor);
}

void UEIRunDirector::TickWorldMutator(float DeltaTime)
{
	if (ActiveMutator.Id.IsNone() || ActiveMutator.Id == TEXT("klar"))
	{
		return;
	}

	if (EscalationNoteSeconds > 0.0f)
	{
		EscalationNoteSeconds = FMath::Max(0.0f, EscalationNoteSeconds - DeltaTime);
	}

	if (ActiveMutator.Id == TEXT("magnet_gewitter") && ActiveMutator.IntervalSeconds > 0.0f)
	{
		MutatorTimer += DeltaTime;
		if (MutatorTimer >= ActiveMutator.IntervalSeconds)
		{
			MutatorTimer = 0.0f;
			UWorld* World = GetWorld();
			if (!World)
			{
				return;
			}

			TArray<AActor*> Candidates;
			for (TActorIterator<AEIHeroCharacter> It(World); It; ++It)
			{
				if (!It->IsDead())
				{
					Candidates.Add(*It);
				}
			}
			for (TActorIterator<AEIHollowArmor> It(World); It; ++It)
			{
				Candidates.Add(*It);
			}

			if (!Candidates.IsEmpty())
			{
				AActor* Target = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
				const FVector TargetLoc = Target->GetActorLocation();
				AEIGroundMark::SpawnGroundMark(World, TargetLoc, 200.0f, 2.5f);

				FTimerHandle Handle;
				World->GetTimerManager().SetTimer(Handle, [this, TargetLoc, Target]()
				{
					if (!IsValid(this) || !GetWorld())
					{
						return;
					}
					EIVisuals::PlaySoundAt(this, TEXT("ExplosionKlein"), TargetLoc);
					if (IsValid(Target))
					{
						if (AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(Target))
						{
							if (Hero->GetHeroMovement() && Hero->GetHeroMovement()->IsBlocking())
							{
								EIVisuals::PlaySoundAt(this, TEXT("Parade"), TargetLoc);
							}
							else
							{
								Hero->TakeDamage(35.0f, FDamageEvent(), nullptr, nullptr);
							}
						}
						else
						{
							Target->TakeDamage(50.0f, FDamageEvent(), nullptr, nullptr);
						}
					}
				}, 2.5f, false);
			}
		}
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
	State->SetSignalCoopBonus(Values.SignalCoopSpeedBonus);
	State->SetSignalHold(0.0f, false, 0, Values.SignalHoldSeconds);
	State->SetCastingOff(false);
	State->SetBloodMoon(bForcedBloodMoon);
	WaveLeft = 0.0f;
	BloodMoonLeft = 0.0f;

	PrepareBoss();
	PrepareContracts();
	PrepareSignalFire();
	if (Barge == nullptr)
	{
		State->SetRunPhase(EEIRunPhase::None);
		return;
	}
	PrepareHuntBoard();
	PrepareForge();
	Barge->ReturnToStart();
	State->SetRunPhase(EEIRunPhase::AtDock);
}

void UEIRunDirector::PrepareHuntBoard()
{
	if (HuntBoard != nullptr || !bHasDock)
	{
		return;
	}
	FVector Ground = HuntBoardPlace;
	FindGround(HuntBoardPlace, Ground);
	// The letters face the middle of the dock, so that whoever walks to the barge sees them.
	const FVector Facing = (DockCenter - HuntBoardPlace).GetSafeNormal2D();
	const FTransform Spawn(Facing.IsNearlyZero() ? FRotator::ZeroRotator : Facing.Rotation(), Ground);
	HuntBoard = GetWorld()->SpawnActorDeferred<AEIHuntBoard>(AEIHuntBoard::StaticClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (HuntBoard != nullptr)
	{
		HuntBoard->Setup(Values.HuntBoardReachMeters);
		HuntBoard->FinishSpawning(Spawn);
	}
}

void UEIRunDirector::PrepareForge()
{
	if (Forge != nullptr || !bHasDock)
	{
		return;
	}
	FVector Ground = ForgePlace;
	FindGround(ForgePlace, Ground);
	// The anvil faces the middle of the dock, like the hunt board.
	const FVector Facing = (DockCenter - ForgePlace).GetSafeNormal2D();
	const FTransform Spawn(Facing.IsNearlyZero() ? FRotator::ZeroRotator : Facing.Rotation(), Ground);
	Forge = GetWorld()->SpawnActorDeferred<AEIForge>(AEIForge::StaticClass(), Spawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Forge != nullptr)
	{
		Forge->Setup(Values.ForgeReachMeters);
		Forge->FinishSpawning(Spawn);
	}
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
	if (Kettle != nullptr)
	{
		Kettle->Destroy();
		Kettle = nullptr;
	}
	for (AEIChest* Chest : StoneChests)
	{
		if (Chest != nullptr)
		{
			Chest->Destroy();
		}
	}
	StoneChests.Reset();
	if (Armor != nullptr)
	{
		Armor->Destroy();
		Armor = nullptr;
	}
	bKettleHeld = false;
	bStoneWork = false;
	CarriedStones.Reset();
	if (Boss != nullptr)
	{
		Boss->Destroy();
		Boss = nullptr;
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const bool bHasBoss = Barge != nullptr && !BossTable.Bosses.IsEmpty() && !BossPlaces.IsEmpty();
	BossPlace = FVector::ZeroVector;
	State->SetBossStage(bHasBoss ? EEIBossStage::Sealed : EEIBossStage::None);
	if (!bHasBoss)
	{
		State->SetHuntLetters(TArray<FEIHuntLetter>(), INDEX_NONE);
		SetBossIndex(INDEX_NONE);
		return;
	}

	// Each letter names another boss, in a new order every time. What a boss drops stays a surprise.
	TArray<int32> Order;
	for (int32 Index = 0; Index < BossTable.Bosses.Num(); ++Index)
	{
		Order.Add(Index);
	}
	for (int32 Index = Order.Num() - 1; Index > 0; --Index)
	{
		Order.Swap(Index, FMath::RandRange(0, Index));
	}
	TArray<FEIHuntLetter> Letters;
	for (int32 Index = 0; Index < FMath::Min(BossTable.HuntLetters, Order.Num()); ++Index)
	{
		const FEIBossValues& Named = BossTable.Bosses[Order[Index]];
		FEIHuntLetter& Letter = Letters.AddDefaulted_GetRef();
		// The letter says where the boss lives: in its own area, or on the whole map.
		Letter.Area = Named.Area.IsEmpty() ? AreaName : Named.Area;
		Letter.Boss = Named.Name;
		Letter.Bannbruch = Named.Bannbruch.Name;
		Letter.Color = Named.Color;
		Letter.BossIndex = Order[Index];
	}
	State->SetHuntLetters(Letters, 0);
	// The boss of the first letter waits, in its area. Taking another letter at the board picks a new place.
	SetBossIndex(Letters[0].BossIndex);
}

void UEIRunDirector::SetBossIndex(int32 NewBossIndex)
{
	BossIndex = NewBossIndex;
	if (NewBossIndex != INDEX_NONE && !BossPlaces.IsEmpty())
	{
		BossPlace = PickBossPlace();
		// A new place also needs a fire that keeps its distance from it.
		PrepareSignalFire();
	}
	GetWorld()->GetGameState<AEIGameState>()->SetBannbruchProgress(0, BossIndex != INDEX_NONE ? BossTable.Bosses[BossIndex].Bannbruch.Count : 0);
}

const FEIPlaceArea* UEIRunDirector::GetCurrentArea() const
{
	if (BossIndex == INDEX_NONE || !BossTable.Bosses.IsValidIndex(BossIndex))
	{
		return nullptr;
	}
	const FString& Named = BossTable.Bosses[BossIndex].Area;
	if (Named.IsEmpty())
	{
		return nullptr;
	}
	return Areas.FindByPredicate([&Named](const FEIPlaceArea& Area) { return Area.Name == Named; });
}

FVector UEIRunDirector::PickBossPlace() const
{
	if (BossPlaces.IsEmpty())
	{
		return FVector::ZeroVector;
	}
	// Inside the area of the taken letter. If it has no place after all, the whole map is the fallback.
	const FEIPlaceArea* Area = GetCurrentArea();
	TArray<const FVector*> Choice;
	for (const FVector& Place : BossPlaces)
	{
		if (Area == nullptr || FVector::Dist2D(Place, Area->Center) <= Area->Radius)
		{
			Choice.Add(&Place);
		}
	}
	if (Choice.IsEmpty())
	{
		for (const FVector& Place : BossPlaces)
		{
			Choice.Add(&Place);
		}
	}
	return *Choice[FMath::RandRange(0, Choice.Num() - 1)];
}

bool UEIRunDirector::ChooseLetter(int32 Index)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (State->GetRunPhase() != EEIRunPhase::AtDock || State->IsCastingOff() || !State->GetHuntLetters().IsValidIndex(Index))
	{
		return false;
	}
	State->SetChosenLetter(Index);
	SetBossIndex(State->GetHuntLetters()[Index].BossIndex);
	return true;
}

void UEIRunDirector::ChooseNextLetter()
{
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const int32 Count = State->GetHuntLetters().Num();
	if (Count > 0)
	{
		ChooseLetter((FMath::Max(State->GetChosenLetter(), 0) + 1) % Count);
	}
}

void UEIRunDirector::ForceBoss(int32 NewBossIndex)
{
	if (BossTable.Bosses.IsValidIndex(NewBossIndex) && GetWorld()->GetGameState<AEIGameState>()->GetBossStage() == EEIBossStage::Sealed && Boss == nullptr)
	{
		SetBossIndex(NewBossIndex);
	}
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
	if (Bannbruch.Kind == EEIBannbruchKind::Kettle)
	{
		SpawnKettle(Ground, Bannbruch);
	}
	else if (Bannbruch.Kind == EEIBannbruchKind::SoulStones)
	{
		SpawnSoulStones(Ground, Bannbruch);
	}
	else
	{
		SpawnAnchors(Ground, Bannbruch);
	}
}

void UEIRunDirector::SpawnAnchors(const FVector& Ground, const FEIBannbruch& Bannbruch)
{
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

void UEIRunDirector::SpawnKettle(const FVector& Ground, const FEIBannbruch& Bannbruch)
{
	// The kettle stands a little away from the grave, on the ground there.
	const FVector Direction = FVector::ForwardVector.RotateAngleAxis(FMath::FRandRange(0.0f, 360.0f), FVector::UpVector);
	const FVector Wanted = Ground + Direction * Bannbruch.DistanceMeters * CentimetersPerMeter;
	FVector KettleGround = Wanted;
	FindGround(Wanted, KettleGround);
	const FTransform KettleSpawn(KettleGround);
	Kettle = GetWorld()->SpawnActorDeferred<AEISoulKettle>(AEISoulKettle::StaticClass(), KettleSpawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Kettle != nullptr)
	{
		Kettle->Setup(Ground, Bannbruch.ZoneMeters, Bannbruch.Count);
		Kettle->FinishSpawning(KettleSpawn);
	}
}

void UEIRunDirector::SpawnSoulStones(const FVector& Ground, const FEIBannbruch& Bannbruch)
{
	// The hollow armor stands where the boss will stand, and its soul stones lie in guarded chests in a ring around it.
	const FTransform ArmorSpawn(Ground);
	Armor = GetWorld()->SpawnActorDeferred<AEIHollowArmor>(AEIHollowArmor::StaticClass(), ArmorSpawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Armor != nullptr)
	{
		Armor->Setup(Bannbruch.ZoneMeters, Bannbruch.Count);
		Armor->FinishSpawning(ArmorSpawn);
	}
	const float Turn = FMath::FRandRange(0.0f, 360.0f);
	for (int32 Index = 0; Index < Bannbruch.Count; ++Index)
	{
		const FVector Direction = FVector::ForwardVector.RotateAngleAxis(Turn + Index * 360.0f / Bannbruch.Count, FVector::UpVector);
		const FVector Wanted = Ground + Direction * Bannbruch.DistanceMeters * CentimetersPerMeter;
		FVector ChestGround = Wanted;
		FindGround(Wanted, ChestGround);
		const FTransform ChestSpawn(ChestGround);
		if (AEISoulStoneChest* Chest = GetWorld()->SpawnActorDeferred<AEISoulStoneChest>(AEISoulStoneChest::StaticClass(), ChestSpawn, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Chest->FinishSpawning(ChestSpawn);
			Chest->PrepareForRun(true);
			StoneChests.Add(Chest);
		}
	}
}

void UEIRunDirector::FeedKettle(const AEIEnemy& Enemy)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (Kettle == nullptr || Boss == nullptr || &Enemy == Boss.Get() || State->GetBossStage() != EEIBossStage::Sealed || !State->AreContractsDone() || !Kettle->IsInZone(&Enemy))
	{
		return;
	}
	Kettle->AddSoul(Enemy.GetActorLocation());
	State->SetBannbruchProgress(Kettle->GetSouls(), Kettle->GetCount());
	if (Kettle->IsFull())
	{
		BreakBossFree(CountLivingHeroes());
	}
}

int32 UEIRunDirector::CountLivingHeroes() const
{
	int32 Heroes = 0;
	for (TActorIterator<AEIHeroCharacter> It(GetWorld()); It; ++It)
	{
		Heroes += It->IsDead() ? 0 : 1;
	}
	return Heroes;
}

TArray<AEIEnemy*> UEIRunDirector::SummonGroup(const FString& Size, const FVector& Center, float RadiusCm)
{
	const FEIEnemyGroup* Group = Groups.Pick(Size);
	if (Group == nullptr)
	{
		return TArray<AEIEnemy*>();
	}
	UEIHordeDirector* Horde = CastChecked<AEIGameMode>(GetOwner())->GetHorde();
	return Horde->SpawnGroup(*Group, GetPlayerFactor(CountLivingHeroes()), Center, RadiusCm);
}

void UEIRunDirector::UpdateBannbruch(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (State->GetBossStage() != EEIBossStage::Sealed || Boss == nullptr || !State->AreContractsDone())
	{
		bKettleHeld = false;
		for (AEIAnchorStone* Anchor : Anchors)
		{
			Anchor->UpdateHold(false, 0.0f);
		}
		return;
	}
	if (Kettle != nullptr)
	{
		// The kettle only fills from enemies that fall, the souls come in NoteEnemyDied. Whoever stands in the circle calls them.
		bKettleHeld = LivingHeroes.ContainsByPredicate([this](const AEIHeroCharacter* Hero) { return Kettle->IsInZone(Hero); });
		return;
	}
	if (Armor != nullptr)
	{
		UpdateStoneChests(LivingHeroes);
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

void UEIRunDirector::UpdateStoneChests(const TArray<AEIHeroCharacter*>& LivingHeroes)
{
	// The guards of a stone chest come when a hero steps into its circle, like the chests of the side jobs.
	for (AEIChest* Chest : StoneChests)
	{
		UpdateChest(Chest, LivingHeroes);
	}
	// A stone of a fallen hero goes back into its chest, so that no Bannbruch gets stuck.
	for (auto It = CarriedStones.CreateIterator(); It; ++It)
	{
		AEIHeroCharacter* Carrier = It->Key.Get();
		AEIChest* Chest = It->Value.Get();
		if (Chest == nullptr || Carrier == nullptr || Carrier->IsDead())
		{
			if (Carrier != nullptr)
			{
				Carrier->SetCarriesSoulStone(false);
			}
			if (Chest != nullptr)
			{
				Chest->CloseAgain();
			}
			It.RemoveCurrent();
		}
	}
	bStoneWork = !CarriedStones.IsEmpty()
		|| LivingHeroes.ContainsByPredicate([this](const AEIHeroCharacter* Hero)
		{
			return StoneChests.ContainsByPredicate([Hero](const TObjectPtr<AEIChest>& Chest) { return Chest != nullptr && Chest->IsInZone(Hero); });
		});
	if (Armor != nullptr)
	{
		GetWorld()->GetGameState<AEIGameState>()->SetBannbruchProgress(Armor->GetInserted(), Armor->GetTotal());
	}
}

void UEIRunDirector::BreakBossFree(int32 HeroCount)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetBossStage(EEIBossStage::Awake);
	WaveLeft = 0.0f;
	// The armor breaks with its Bannbruch, and every stone still on the way goes back to nobody.
	if (Armor != nullptr)
	{
		Armor->Destroy();
		Armor = nullptr;
	}
	for (AEIChest* Chest : StoneChests)
	{
		if (Chest != nullptr)
		{
			Chest->Destroy();
		}
	}
	StoneChests.Reset();
	for (TPair<TWeakObjectPtr<AEIHeroCharacter>, TWeakObjectPtr<AEIChest>>& Pair : CarriedStones)
	{
		if (AEIHeroCharacter* Carrier = Pair.Key.Get())
		{
			Carrier->SetCarriesSoulStone(false);
		}
	}
	CarriedStones.Reset();
	bStoneWork = false;
	Boss->BreakFree(HeroCount);
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
	if (Kettle != nullptr)
	{
		Kettle->SetSouls(Kettle->GetCount());
	}
	if (Armor != nullptr)
	{
		Armor->SetInserted(Armor->GetTotal());
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (Boss != nullptr && State->GetBossStage() == EEIBossStage::Sealed)
	{
		State->SetBannbruchProgress(State->GetBannbruchTotal(), State->GetBannbruchTotal());
		BreakBossFree(CountLivingHeroes());
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

	// Everybody rolls for themselves, and whoever has fallen in the fight still has a share of it.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (const AEIPlayerController* Player = Cast<AEIPlayerController>(It->Get()))
		{
			Player->GetProfile()->GrantBossLoot(Fallen.Name, Fallen.RollGold(), Fallen.RollLoot());
		}
	}
}

void UEIRunDirector::PrepareSignalFire()
{
	TArray<AEISignalFire*> Fires;
	TArray<AEISignalFire*> FarFires;
	TArray<AEISignalFire*> AllFires;
	const double MinDistance = BossTable.FireMinDistanceMeters * CentimetersPerMeter;
	// Only the fires of the taken letter's area count, so the run never points far out of it.
	const FEIPlaceArea* Area = GetCurrentArea();
	for (TActorIterator<AEISignalFire> It(GetWorld()); It; ++It)
	{
		It->SetLit(false);
		It->SetInUse(false);
		AllFires.Add(*It);
		if (Area != nullptr && FVector::Dist2D(It->GetActorLocation(), Area->Center) > Area->Radius)
		{
			continue;
		}
		Fires.Add(*It);
		if (BossIndex == INDEX_NONE || FVector::Dist2D(It->GetActorLocation(), BossPlace) >= MinDistance)
		{
			FarFires.Add(*It);
		}
	}
	// After the boss there should be a way to run. Without fires in the area, any fire is better than none.
	const TArray<AEISignalFire*>& Choice = !FarFires.IsEmpty() ? FarFires : (!Fires.IsEmpty() ? Fires : AllFires);
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
	// The barge sets the team down inside the area of the taken letter.
	const FEIPlaceArea* Area = GetCurrentArea();
	TArray<int32> ArrivalChoice;
	for (int32 Index = 0; Index < Arrivals.Num(); ++Index)
	{
		if (Area == nullptr || FVector::Dist2D(Arrivals[Index].Location, Area->Center) <= Area->Radius)
		{
			ArrivalChoice.Add(Index);
		}
	}
	if (ArrivalChoice.IsEmpty())
	{
		for (int32 Index = 0; Index < Arrivals.Num(); ++Index)
		{
			ArrivalChoice.Add(Index);
		}
	}
	const FEIArrivalPoint& Arrival = Arrivals[ArrivalChoice[FMath::RandRange(0, ArrivalChoice.Num() - 1)]];
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
	const FEIEnemyGroup* Group = Groups.Pick(Size);
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
		TArray<AEIEnemy*> Guards;
		if (const FEIEnemyGroup* Group = Groups.Pick(Groups.ChestGuards))
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
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const AEIPlayerController* Player = Cast<AEIPlayerController>(It->Get());
		const AEIHeroCharacter* Hero = Player != nullptr ? Player->GetPawn<AEIHeroCharacter>() : nullptr;
		if (Hero != nullptr && !Hero->IsDead() && FVector::Dist2D(Hero->GetActorLocation(), Contract.Location) <= Player->GetProfile()->GetTable().Rewards.RadiusMeters * CentimetersPerMeter)
		{
			Player->GetProfile()->GrantContract(Contract.Faction, ContractTable.GetGold(), ContractTable.GetPoints());
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

bool UEIRunDirector::TakeSoulStone(AEIHeroCharacter* Hero)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (State->GetBossStage() != EEIBossStage::Sealed || !State->AreContractsDone() || Hero->CarriesSoulStone())
	{
		return false;
	}
	for (AEIChest* Chest : StoneChests)
	{
		if (Chest != nullptr && !Chest->IsOpen() && Chest->IsInReach(Hero))
		{
			if (Chest->IsCursed() && !Chest->IsCurseActive())
			{
				Chest->TriggerCurseAmbush(Hero);
				return true;
			}
			if (Chest->GetState() == EEIChestState::Free)
			{
				Chest->Open();
				Hero->SetCarriesSoulStone(true);
				CarriedStones.Add(Hero, Chest);
				return true;
			}
		}
	}
	return false;
}

bool UEIRunDirector::InsertSoulStone(AEIHeroCharacter* Hero)
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (Armor == nullptr || State->GetBossStage() != EEIBossStage::Sealed || !State->AreContractsDone() || !Hero->CarriesSoulStone() || !Armor->IsInReach(Hero))
	{
		return false;
	}
	Armor->InsertStone();
	Hero->SetCarriesSoulStone(false);
	CarriedStones.Remove(Hero);
	State->SetBannbruchProgress(Armor->GetInserted(), Armor->GetTotal());
	if (Armor->IsFull())
	{
		BreakBossFree(CountLivingHeroes());
	}
	return true;
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
	FeedKettle(Enemy);
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
			Player->GetProfile()->GrantEnemyReward(Enemy.GetValues().Experience);
		}
	}
}

void UEIRunDirector::EndRunForAll(bool bEscaped)
{
	GetWorld()->GetGameState<AEIGameState>()->SetBloodMoon(bForcedBloodMoon);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (const AEIPlayerController* Player = Cast<AEIPlayerController>(It->Get()))
		{
			Player->GetProfile()->EndRun(bEscaped);
		}
	}
}

void UEIRunDirector::AbortRun()
{
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	const EEIRunPhase Phase = State->GetRunPhase();
	if (Phase != EEIRunPhase::Crossing && Phase != EEIRunPhase::Exploring && Phase != EEIRunPhase::SignalLit && Phase != EEIRunPhase::BargeWaiting)
	{
		return;
	}
	State->StopRunClock();
	State->SetRunPhase(EEIRunPhase::Failed);
	EndRunForAll(false);
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
		else if (HuntBoard != nullptr && HuntBoard->IsInReach(Hero))
		{
			ChooseNextLetter();
		}
		return;
	}
	const bool bRunning = Phase == EEIRunPhase::Exploring || Phase == EEIRunPhase::SignalLit || Phase == EEIRunPhase::BargeWaiting;
	if (bRunning && InsertSoulStone(Hero))
	{
		return;
	}
	if (bRunning && TakeSoulStone(Hero))
	{
		return;
	}
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
	WaveLeft = 0.0f;
	State->SetSignalCoopBonus(Values.SignalCoopSpeedBonus);
	State->SetSignalHold(HoldLeft, true, 1, Values.SignalHoldSeconds);
	State->SetRunPhase(EEIRunPhase::SignalLit);

	// The barge comes down on the side towards the middle of the map, where it is open.
	const FVector FireLocation = ActiveFire->GetActorLocation();
	FVector Hover = FireLocation - FireLocation.GetSafeNormal2D() * Values.BargeDistanceMeters * CentimetersPerMeter;
	Hover.Z = FireLocation.Z + BargeHoverHeightCm;
	Barge->BeginApproach(Hover);
}

bool UEIRunDirector::SendGroup(const FString& Size, int32 HeroCount)
{
	UEIHordeDirector* Horde = CastChecked<AEIGameMode>(GetOwner())->GetHorde();
	const FEIEnemyGroup* Group = Groups.Pick(Size);
	if (Group == nullptr || Horde->CountThreat() >= Groups.ThreatCap * GetPlayerFactor(HeroCount))
	{
		return true;
	}
	return !Horde->SpawnReinforcement(*Group, GetPlayerFactor(HeroCount)).IsEmpty();
}

const FEIWaveRule* UEIRunDirector::FindCurrentWave(EEIRunPhase Phase) const
{
	const AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	if (Phase == EEIRunPhase::SignalLit && State->IsSignalHeld())
	{
		return &Groups.FireWave;
	}
	if (State->GetBossStage() == EEIBossStage::Down)
	{
		return &Groups.PursuitWave;
	}
	if (State->GetBossStage() == EEIBossStage::Awake)
	{
		return &Groups.FightWave;
	}
	if (IsAnchorHeld())
	{
		return &Groups.StoneWave;
	}
	if (bKettleHeld)
	{
		return &Groups.KettleWave;
	}
	if (bStoneWork)
	{
		return &Groups.SoulStoneWave;
	}
	return State->IsContractHeld() ? &Groups.AltarWave : nullptr;
}

void UEIRunDirector::UpdateBloodMoon(EEIRunPhase Phase, const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	// Nothing rises by itself. The blood moon stands as long as a task calls it, and a little longer, so that the sky does not flicker on the edge of a zone.
	const FEIWaveRule* Wave = FindCurrentWave(Phase);
	BloodMoonLeft = Wave != nullptr ? Groups.BloodMoonLingerSeconds : FMath::Max(0.0f, BloodMoonLeft - DeltaTime);
	GetWorld()->GetGameState<AEIGameState>()->SetBloodMoon(bForcedBloodMoon || Wave != nullptr || BloodMoonLeft > 0.0f);
	if (Wave == nullptr)
	{
		// Stepping out of a zone neither brings the next wave nor saves it. Only once the sky has cleared the next one comes at once.
		WaveLeft = BloodMoonLeft > 0.0f ? WaveLeft : 0.0f;
		return;
	}
	WaveLeft -= DeltaTime;
	if (WaveLeft <= 0.0f && SendGroup(Wave->Size, LivingHeroes.Num()))
	{
		WaveLeft = Wave->EverySeconds;
	}
}

void UEIRunDirector::HoldSignal(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime)
{
	int32 HeroesInZone = 0;
	if (ActiveFire != nullptr)
	{
		for (const AEIHeroCharacter* Hero : LivingHeroes)
		{
			if (Hero != nullptr && ActiveFire->IsInZone(Hero))
			{
				++HeroesInZone;
			}
		}
	}
	const bool bHeld = HeroesInZone > 0;
	if (bHeld)
	{
		const float SpeedMultiplier = 1.0f + FMath::Max(0, HeroesInZone - 1) * Values.SignalCoopSpeedBonus;
		HoldLeft = FMath::Max(0.0f, HoldLeft - DeltaTime * SpeedMultiplier);
	}
	AEIGameState* State = GetWorld()->GetGameState<AEIGameState>();
	State->SetSignalHold(HoldLeft, bHeld, HeroesInZone, Values.SignalHoldSeconds);
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
		UpdateBloodMoon(Phase, LivingHeroes, DeltaTime);
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

	TickWorldMutator(DeltaTime);
}
