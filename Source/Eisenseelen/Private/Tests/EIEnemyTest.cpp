#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EIEnemy.h"
#include "EIGroundMark.h"
#include "Engine/DamageEvents.h"
#include "EIGameMode.h"
#include "EIGameState.h"
#include "EIHealthComponent.h"
#include "EIHordeDirector.h"
#include "EISwordComponent.h"
#include "EITestScript.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace EITestScript;

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	const FVector OpenFloor(-1500.0f, 0.0f, 92.0f);

	AEIGameMode* GetGameMode(const FEIPlayContext& Context)
	{
		return Context.World->GetAuthGameMode<AEIGameMode>();
	}

	AEIEnemy* FindEnemy(const FEIPlayContext& Context)
	{
		TActorIterator<AEIEnemy> It(Context.World);
		return It ? *It : nullptr;
	}

	AEIEnemy* SpawnEnemyAhead(const FEIPlayContext& Context, float DistanceCm)
	{
		const FVector Location = Context.Hero->GetActorLocation() + Context.Hero->GetActorForwardVector() * DistanceCm;
		return Context.World->SpawnActor<AEIEnemy>(AEIEnemy::StaticClass(), Location, FRotator(0.0f, 180.0f, 0.0f));
	}
}

namespace
{
	AEIEnemy* SpawnKindAhead(const FEIPlayContext& Context, EEIEnemyKind Kind, float DistanceCm)
	{
		const FVector Location = Context.Hero->GetActorLocation() + Context.Hero->GetActorForwardVector() * DistanceCm;
		const FTransform Spawn((Context.Hero->GetActorLocation() - Location).Rotation(), Location);
		AEIEnemy* Enemy = Context.World->SpawnActorDeferred<AEIEnemy>(AEIEnemy::StaticClass(), Spawn);
		Enemy->SetKind(Kind);
		Enemy->FinishSpawning(Spawn);
		return Enemy;
	}

