#include "EIGameMode.h"

#include "EIGameState.h"
#include "EIHeroCharacter.h"
#include "EIHordeDirector.h"
#include "EIHud.h"
#include "EIPlayerController.h"
#include "Engine/World.h"

AEIGameMode::AEIGameMode()
{
	DefaultPawnClass = AEIHeroCharacter::StaticClass();
	PlayerControllerClass = AEIPlayerController::StaticClass();
	HUDClass = AEIHud::StaticClass();
	GameStateClass = AEIGameState::StaticClass();

	Horde = CreateDefaultSubobject<UEIHordeDirector>(TEXT("Horde"));
}

void AEIGameMode::BeginPlay()
{
	Super::BeginPlay();
	Horde->LoadValues();
}

void AEIGameMode::ToggleHorde()
{
	const bool bActive = !Horde->IsHordeActive();
	Horde->SetHordeActive(bActive);
	GetGameState<AEIGameState>()->SetHordeActive(bActive);
	if (!bActive)
	{
		Horde->RemoveAllEnemies();
	}
}

void AEIGameMode::RestartRun()
{
	Horde->SetHordeActive(false);
	GetGameState<AEIGameState>()->SetHordeActive(false);
	Horde->RemoveAllEnemies();
	Horde->LoadValues();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Player = It->Get();
		if (APawn* Pawn = Player->GetPawn())
		{
			Player->UnPossess();
			Pawn->Destroy();
		}
		RestartPlayer(Player);
	}
}
