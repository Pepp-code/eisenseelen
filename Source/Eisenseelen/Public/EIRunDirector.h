#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EIBosses.h"
#include "EIContracts.h"
#include "EIEnemyGroups.h"
#include "EIRunDirector.generated.h"

class AEIAnchorStone;
class AEIBoss;
class AEIChest;
class AEIHollowArmor;
class AEIEnemy;
class AEIEnemyCamp;
class AEIForge;
class AEIHeroCharacter;
class AEIHuntBoard;
class AEISignalFire;
class AEISkyBarge;
class AEISoulKettle;
enum class EEIRunPhase : uint8;

/** The numbers of a run from Data/lauf.json. */
struct EISENSEELEN_API FEIRunValues
{
	float CampFactor = 1.0f;
	float CampRadiusMeters = 3.0f;
	float ChestReachMeters = 3.0f;
	float ChestZoneMeters = 10.0f;
	float ChestGuardDistanceMeters = 6.0f;
	float CastOffSeconds = 3.0f;
	float CrossingSeconds = 4.0f;
	float ArrivalSpacingMeters = 2.5f;
	float ArrivalDepartMetersPerSecond = 14.0f;
	float SignalHoldSeconds = 60.0f;
	float SignalReachMeters = 4.0f;
	float SignalZoneMeters = 15.0f;
	float BargeDistanceMeters = 12.0f;
	float BargeDepartMetersPerSecond = 6.0f;
	float HuntBoardReachMeters = 4.0f;
	float ForgeReachMeters = 4.0f;
	EEIEnemyKind LeaderKind = EEIEnemyKind::ShieldBearer;
	float LeaderLifeFactor = 2.5f;
	float LeaderDamageFactor = 1.5f;
	float LeaderScaleFactor = 1.25f;

	static bool LoadFromProjectFile(FEIRunValues& OutValues, FString& OutError);
};

/** A place at the edge of the map where the barge sets the team down, from Data/laufkarte.json. */
struct EISENSEELEN_API FEIArrivalPoint
{
	FVector Location = FVector::ZeroVector;
	float YawDegrees = 0.0f;
};

/** One named area of the map, from the gebiete list in Data/laufkarte.json. Places inside the radius belong to it. */
struct EISENSEELEN_API FEIPlaceArea
{
	FString Name;
	FVector Center = FVector::ZeroVector;
	double Radius = 0.0;
};

/**
 * Leads a run from the dock to the flight home. Lives on the game mode, so only the server has it, and only does anything
 * on a map with a barge. What the players need to know goes through the game state.
 */
UCLASS()
class EISENSEELEN_API UEIRunDirector : public UActorComponent
{
	GENERATED_BODY()

public:
	UEIRunDirector();

	void LoadValues();
	const FEIRunValues& GetValues() const { return Values; }
	void ApplyValues(const FEIRunValues& NewValues) { Values = NewValues; }

	/** Who comes when. Tests may change it. */
	FEIEnemyGroups& GetGroups() { return Groups; }

	/** Which contracts a run offers. Tests may change it, and it counts from the next reset on. */
	FEIContractTable& GetContractTable() { return ContractTable; }

	const TArray<FEIArrivalPoint>& GetArrivals() const { return Arrivals; }

	/** The named areas of the map. A letter picks its boss's area, and the run then only uses places inside it. */
	const TArray<FEIPlaceArea>& GetAreas() const { return Areas; }

	/** The name of the whole map, for letters whose boss lives everywhere. */
	const FString& GetMapName() const { return AreaName; }

	/** Which bosses there are. Tests may change it, and it counts from the next reset on. With an empty table a run has no boss. */
	FEIBossTable& GetBossTable() { return BossTable; }
	AEIBoss* GetBoss() const { return Boss; }

	/** Which boss of the table waits in this run, or INDEX_NONE. */
	int32 GetBossIndex() const { return BossIndex; }
	const FVector& GetBossPlace() const { return BossPlace; }
	const TArray<TObjectPtr<AEIAnchorStone>>& GetAnchors() const { return Anchors; }
	AEISoulKettle* GetKettle() const { return Kettle; }
	const TArray<TObjectPtr<AEIChest>>& GetStoneChests() const { return StoneChests; }
	AEIHollowArmor* GetArmor() const { return Armor; }
	AEIHuntBoard* GetHuntBoard() const { return HuntBoard; }
	AEIForge* GetForge() const { return Forge; }

	/** Puts everything back to the dock, with new contracts and new hunt letters. */
	void ResetRun();

	/** Ends the run as failed, as if every hero had fallen. Only during a run. */
	void AbortRun();

	/** At the dock the team takes one of the hunt letters, and its boss waits in the run. False if it cannot be taken now. */
	bool ChooseLetter(int32 Index);

	/** The area of the current boss, or null if it lives on the whole map. */
	const FEIPlaceArea* GetCurrentArea() const;

	/** A boss place inside the area of the letter the team took. */
	FVector PickBossPlace() const;

	/** At the dock: the next hunt letter, after the last one the first again. */
	void ChooseNextLetter();

	/** For tests and preview pictures: this boss comes, whatever letter hangs at the board. Between ResetRun and BeginRunNow. */
	void ForceBoss(int32 NewBossIndex);

