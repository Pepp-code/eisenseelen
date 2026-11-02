#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "EIHeroCharacter.h"
#include "EIPlayerController.h"
#include "EITestKeys.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
	const TCHAR* ArenaMap = TEXT("/Game/Maps/L_Testarena");
	constexpr double StartTimeoutSeconds = 120.0;
	constexpr double RunSeconds = 1.0;
	constexpr double SettleSeconds = 0.6;
	constexpr double HoldJumpSeconds = 0.2;
	constexpr double JumpTimeoutSeconds = 4.0;
	constexpr float CentimetersPerMeter = 100.0f;
}

/**
 * Plays the test arena in PIE through the real controls: W must run at the speed from Data/held.json and the space
 * bar must jump as high as that file says.
 */
class FEITestarenaPlayCommand : public IAutomationLatentCommand
{
public:
	explicit FEITestarenaPlayCommand(FAutomationTestBase* InTest) : Test(InTest), StartedAt(FPlatformTime::Seconds()) {}

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

		const double Now = World->GetTimeSeconds();
		const UCharacterMovementComponent* Movement = Hero->GetCharacterMovement();
		switch (Step)
		{
		case EStep::WaitForGround:
			if (!Movement->IsMovingOnGround())
			{
				if (FPlatformTime::Seconds() - StartedAt > StartTimeoutSeconds)
				{
					Test->AddError(TEXT("The hero never stood on the floor of the test arena"));
					return true;
				}
				return false;
			}
			CheckValuesAndLook(Player, Hero);
			Player->SetControlRotation(FRotator::ZeroRotator);
			EISendTestKey(Player, EKeys::W, IE_Pressed);
			Step = EStep::Run;
			StepAt = Now;
			return false;

		case EStep::Run:
			if (Now - StepAt < RunSeconds)
			{
				return false;
			}
			CheckRun(Hero);
			EISendTestKey(Player, EKeys::W, IE_Released);
			Step = EStep::Settle;
			StepAt = Now;
			return false;

		case EStep::Settle:
			if (Now - StepAt < SettleSeconds)
			{
				return false;
			}
			JumpStartZ = Hero->GetActorLocation().Z;
			JumpPeakZ = JumpStartZ;
			EISendTestKey(Player, EKeys::SpaceBar, IE_Pressed);
			Step = EStep::Jump;
			StepAt = Now;
			return false;

		case EStep::Jump:
			JumpPeakZ = FMath::Max(JumpPeakZ, Hero->GetActorLocation().Z);
			AirSeconds = Now - StepAt;
			if (Now - StepAt < HoldJumpSeconds)
			{
				return false;
			}
			if (!bJumpReleased)
			{
				EISendTestKey(Player, EKeys::SpaceBar, IE_Released);
				bJumpReleased = true;
			}
			if (!Movement->IsMovingOnGround())
			{
				if (Now - StepAt > JumpTimeoutSeconds)
				{
					Test->AddError(TEXT("The hero did not land after the jump"));
					return true;
				}
				return false;
			}
			CheckJump(Hero);
			return true;
		}
		return true;
	}

private:
	enum class EStep { WaitForGround, Run, Settle, Jump };

	void CheckValuesAndLook(AEIPlayerController* Player, const AEIHeroCharacter* Hero) const
	{
		FEIHeroValues FromFile;
		FString Error;
		Test->TestTrue(TEXT("Data/held.json loads"), FEIHeroValues::LoadFromProjectFile(FromFile, Error));
		Test->TestEqual(TEXT("The hero plays with the run speed from the file"), Hero->GetValues().RunSpeedMetersPerSecond, FromFile.RunSpeedMetersPerSecond);
		Test->TestEqual(TEXT("The hero plays with the jump height from the file"), Hero->GetValues().JumpHeightMeters, FromFile.JumpHeightMeters);
		Test->TestEqual(TEXT("The hero plays with the gravity from the file"), Hero->GetCharacterMovement()->GravityScale, FromFile.GravityFactor);

		Player->RotationInput = FRotator::ZeroRotator;
		Player->ApplyLookInput(FVector2D(1.0, 0.0));
		Test->TestTrue(TEXT("Mouse right turns right"), Player->RotationInput.Yaw > 0.0f);
		Player->RotationInput = FRotator::ZeroRotator;
		Player->ApplyLookInput(FVector2D(0.0, 1.0));
		Test->TestTrue(TEXT("Mouse up looks up"), Player->RotationInput.Pitch > 0.0f);
		Player->RotationInput = FRotator::ZeroRotator;
	}

	void CheckRun(const AEIHeroCharacter* Hero) const
	{
		const FVector Velocity = Hero->GetVelocity();
		const float Expected = Hero->GetValues().RunSpeedMetersPerSecond * CentimetersPerMeter;
		Test->AddInfo(FString::Printf(TEXT("W runs at %.0f cm/s, expected %.0f"), Velocity.Size2D(), Expected));
		Test->TestNearlyEqual(TEXT("W reaches the run speed"), static_cast<float>(Velocity.Size2D()), Expected, Expected * 0.05f);
		Test->TestTrue(TEXT("W runs forward"), Velocity.X > 0.0);
	}

	void CheckJump(const AEIHeroCharacter* Hero) const
	{
		const float Height = static_cast<float>(JumpPeakZ - JumpStartZ);
		const float Expected = Hero->GetValues().JumpHeightMeters * CentimetersPerMeter;
		Test->AddInfo(FString::Printf(TEXT("The space bar jumps %.0f cm, expected %.0f"), Height, Expected));
		Test->TestNearlyEqual(TEXT("The space bar reaches the jump height"), Height, Expected, Expected * 0.1f);

		const float Gravity = FMath::Abs(Hero->GetCharacterMovement()->GetGravityZ());
		const float ExpectedAirSeconds = 2.0f * FMath::Sqrt(2.0f * Expected / Gravity);
		Test->AddInfo(FString::Printf(TEXT("The jump stays in the air %.2f s, expected %.2f (%.0f cm)"), AirSeconds, ExpectedAirSeconds, Height));
		Test->TestNearlyEqual(TEXT("Gravity sets the air time"), static_cast<float>(AirSeconds), ExpectedAirSeconds, ExpectedAirSeconds * 0.15f);
	}

	FAutomationTestBase* Test;
	double StartedAt;
	EStep Step = EStep::WaitForGround;
	double StepAt = 0.0;
	double JumpStartZ = 0.0;
	double JumpPeakZ = 0.0;
	double AirSeconds = 0.0;
	bool bJumpReleased = false;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEITestarenaPlayTest,
	"Eisenseelen.Play.Testarena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEITestarenaPlayTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(ArenaMap))
	{
		AddError(TEXT("Cannot open the test arena for PIE"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FEITestarenaPlayCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
