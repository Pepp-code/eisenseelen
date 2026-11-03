#include "EIGameState.h"

#include "EIVisuals.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float CorruptionNoteSeconds = 4.0f;
	constexpr float LookChangePerSecond = 0.2f;
}

AEIGameState::AEIGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEIGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIGameState, bHordeActive);
	DOREPLIFETIME(AEIGameState, RunPhase);
	DOREPLIFETIME(AEIGameState, RunStartedAt);
	DOREPLIFETIME(AEIGameState, RunEndedAfter);
	DOREPLIFETIME(AEIGameState, BeaconHoldLeft);
	DOREPLIFETIME(AEIGameState, bBeaconHeld);
	DOREPLIFETIME(AEIGameState, AltarsBroken);
	DOREPLIFETIME(AEIGameState, AltarsTotal);
	DOREPLIFETIME(AEIGameState, AltarHoldLeft);
	DOREPLIFETIME(AEIGameState, bAltarHeld);
	DOREPLIFETIME(AEIGameState, CorruptionStage);
	DOREPLIFETIME(AEIGameState, CorruptionShare);
	DOREPLIFETIME(AEIGameState, bBloodMoon);
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

void AEIGameState::SetAltarProgress(int32 Broken, int32 Total)
{
	AltarsBroken = Broken;
	AltarsTotal = Total;
}

void AEIGameState::SetAltarHold(float SecondsLeft, bool bHeld)
{
	AltarHoldLeft = SecondsLeft;
	bAltarHeld = bHeld;
}

void AEIGameState::SetCorruption(int32 Stage, float Share, bool bIsBloodMoon)
{
	const int32 PreviousStage = CorruptionStage;
	CorruptionStage = Stage;
	CorruptionShare = Share;
	bBloodMoon = bIsBloodMoon;
	OnRep_CorruptionStage(PreviousStage);
}

void AEIGameState::OnRep_CorruptionStage(int32 PreviousStage)
{
	if (CorruptionStage > PreviousStage)
	{
		CorruptionNoteRemaining = CorruptionNoteSeconds;
		EIVisuals::PlaySound2D(this, TEXT("Schlachtruf"));
	}
	else
	{
		CorruptionNoteRemaining = 0.0f;
	}
}

void AEIGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CorruptionNoteRemaining = FMath::Max(0.0f, CorruptionNoteRemaining - DeltaSeconds);
	if (!FMath::IsNearlyEqual(CorruptionLook, CorruptionShare))
	{
		CorruptionLook = FMath::FInterpConstantTo(CorruptionLook, CorruptionShare, DeltaSeconds, LookChangePerSecond);
		EIVisuals::ApplyCorruptionLook(GetWorld(), CorruptionLook);
	}
}
