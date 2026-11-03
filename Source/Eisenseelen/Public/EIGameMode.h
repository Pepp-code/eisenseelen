#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EIGameMode.generated.h"

class UEIHordeDirector;
class UEIRunDirector;

UCLASS()
class EISENSEELEN_API AEIGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEIGameMode();

	UEIHordeDirector* GetHorde() const { return Horde; }
	UEIRunDirector* GetRun() const { return Run; }

	/** For the test arena. During a run the run director decides about the horde. */
	void ToggleHorde();

	/** Removes all enemies and starts every hero fresh. All Data files are read again. */
	void RestartRun();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIHordeDirector> Horde;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UEIRunDirector> Run;
};
