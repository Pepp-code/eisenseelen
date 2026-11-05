#include "EIHeroBodyComponent.h"

#include "Components/StaticMeshComponent.h"
#include "EIEnemy.h"
#include "EIGearComponent.h"
#include "EIHeroCharacter.h"
#include "EIItems.h"
#include "EILookComponent.h"
#include "EIVisuals.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	// Centimetres from the soles of the feet, x forward. The suit is as tall as the capsule.
	const FVector TorsoAt(0.0f, 0.0f, 122.0f);
	const FVector TorsoSizeCm(34.0f, 46.0f, 58.0f);
	const FVector HelmetAt(0.0f, 0.0f, 168.0f);
	const FVector HelmetSizeCm(26.0f, 24.0f, 30.0f);
	const FVector VisorAt(13.5f, 0.0f, 171.0f);
	const FVector VisorSizeCm(2.0f, 18.0f, 4.0f);
	constexpr float PauldronHeightCm = 150.0f;
	const FVector PauldronSizeCm(22.0f, 20.0f, 12.0f);
	constexpr float HipHeightCm = 92.0f;
	constexpr float HipSideCm = 11.0f;
	const FVector LegSizeCm(16.0f, 16.0f, 88.0f);
	constexpr float ShoulderHeightCm = 146.0f;
	constexpr float ShoulderSideCm = 30.0f;
	const FVector ArmSizeCm(14.0f, 14.0f, 60.0f);

	constexpr float BladeStartCm = 5.0f;
	const FVector BladeSizeCm(100.0f, 3.0f, 7.0f);
	const FVector BladeEdgeSizeCm(96.0f, 3.6f, 1.6f);
	const FVector GuardSizeCm(3.0f, 4.0f, 16.0f);
	const FVector ShaftSizeCm(140.0f, 4.0f, 4.0f);
	constexpr float ShaftBackCm = 40.0f;
	constexpr float StaffHeadSizeCm = 14.0f;
	constexpr float WeaponGlow = 1.2f;
	constexpr float VisorGlow = 2.0f;

	const FLinearColor BladewardenColor(0.45f, 0.47f, 0.52f);
	const FLinearColor ArcanistColor(0.32f, 0.25f, 0.45f);
	const FLinearColor BladewardenVisor(0.55f, 0.85f, 1.0f);
	const FLinearColor ArcanistVisor(0.8f, 0.5f, 1.0f);
	const FLinearColor BladeColor(0.42f, 0.45f, 0.5f);
	const FLinearColor GuardColor(0.3f, 0.22f, 0.06f);
	const FLinearColor ShaftColor(0.12f, 0.07f, 0.03f);

	// How the blocks move: one stride is this long, and the limbs swing this far.
	constexpr float StrideCm = 150.0f;
	constexpr float FullWalkCm = 350.0f;
	constexpr float LegSwingDegrees = 28.0f;
	constexpr float ArmSwingShare = 0.7f;
	constexpr float WeaponArmRestDegrees = 25.0f;
	constexpr float StrikeArmDegrees = 100.0f;
	constexpr float BlockArmShare = 0.6f;
	constexpr float ActionFadePerSecond = 4.0f;
	constexpr float AirFrontLegDegrees = 35.0f;
	constexpr float AirBackLegDegrees = 10.0f;
	constexpr float FallDegrees = 85.0f;
	constexpr float BlendSpeed = 8.0f;
	constexpr float FallSpeed = 5.0f;
	constexpr float RunFromWalkShare = 1.15f;

}

FString EIHeroClassLookName(EEIHeroClass HeroClass)
{
	return HeroClass == EEIHeroClass::Arcanist ? TEXT("arkanist") : TEXT("klingenwacht");
}

TArray<FString> EIWeaponLookKeys(const UEIGearComponent* Gear, bool bStaff)
{
	TArray<FString> Keys;
	const FEIItem* Weapon = Gear != nullptr ? Gear->GetWorn(EEISlot::Weapon) : nullptr;
	const FEIWeaponForm* Form = Gear != nullptr ? Gear->GetWeaponForm() : nullptr;
	if (Weapon != nullptr)
	{
		Keys.Add(TEXT("item.") + Weapon->Name);
	}
	Keys.Add(TEXT("form.") + (Form != nullptr ? Form->Key : FString(bStaff ? TEXT("stab") : TEXT("langschwert"))));
	return Keys;
}

UEIHeroBodyComponent::UEIHeroBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

USceneComponent* UEIHeroBodyComponent::AddPivot(USceneComponent* Parent, const FVector& Location)
{
	USceneComponent* Pivot = NewObject<USceneComponent>(GetOwner());
	Pivot->SetupAttachment(Parent);
	Pivot->SetRelativeLocation(Location);
	Pivot->RegisterComponent();
	return Pivot;
}

UStaticMeshComponent* UEIHeroBodyComponent::AddPart(USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Location, const FVector& SizeCm)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(GetOwner());
	Part->SetStaticMesh(Mesh);
	Part->SetupAttachment(Parent);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeScale3D(SizeCm / 100.0f);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	Part->SetOwnerNoSee(true);
	Part->RegisterComponent();
	return Part;
}

