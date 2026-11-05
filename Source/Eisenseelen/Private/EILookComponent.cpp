#include "EILookComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EIDataFile.h"
#include "EILooks.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* LookFileName = TEXT("aussehen.json");
	const TCHAR* OverlayPath = TEXT("/Game/Materials/M_Ueberzug.M_Ueberzug");
	const FName SlotName(TEXT("DefaultSlot"));
	const FName SpeedValue(TEXT("Tempo"));
	const FName InAirValue(TEXT("InDerLuft"));
	const FName Standing(TEXT("stehen"));
	const FName Walking(TEXT("laufen"));
	const FName Running(TEXT("rennen"));
	const FName Jumping(TEXT("springen"));
	const FName Death(TEXT("tod"));
	const FName Active(TEXT("aktiv"));
	constexpr float StandBelowCm = 10.0f;
	constexpr float BlendSeconds = 0.1f;
	constexpr float StatusThreshold = 0.01f;

	/** Each missing file is reported once, not every time an enemy appears. */
	void ReportOnce(const FString& Path, const FString& Error)
	{
		static TSet<FString> Reported;
		if (!Reported.Contains(Path))
		{
			Reported.Add(Path);
			FEIDataFile::ReportError(LookFileName, Error);
		}
	}
}

UEILookComponent::UEILookComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UEILookComponent::SetViewOptions(bool bInOnlyOwnerSee, bool bInOwnerNoSee, bool bInCastShadow)
{
	bOnlyOwnerSee = bInOnlyOwnerSee;
	bOwnerNoSee = bInOwnerNoSee;
	bCastShadow = bInCastShadow;
}

void UEILookComponent::Clear()
{
	for (USceneComponent* Part : HiddenParts)
	{
		if (IsValid(Part))
		{
			Part->SetHiddenInGame(false, false);
		}
	}
	HiddenParts.Reset();
	for (UMeshComponent* Piece : Pieces)
	{
		if (IsValid(Piece))
		{
			Piece->DestroyComponent();
		}
	}
	Pieces.Reset();
	if (IsValid(Model))
	{
		Model->DestroyComponent();
	}
	Model = nullptr;
	ModelKey.Reset();
	Socket = TEXT("waffe_r");
	Animations.Reset();
	AnimClass = nullptr;
	PlayingLoop = nullptr;
	OnceSecondsLeft = 0.0f;
	bHolding = false;
	bOverlayOn = false;
	SetComponentTickEnabled(false);
}

void UEILookComponent::Setup(const TArray<FString>& Keys, const TArray<USceneComponent*>& Placeholders, const TArray<USceneComponent*>& GlowParts)
{
	// The gear changes often without changing what is seen, and building the model again would restart its animation.
	TArray<TWeakObjectPtr<USceneComponent>> Parts;
	for (USceneComponent* Part : Placeholders)
	{
		Parts.Add(Part);
	}
	for (USceneComponent* Part : GlowParts)
	{
		Parts.Add(Part);
	}
	if (bSetUp && Keys == SetupKeys && Parts == SetupParts)
	{
		return;
	}
	Clear();
	bSetUp = true;
	SetupKeys = Keys;
	SetupParts = MoveTemp(Parts);
	FString Key;
	const FEILookEntry* Entry = FEILookTable::Get().FindFirstModel(Keys, &Key);
	if (Entry == nullptr || GetOwner() == nullptr)
	{
		return;
	}
	Model = CreateModel(*Entry, Key, this, NAME_None);
	if (Model == nullptr)
	{
		return;
	}
	ModelKey = Key;
	Socket = Entry->Socket;
	for (USceneComponent* Part : Placeholders)
	{
		if (Part != nullptr)
		{
			Part->SetHiddenInGame(true, false);
			HiddenParts.Add(Part);
		}
	}
	if (!Entry->bKeepGlowParts)
	{
		for (USceneComponent* Part : GlowParts)
		{
			if (Part != nullptr)
			{
				Part->SetHiddenInGame(true, false);
				HiddenParts.Add(Part);
			}
		}
	}

	if (USkeletalMeshComponent* Skeletal = GetSkeletalModel())
	{
		if (!Entry->AnimBlueprint.IsEmpty())
		{
			AnimClass = LoadClass<UAnimInstance>(nullptr, *Entry->AnimBlueprint);
			if (AnimClass == nullptr)
			{
				ReportOnce(Entry->AnimBlueprint, FString::Printf(TEXT("%s: Den Animations-Blueprint %s gibt es nicht. Die Figur spielt ihre einzelnen Animationen."), *Key, *Entry->AnimBlueprint));
			}
		}
		LoadAnimations(*Entry, Key);
		if (AnimClass != nullptr)
		{
			Skeletal->SetAnimInstanceClass(AnimClass);
		}
		SetComponentTickEnabled(true);
		if (bDead)
		{
			bDead = false;
			PlayEvent(Death);
		}
		else if (bWantActive)
		{
			ShowActive(true);
		}
		else
		{
			PlayLoop();
		}
	}
	ApplyPieces();
}

