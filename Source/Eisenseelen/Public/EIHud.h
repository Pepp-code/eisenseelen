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
	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& Color);
	void DrawLine(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.0f);

	/** Red marks at the screen edge for enemies winding up outside the view. */
	void DrawThreats(const class AEIHeroCharacter* Hero);
};
