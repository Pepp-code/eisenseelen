#include "EIHeroMovementComponent.h"

#include "EIHeroCharacter.h"
#include "EIHeroValues.h"
#include "GameFramework/Character.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;
}

/** Sends the dash wish to the server and restores the predicted state when a move is replayed. */
class FEIHeroSavedMove : public FSavedMove_Character
{
public:
	virtual void Clear() override
	{
		FSavedMove_Character::Clear();
		bSavedWantsToDash = false;
		SavedDashCooldownRemaining = 0.0f;
		SavedSpeedChainWindowRemaining = 0.0f;
	}

	virtual uint8 GetCompressedFlags() const override
	{
		uint8 Flags = FSavedMove_Character::GetCompressedFlags();
		if (bSavedWantsToDash)
		{
			Flags |= FLAG_Custom_0;
		}
		return Flags;
	}

	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override
	{
		// Timers tick per move, so moves only combine while no timer runs.
		const FEIHeroSavedMove* New = static_cast<const FEIHeroSavedMove*>(NewMove.Get());
		const bool bTimersIdle = SavedDashCooldownRemaining <= 0.0f && SavedSpeedChainWindowRemaining <= 0.0f
			&& New->SavedDashCooldownRemaining <= 0.0f && New->SavedSpeedChainWindowRemaining <= 0.0f;
		return bTimersIdle && FSavedMove_Character::CanCombineWith(NewMove, Character, MaxDelta);
	}

	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override
	{
		FSavedMove_Character::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
		const UEIHeroMovementComponent* Movement = CastChecked<UEIHeroMovementComponent>(Character->GetCharacterMovement());
		bSavedWantsToDash = Movement->bWantsToDash;
		SavedDashCooldownRemaining = Movement->DashCooldownRemaining;
		SavedSpeedChainWindowRemaining = Movement->SpeedChainWindowRemaining;
	}

	virtual void PrepMoveFor(ACharacter* Character) override
	{
		FSavedMove_Character::PrepMoveFor(Character);
		UEIHeroMovementComponent* Movement = CastChecked<UEIHeroMovementComponent>(Character->GetCharacterMovement());
		Movement->bWantsToDash = bSavedWantsToDash;
		Movement->DashCooldownRemaining = SavedDashCooldownRemaining;
		Movement->SpeedChainWindowRemaining = SavedSpeedChainWindowRemaining;
	}

private:
	bool bSavedWantsToDash = false;
	float SavedDashCooldownRemaining = 0.0f;
	float SavedSpeedChainWindowRemaining = 0.0f;
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
	bWantsToDash = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

void UEIHeroMovementComponent::ApplyValues(const FEIHeroValues& Values)
{
	MaxWalkSpeed = Values.RunSpeedMetersPerSecond * CentimetersPerMeter;
	AirControl = Values.AirControl;
	GravityScale = Values.GravityFactor;
	// A jump of height h starts at v = sqrt(2 * g * h), so gravity changes the air time and not the height.
	JumpZVelocity = FMath::Sqrt(2.0f * FMath::Abs(GetGravityZ()) * Values.JumpHeightMeters * CentimetersPerMeter);

	DashSpeedBonusCm = Values.DashSpeedBonusMetersPerSecond * CentimetersPerMeter;
	DashHopCm = Values.DashHopMetersPerSecond * CentimetersPerMeter;
	DashCooldownSeconds = Values.DashCooldownSeconds;
	SpeedChainWindowSeconds = Values.SpeedChainWindowSeconds;
	MaxSpeedCm = Values.MaxSpeedMetersPerSecond * CentimetersPerMeter;
}

void UEIHeroMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	DashCooldownRemaining = FMath::Max(0.0f, DashCooldownRemaining - DeltaSeconds);
	SpeedChainWindowRemaining = FMath::Max(0.0f, SpeedChainWindowRemaining - DeltaSeconds);
	if (bWantsToDash)
	{
		bWantsToDash = false;
		if (IsDashReady() && (IsMovingOnGround() || IsFalling()))
		{
			PerformDash();
		}
	}
}

void UEIHeroMovementComponent::PerformDash()
{
	const FVector InputDirection = Acceleration.GetSafeNormal2D();
	const FVector Direction = InputDirection.IsNearlyZero() ? GetCharacterOwner()->GetActorForwardVector().GetSafeNormal2D() : InputDirection;
	const float NewSpeed = FMath::Min(static_cast<float>(Velocity.Size2D()) + DashSpeedBonusCm, MaxSpeedCm);
	Velocity.X = Direction.X * NewSpeed;
	Velocity.Y = Direction.Y * NewSpeed;
	Velocity.Z = FMath::Max(Velocity.Z, static_cast<double>(DashHopCm));
	SetMovementMode(MOVE_Falling);
	DashCooldownRemaining = DashCooldownSeconds;

	// A replayed move must not repeat the feedback.
	if (!CharacterOwner->bClientUpdating)
	{
		if (AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(CharacterOwner))
		{
			Hero->OnDashPerformed();
		}
	}
}

void UEIHeroMovementComponent::ProcessLanded(const FHitResult& Hit, float RemainingTime, int32 Iterations)
{
	SpeedChainWindowRemaining = SpeedChainWindowSeconds;
	Super::ProcessLanded(Hit, RemainingTime, Iterations);
}

void UEIHeroMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	// Right after landing the ground does not brake yet: whoever jumps in time keeps the speed.
	const bool bKeepSpeed = SpeedChainWindowRemaining > 0.0f && IsMovingOnGround() && Velocity.Size2D() > MaxWalkSpeed;
	if (!bKeepSpeed)
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	}

	const FVector Horizontal(Velocity.X, Velocity.Y, 0.0);
	if (MaxSpeedCm > 0.0f && Horizontal.SizeSquared() > FMath::Square(MaxSpeedCm))
	{
		const FVector Clamped = Horizontal.GetClampedToMaxSize(MaxSpeedCm);
		Velocity.X = Clamped.X;
		Velocity.Y = Clamped.Y;
	}
}
