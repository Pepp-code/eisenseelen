#include "EIOmenComponent.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EIDataFile.h"
#include "EIHealthComponent.h"
#include "EIHeroCharacter.h"
#include "EIVisuals.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float FoundNoteSeconds = 4.0f;

	bool ReadTable(const FJsonObject& Root, FEIOmenTable& OutTable, FString& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
		if (!Root.TryGetArrayField(TEXT("omen"), Lines))
		{
			OutError = TEXT("Die Liste omen fehlt.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Line : *Lines)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FString EffectText;
			FString ClassText;
			FEIOmen Omen;
			if (!Line->TryGetObject(Object) || !(*Object)->TryGetStringField(TEXT("name"), Omen.Name))
			{
				OutError = TEXT("Einem Omen fehlt der Name.");
				return false;
			}
			if (!(*Object)->TryGetStringField(TEXT("wirkung"), EffectText) || !EIStatFromKey(EffectText, Omen.Effect))
			{
				OutError = FString::Printf(TEXT("%s hat keine bekannte Wirkung."), *Omen.Name);
				return false;
			}
			if (!FEIDataFile::ReadNumber(**Object, TEXT("wert"), 0.0, 10000.0, Omen.Amount, OutError))
			{
				return false;
			}
			if (!(*Object)->TryGetStringField(TEXT("klasse"), ClassText) || (ClassText != TEXT("alle") && ClassText != TEXT("klingenwacht") && ClassText != TEXT("arkanist")))
			{
				OutError = FString::Printf(TEXT("%s braucht als klasse alle, klingenwacht oder arkanist."), *Omen.Name);
				return false;
			}
			Omen.bForBladewarden = ClassText != TEXT("arkanist");
			Omen.bForArcanist = ClassText != TEXT("klingenwacht");
			OutTable.Omens.Add(Omen);
		}
		return true;
	}
}

FString FEIOmen::Describe() const
{
	return EIDescribeStat(Effect, Amount);
}

bool FEIOmenTable::LoadFromString(const FString& Json, FEIOmenTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEIOmenTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}

bool FEIOmenTable::LoadFromProjectFile(FEIOmenTable& OutTable, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("omen.json"), OutError);
	FEIOmenTable Loaded;
	if (!Root.IsValid() || !ReadTable(*Root, Loaded, OutError))
	{
		return false;
	}
	OutTable = Loaded;
	return true;
}

UEIOmenComponent::UEIOmenComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UEIOmenComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UEIOmenComponent, Held, COND_OwnerOnly);
}

void UEIOmenComponent::LoadValues()
{
	FString Error;
	if (!FEIOmenTable::LoadFromProjectFile(Table, Error))
	{
		FEIDataFile::ReportError(TEXT("omen.json"), Error);
	}
}

AEIHeroCharacter* UEIOmenComponent::GetHero() const
{
	return CastChecked<AEIHeroCharacter>(GetOwner());
}

int32 UEIOmenComponent::CountOf(int32 Index) const
{
	int32 Count = 0;
	for (const int32 HeldIndex : Held)
	{
		Count += HeldIndex == Index ? 1 : 0;
	}
	return Count;
}

float UEIOmenComponent::GetBonus(EEIStat Effect) const
{
	float Bonus = 0.0f;
	for (const int32 Index : Held)
	{
		if (Table.Omens.IsValidIndex(Index) && Table.Omens[Index].Effect == Effect)
		{
			Bonus += Table.Omens[Index].Amount;
		}
	}
	return Bonus;
}

void UEIOmenComponent::GrantRandomOmen()
{
	const bool bArcanist = GetHero()->GetHeroClass() == EEIHeroClass::Arcanist;
	TArray<int32> Fitting;
	TArray<int32> FittingAndNew;
	for (int32 Index = 0; Index < Table.Omens.Num(); ++Index)
	{
		if (bArcanist ? Table.Omens[Index].bForArcanist : Table.Omens[Index].bForBladewarden)
		{
			Fitting.Add(Index);
			if (CountOf(Index) == 0)
			{
				FittingAndNew.Add(Index);
			}
		}
	}
	const TArray<int32>& Choice = FittingAndNew.IsEmpty() ? Fitting : FittingAndNew;
	if (!Choice.IsEmpty())
	{
		AddOmen(Choice[FMath::RandRange(0, Choice.Num() - 1)]);
	}
}

void UEIOmenComponent::AddOmen(int32 Index)
{
	if (!Table.Omens.IsValidIndex(Index))
	{
		return;
	}
	Held.Add(Index);
	if (Table.Omens[Index].Effect == EEIStat::Life)
	{
		GetHero()->GetHealth()->AddMaxHealth(Table.Omens[Index].Amount);
	}
	OnRep_Held();
}

void UEIOmenComponent::OnRep_Held()
{
	AEIHeroCharacter* Hero = GetHero();
	Hero->OnOmensChanged();
	if (Held.Num() > KnownCount && Hero->IsLocallyControlled())
	{
		FoundNoteRemaining = FoundNoteSeconds;
		EIVisuals::PlaySound2D(this, TEXT("Sprungfeld"));
	}
	KnownCount = Held.Num();
}

void UEIOmenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FoundNoteRemaining = FMath::Max(0.0f, FoundNoteRemaining - DeltaTime);
}
