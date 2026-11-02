#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Components/CapsuleComponent.h"
#include "EIHeroCharacter.h"
#include "EIPlayerController.h"
#include "EITestKeys.h"
#include "Editor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
	const TCHAR* ArenaMap = TEXT("/Game/Maps/L_Testarena");
	const FName CourseTag(TEXT("Parcours"));
	const FName RampTag(TEXT("Rampe"));

	constexpr double StartTimeoutSeconds = 120.0;
	constexpr double RunOutSeconds = 1.0;
	constexpr double HoldJumpSeconds = 0.2;
	constexpr float CentimetersPerMeter = 100.0f;
	constexpr float JumpBeforeEdgeCm = 100.0f;
	constexpr float OnTopToleranceCm = 15.0f;
	// A block this close to the jump height may or may not work, so it is not a fair test either way.
	constexpr float MarginalJumpCm = 25.0f;

	/** The map's floor lies at z = 0, see tools/make_testarena.py. */
	float FeetHeightCm(const AEIHeroCharacter* Hero)
	{
		return static_cast<float>(Hero->GetActorLocation().Z) - Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	bool ReadTagNumber(const AActor* Marker, const FString& Key, float& OutNumber)
	{
		const FString Prefix = Key + TEXT("=");
		for (const FName& Tag : Marker->Tags)
		{
			const FString Text = Tag.ToString();
			if (Text.StartsWith(Prefix))
			{
				OutNumber = FCString::Atof(*Text.RightChop(Prefix.Len()));
				return true;
			}
		}
		return false;
	}
}

/**
 * Runs every course the arena marks with a target point, through the real controls. Ramps must carry the hero to the
 * top as long as they are walkable, and blocks must be reachable exactly when the jump height from Data/held.json
 * clears them.
 */
class FEITestarenaCourseCommand : public IAutomationLatentCommand
{
public:
	explicit FEITestarenaCourseCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

	virtual bool Update() override
	{
		UWorld* World = GEditor != nullptr ? GEditor->PlayWorld.Get() : nullptr;
		AEIPlayerController* Player = World != nullptr ? Cast<AEIPlayerController>(World->GetFirstPlayerController()) : nullptr;
		AEIHeroCharacter* Hero = Player != nullptr ? Cast<AEIHeroCharacter>(Player->GetPawn()) : nullptr;
		if (Hero == nullptr || !Hero->HasActorBegunPlay())
		{
			if (FPlatformTime::Seconds() - StartedAt > StartTimeoutSeconds)
			{
				Test->AddError(TEXT("PIE of the test arena did not give a hero"));
				return true;
			}
			return false;
		}

		if (!bCollected)
		{
			bCollected = true;
			if (!CollectCourses(World))
			{
				return true;
			}
			BeginCourse(World, Player, Hero);
			return false;
		}

		const FCourse& Course = Courses[Index];
		const UCharacterMovementComponent* Movement = Hero->GetCharacterMovement();
		const double Now = World->GetTimeSeconds();
		bOnTop = bOnTop || (Movement->IsMovingOnGround() && FeetHeightCm(Hero) >= Course.HeightCm - OnTopToleranceCm);
		if (!Course.bRamp)
		{
			UpdateJump(Player, Hero, Course, Now);
		}
		if (Now - CourseStartedAt < CourseSeconds(Course, Hero))
		{
			return false;
		}

		EISendTestKey(Player, EKeys::W, IE_Released);
		CheckCourse(Course, Hero);
		if (++Index == Courses.Num())
		{
			return true;
		}
		BeginCourse(World, Player, Hero);
		return false;
	}

private:
	struct FCourse
	{
		FVector Start = FVector::ZeroVector;
		bool bRamp = false;
		float HeightCm = 0.0f;
		float RunUpCm = 0.0f;
		float AngleDegrees = 0.0f;
	};

	bool CollectCourses(UWorld* World)
	{
		for (TActorIterator<ATargetPoint> It(World); It; ++It)
		{
			if (!It->ActorHasTag(CourseTag))
			{
				continue;
			}
			FCourse Course;
			Course.Start = It->GetActorLocation();
			Course.bRamp = It->ActorHasTag(RampTag);
			const bool bComplete = ReadTagNumber(*It, TEXT("Hoehe"), Course.HeightCm)
				&& ReadTagNumber(*It, TEXT("Anlauf"), Course.RunUpCm)
				&& (!Course.bRamp || ReadTagNumber(*It, TEXT("Winkel"), Course.AngleDegrees));
			if (!bComplete)
			{
				Test->AddError(FString::Printf(TEXT("The course marker %s has incomplete tags"), *It->GetName()));
				return false;
			}
			Courses.Add(Course);
		}
		if (Courses.IsEmpty())
		{
			Test->AddError(TEXT("The test arena has no course markers. Rebuild it with tools/make_testarena.py"));
			return false;
		}
		return true;
	}

