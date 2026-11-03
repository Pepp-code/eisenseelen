#include "EIHeroMovementComponent.h"

#include "EIHeroCharacter.h"
#include "EIHeroValues.h"
#include "GameFramework/Character.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
	// Input further away than this from the flight direction means "stop", not "turn around at full speed".
	constexpr double MaxSteerAngleDegrees = 135.0;
}

/** Sends the sprint, block and dodge wishes to the server and restores the predicted state when a move is replayed. */
class FEIHeroSavedMove : public FSavedMove_Character
{
public:
	virtual void Clear() override
	{
		FSavedMove_Character::Clear();
		bSavedWantsToSprint = false;
		bSavedWantsToBlock = false;
		bSavedWantsToDodge = false;
		SavedDodgeCooldownRemaining = 0.0f;
	}

	virtual uint8 GetCompressedFlags() const override
	{
		uint8 Flags = FSavedMove_Character::GetCompressedFlags();
		if (bSavedWantsToDodge)
		{
			Flags |= FLAG_Custom_0;
		}
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
		// The timer ticks per move, so moves only combine while it does not run.
		const FEIHeroSavedMove* New = static_cast<const FEIHeroSavedMove*>(NewMove.Get());
		const bool bTimerIdle = SavedDodgeCooldownRemaining <= 0.0f && New->SavedDodgeCooldownRemaining <= 0.0f;
		return bTimerIdle && bSavedWantsToSprint == New->bSavedWantsToSprint && bSavedWantsToBlock == New->bSavedWantsToBlock
			&& FSavedMove_Character::CanCombineWith(NewMove, Character, MaxDelta);
	}

	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override
	{
		FSavedMove_Character::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
		const UEIHeroMovementComponent* Movement = CastChecked<UEIHeroMovementComponent>(Character->GetCharacterMovement());
		bSavedWantsToSprint = Movement->bWantsToSprint;
		bSavedWantsToBlock = Movement->bWantsToBlock;
		bSavedWantsToDodge = Movement->bWantsToDodge;
		SavedDodgeCooldownRemaining = Movement->DodgeCooldownRemaining;
	}

	virtual void PrepMoveFor(ACharacter* Character) override
	{
		FSavedMove_Character::PrepMoveFor(Character);
		UEIHeroMovementComponent* Movement = CastChecked<UEIHeroMovementComponent>(Character->GetCharacterMovement());
		Movement->bWantsToSprint = bSavedWantsToSprint;
		Movement->bWantsToBlock = bSavedWantsToBlock;
		Movement->bWantsToDodge = bSavedWantsToDodge;
		Movement->DodgeCooldownRemaining = SavedDodgeCooldownRemaining;
	}

private:
	bool bSavedWantsToSprint = false;
	bool bSavedWantsToBlock = false;
	bool bSavedWantsToDodge = false;
	float SavedDodgeCooldownRemaining = 0.0f;
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
	bWantsToDodge = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bWantsToBlock = (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
}

void UEIHeroMovementComponent::ApplyValues(const FEIHeroValues& Values)
{
	MaxWalkSpeed = Values.RunSpeedMetersPerSecond * CentimetersPerMeter;
	AirControl = Values.AirControl;
	GravityScale = Values.GravityFactor;
	// A jump of height h starts at v = sqrt(2 * g * h), so gravity changes the air time and not the height.
	JumpZVelocity = FMath::Sqrt(2.0f * FMath::Abs(GetGravityZ()) * Values.JumpHeightMeters * CentimetersPerMeter);

	SprintSpeedCm = Values.SprintSpeedMetersPerSecond * CentimetersPerMeter;
	BlockSpeedCm = Values.BlockSpeedMetersPerSecond * CentimetersPerMeter;
	DiveFromHeightCm = Values.DiveFromHeightMeters * CentimetersPerMeter;
	DiveSpeedCm = Values.DiveSpeedMetersPerSecond * CentimetersPerMeter;
	DiveFallSpeedCm = Values.DiveFallSpeedMetersPerSecond * CentimetersPerMeter;
	DodgeSpeedCm = Values.DodgeSpeedMetersPerSecond * CentimetersPerMeter;
	DodgeHopCm = Values.DodgeHopMetersPerSecond * CentimetersPerMeter;
	DodgeCooldownSeconds = Values.DodgeCooldownSeconds;
	AirTurnDegreesPerSecond = Values.AirTurnDegreesPerSecond;
}

void UEIHeroMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	DodgeCooldownRemaining = FMath::Max(0.0f, DodgeCooldownRemaining - DeltaSeconds);
	BlockHeldSeconds = bWantsToBlock ? BlockHeldSeconds + DeltaSeconds : 0.0f;
	if (bWantsToDodge)
	{
		bWantsToDodge = false;
		if (IsDodgeReady() && (IsMovingOnGround() || (IsFalling() && !IsDiving())))
		{
			PerformDodge();
		}
	}
}

float UEIHeroMovementComponent::GetMaxSpeed() const
{
	if (!IsMovingOnGround() && !IsFalling())
	{
		return Super::GetMaxSpeed();
	}
	if (IsDiving())
	{
		return DiveSpeedCm;
	}
	if (bWantsToBlock)
	{
		return BlockSpeedCm;
	}
	return bWantsToSprint ? SprintSpeedCm : Super::GetMaxSpeed();
}

bool UEIHeroMovementComponent::IsDiving() const
{
	return IsFalling() && UpdatedComponent != nullptr && UpdatedComponent->GetComponentLocation().Z > DiveFromHeightCm;
}

FVector UEIHeroMovementComponent::NewFallVelocity(const FVector& InitialVelocity, const FVector& Gravity, float DeltaTime) const
{
	FVector Result = Super::NewFallVelocity(InitialVelocity, Gravity, DeltaTime);
	if (IsDiving())
	{
		Result.Z = FMath::Max(Result.Z, static_cast<double>(-DiveFallSpeedCm));
	}
	return Result;
}

void UEIHeroMovementComponent::PerformDodge()
{
	const FVector InputDirection = Acceleration.GetSafeNormal2D();
	// Without input the dodge goes backwards, away from what the hero is looking at.
	const FVector Direction = InputDirection.IsNearlyZero() ? -GetCharacterOwner()->GetActorForwardVector().GetSafeNormal2D() : InputDirection;
	// A fixed speed instead of a bonus on top, so dodges cannot build up speed.
	Velocity.X = Direction.X * DodgeSpeedCm;
	Velocity.Y = Direction.Y * DodgeSpeedCm;
	// The hop is only for the ground: in the air a dodge must not stop the fall, or dodging would carry the hero far.
	if (IsMovingOnGround())
	{
		Velocity.Z = FMath::Max(Velocity.Z, static_cast<double>(DodgeHopCm));
		SetMovementMode(MOVE_Falling);
	}
	DodgeCooldownRemaining = DodgeCooldownSeconds;

	// A replayed move must not repeat the feedback.
	if (!CharacterOwner->bClientUpdating)
	{
		if (AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(CharacterOwner))
		{
			Hero->OnDodgePerformed();
		}
	}
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