void UEILookComponent::SetPieces(const TArray<FString>& Keys)
{
	if (Keys == PieceKeys)
	{
		return;
	}
	PieceKeys = Keys;
	ApplyPieces();
}

void UEILookComponent::ApplyPieces()
{
	for (UMeshComponent* Piece : Pieces)
	{
		if (IsValid(Piece))
		{
			Piece->DestroyComponent();
		}
	}
	Pieces.Reset();
	const FEILookTable& Table = FEILookTable::Get();
	USkeletalMeshComponent* Leader = GetSkeletalModel();
	for (const FString& Key : PieceKeys)
	{
		const FEILookEntry* Entry = Table.Find(Key);
		if (Entry == nullptr || !Entry->HasModel())
		{
			continue;
		}
		const bool bOnSocket = Leader != nullptr && Leader->DoesSocketExist(Entry->Socket);
		UMeshComponent* Piece = CreateModel(*Entry, Key, bOnSocket ? static_cast<USceneComponent*>(Leader) : this, bOnSocket ? Entry->Socket : NAME_None);
		if (Piece == nullptr)
		{
			continue;
		}
		if (USkeletalMeshComponent* SkeletalPiece = Cast<USkeletalMeshComponent>(Piece))
		{
			if (Leader == nullptr)
			{
				// Armour with bones only fits on a body with the same bones.
				ReportOnce(Key + TEXT("#ohne_skelett"), FString::Printf(TEXT("%s hat ein Skelett, die Figur darunter aber nicht. Das Teil wird nicht gezeigt."), *Key));
				Piece->DestroyComponent();
				continue;
			}
			SkeletalPiece->AttachToComponent(Leader, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SkeletalPiece->SetLeaderPoseComponent(Leader);
		}
		if (bOverlayOn)
		{
			ApplyOverlay(Piece);
		}
		Pieces.Add(Piece);
	}
}

UMeshComponent* UEILookComponent::CreateModel(const FEILookEntry& Entry, const FString& Key, USceneComponent* Parent, FName ParentSocket)
{
	UObject* Asset = LoadObject<UObject>(nullptr, *Entry.Model);
	UMeshComponent* Mesh = nullptr;
	if (UStaticMesh* Static = Cast<UStaticMesh>(Asset))
	{
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(GetOwner());
		Component->SetStaticMesh(Static);
		Mesh = Component;
	}
	else if (USkeletalMesh* Skeletal = Cast<USkeletalMesh>(Asset))
	{
		USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(GetOwner());
		Component->SetSkeletalMeshAsset(Skeletal);
		Component->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		Mesh = Component;
	}
	else
	{
		ReportOnce(Entry.Model, Asset == nullptr
			? FString::Printf(TEXT("%s: Das Modell %s gibt es nicht. Der Platzhalter bleibt."), *Key, *Entry.Model)
			: FString::Printf(TEXT("%s: %s ist kein Modell, sondern %s. Der Platzhalter bleibt."), *Key, *Entry.Model, *Asset->GetClass()->GetName()));
		return nullptr;
	}
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	ApplyViewOptions(Mesh);
	Mesh->SetupAttachment(Parent, ParentSocket);
	Mesh->SetRelativeTransform(Entry.GetTransform());
	Mesh->RegisterComponent();
	return Mesh;
}

void UEILookComponent::ApplyViewOptions(UMeshComponent* Mesh) const
{
	Mesh->SetOnlyOwnerSee(bOnlyOwnerSee);
	Mesh->SetOwnerNoSee(bOwnerNoSee);
	Mesh->SetCastShadow(bCastShadow);
}

void UEILookComponent::LoadAnimations(const FEILookEntry& Entry, const FString& Key)
{
	for (const TPair<FName, FString>& Pair : Entry.Animations)
	{
		UAnimSequenceBase* Animation = LoadObject<UAnimSequenceBase>(nullptr, *Pair.Value);
		if (Animation == nullptr)
		{
			ReportOnce(Pair.Value, FString::Printf(TEXT("%s: Die Animation %s fuer %s gibt es nicht."), *Key, *Pair.Value, *Pair.Key.ToString()));
			continue;
		}
		Animations.Add(Pair.Key, Animation);
	}
}

USkeletalMeshComponent* UEILookComponent::GetSkeletalModel() const
{
	return Cast<USkeletalMeshComponent>(Model);
}

bool UEILookComponent::AttachToModelSocket(USceneComponent* Child, FName SocketName) const
{
	USkeletalMeshComponent* Skeletal = GetSkeletalModel();
	if (Child == nullptr || Skeletal == nullptr || !Skeletal->DoesSocketExist(SocketName))
	{
		return false;
	}
	Child->AttachToComponent(Skeletal, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
	return true;
}

UAnimSequenceBase* UEILookComponent::Resolve(FName Event) const
{
	for (FName Current = Event; !Current.IsNone(); Current = FEILookTable::GetFallback(Current))
	{
		if (const TObjectPtr<UAnimSequenceBase>* Found = Animations.Find(Current))
		{
			return *Found;
		}
	}
	return nullptr;
}

void UEILookComponent::SetMotion(float SpeedCm, float RunFromCm, bool bInAir)
{
	LastSpeed = SpeedCm;
	bLastInAir = bInAir;
	if (bInAir && Animations.Contains(Jumping))
	{
		LoopEvent = Jumping;
	}
	else
	{
		LoopEvent = SpeedCm < StandBelowCm ? Standing : (SpeedCm >= RunFromCm ? Running : Walking);
	}
	if (AnimClass != nullptr)
	{
		WriteAnimValues(SpeedCm, bInAir);
		return;
	}
	if (!bHolding && OnceSecondsLeft <= 0.0f)
	{
		PlayLoop();
	}
}

void UEILookComponent::WriteAnimValues(float Speed, bool bInAir)
{
	USkeletalMeshComponent* Skeletal = GetSkeletalModel();
	UAnimInstance* Instance = Skeletal != nullptr ? Skeletal->GetAnimInstance() : nullptr;
	if (Instance == nullptr)
	{
		return;
	}
	// Blueprint numbers are doubles, numbers written in C++ may be floats.
	if (FDoubleProperty* Double = FindFProperty<FDoubleProperty>(Instance->GetClass(), SpeedValue))
	{
		Double->SetPropertyValue_InContainer(Instance, Speed);
	}
	else if (FFloatProperty* Float = FindFProperty<FFloatProperty>(Instance->GetClass(), SpeedValue))
	{
		Float->SetPropertyValue_InContainer(Instance, Speed);
	}
	if (FBoolProperty* Bool = FindFProperty<FBoolProperty>(Instance->GetClass(), InAirValue))
	{
		Bool->SetPropertyValue_InContainer(Instance, bInAir);
	}
}

void UEILookComponent::PlayLoop()
{
	USkeletalMeshComponent* Skeletal = GetSkeletalModel();
	if (Skeletal == nullptr || AnimClass != nullptr)
	{
		return;
	}
	UAnimSequenceBase* Loop = Resolve(LoopEvent);
	if (Loop != nullptr && Loop != PlayingLoop)
	{
		Skeletal->PlayAnimation(Loop, true);
	}
	PlayingLoop = Loop;
}

void UEILookComponent::PlayOnce(UAnimSequenceBase* Animation, bool bHold)
{
	USkeletalMeshComponent* Skeletal = GetSkeletalModel();
	if (Skeletal == nullptr || Animation == nullptr)
	{
		return;
	}
	PlayingLoop = nullptr;
	if (bHold)
	{
		// One animation on its own keeps its last frame, with or without an animation blueprint.
		bHolding = true;
		OnceSecondsLeft = 0.0f;
		Skeletal->PlayAnimation(Animation, false);
		return;
	}
	if (AnimClass != nullptr)
	{
		if (UAnimInstance* Instance = Skeletal->GetAnimInstance())
		{
			if (UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
			{
				Instance->Montage_Play(Montage);
			}
			else
			{
				Instance->PlaySlotAnimationAsDynamicMontage(Animation, SlotName, BlendSeconds, BlendSeconds);
			}
		}
		return;
	}
	Skeletal->PlayAnimation(Animation, false);
	OnceSecondsLeft = FMath::Max(Animation->GetPlayLength(), BlendSeconds);
}

void UEILookComponent::PlayEvent(FName Event)
{
	if (Event == Death)
	{
		// Remembered, so a model that only arrives later still lies down.
		bDead = true;
		PlayOnce(Resolve(Death), true);
		return;
	}
	if (!bDead && !bHolding)
	{
		PlayOnce(Resolve(Event), false);
	}
}

void UEILookComponent::ShowActive(bool bOn)
{
	bWantActive = bOn;
	if (bDead || GetSkeletalModel() == nullptr)
	{
		return;
	}
	if (bOn)
	{
		PlayOnce(Resolve(Active), true);
		return;
	}
	if (bHolding)
	{
		bHolding = false;
		UseAnimBlueprintAgain();
		PlayingLoop = nullptr;
		PlayLoop();
	}
}

void UEILookComponent::UseAnimBlueprintAgain()
{
	USkeletalMeshComponent* Skeletal = GetSkeletalModel();
	if (Skeletal != nullptr && AnimClass != nullptr)
	{
		Skeletal->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Skeletal->SetAnimInstanceClass(AnimClass);
	}
}

void UEILookComponent::SetPaused(bool bPaused)
{
	if (USkeletalMeshComponent* Skeletal = GetSkeletalModel())
	{
		Skeletal->bPauseAnims = bPaused;
	}
}

void UEILookComponent::SetStatus(const FLinearColor& Color, float Strength)
{
	if (Model == nullptr)
	{
		return;
	}
	const bool bOn = Strength >= StatusThreshold;
	if (bOn)
	{
		if (Overlay == nullptr)
		{
			UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, OverlayPath);
			if (Base == nullptr)
			{
				ReportOnce(OverlayPath, TEXT("Das Material M_Ueberzug fehlt, die Modelle zeigen keine Warnfarben. tools/rebuild_content.ps1 baut es neu."));
				return;
			}
			Overlay = UMaterialInstanceDynamic::Create(Base, this);
		}
		Overlay->SetVectorParameterValue(TEXT("Farbe"), Color);
		Overlay->SetScalarParameterValue(TEXT("Staerke"), Strength);
	}
	if (bOn == bOverlayOn)
	{
		return;
	}
	bOverlayOn = bOn;
	ApplyOverlay(Model);
	for (UMeshComponent* Piece : Pieces)
	{
		ApplyOverlay(Piece);
	}
}

void UEILookComponent::ApplyOverlay(UMeshComponent* Mesh) const
{
	if (IsValid(Mesh))
	{
		Mesh->SetOverlayMaterial(bOverlayOn ? Overlay.Get() : nullptr);
	}
}

bool UEILookComponent::LeaveCorpse(float Seconds)
{
	USkeletalMeshComponent* Skeletal = GetSkeletalModel();
	UAnimSequenceBase* DeathAnimation = Resolve(Death);
	UWorld* World = GetWorld();
	if (Skeletal == nullptr || DeathAnimation == nullptr || World == nullptr)
	{
		return false;
	}
	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEILookCorpse* Corpse = World->SpawnActor<AEILookCorpse>(AEILookCorpse::StaticClass(), Skeletal->GetComponentTransform(), Parameters);
	if (Corpse == nullptr)
	{
		return false;
	}
	Corpse->Setup(Skeletal, DeathAnimation, Seconds);
	bDead = true;
	SetVisibility(false, true);
	return true;
}

void UEILookComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (OnceSecondsLeft <= 0.0f)
	{
		return;
	}
	OnceSecondsLeft -= DeltaTime;
	if (OnceSecondsLeft <= 0.0f && !bHolding)
	{
		PlayLoop();
	}
}

AEILookCorpse::AEILookCorpse()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	SetRootComponent(Mesh);
}

void AEILookCorpse::Setup(const USkeletalMeshComponent* From, UAnimSequenceBase* DeathAnimation, float Seconds)
{
	Mesh->SetSkeletalMeshAsset(From->GetSkeletalMeshAsset());
	Mesh->SetWorldScale3D(From->GetComponentScale());
	for (int32 Index = 0; Index < From->GetNumMaterials(); ++Index)
	{
		Mesh->SetMaterial(Index, From->GetMaterial(Index));
	}
	Mesh->SetCastShadow(From->CastShadow);
	Mesh->PlayAnimation(DeathAnimation, false);
	SetLifeSpan(FMath::Max(Seconds, 0.1f));
}
