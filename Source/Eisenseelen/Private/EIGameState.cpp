#include "EIGameState.h"

#include "Net/UnrealNetwork.h"

void AEIGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIGameState, bHordeActive);
	DOREPLIFETIME(AEIGameState, RunPhase);
	DOREPLIFETIME(AEIGameState, RunStartedAt);
	DOREPLIFETIME(AEIGameState, RunEndedAfter);
	DOREPLIFETIME(AEIGameState, BeaconHoldLeft);
	DOREPLIFETIME(AEIGameState, bBeaconHeld);
}

float AEIGameState::GetRunSeconds() const
{
	if (RunEndedAfter >= 0.0f)
	{
		return RunEndedAfter;
	}
	return static_cast<float>(GetServerWorldTimeSeconds() - RunStartedAt);
}

void AEIGameState::ResetRunClock()
{
	RunStartedAt = -1.0;
	RunEndedAfter = 0.0f;
}

void AEIGameState::StartRunClock()
{
	RunStartedAt = GetServerWorldTimeSeconds();
	RunEndedAfter = -1.0f;
}

void AEIGameState::StopRunClock()
{
	RunEndedAfter = RunStartedAt < 0.0 ? 0.0f : GetRunSeconds();
}

void AEIGameState::SetBeaconHold(float SecondsLeft, bool bHeld)
{
	BeaconHoldLeft = SecondsLeft;
	bBeaconHeld = bHeld;
}
