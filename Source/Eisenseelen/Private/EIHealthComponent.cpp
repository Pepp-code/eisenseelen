#include "EIHealthComponent.h"

#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float NoLifeLeft = 0.001f;
}

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

void UEIHealthComponent::AddMaxHealth(float Amount)
{
	MaxHealth += Amount;
	if (!IsDead())
	{
		Health += Amount;
	}
}

float UEIHealthComponent::ApplyDamage(float Amount)
{
	if (IsDead() || Amount <= 0.0f)
	{
		return 0.0f;
	}
	const float Applied = FMath::Min(Amount, Health);
	Health -= Applied;
	// A reduced hit leaves rounding errors, and nobody should survive on a thousandth of a life point.
	if (Health < NoLifeLeft)
	{
		Health = 0.0f;
	}
	if (IsDead())
	{
		OnDied.Broadcast();
	}
	return Applied;
}
