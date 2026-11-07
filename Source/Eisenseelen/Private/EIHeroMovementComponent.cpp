#include "EIHeroMovementComponent.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"
#include "EIHeroValues.h"
#include "GameFramework/Character.h"
#include "LandscapeProxy.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	// Input further away than this from the flight direction means "stop", not "turn around at full speed".
	constexpr double MaxSteerAngleDegrees = 135.0;
	constexpr float DefaultWaterSpeedFactor = 0.85f;
	constexpr float WaterDepthThresholdCm = -25.0f;
}

/** Sends the sprint and block wishes to the server and restores the predicted state when a move is replayed. */
class FEIHeroSavedMove : public FSavedMove_Character
{
public:
	virtual void Clear() override
	{
		FSavedMove_Character::Clear();
		bSavedWantsToSprint = false;
		bSavedWantsToBlock = false;
	}

	virtual uint8 GetCompressedFlags() const override
	{
		uint8 Flags = FSavedMove_Character::GetCompressedFlags();
		if (bSavedWantsToSprint)
		{
			Flags |= FLAG_Custom_1;
		}
		if (bSavedWantsToBlock)
		{
			Flags |= FLAG_Custom_2;
		}
		return Flags;
	}

	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override
	{
		const FEIHeroSavedMove* New = static_cast<const FEIHeroSavedMove*>(NewMove.Get());
		return bSavedWantsToSprint == New->bSavedWantsToSprint && bSavedWantsToBlock == New->bSavedWantsToBlock
			&& FSavedMove_Character::CanCombineWith(NewMove, Character, MaxDelta);
	}

	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override
	{
		FSavedMove_Character::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
		const UEIHeroMovementComponent* Movement = CastChecked<UEIHeroMovementComponent>(Character->GetCharacterMovement());
		bSavedWantsToSprint = Movement->bWantsToSprint;
		bSavedWantsToBlock = Movement->bWantsToBlock;
	}

	virtual void PrepMoveFor(ACharacter* Character) override
	{
		FSavedMove_Character::PrepMoveFor(Character);
		UEIHeroMovementComponent* Movement = CastChecked<UEIHeroMovementComponent>(Character->GetCharacterMovement());
		Movement->bWantsToSprint = bSavedWantsToSprint;
		Movement->bWantsToBlock = bSavedWantsToBlock;
	}

private:
	bool bSavedWantsToSprint = false;
	bool bSavedWantsToBlock = false;
};

class FEIHeroPredictionData : public FNetworkPredictionData_Client_Character
{
public:
	explicit FEIHeroPredictionData(const UCharacterMovementComponent& Movement) : FNetworkPredictionData_Client_Character(Movement) {}

	virtual FSavedMovePtr AllocateNewMove() override
	{
		return MakeShared<FEIHeroSavedMove>();
	}
};

FNetworkPredictionData_Client* UEIHeroMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UEIHeroMovementComponent* MutableThis = const_cast<UEIHeroMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FEIHeroPredictionData(*this);
	}
	return ClientPredictionData;
}

void UEIHeroMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bWantsToBlock = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
}

void UEIHeroMovementComponent::ApplyValues(const FEIHeroValues& Values, float SpeedFactor)
{
	MaxWalkSpeed = Values.RunSpeedMetersPerSecond * CentimetersPerMeter * SpeedFactor;
	AirControl = Values.AirControl;
	GravityScale = Values.GravityFactor;
	// A jump of height h starts at v = sqrt(2 * g * h), so gravity changes the air time and not the height.
	JumpZVelocity = FMath::Sqrt(2.0f * FMath::Abs(GetGravityZ()) * Values.JumpHeightMeters * CentimetersPerMeter);

	SprintSpeedCm = Values.SprintSpeedMetersPerSecond * CentimetersPerMeter * SpeedFactor;
	BlockSpeedCm = Values.BlockSpeedMetersPerSecond * CentimetersPerMeter * SpeedFactor;
	AirTurnDegreesPerSecond = Values.AirTurnDegreesPerSecond;
	WaterSpeedFactor = DefaultWaterSpeedFactor;
	FString WorldError;
	const TSharedPtr<FJsonObject> WorldRoot = FEIDataFile::Load(TEXT("welt.json"), WorldError);
	if (WorldRoot.IsValid() && WorldRoot->HasField(TEXT("wasser_tempo_faktor")))
	{
		FEIDataFile::ReadNumber(*WorldRoot, TEXT("wasser_tempo_faktor"), 0.1, 1.0, WaterSpeedFactor, WorldError);
	}
}

void UEIHeroMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	BlockHeldSeconds = bWantsToBlock ? BlockHeldSeconds + DeltaSeconds : 0.0f;
}

bool UEIHeroMovementComponent::IsInShallowWater() const
{
	return IsMovingOnGround() && GetActorFeetLocation().Z < WaterDepthThresholdCm;
}

float UEIHeroMovementComponent::GetMaxSpeed() const
{
	const float BaseSpeed = Super::GetMaxSpeed() > 0.0f ? Super::GetMaxSpeed() : MaxWalkSpeed;
	if (!IsMovingOnGround() && !IsFalling())
	{
		return BaseSpeed * CarrySpeedModifier;
	}
	const float Factor = (IsInShallowWater() ? WaterSpeedFactor : 1.0f) * CarrySpeedModifier;
	if (bWantsToBlock)
	{
		return BlockSpeedCm * Factor;
	}
	return (bWantsToSprint ? SprintSpeedCm : BaseSpeed) * Factor;
}

bool UEIHeroMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	if (!Super::DoJump(bReplayingMoves, DeltaTime))
	{
		return false;
	}
	// A jump never carries more than the speed on the ground, so hopping is no faster than running.
	const FVector Clamped = FVector(Velocity.X, Velocity.Y, 0.0).GetClampedToMaxSize(GetMaxSpeed());
	Velocity.X = Clamped.X;
	Velocity.Y = Clamped.Y;
	return true;
}

void UEIHeroMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	if (IsFalling())
	{
		SteerTowardsInput(DeltaTime);
	}
}

void UEIHeroMovementComponent::SteerTowardsInput(float DeltaTime)
{
	// The engine steers in the air with a fixed acceleration, so the faster the flight, the slower the turn. Above the
	// run speed the flight direction turns at a fixed rate instead, and the speed stays.
	const FVector Horizontal(Velocity.X, Velocity.Y, 0.0);
	const double Speed = Horizontal.Size();
	const FVector Wish = Acceleration.GetSafeNormal2D();
	if (Speed <= MaxWalkSpeed || Wish.IsNearlyZero())
	{
		return;
	}
	const FVector Current = Horizontal / Speed;
	const double AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Current, Wish), -1.0, 1.0)));
	if (AngleDegrees > MaxSteerAngleDegrees)
	{
		return;
	}
	const double Step = FMath::Min(AngleDegrees, static_cast<double>(AirTurnDegreesPerSecond) * DeltaTime);
	const double Side = FVector::CrossProduct(Current, Wish).Z >= 0.0 ? 1.0 : -1.0;
	const FVector Turned = Current.RotateAngleAxis(Step * Side, FVector::UpVector);
	Velocity.X = Turned.X * Speed;
	Velocity.Y = Turned.Y * Speed;
}
