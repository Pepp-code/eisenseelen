#pragma once

#include "CoreMinimal.h"
#include "EIChest.h"
#include "EISoulStoneChest.generated.h"

/**
 * A chest that holds a soul stone for the hollow armor of a boss. It works like a contract chest — whoever steps into
 * its circle calls its guards, and only when they have fallen can it be opened — but what it holds goes into the armor.
 * The server spawns it, and the HUD finds it by this class.
 */
UCLASS()
class EISENSEELEN_API AEISoulStoneChest : public AEIChest
{
	GENERATED_BODY()
};