	void BeginCourse(UWorld* World, AEIPlayerController* Player, AEIHeroCharacter* Hero)
	{
		Hero->SetActorLocation(Courses[Index].Start, false, nullptr, ETeleportType::TeleportPhysics);
		Hero->GetCharacterMovement()->StopMovementImmediately();
		Player->SetControlRotation(FRotator::ZeroRotator);
		EISendTestKey(Player, EKeys::W, IE_Pressed);
		CourseStartedAt = World->GetTimeSeconds();
		bOnTop = false;
		bJumped = false;
		bJumpReleased = false;
	}

	void UpdateJump(AEIPlayerController* Player, const AEIHeroCharacter* Hero, const FCourse& Course, double Now)
	{
		const double JumpAtX = Course.Start.X + Course.RunUpCm - JumpBeforeEdgeCm;
		if (!bJumped && Hero->GetActorLocation().X >= JumpAtX)
		{
			EISendTestKey(Player, EKeys::SpaceBar, IE_Pressed);
			bJumped = true;
			JumpedAt = Now;
		}
		else if (bJumped && !bJumpReleased && Now - JumpedAt >= HoldJumpSeconds)
		{
			EISendTestKey(Player, EKeys::SpaceBar, IE_Released);
			bJumpReleased = true;
		}
	}

	static double CourseSeconds(const FCourse& Course, const AEIHeroCharacter* Hero)
	{
		const FEIHeroValues& Values = Hero->GetValues();
		const double SpeedCm = Values.RunSpeedMetersPerSecond * CentimetersPerMeter;
		if (Course.bRamp)
		{
			const double RampRunCm = Course.HeightCm / FMath::Tan(FMath::DegreesToRadians(Course.AngleDegrees));
			return (Course.RunUpCm + RampRunCm) / SpeedCm + RunOutSeconds;
		}
		const double Gravity = FMath::Abs(Hero->GetCharacterMovement()->GetGravityZ());
		const double AirSeconds = 2.0 * FMath::Sqrt(2.0 * Values.JumpHeightMeters * CentimetersPerMeter / Gravity);
		return Course.RunUpCm / SpeedCm + AirSeconds + RunOutSeconds;
	}

	void CheckCourse(const FCourse& Course, const AEIHeroCharacter* Hero) const
	{
		const TCHAR* Outcome = bOnTop ? TEXT("oben") : TEXT("nicht oben");
		if (Course.bRamp)
		{
			const bool bExpected = Course.AngleDegrees <= Hero->GetCharacterMovement()->GetWalkableFloorAngle();
			const FString Name = FString::Printf(TEXT("Rampe %.0f Grad, %.0f cm hoch: %s"), Course.AngleDegrees, Course.HeightCm, Outcome);
			Test->AddInfo(Name);
			Test->TestEqual(*Name, bOnTop, bExpected);
			return;
		}

		const float JumpCm = Hero->GetValues().JumpHeightMeters * CentimetersPerMeter;
		const FString Name = FString::Printf(TEXT("Klotz %.0f cm bei %.0f cm Sprung: %s"), Course.HeightCm, JumpCm, Outcome);
		Test->AddInfo(Name);
		if (FMath::Abs(Course.HeightCm - JumpCm) > MarginalJumpCm)
		{
			Test->TestEqual(*Name, bOnTop, Course.HeightCm < JumpCm);
		}
	}

	FAutomationTestBase* Test;
	double StartedAt;
	TArray<FCourse> Courses;
	bool bCollected = false;
	int32 Index = 0;
	double CourseStartedAt = 0.0;
	double JumpedAt = 0.0;
	bool bOnTop = false;
	bool bJumped = false;
	bool bJumpReleased = false;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITestarenaCourseTest,
	"Eisenseelen.Play.TestarenaCourses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEITestarenaCourseTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(ArenaMap))
	{
		AddError(TEXT("Cannot open the test arena for PIE"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FEITestarenaCourseCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