void UEIHeroBodyComponent::Build()
{
	UStaticMesh* Cube = EIVisuals::GetCube();
	Frame = AddPivot(this, FVector::ZeroVector);

	ArmourParts.Add(AddPart(Frame, Cube, TorsoAt, TorsoSizeCm));
	ArmourParts.Add(AddPart(Frame, Cube, HelmetAt, HelmetSizeCm));
	Visor = AddPart(Frame, Cube, VisorAt, VisorSizeCm);
	Visor->SetCastShadow(false);
	for (const float Side : {-1.0f, 1.0f})
	{
		ArmourParts.Add(AddPart(Frame, Cube, FVector(0.0f, ShoulderSideCm * Side, PauldronHeightCm), PauldronSizeCm));
	}

	LeftHip = AddPivot(Frame, FVector(0.0f, -HipSideCm, HipHeightCm));
	RightHip = AddPivot(Frame, FVector(0.0f, HipSideCm, HipHeightCm));
	LeftShoulder = AddPivot(Frame, FVector(0.0f, -ShoulderSideCm, ShoulderHeightCm));
	RightShoulder = AddPivot(Frame, FVector(0.0f, ShoulderSideCm, ShoulderHeightCm));
	for (USceneComponent* Hip : {LeftHip.Get(), RightHip.Get()})
	{
		ArmourParts.Add(AddPart(Hip, Cube, FVector(0.0f, 0.0f, -LegSizeCm.Z / 2.0f), LegSizeCm));
	}
	for (USceneComponent* Shoulder : {LeftShoulder.Get(), RightShoulder.Get()})
	{
		ArmourParts.Add(AddPart(Shoulder, Cube, FVector(0.0f, 0.0f, -ArmSizeCm.Z / 2.0f), ArmSizeCm));
	}
	RightHand = AddPivot(RightShoulder, FVector(0.0f, 0.0f, -ArmSizeCm.Z));

	WeaponLook = NewObject<UEILookComponent>(GetOwner());
	WeaponLook->SetupAttachment(RightHand);
	WeaponLook->SetViewOptions(false, true, true);
	WeaponLook->RegisterComponent();
	// The grip sits in the hand and the blade points forward, as in Data/aussehen.json.
	SwordParts = AddPivot(WeaponLook, FVector::ZeroVector);
	Blade = AddPart(SwordParts, Cube, FVector(BladeStartCm + BladeSizeCm.X / 2.0f, 0.0f, 0.0f), BladeSizeCm);
	BladeEdge = AddPart(SwordParts, Cube, FVector(BladeStartCm + BladeSizeCm.X / 2.0f, 0.0f, 0.0f), BladeEdgeSizeCm);
	Guard = AddPart(SwordParts, Cube, FVector(BladeStartCm - 1.0f, 0.0f, 0.0f), GuardSizeCm);
	StaffParts = AddPivot(WeaponLook, FVector::ZeroVector);
	Shaft = AddPart(StaffParts, Cube, FVector(ShaftSizeCm.X / 2.0f - ShaftBackCm, 0.0f, 0.0f), ShaftSizeCm);
	StaffHead = AddPart(StaffParts, EIVisuals::GetSphere(), FVector(ShaftSizeCm.X - ShaftBackCm + StaffHeadSizeCm / 2.0f, 0.0f, 0.0f), FVector(StaffHeadSizeCm));
	EIVisuals::SetColor(Blade, BladeColor);
	EIVisuals::SetColor(Guard, GuardColor);
	EIVisuals::SetColor(Shaft, ShaftColor);

	Look = NewObject<UEILookComponent>(GetOwner());
	Look->SetupAttachment(this);
	Look->SetViewOptions(false, true, true);
	Look->RegisterComponent();
	SetComponentTickEnabled(true);
}

