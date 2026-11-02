#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EIGameMode.generated.h"

class UEIHordeDirector;

UCLASS()
class EISENSEELEN_API AEIGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEIGameMode();

	UEIHordeDirector* GetHorde() const { return Horde; }

	void ToggleHorde();

	/** Removes all enemies and starts every hero fresh. All Data files are read again. */
	void RestartRun();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIHordeDirector> Horde;
};
