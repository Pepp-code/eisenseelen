#include "EIHealthComponent.h"

#include "Net/UnrealNetwork.h"

UEIHealthComponent::UEIHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UEIHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEIHealthComponent, Health);
	DOREPLIFETIME(UEIHealthComponent, MaxHealth);
}

void UEIHealthComponent::Reset(float NewMaxHealth)
{
	MaxHealth = NewMaxHealth;
	Health = NewMaxHealth;
}

float UEIHealthComponent::ApplyDamage(float Amount)
{
	if (IsDead() || Amount <= 0.0f)
	{
		return 0.0f;
	}
	const float Applied = FMath::Min(Amount, Health);
	Health -= Applied;
	if (IsDead())
	{
		OnDied.Broadcast();
	}
	return Applied;
}