void UEIHeroBodyComponent::Refresh()
{
	const AEIHeroCharacter* Hero = Cast<AEIHeroCharacter>(GetOwner());
	if (Hero == nullptr || !IsRegistered())
	{
		return;
	}
	if (Frame == nullptr)
	{
		Build();
	}
	const EEIHeroClass HeroClass = Hero->GetHeroClass();
	const bool bStaff = HeroClass == EEIHeroClass::Arcanist;
	const UEIGearComponent* Gear = Hero->GetGear();

	for (UStaticMeshComponent* Part : ArmourParts)
	{
		EIVisuals::SetColor(Part, bStaff ? ArcanistColor : BladewardenColor);
	}
	EIVisuals::SetColor(Visor, bStaff ? ArcanistVisor : BladewardenVisor, VisorGlow);

	SwordParts->SetVisibility(!bStaff, true);
	StaffParts->SetVisibility(bStaff, true);
	const FEIWeaponForm* Form = Gear->GetWeaponForm();
	const float Length = Form != nullptr ? Form->LengthFactor : 1.0f;
	const FVector Middle(BladeStartCm + BladeSizeCm.X * Length / 2.0f, 0.0f, 0.0f);
	Blade->SetRelativeLocation(Middle);
	Blade->SetRelativeScale3D(BladeSizeCm * FVector(Length, 1.0f, 1.0f) / 100.0f);
	BladeEdge->SetRelativeLocation(Middle);
	BladeEdge->SetRelativeScale3D(BladeEdgeSizeCm * FVector(Length, 1.0f, 1.0f) / 100.0f);
	const FLinearColor Element = EIElementColor(Gear->GetWeaponElement());
	EIVisuals::SetColor(BladeEdge, Element, WeaponGlow);
	EIVisuals::SetColor(StaffHead, Element, WeaponGlow);

	// The weapon goes back into the block hand first, so a body model that changes does not take it along.
	WeaponLook->AttachToComponent(RightHand, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	TArray<USceneComponent*> Placeholders;
	for (UStaticMeshComponent* Part : ArmourParts)
	{
		Placeholders.Add(Part);
	}
	Look->Setup({TEXT("held.") + EIHeroClassLookName(HeroClass)}, Placeholders, {Visor});

	TArray<FString> Pieces;
	for (const EEISlot Slot : {EEISlot::Helmet, EEISlot::Chest, EEISlot::Gloves, EEISlot::Boots})
	{
		if (const FEIItem* Worn = Gear->GetWorn(Slot))
		{
			Pieces.Add(TEXT("item.") + Worn->Name);
		}
	}
	Look->SetPieces(Pieces);

	if (bStaff)
	{
		WeaponLook->Setup(EIWeaponLookKeys(Gear, true), {Shaft}, {StaffHead});
	}
	else
	{
		WeaponLook->Setup(EIWeaponLookKeys(Gear, false), {Blade, Guard}, {BladeEdge});
	}
	Look->AttachToModelSocket(WeaponLook, WeaponLook->GetSocket());
}

void UEIHeroBodyComponent::PlayEvent(FName Event)
{
	if (bDead)
	{
		return;
	}
	static const TSet<FName> ArmEvents = {TEXT("hieb"), TEXT("schwerer_hieb"), TEXT("schuss"), TEXT("zauber"), TEXT("rune")};
	if (ArmEvents.Contains(Event))
	{
		ActionBlend = 1.0f;
	}
	else if (Event == TEXT("blocken"))
	{
		ActionBlend = FMath::Max(ActionBlend, BlockArmShare);
	}
	if (Look != nullptr)
	{
		Look->PlayEvent(Event);
	}
}

void UEIHeroBodyComponent::ShowDeath()
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	if (Look != nullptr)
	{
		Look->PlayEvent(TEXT("tod"));
		bToppleModel = Look->HasModel() && !Look->HasAnimation(TEXT("tod"));
	}
}

void UEIHeroBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const ACharacter* Hero = Cast<ACharacter>(GetOwner());
	if (Frame == nullptr || Hero == nullptr)
	{
		return;
	}
	const UCharacterMovementComponent* Movement = Hero->GetCharacterMovement();
	const float Speed = Hero->GetVelocity().Size2D();
	const bool bInAir = Movement != nullptr && Movement->IsFalling();

	WalkBlend = FMath::FInterpTo(WalkBlend, bDead ? 0.0f : FMath::Clamp(Speed / FullWalkCm, 0.0f, 1.0f), DeltaTime, BlendSpeed);
	AirBlend = FMath::FInterpTo(AirBlend, bInAir && !bDead ? 1.0f : 0.0f, DeltaTime, BlendSpeed);
	FallBlend = bDead ? FMath::FInterpTo(FallBlend, 1.0f, DeltaTime, FallSpeed) : 0.0f;
	ActionBlend = FMath::Max(0.0f, ActionBlend - ActionFadePerSecond * DeltaTime);
	StridePhase = FMath::Fmod(StridePhase + Speed * DeltaTime / StrideCm * UE_TWO_PI, UE_TWO_PI);

	const float Swing = FMath::Sin(StridePhase) * LegSwingDegrees * WalkBlend;
	LeftHip->SetRelativeRotation(FRotator(Swing + AirBlend * AirFrontLegDegrees, 0.0f, 0.0f));
	RightHip->SetRelativeRotation(FRotator(-Swing + AirBlend * AirBackLegDegrees, 0.0f, 0.0f));
	LeftShoulder->SetRelativeRotation(FRotator(-Swing * ArmSwingShare, 0.0f, 0.0f));
	RightShoulder->SetRelativeRotation(FRotator(Swing * ArmSwingShare * (1.0f - ActionBlend) + WeaponArmRestDegrees + ActionBlend * StrikeArmDegrees, 0.0f, 0.0f));
	const FRotator Fall(FallBlend * FallDegrees, 0.0f, 0.0f);
	Frame->SetRelativeRotation(Fall);
	if (bToppleModel)
	{
		Look->SetRelativeRotation(Fall);
	}
	const float RunFrom = Movement != nullptr ? Movement->MaxWalkSpeed * RunFromWalkShare : FullWalkCm;
	Look->SetMotion(Speed, RunFrom, bInAir);
}
