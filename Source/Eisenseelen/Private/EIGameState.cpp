#include "EIGameState.h"

#include "EIVisuals.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float BloodMoonNoteSeconds = 4.0f;
	constexpr float LookChangePerSecond = 0.33f;
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
	DOREPLIFETIME(AEIGameState, SignalHoldLeft);
	DOREPLIFETIME(AEIGameState, bSignalHeld);
	DOREPLIFETIME(AEIGameState, bCastingOff);
	DOREPLIFETIME(AEIGameState, BossStage);
	DOREPLIFETIME(AEIGameState, BannbruchDone);
	DOREPLIFETIME(AEIGameState, BannbruchTotal);
	DOREPLIFETIME(AEIGameState, Contracts);
	DOREPLIFETIME(AEIGameState, bBloodMoon);
	DOREPLIFETIME(AEIGameState, HuntLetters);
	DOREPLIFETIME(AEIGameState, ChosenLetter);
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

void AEIGameState::SetSignalHold(float SecondsLeft, bool bHeld)
{
	SignalHoldLeft = SecondsLeft;
	bSignalHeld = bHeld;
}

void AEIGameState::SetContracts(const TArray<FEIContract>& NewContracts)
{
	Contracts = NewContracts;
}

int32 AEIGameState::CountContractsDone() const
{
	int32 Done = 0;
	for (const FEIContract& Contract : Contracts)
	{
		Done += Contract.bDone ? 1 : 0;
	}
	return Done;
}

bool AEIGameState::AreContractsDone() const
{
	return CountContractsDone() >= Contracts.Num();
}

bool AEIGameState::IsContractHeld() const
{
	return Contracts.ContainsByPredicate([](const FEIContract& Contract) { return Contract.bHeld; });
}

bool AEIGameState::CanLightSignal() const
{
	return RunPhase == EEIRunPhase::Exploring && (BossStage == EEIBossStage::None || BossStage == EEIBossStage::Down);
}

void AEIGameState::SetBannbruchProgress(int32 Done, int32 Total)
{
	BannbruchDone = Done;
	BannbruchTotal = Total;
}

void AEIGameState::SetHuntLetters(const TArray<FEIHuntLetter>& NewLetters, int32 NewChosen)
{
	HuntLetters = NewLetters;
	ChosenLetter = NewChosen;
}

void AEIGameState::SetChosenLetter(int32 NewChosen)
{
	if (ChosenLetter != NewChosen)
	{
		ChosenLetter = NewChosen;
		OnRep_ChosenLetter();
	}
}

void AEIGameState::OnRep_ChosenLetter()
{
	EIVisuals::PlaySound2D(this, TEXT("Blocken"));
}

void AEIGameState::SetBloodMoon(bool bNewBloodMoon)
{
	if (bBloodMoon != bNewBloodMoon)
	{
		bBloodMoon = bNewBloodMoon;
		OnRep_BloodMoon();
	}
}

void AEIGameState::OnRep_BloodMoon()
{
	BloodMoonNoteRemaining = bBloodMoon ? BloodMoonNoteSeconds : 0.0f;
	if (bBloodMoon)
	{
		EIVisuals::PlaySound2D(this, TEXT("Schlachtruf"));
	}
}

void AEIGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BloodMoonNoteRemaining = FMath::Max(0.0f, BloodMoonNoteRemaining - DeltaSeconds);
	const float WantedLook = bBloodMoon ? 1.0f : 0.0f;
	if (!FMath::IsNearlyEqual(BloodMoonLook, WantedLook))
	{
		BloodMoonLook = FMath::FInterpConstantTo(BloodMoonLook, WantedLook, DeltaSeconds, LookChangePerSecond);
		EIVisuals::ApplyBloodMoonLook(GetWorld(), BloodMoonLook);
	}
}