	/** A boss calls a group to its side, beside all caps. Server only. */
	TArray<AEIEnemy*> SummonGroup(const FString& Size, const FVector& Center, float RadiusCm);

	/** A hero pressed the use key. */
	void Interact(AEIHeroCharacter* Hero);

	/** Whoever stood near when an enemy fell has earned something. */
	void NoteEnemyDied(const AEIEnemy& Enemy);

	AEISkyBarge* GetBarge() const { return Barge; }
	AEISignalFire* GetActiveFire() const { return ActiveFire; }

	/** The crossing is over: the barge sets everybody who lives down at the edge of the map. Tests and preview pictures call it directly, to skip the way there. */
	void BeginRunNow();

	/** For preview pictures: the blood moon stands, whatever happens. */
	void ForceBloodMoon(bool bForce) { bForcedBloodMoon = bForce; }

	/** For tests and preview pictures: the Bannbruch is done at once, as if somebody had just finished it. */
	void SkipBannbruch();

	/** For tests and preview pictures: so many open contracts count as done, as if somebody had just finished them. */
	void SkipContracts(int32 Count);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void PrepareBoss();
	void PrepareContracts();
	void PrepareSignalFire();
	void PrepareHuntBoard();
	void PrepareForge();
	void SetBossIndex(int32 NewBossIndex);
	void SpawnLeaders();
	void SpawnBoss(int32 HeroCount);
	void SpawnAnchors(const FVector& Ground, const FEIBannbruch& Bannbruch);
	void SpawnKettle(const FVector& Ground, const FEIBannbruch& Bannbruch);
	void SpawnSoulStones(const FVector& Ground, const FEIBannbruch& Bannbruch);
	void FeedKettle(const AEIEnemy& Enemy);
	int32 CountLivingHeroes() const;
	void UpdateBannbruch(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void UpdateStoneChests(const TArray<AEIHeroCharacter*>& LivingHeroes);
	void BreakBossFree(int32 HeroCount);
	void BossFell();
	bool IsAnchorHeld() const;
	void UpdateDock(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void UpdateContracts(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void UpdateChest(AEIChest* Chest, const TArray<AEIHeroCharacter*>& LivingHeroes);
	void CompleteContract(int32 Index);
	bool OpenChest(AEIHeroCharacter* Hero);
	bool TakeSoulStone(AEIHeroCharacter* Hero);
	bool InsertSoulStone(AEIHeroCharacter* Hero);
	bool FindGround(const FVector& Location, FVector& OutGround) const;
	void LightSignal();
	void HoldSignal(const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void LeaveOnceAllAboard(const TArray<AEIHeroCharacter*>& LivingHeroes);
	const FEIWaveRule* FindCurrentWave(EEIRunPhase Phase) const;
	void UpdateBloodMoon(EEIRunPhase Phase, const TArray<AEIHeroCharacter*>& LivingHeroes, float DeltaTime);
	void FillCamp(AEIEnemyCamp* Camp, const FString& Size, int32 HeroCount);
	bool SendGroup(const FString& Size, int32 HeroCount);
	float GetPlayerFactor(int32 HeroCount) const;
	void EndRunForAll(bool bEscaped);

	UPROPERTY(Transient)
	TObjectPtr<AEISkyBarge> Barge;

	UPROPERTY(Transient)
	TObjectPtr<AEISignalFire> ActiveFire;

	UPROPERTY(Transient)
	TObjectPtr<AEIBoss> Boss;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AEIAnchorStone>> Anchors;

	UPROPERTY(Transient)
	TObjectPtr<AEISoulKettle> Kettle;

	/** The guarded chests that hold the soul stones of the hollow armor. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AEIChest>> StoneChests;

	UPROPERTY(Transient)
	TObjectPtr<AEIHollowArmor> Armor;

	UPROPERTY(Transient)
	TObjectPtr<AEIHuntBoard> HuntBoard;

	UPROPERTY(Transient)
	TObjectPtr<AEIForge> Forge;

	FEIBossTable BossTable;
	FString AreaName;
	TArray<FEIPlaceArea> Areas;
	FVector DockCenter = FVector::ZeroVector;
	FVector HuntBoardPlace = FVector::ZeroVector;
	FVector ForgePlace = FVector::ZeroVector;
	bool bHasDock = false;
	bool bKettleHeld = false;
	/** Whether a hero carries a soul stone or stands at a stone chest right now. */
	bool bStoneWork = false;
	/** Which hero carries the stone of which chest, so that a fallen hero gives it back. */
	TMap<TWeakObjectPtr<AEIHeroCharacter>, TWeakObjectPtr<AEIChest>> CarriedStones;
	TArray<FVector> BossPlaces;
	int32 BossIndex = INDEX_NONE;
	FVector BossPlace = FVector::ZeroVector;
	FEIRunValues Values;
	FEIEnemyGroups Groups;
	FEIContractTable ContractTable;
	TArray<FEIArrivalPoint> Arrivals;
	bool bPrepared = false;
	float CastOffLeft = 0.0f;
	float CrossingLeft = 0.0f;
	float HoldLeft = 0.0f;
	float WaveLeft = 0.0f;
	float BloodMoonLeft = 0.0f;
	bool bForcedBloodMoon = false;
};
