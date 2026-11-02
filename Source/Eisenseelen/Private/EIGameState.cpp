#include "EIGameState.h"

#include "Net/UnrealNetwork.h"

void AEIGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEIGameState, bHordeActive);
}