	/** It stands there and does nothing, so that only what is tested hurts it. */
	AEIEnemy* SpawnStill(const FEIPlayContext& Context, EEIEnemyKind Kind, const FVector& Location, float Life)
	{
		const FTransform Spawn((Context.Hero->GetActorLocation() - Location).GetSafeNormal2D().Rotation(), Location);
		AEIEnemy* Enemy = Context.World->SpawnActorDeferred<AEIEnemy>(AEIEnemy::StaticClass(), Spawn);
		Enemy->SetKind(Kind);
		Enemy->FinishSpawning(Spawn);
		FEIEnemyValues Still = Enemy->GetValues();
		Still.SpeedMetersPerSecond = 0.0f;
		Still.AttackFromMeters = 0.1f;
		Enemy->ApplyValues(Still);
		Enemy->GetHealth()->Reset(Life);
		return Enemy;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIEnemyKindsTest,
	"Eisenseelen.Play.Enemy.Kinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIEnemyKindsTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TWeakObjectPtr<AEIEnemy> Enemy;
		float Life = 0.0f;
		bool bBoneFlew = false;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	constexpr float Hit = 100.0f;

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, 0.0f); }));
	Steps.Add(Wait(0.2));

	// The thrower stays away and hits from afar.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->Enemy = SpawnKindAhead(Context, EEIEnemyKind::Thrower, 1000.0f);
		State->Life = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(WaitUntil(TEXT("a thrown bone hits the hero"), 4.0, [State](const FEIPlayContext& Context)
	{
		TActorIterator<AEIThrownBone> Bone(Context.World);
		State->bBoneFlew = State->bBoneFlew || static_cast<bool>(Bone);
		return Context.Hero->GetHealth()->GetHealth() < State->Life;
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const AEIEnemy* Thrower = State->Enemy.Get();
		const float Lost = State->Life - Context.Hero->GetHealth()->GetHealth();
		Context.Test->TestTrue(TEXT("The hit comes from a bone in flight"), State->bBoneFlew);
		Context.Test->TestEqual(TEXT("A bone does the thrower's damage"), Lost, Thrower->GetValues().Damage);
		Context.Test->TestTrue(TEXT("The thrower stays at a distance"), FVector::Dist(Thrower->GetActorLocation(), Context.Hero->GetActorLocation()) > 600.0);
		State->Enemy->Destroy();
		Context.Hero->SetCanBeDamaged(false);
	}));
	Steps.Add(Wait(0.1));

	// The shield holds from the front, but not from behind, and it does not stop a heavy hit.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIEnemy* Bearer = SpawnKindAhead(Context, EEIEnemyKind::ShieldBearer, 200.0f);
		State->Enemy = Bearer;
		FEIEnemyValues Frozen = Bearer->GetValues();
		Frozen.SpeedMetersPerSecond = 0.0f;
		Frozen.AttackFromMeters = 0.1f;
		Bearer->ApplyValues(Frozen);
		Bearer->GetHealth()->Reset(5000.0f);

		const float Full = Bearer->GetHealth()->GetHealth();
		Bearer->TakeDamage(Hit, FDamageEvent(), Context.Hero->GetController(), Context.Hero);
		const float AfterFront = Bearer->GetHealth()->GetHealth();
		Context.Test->TestNearlyEqual(TEXT("From the front the shield holds"), Full - AfterFront, Hit * Frozen.ShieldDamageFactor, 0.01f);
		const FRotator Front = Bearer->GetActorRotation();
		Bearer->SetActorRotation(Front + FRotator(0.0f, 180.0f, 0.0f));
		Bearer->TakeDamage(Hit, FDamageEvent(), Context.Hero->GetController(), Context.Hero);
		Context.Test->TestNearlyEqual(TEXT("From behind the shield does not help"), AfterFront - Bearer->GetHealth()->GetHealth(), Hit, 0.01f);
		Bearer->SetActorRotation(Front);
		State->Life = Bearer->GetHealth()->GetHealth();
	}));
	Steps.Add(Press(EKeys::LeftMouseButton));
	Steps.Add(Wait(0.75));
	Steps.Add(Release(EKeys::LeftMouseButton));
	Steps.Add(WaitUntil(TEXT("the heavy swing is over"), 3.0, [](const FEIPlayContext& Context) { return !Context.Hero->GetSword()->IsSwinging(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIEnemy* Bearer = State->Enemy.Get();
		const float Lost = State->Life - Bearer->GetHealth()->GetHealth();
		Context.Test->AddInfo(FString::Printf(TEXT("A heavy hit at 200 cm takes %.0f life from the shield bearer"), Lost));
		Context.Test->TestNearlyEqual(TEXT("A heavy hit goes through the shield"), Lost, Context.Hero->GetSword()->GetValues().Heavy.Damage, 0.01f);
		Context.Test->TestTrue(TEXT("A heavy hit staggers the shield bearer"), Bearer->IsStaggering());
		Bearer->Destroy();
	}));
	Steps.Add(Wait(0.1));

	// The colossus is big, tough and does not stagger.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		AEIEnemy* Colossus = SpawnKindAhead(Context, EEIEnemyKind::Colossus, 500.0f);
		const FEIEnemyValues& Values = Colossus->GetValues();
		Context.Test->TestEqual(TEXT("The colossus has its size"), static_cast<float>(Colossus->GetActorScale3D().Z), Values.Scale);
		Context.Test->TestTrue(TEXT("The colossus is bigger than a runner"), Values.Scale > 1.0f);
		Context.Test->TestEqual(TEXT("The colossus has its life"), Colossus->GetHealth()->GetHealth(), Values.Life);
		Colossus->StaggerFromHeavyHit(1.0f);
		Context.Test->TestFalse(TEXT("A heavy hit does not stagger the colossus"), Colossus->IsStaggering());
		Colossus->Destroy();
		Context.Hero->SetCanBeDamaged(true);
	}));
	Steps.Add(Wait(0.1));

	// From a distance the colossus marks its path, and then it charges along it.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->Enemy = SpawnKindAhead(Context, EEIEnemyKind::Colossus, 1000.0f);
		State->Life = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(WaitUntil(TEXT("the colossus marks its path"), 1.0, [State](const FEIPlayContext& Context) { return State->Enemy->IsStripMarked(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("While it marks its path, nothing has happened yet"), Context.Hero->GetHealth()->GetHealth(), State->Life);
	}));
	Steps.Add(WaitUntil(TEXT("the charge hits the hero"), 4.0, [State](const FEIPlayContext& Context) { return Context.Hero->GetHealth()->GetHealth() < State->Life; }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const float Lost = State->Life - Context.Hero->GetHealth()->GetHealth();
		Context.Test->AddInfo(FString::Printf(TEXT("The charge from 1000 cm takes %.0f life"), Lost));
		Context.Test->TestEqual(TEXT("The charge does its damage"), Lost, State->Enemy->GetValues().ChargeDamage);
		Context.Test->TestTrue(TEXT("The way stays marked while it runs"), State->Enemy->IsStripMarked());
		Context.Test->TestTrue(TEXT("The screen says what it was"), Context.Hero->GetLastHitNote().Contains(TEXT("Kettenkoloss: Ansturm")));
		Context.Hero->SetCanBeDamaged(false);
	}));
	Steps.Add(WaitUntil(TEXT("the charge ends"), 4.0, [State](const FEIPlayContext& Context) { return !State->Enemy->IsStripMarked(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->Enemy->Destroy(); }));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIEnemyRolesTest,
	"Eisenseelen.Play.Enemy.Roles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIEnemyRolesTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TWeakObjectPtr<AEIEnemy> Enemy;
		TWeakObjectPtr<AEIEnemy> Neighbour;
		float Life = 0.0f;
		float Damage = 0.0f;
		float EnemyDamage = 0.0f;
		double Distance = 0.0;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	constexpr float Hit = 100.0f;
	constexpr float NeighbourLife = 1000.0f;
	const auto IsTelegraphing = [State](const FEIPlayContext& Context) { return State->Enemy.IsValid() && State->Enemy->IsTelegraphing(); };
	const auto IsNotTelegraphing = [State](const FEIPlayContext& Context) { return State->Enemy.IsValid() && !State->Enemy->IsTelegraphing(); };

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, 0.0f); }));
	Steps.Add(Wait(0.2));

	// The ember skull marks a circle around itself and bursts. Whoever is still inside gets hurt, its own kind too.
	Steps.Add(Do([State, NeighbourLife](const FEIPlayContext& Context)
	{
		const FVector Ahead = Context.Hero->GetActorLocation() + Context.Hero->GetActorForwardVector() * 180.0f;
		AEIEnemy* Skull = SpawnKindAhead(Context, EEIEnemyKind::EmberSkull, 180.0f);
		State->Enemy = Skull;
		State->Damage = Skull->GetValues().Damage;
		State->EnemyDamage = Skull->GetValues().ExplosionEnemyDamage;
		State->Neighbour = SpawnStill(Context, EEIEnemyKind::Runner, Ahead + Context.Hero->GetActorRightVector() * 150.0f, NeighbourLife);
		State->Life = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(WaitUntil(TEXT("the ember skull lights its fuse"), 2.0, IsTelegraphing));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const AEIEnemy* Skull = State->Enemy.Get();
		const FEIGroundMarkData& Mark = Skull->GetGroundMark();
		Context.Test->TestTrue(TEXT("It marks a circle"), Mark.bVisible && Mark.Shape == EEIMarkShape::Circle);
		Context.Test->TestEqual(TEXT("As big as its blast"), Mark.LengthCm, Skull->GetValues().HitZoneMeters * CentimetersPerMeter * 2.0f);
		Context.Test->TestEqual(TEXT("While the fuse burns, nothing has happened yet"), Context.Hero->GetHealth()->GetHealth(), State->Life);
	}));
	Steps.Add(WaitUntil(TEXT("the ember skull bursts"), 3.0, [State](const FEIPlayContext& Context) { return !State->Enemy.IsValid() || State->Enemy->IsDead(); }));
	Steps.Add(Do([State, NeighbourLife](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The blast hurts the hero"), State->Life - Context.Hero->GetHealth()->GetHealth(), State->Damage);
		Context.Test->TestTrue(TEXT("The screen says what it was"), Context.Hero->GetLastHitNote().Contains(TEXT("Glutsch\u00E4del: Explosion")));
		Context.Test->TestEqual(TEXT("The blast hurts its own kind"), NeighbourLife - State->Neighbour->GetHealth()->GetHealth(), State->EnemyDamage);
		State->Neighbour->Destroy();
	}));
	Steps.Add(Wait(0.3));

	// Shot down before its fuse is out, it bursts too, but only its own kind gets hurt. Another skull in reach bursts along.
	Steps.Add(Do([State, NeighbourLife](const FEIPlayContext& Context)
	{
		const FVector Ahead = Context.Hero->GetActorLocation() + Context.Hero->GetActorForwardVector() * 150.0f;
		const FVector Right = Context.Hero->GetActorRightVector();
		State->Life = Context.Hero->GetHealth()->GetHealth();
		AEIEnemy* Skull = SpawnKindAhead(Context, EEIEnemyKind::EmberSkull, 150.0f);
		AEIEnemy* Other = SpawnStill(Context, EEIEnemyKind::EmberSkull, Ahead - Right * 200.0f, 1.0f);
		AEIEnemy* Neighbour = SpawnStill(Context, EEIEnemyKind::Runner, Ahead + Right * 150.0f, NeighbourLife);
		Skull->TakeDamage(10000.0f, FDamageEvent(), Context.Hero->GetController(), Context.Hero);
		Context.Test->TestTrue(TEXT("Shot down, it falls"), Skull->IsDead());
		Context.Test->TestEqual(TEXT("Shot down, it still tears its own kind apart"), NeighbourLife - Neighbour->GetHealth()->GetHealth(), State->EnemyDamage);
		Context.Test->TestTrue(TEXT("Another skull in reach bursts along"), Other->IsDead());
		Context.Test->TestEqual(TEXT("Shot down, it spares the hero"), Context.Hero->GetHealth()->GetHealth(), State->Life);
		Neighbour->Destroy();
		Context.Hero->SetCanBeDamaged(false);
	}));
	Steps.Add(Wait(0.3));

	// A spell goes around the shield. Right after its own blow the shield hangs low, and then everything goes through.
	Steps.Add(Do([State, Hit](const FEIPlayContext& Context)
	{
		AEIEnemy* Guard = SpawnKindAhead(Context, EEIEnemyKind::ShieldBearer, 170.0f);
		State->Enemy = Guard;
		FEIEnemyValues Rooted = Guard->GetValues();
		Rooted.SpeedMetersPerSecond = 0.0f;
		Guard->ApplyValues(Rooted);
		Guard->GetHealth()->Reset(5000.0f);
		float Before = Guard->GetHealth()->GetHealth();
		Guard->TakeDamage(Hit, FDamageEvent(UEISpellDamage::StaticClass()), Context.Hero->GetController(), Context.Hero);
		Context.Test->TestNearlyEqual(TEXT("A spell goes around the shield"), Before - Guard->GetHealth()->GetHealth(), Hit, 0.01f);
		Before = Guard->GetHealth()->GetHealth();
		Guard->TakeDamage(Hit, FDamageEvent(), Context.Hero->GetController(), Context.Hero);
		Context.Test->TestNearlyEqual(TEXT("Before its blow the shield holds"), Before - Guard->GetHealth()->GetHealth(), Hit * Rooted.ShieldDamageFactor, 0.01f);
		Context.Test->TestFalse(TEXT("Before its blow the shield is up"), Guard->IsShieldDown());
	}));
	Steps.Add(WaitUntil(TEXT("the shield bearer winds up"), 3.0, IsTelegraphing));
	Steps.Add(WaitUntil(TEXT("the shield bearer strikes"), 3.0, IsNotTelegraphing));
	Steps.Add(Do([State, Hit](const FEIPlayContext& Context)
	{
		AEIEnemy* Guard = State->Enemy.Get();
		const float Before = Guard->GetHealth()->GetHealth();
		Guard->TakeDamage(Hit, FDamageEvent(), Context.Hero->GetController(), Context.Hero);
		Context.Test->TestNearlyEqual(TEXT("Right after its blow the shield hangs low"), Before - Guard->GetHealth()->GetHealth(), Hit, 0.01f);
	}));
	Steps.Add(WaitUntil(TEXT("everyone sees the shield sink"), 1.0, [State](const FEIPlayContext& Context) { return State->Enemy->IsShieldDown(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->Enemy->Destroy(); }));
	Steps.Add(Wait(0.1));

	// After a throw the thrower steps back from a hero who came too close.
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->Enemy = SpawnKindAhead(Context, EEIEnemyKind::Thrower, 400.0f); }));
	Steps.Add(WaitUntil(TEXT("the thrower winds up"), 3.0, IsTelegraphing));
	Steps.Add(WaitUntil(TEXT("the thrower throws"), 3.0, IsNotTelegraphing));
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->Distance = FVector::Dist2D(State->Enemy->GetActorLocation(), Context.Hero->GetActorLocation()); }));
	Steps.Add(Wait(0.8));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const double Now = FVector::Dist2D(State->Enemy->GetActorLocation(), Context.Hero->GetActorLocation());
		Context.Test->AddInfo(FString::Printf(TEXT("The thrower stepped back from %.0f cm to %.0f cm"), State->Distance, Now));
		Context.Test->TestTrue(TEXT("After the throw it steps back"), Now > State->Distance + 100.0);
		State->Enemy->Destroy();
	}));
	Steps.Add(Wait(0.1));

	// Once the colossus runs, neither a parry nor frost stops it.
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->Enemy = SpawnKindAhead(Context, EEIEnemyKind::Colossus, 1000.0f); }));
	Steps.Add(WaitUntil(TEXT("the colossus marks its way"), 1.0, [State](const FEIPlayContext& Context) { return State->Enemy->IsStripMarked(); }));
	Steps.Add(WaitUntil(TEXT("the charge begins"), 3.0, IsNotTelegraphing));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		AEIEnemy* Colossus = State->Enemy.Get();
		Colossus->Stagger(2.0f);
		Colossus->Freeze(2.0f);
		Context.Test->TestFalse(TEXT("A parry does not stop the charge"), Colossus->IsStaggering());
		Context.Test->TestFalse(TEXT("Frost does not stop the charge"), Colossus->IsFrozen());
		Context.Test->TestTrue(TEXT("It keeps running"), Colossus->IsStripMarked());
	}));
	Steps.Add(WaitUntil(TEXT("the charge ends"), 4.0, [State](const FEIPlayContext& Context) { return !State->Enemy->IsStripMarked(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->Enemy->Freeze(2.0f);
		Context.Test->TestTrue(TEXT("After the charge frost holds it again"), State->Enemy->IsFrozen());
		State->Enemy->Destroy();
		Context.Hero->SetCanBeDamaged(true);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIEnemyStrikeZoneTest,
	"Eisenseelen.Play.Enemy.StrikeZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIEnemyStrikeZoneTest::RunTest(const FString& Parameters)
{
	// What a mark holds, whatever it looks like.
	FEIGroundMarkData Strip;
	Strip.Shape = EEIMarkShape::Strip;
	Strip.Location = FVector(100.0f, 0.0f, 0.0f);
	Strip.YawDegrees = 90.0f;
	Strip.LengthCm = 500.0f;
	Strip.WidthCm = 200.0f;
	TestTrue(TEXT("On the strip"), Strip.Contains(FVector(100.0f, 300.0f, 50.0f)));
	TestTrue(TEXT("At the edge of the strip"), Strip.Contains(FVector(195.0f, 300.0f, 0.0f)));
	TestFalse(TEXT("Beside the strip"), Strip.Contains(FVector(205.0f, 300.0f, 0.0f)));
	TestFalse(TEXT("Behind its start"), Strip.Contains(FVector(100.0f, -10.0f, 0.0f)));
	TestFalse(TEXT("Beyond its end"), Strip.Contains(FVector(100.0f, 510.0f, 0.0f)));
	FEIGroundMarkData Circle;
	Circle.Shape = EEIMarkShape::Circle;
	Circle.LengthCm = 400.0f;
	TestTrue(TEXT("In the circle"), Circle.Contains(FVector(190.0f, 0.0f, 80.0f)));
	TestFalse(TEXT("Outside the circle"), Circle.Contains(FVector(210.0f, 0.0f, 0.0f)));

	struct FState
	{
		TWeakObjectPtr<AEIEnemy> Enemy;
		float Life = 0.0f;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	const auto WindsUp = [State](const TCHAR* What)
	{
		return WaitUntil(What, 4.0, [State](const FEIPlayContext& Context) { return State->Enemy.IsValid() && State->Enemy->IsTelegraphing(); });
	};
	const auto WindupEnds = [State](const TCHAR* What)
	{
		return WaitUntil(What, 4.0, [State](const FEIPlayContext& Context) { return !State->Enemy->IsTelegraphing(); });
	};
	constexpr double AfterFollowSeconds = 0.25;
	constexpr float BehindCm = 150.0f;

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, 0.0f); }));
	Steps.Add(Wait(0.2));

	// A runner strikes what lies in front of it. Close behind it nothing happens.
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->Enemy = SpawnKindAhead(Context, EEIEnemyKind::Runner, 130.0f);
		State->Life = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(WindsUp(TEXT("the runner winds up")));
	Steps.Add(Wait(AfterFollowSeconds));
	Steps.Add(Do([State, BehindCm](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = State->Enemy.Get();
		Teleport(Context, Enemy->GetActorLocation() - Enemy->GetActorForwardVector() * BehindCm, 0.0f);
	}));
	Steps.Add(WindupEnds(TEXT("the runner strikes")));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Behind it the blow misses"), Context.Hero->GetHealth()->GetHealth(), State->Life);
	}));

	// In front of it the blow lands, and the screen says what it was.
	Steps.Add(WindsUp(TEXT("the runner winds up again")));
	Steps.Add(WindupEnds(TEXT("the runner strikes again")));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const float Lost = State->Life - Context.Hero->GetHealth()->GetHealth();
		Context.Test->TestEqual(TEXT("In front of it the blow lands"), Lost, State->Enemy->GetValues().Damage);
		Context.Test->TestTrue(TEXT("The screen says what it was"), Context.Hero->GetLastHitNote().Contains(TEXT("Knochenl\u00E4ufer: Hieb")));
		Context.Test->TestTrue(TEXT("The screen says it for a while"), Context.Hero->GetHitNoteLeft() > 0.0f);
		Context.Test->TestFalse(TEXT("A small enemy does not mark its blow"), State->Enemy->IsStripMarked());
		State->Enemy->Destroy();
		State->Life = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(Wait(0.1));

	// The blow of a colossus lies on the floor. Just beside it nothing happens, close as it may be.
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->Enemy = SpawnKindAhead(Context, EEIEnemyKind::Colossus, 250.0f); }));
	Steps.Add(WindsUp(TEXT("the colossus winds up")));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = State->Enemy.Get();
		const FEIGroundMarkData& Mark = Enemy->GetGroundMark();
		Context.Test->TestTrue(TEXT("Its blow is marked"), Enemy->IsStripMarked());
		Context.Test->TestEqual(TEXT("As wide as its zone"), Mark.WidthCm, Enemy->GetValues().HitWidthMeters * CentimetersPerMeter);
		Context.Test->TestEqual(TEXT("It fills up until the blow"), Mark.FillSeconds, Enemy->GetValues().WindupSeconds);
		Context.Test->TestTrue(TEXT("The floor shows it"), TActorIterator<AEIGroundMark>(Context.World) ? true : false);
		const FVector Side = FRotationMatrix(Enemy->GetActorRotation()).GetUnitAxis(EAxis::Y);
		const FVector Spot = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * 100.0f + Side * (Mark.WidthCm / 2.0f + 60.0f);
		Teleport(Context, FVector(Spot.X, Spot.Y, OpenFloor.Z), 0.0f);
	}));
	Steps.Add(WindupEnds(TEXT("the colossus strikes")));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Beside the mark the blow misses"), Context.Hero->GetHealth()->GetHealth(), State->Life);
		Context.Test->TestFalse(TEXT("The mark is gone"), State->Enemy->IsStripMarked());
	}));
	Steps.Add(Wait(0.05));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestFalse(TEXT("The floor is clean"), TActorIterator<AEIGroundMark>(Context.World) ? true : false);
	}));

	// Inside it lands.
	Steps.Add(WindsUp(TEXT("the colossus winds up again")));
	Steps.Add(WindupEnds(TEXT("the colossus strikes again")));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("On the mark the blow lands"), State->Life - Context.Hero->GetHealth()->GetHealth(), State->Enemy->GetValues().Damage);
		State->Enemy->Destroy();
		State->Life = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(Wait(0.1));

	// Beside the way of a charge nothing happens, even as it thunders past.
	Steps.Add(Do([State](const FEIPlayContext& Context) { State->Enemy = SpawnKindAhead(Context, EEIEnemyKind::Colossus, 1000.0f); }));
	Steps.Add(WaitUntil(TEXT("the colossus marks its way"), 1.0, [State](const FEIPlayContext& Context) { return State->Enemy->IsStripMarked(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = State->Enemy.Get();
		const FEIGroundMarkData& Mark = Enemy->GetGroundMark();
		Context.Test->TestEqual(TEXT("The way is as wide as the file says"), Mark.WidthCm, Enemy->GetValues().ChargeWidthMeters * CentimetersPerMeter);
		const FVector Side = FRotationMatrix(Enemy->GetActorRotation()).GetUnitAxis(EAxis::Y);
		const FVector Spot = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * 500.0f + Side * (Mark.WidthCm / 2.0f + 80.0f);
		Teleport(Context, FVector(Spot.X, Spot.Y, OpenFloor.Z), 0.0f);
	}));
	Steps.Add(WindupEnds(TEXT("the charge begins")));
	Steps.Add(WaitUntil(TEXT("the charge ends"), 4.0, [State](const FEIPlayContext& Context) { return !State->Enemy->IsStripMarked(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Beside the way the charge misses"), Context.Hero->GetHealth()->GetHealth(), State->Life);
		Context.Hero->SetCanBeDamaged(false);
		State->Enemy->Destroy();
	}));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIEnemyAttackTest,
	"Eisenseelen.Play.Enemy.Attack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIEnemyAttackTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		double TelegraphAt = 0.0;
		float LifeBefore = 0.0f;
	};
	const TSharedRef<FState> State = MakeShared<FState>();

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Teleport(Context, OpenFloor, 0.0f);
		State->LifeBefore = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Test->TestNotNull(TEXT("An enemy can spawn"), SpawnEnemyAhead(Context, 800.0f)); }));

	// It comes running.
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		const float Expected = Enemy->GetValues().SpeedMetersPerSecond * CentimetersPerMeter;
		Context.Test->AddInfo(FString::Printf(TEXT("The runner runs at %.0f cm/s, expected %.0f"), Enemy->GetVelocity().Size2D(), Expected));
		Context.Test->TestNearlyEqual(TEXT("The enemy runs at its speed"), static_cast<float>(Enemy->GetVelocity().Size2D()), Expected, Expected * 0.25f);
	}));

	// It warns first, and only hits after the windup.
	Steps.Add(WaitUntil(TEXT("the enemy winds up"), 5.0, [](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		return Enemy != nullptr && Enemy->IsTelegraphing();
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->TelegraphAt = Context.Now;
		Context.Test->TestEqual(TEXT("No damage before the windup ends"), Context.Hero->GetHealth()->GetHealth(), State->LifeBefore);
	}));
	Steps.Add(WaitUntil(TEXT("the hero takes damage"), 3.0, [State](const FEIPlayContext& Context)
	{
		return Context.Hero->GetHealth()->GetHealth() < State->LifeBefore;
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		const double Windup = Context.Now - State->TelegraphAt;
		Context.Test->AddInfo(FString::Printf(TEXT("Warning to damage: %.2f s, the plan asks for at least 0.40"), Windup));
		Context.Test->TestTrue(TEXT("The warning comes at least 0.4 s before the damage"), Windup >= 0.4);
		const float Damage = State->LifeBefore - Context.Hero->GetHealth()->GetHealth();
		Context.Test->TestEqual(TEXT("The hit does the enemy's damage"), Damage, FindEnemy(Context)->GetValues().Damage);
	}));

	// Whoever moves away during the windup is not hit.
	Steps.Add(WaitUntil(TEXT("the enemy winds up again"), 5.0, [](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		return Enemy != nullptr && Enemy->IsTelegraphing();
	}));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->LifeBefore = Context.Hero->GetHealth()->GetHealth();
		Teleport(Context, Context.Hero->GetActorLocation() - FVector(600.0f, 0.0f, 0.0f), 0.0f);
	}));
	Steps.Add(WaitUntil(TEXT("the windup ends"), 3.0, [](const FEIPlayContext& Context) { return !FindEnemy(Context)->IsTelegraphing(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("Moving away in time avoids the hit"), Context.Hero->GetHealth()->GetHealth(), State->LifeBefore);
	}));

	// Two light swings kill it, and it leaves the world.
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		AEIEnemy* Enemy = FindEnemy(Context);
		Teleport(Context, Enemy->GetActorLocation() - FVector(180.0f, 0.0f, 0.0f), 0.0f);
	}));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.1));
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(WaitUntil(TEXT("the enemy is gone"), 3.0, [](const FEIPlayContext& Context) { return FindEnemy(Context) == nullptr; }));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIBlockTest,
	"Eisenseelen.Play.Enemy.Block",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIBlockTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		float LifeBefore = 0.0f;
		double TelegraphAt = 0.0;
	};
	const TSharedRef<FState> State = MakeShared<FState>();
	constexpr float TestHit = 10.0f;
	constexpr float LifeTolerance = 0.01f;
	constexpr double RaiseBeforeStrikeSeconds = 0.15;

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, 0.0f); }));
	Steps.Add(Wait(0.2));

	// The second key raises the block, and the block slows the hero even with shift held.
	Steps.Add(Press(EKeys::RightMouseButton));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Test->TestTrue(TEXT("The second key blocks"), Context.Movement->IsBlocking()); }));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Press(EKeys::LeftShift));
	Steps.Add(Wait(0.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const float Speed = static_cast<float>(Context.Hero->GetVelocity().Size2D());
		const float Expected = Context.Hero->GetValues().BlockSpeedMetersPerSecond * CentimetersPerMeter;
		Context.Test->AddInfo(FString::Printf(TEXT("Blocking with W and shift: %.0f cm/s, expected %.0f"), Speed, Expected));
		Context.Test->TestNearlyEqual(TEXT("The block slows the hero"), Speed, Expected, 20.0f);
	}));
	Steps.Add(Release(EKeys::LeftShift));
	Steps.Add(Release(EKeys::W));
	Steps.Add(Wait(0.3));

	// An attack goes first, and the block comes back after it.
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The hero attacks out of the block"), Context.Hero->GetSword()->IsSwinging());
		Context.Test->TestFalse(TEXT("There is no block during a swing"), Context.Movement->IsBlocking());
	}));
	Steps.Add(WaitUntil(TEXT("the swing ends"), 2.0, [](const FEIPlayContext& Context) { return !Context.Hero->GetSword()->IsSwinging(); }));
	Steps.Add(Wait(0.1));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Test->TestTrue(TEXT("The block comes back"), Context.Movement->IsBlocking()); }));
	Steps.Add(Wait(0.3));

	// A hit from the front is reduced, without a parry after the first moment.
	Steps.Add(Do([State, TestHit, LifeTolerance](const FEIPlayContext& Context)
	{
		AEIEnemy* Enemy = SpawnEnemyAhead(Context, 800.0f);
		UEIHealthComponent* Health = Context.Hero->GetHealth();
		const float Before = Health->GetHealth();
		Context.Hero->TakeDamage(TestHit, FDamageEvent(), nullptr, Enemy);
		const float Expected = TestHit * (1.0f - Context.Hero->GetSword()->GetValues().Block.DamageReduction);
		Context.Test->TestNearlyEqual(TEXT("A block reduces a hit from the front"), Before - Health->GetHealth(), Expected, LifeTolerance);
		Context.Test->TestFalse(TEXT("A late block does not stagger"), Enemy->IsStaggering());
		Context.Test->TestTrue(TEXT("The block shows"), Context.Hero->GetSword()->GetBlockFlash() > 0.0f && !Context.Hero->GetSword()->WasParry());
		Context.Player->SetControlRotation(FRotator(0.0f, 180.0f, 0.0f));
	}));

	// A hit from behind goes through.
	Steps.Add(Wait(0.1));
	Steps.Add(Do([TestHit, LifeTolerance](const FEIPlayContext& Context)
	{
		UEIHealthComponent* Health = Context.Hero->GetHealth();
		const float Before = Health->GetHealth();
		Context.Hero->TakeDamage(TestHit, FDamageEvent(), nullptr, FindEnemy(Context));
		Context.Test->TestNearlyEqual(TEXT("A hit from behind goes through the block"), Before - Health->GetHealth(), TestHit, LifeTolerance);
		Context.Player->SetControlRotation(FRotator::ZeroRotator);
	}));
	Steps.Add(Release(EKeys::RightMouseButton));

	// The real thing: a block raised just before the strike is a parry. It takes nothing and staggers the runner.
	Steps.Add(WaitUntil(TEXT("the enemy winds up"), 5.0, [](const FEIPlayContext& Context) { return FindEnemy(Context)->IsTelegraphing(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		State->TelegraphAt = Context.Now;
		State->LifeBefore = Context.Hero->GetHealth()->GetHealth();
	}));
	Steps.Add(WaitUntil(TEXT("the strike is near"), 2.0, [State, RaiseBeforeStrikeSeconds](const FEIPlayContext& Context)
	{
		return Context.Now - State->TelegraphAt >= FindEnemy(Context)->GetValues().WindupSeconds - RaiseBeforeStrikeSeconds;
	}));
	Steps.Add(Press(EKeys::RightMouseButton));
	Steps.Add(WaitUntil(TEXT("the windup ends"), 2.0, [](const FEIPlayContext& Context) { return !FindEnemy(Context)->IsTelegraphing(); }));
	Steps.Add(Do([State](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("A parry takes no damage"), Context.Hero->GetHealth()->GetHealth(), State->LifeBefore);
		Context.Test->TestTrue(TEXT("A parry staggers the runner"), FindEnemy(Context)->IsStaggering());
		Context.Test->TestTrue(TEXT("The parry shows"), Context.Hero->GetSword()->GetBlockFlash() > 0.0f && Context.Hero->GetSword()->WasParry());
	}));

	// The runner does nothing for a while, then goes on. The block held since then is no parry any more.
	Steps.Add(Wait(0.2));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Test->TestFalse(TEXT("A staggered runner does not wind up"), FindEnemy(Context)->IsTelegraphing()); }));
	Steps.Add(WaitUntil(TEXT("the stagger ends"), 3.0, [](const FEIPlayContext& Context) { return !FindEnemy(Context)->IsStaggering(); }));
	Steps.Add(WaitUntil(TEXT("the runner hits the block"), 5.0, [State](const FEIPlayContext& Context)
	{
		return Context.Hero->GetHealth()->GetHealth() < State->LifeBefore;
	}));
	Steps.Add(Do([State, LifeTolerance](const FEIPlayContext& Context)
	{
		const AEIEnemy* Enemy = FindEnemy(Context);
		const float Expected = Enemy->GetValues().Damage * (1.0f - Context.Hero->GetSword()->GetValues().Block.DamageReduction);
		Context.Test->TestNearlyEqual(TEXT("A long block reduces the real strike"), State->LifeBefore - Context.Hero->GetHealth()->GetHealth(), Expected, LifeTolerance);
		Context.Test->TestFalse(TEXT("A long block staggers nobody"), Enemy->IsStaggering());
	}));
	Steps.Add(Release(EKeys::RightMouseButton));

	// A reduced hit that takes all the life but for a rounding error kills.
	Steps.Add(Wait(0.1));
	Steps.Add(Press(EKeys::RightMouseButton));
	Steps.Add(Wait(0.4));
	Steps.Add(Do([TestHit](const FEIPlayContext& Context)
	{
		const float Reduced = TestHit * (1.0f - Context.Hero->GetSword()->GetValues().Block.DamageReduction);
		Context.Hero->GetHealth()->Reset(Reduced + 0.0001f);
		Context.Hero->TakeDamage(TestHit, FDamageEvent(), nullptr, FindEnemy(Context));
		Context.Test->TestTrue(TEXT("No hero survives on a rounding error"), Context.Hero->IsDead());
	}));
	Steps.Add(Release(EKeys::RightMouseButton));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIHeavyStaggerTest,
	"Eisenseelen.Play.Enemy.HeavyStagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIHeavyStaggerTest::RunTest(const FString& Parameters)
{
	// Tough enough to survive, because a heavy swing kills a normal runner outright.
	constexpr float ToughLife = 1000.0f;
	const auto SwingHits = [ToughLife](const TCHAR* What)
	{
		return WaitUntil(What, 2.0, [ToughLife](const FEIPlayContext& Context) { return FindEnemy(Context)->GetHealth()->GetHealth() < ToughLife; });
	};
	const auto WaitForSwingEnd = []()
	{
		return WaitUntil(TEXT("the swing ends"), 2.0, [](const FEIPlayContext& Context) { return !Context.Hero->GetSword()->IsSwinging(); });
	};
	const auto AddHeavySwing = [](TArray<FEIPlayStep>& InSteps)
	{
		InSteps.Add(Press(EKeys::LeftMouseButton));
		InSteps.Add(WaitUntil(TEXT("the sword charges"), 2.0, [](const FEIPlayContext& Context) { return Context.Hero->GetSword()->GetChargeFraction() >= 1.0f; }));
		InSteps.Add(Release(EKeys::LeftMouseButton));
	};

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context) { Teleport(Context, OpenFloor, 0.0f); }));
	Steps.Add(Wait(0.2));
	Steps.Add(Do([ToughLife](const FEIPlayContext& Context) { SpawnEnemyAhead(Context, 180.0f)->GetHealth()->Reset(ToughLife); }));

	// A light swing does not stagger.
	AddTap(Steps, EKeys::LeftMouseButton);
	Steps.Add(SwingHits(TEXT("the light swing hits")));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Test->TestFalse(TEXT("A light swing does not stagger"), FindEnemy(Context)->IsStaggering()); }));
	Steps.Add(WaitForSwingEnd());
	Steps.Add(Do([ToughLife](const FEIPlayContext& Context) { FindEnemy(Context)->GetHealth()->Reset(ToughLife); }));

	// A heavy swing does, and the runner goes on afterwards.
	AddHeavySwing(Steps);
	Steps.Add(SwingHits(TEXT("the heavy swing hits")));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("A heavy swing staggers"), FindEnemy(Context)->IsStaggering());
		Context.Test->TestFalse(TEXT("The stagger stops the windup"), FindEnemy(Context)->IsTelegraphing());
	}));
	Steps.Add(WaitUntil(TEXT("the stagger ends"), 3.0, [](const FEIPlayContext& Context) { return !FindEnemy(Context)->IsStaggering(); }));
	Steps.Add(WaitForSwingEnd());

	// A kind of enemy with a factor of 0 does not care.
	Steps.Add(Do([ToughLife](const FEIPlayContext& Context)
	{
		AEIEnemy* Enemy = FindEnemy(Context);
		FEIEnemyValues Resistant = Enemy->GetValues();
		Resistant.HeavyHitStaggerFactor = 0.0f;
		Enemy->ApplyValues(Resistant);
		Enemy->GetHealth()->Reset(ToughLife);
	}));
	AddHeavySwing(Steps);
	Steps.Add(SwingHits(TEXT("the heavy swing hits the resistant kind")));
	Steps.Add(Do([](const FEIPlayContext& Context) { Context.Test->TestFalse(TEXT("A resistant kind does not stagger"), FindEnemy(Context)->IsStaggering()); }));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIHordeTest,
	"Eisenseelen.Play.Enemy.Horde",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIHordeTest::RunTest(const FString& Parameters)
{
	const TSharedRef<float> NearestSpawn = MakeShared<float>(TNumericLimits<float>::Max());
	const TSharedRef<TSet<const AActor*>> Seen = MakeShared<TSet<const AActor*>>();

	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Hero->SetCanBeDamaged(false);
		Context.Test->TestEqual(TEXT("The horde starts switched off"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
	}));

	// H: the horde grows to its limit, spawning at a distance.
	AddTap(Steps, EKeys::H);
	Steps.Add(WaitUntil(TEXT("the horde reaches its limit"), 15.0, [NearestSpawn, Seen](const FEIPlayContext& Context)
	{
		for (TActorIterator<AEIEnemy> It(Context.World); It; ++It)
		{
			if (!Seen->Contains(*It))
			{
				Seen->Add(*It);
				*NearestSpawn = FMath::Min(*NearestSpawn, static_cast<float>(FVector::Dist2D(It->GetActorLocation(), Context.Hero->GetActorLocation())));
			}
		}
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		return Horde->CountEnemies() >= FMath::RoundToInt(Horde->GetValues().MaxAlive);
	}));
	Steps.Add(Do([NearestSpawn](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		const float MinCm = Horde->GetValues().MinDistanceMeters * CentimetersPerMeter;
		Context.Test->AddInfo(FString::Printf(TEXT("The nearest spawn was %.0f cm away, the minimum is %.0f cm"), *NearestSpawn, MinCm));
		Context.Test->TestTrue(TEXT("Enemies spawn at a distance"), *NearestSpawn >= MinCm * 0.9f);
		Context.Test->TestTrue(TEXT("The game state knows the horde is on"), Context.World->GetGameState<AEIGameState>()->IsHordeActive());
	}));

	// The limit holds.
	Steps.Add(Wait(1.5));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHordeDirector* Horde = GetGameMode(Context)->GetHorde();
		Context.Test->TestTrue(TEXT("The horde stays at its limit"), Horde->CountEnemies() <= FMath::RoundToInt(Horde->GetValues().MaxAlive));
	}));

	// R: all gone, and the horde is off again.
	AddTap(Steps, EKeys::R);
	Steps.Add(Wait(0.3));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestEqual(TEXT("The restart removes all enemies"), GetGameMode(Context)->GetHorde()->CountEnemies(), 0);
		Context.Test->TestFalse(TEXT("The restart switches the horde off"), Context.World->GetGameState<AEIGameState>()->IsHordeActive());
	}));
	return RunInArena(this, MoveTemp(Steps));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEIHeroDeathTest,
	"Eisenseelen.Play.Enemy.HeroDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEIHeroDeathTest::RunTest(const FString& Parameters)
{
	const TSharedRef<FVector> DeadAt = MakeShared<FVector>();
	TArray<FEIPlayStep> Steps;
	Steps.Add(Do([DeadAt](const FEIPlayContext& Context)
	{
		Context.Hero->TakeDamage(100000.0f, FDamageEvent(), nullptr, nullptr);
		Context.Test->TestTrue(TEXT("Too much damage kills the hero"), Context.Hero->IsDead());
		*DeadAt = Context.Hero->GetActorLocation();
	}));

	// A dead hero does not move.
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.5));
	Steps.Add(Release(EKeys::W));
	Steps.Add(Do([DeadAt](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("A dead hero does not move"), FVector::Dist2D(*DeadAt, Context.Hero->GetActorLocation()) < 5.0);
	}));

	// R brings a fresh one.
	AddTap(Steps, EKeys::R);
	Steps.Add(WaitUntil(TEXT("a fresh hero stands"), 3.0, [](const FEIPlayContext& Context) { return !Context.Hero->IsDead() && Context.Movement->IsMovingOnGround(); }));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		const UEIHealthComponent* Health = Context.Hero->GetHealth();
		Context.Test->TestEqual(TEXT("The fresh hero has full life"), Health->GetHealth(), Health->GetMaxHealth());
	}));
	Steps.Add(Press(EKeys::W));
	Steps.Add(Wait(0.4));
	Steps.Add(Release(EKeys::W));
	Steps.Add(Do([](const FEIPlayContext& Context)
	{
		Context.Test->TestTrue(TEXT("The fresh hero moves"), Context.Hero->GetVelocity().Size2D() > 100.0);
	}));
	return RunInArena(this, MoveTemp(Steps));
}

#endif
