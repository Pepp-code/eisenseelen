#pragma once

#include "CoreMinimal.h"
#include "EIHeroValues.h"
#include "GameFramework/Character.h"
#include "EIHeroCharacter.generated.h"

class UCameraComponent;

/** The first-person hero. Stage 0 has no model yet: only a capsule and a camera. */
UCLASS()
class EISENSEELEN_API AEIHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEIHeroCharacter();

	void ApplyValues(const FEIHeroValues& NewValues);
	const FEIHeroValues& GetValues() const { return Values; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

	FEIHeroValues Values;
};
