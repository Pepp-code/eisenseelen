#pragma once

#include "CoreMinimal.h"
#include "EIPlayerController.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"

/** Presses or releases a key through the real controls, the way the game viewport would. */
inline void EISendTestKey(AEIPlayerController* Player, const FKey& Key, EInputEvent Event)
{
	Player->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key, Event, FPlatformTime::Cycles64()));
}
