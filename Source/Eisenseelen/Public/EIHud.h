#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EIHud.generated.h"

/** The whole interface for now, drawn from code: crosshair, life, cooldowns and the keys. */
UCLASS()
class EISENSEELEN_API AEIHud : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawScreen();
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color);
	void DrawLine(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.0f);

	/** Red marks at the screen edge for enemies winding up outside the view. */
	void DrawThreats(const class AEIHeroCharacter* Hero);

	/** Where the run stands, what to do next and where that is. Nothing on a map without a run. */
	void DrawRun(const class AEIHeroCharacter* Hero, const class AEIGameState* State);
	void DrawWorldMark(const FVector& Location, const FString& Label, const FLinearColor& Color);

	/** The gear page, over everything else. */
	void DrawMenu(class AEIPlayerController* Player, const class AEIHeroCharacter& Hero);

	/** What a run has brought, once it is over. */
	void DrawSettlement(const class AEIHeroCharacter* Hero, const class AEIGameState* State, const FString& Clock);

	/** The marks of the open contracts in the world, and what to do at the one the hero stands at. */
	void DrawContracts(const class AEIHeroCharacter* Hero, const class AEIGameState* State, const struct FEIContractTable& Table, float PromptY);

	/** Where the team stands at the dock, and what to do there. */
	void DrawDock(const class AEIHeroCharacter* Hero, const class AEIGameState* State, float LineY, float PromptY);

	/** The picture goes black for the way across, and comes back at the other side. */
	void DrawCrossing(const class AEIGameState* State);

	/** The overview: a small map of the contracts and a list of what they ask for and pay. */
	void DrawContractBoard(const class AEIHeroCharacter* Hero, const class AEIGameState* State, const struct FEIContractTable& Table);

	float CrossingFade = 0.0f;
};
