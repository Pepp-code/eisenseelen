#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EIEnemyCamp.generated.h"

class AEIEnemy;
class UStaticMeshComponent;

/**
 * A fixed place on the map where enemies stand watch from the start of a run. Later in the run reinforcements come out
 * of these places. The camp itself marks the spot and knows who belongs to it, the run director fills it.
 */
UCLASS()
class EISENSEELEN_API AEIEnemyCamp : public AActor
{
	GENERATED_BODY()

public:
	AEIEnemyCamp();

	const FString& GetStartSize() const { return StartSize; }

	/** Server only. */
	void SetMembers(const TArray<AEIEnemy*>& Enemies);
	int32 CountMembers() const;

protected:
	virtual void BeginPlay() override;

private:
	/** Set by tools/make_runmap.py from Data/laufkarte.json. */
	UPROPERTY(EditAnywhere, Category = "Eisenseelen")
	FString StartSize = TEXT("klein");

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Ashes;

	TArray<TWeakObjectPtr<AEIEnemy>> Members;
};
