#include "EIGameMode.h"

#include "EIHeroCharacter.h"
#include "EIPlayerController.h"

AEIGameMode::AEIGameMode()
{
	DefaultPawnClass = AEIHeroCharacter::StaticClass();
	PlayerControllerClass = AEIPlayerController::StaticClass();
}
