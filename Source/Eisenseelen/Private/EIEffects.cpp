#include "EIEffects.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EIDataFile.h"
#include "EILooks.h"
#include "EIVisuals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

namespace
{
	const TCHAR* LookFileName = TEXT("aussehen.json");
	const FName Opacity(TEXT("Deckkraft"));
	constexpr float MeshSizeCm = 100.0f;
	constexpr float SafetySeconds = 30.0f;
	constexpr float LightFadeSeconds = 0.3f;
	constexpr float BoltRejitterSeconds = 0.05f;
	constexpr float SkyBoltHeightCm = 700.0f;
	constexpr float MinBoltLengthCm = 10.0f;
	constexpr float RingThickness = 0.02f;

	enum class EShape : uint8
	{
		/** Around the middle, within Spread. */
		Point,
		/** Anywhere in a ball of the size Spread. */
		Sphere,
		/** On a flat circle of the size Spread. */
		Ring,
		/** Anywhere on a flat circle of the size Spread. */
		Disc,
		/** Along a wall that reaches the radius to each side and stands Spread thick. */
		Wall,
		/** Around the middle and flying out within the angle of the effect. */
		Cone,
		/** Around the target, within Spread. */
		Target,
	};

	enum class ELength : uint8
	{
		/** Makes its parts for Seconds of the recipe and goes once they faded. */
		Once,
		/** Makes its parts for the seconds the caller gives. */
		Seconds,
		/** Makes its parts until Stop. */
		UntilStop,
	};

	/** One kind of placeholder particle. Sizes, places and speeds are for a radius of 100 cm. */
	struct FEmitter
	{
		bool bCube = false;
		FLinearColor Tint = FLinearColor::White;
		/** 0 keeps the color the caller gives, 1 takes the tint. */
		float TintShare = 0.0f;
		float Glow = 2.0f;
		int32 Burst = 0;
		float Rate = 0.0f;
		/** The rate grows with the radius to this power, so a long wall burns as thick as a short one. */
		float RateExponent = 0.0f;
		float LifeMin = 0.4f;
		float LifeMax = 0.7f;
		EShape Shape = EShape::Point;
		float Spread = 0.0f;
		float ZFrom = 0.0f;
		float ZTo = 0.0f;
		float SizeStart = 10.0f;
		float SizePeak = 10.0f;
		float SizeEnd = 0.0f;
		float PeakAt = 0.2f;
		/** Longer along its flight, such as a spark or a streak of wind. */
		float Stretch = 1.0f;
		float SpeedOut = 0.0f;
		float SpeedUp = 0.0f;
		float SpeedForward = 0.0f;
		float SpeedRandom = 0.0f;
		float Gravity = 0.0f;
		float Drag = 0.0f;
		/** Degrees per second around the upright line through the middle. */
		float Swirl = 0.0f;
		/** Draws it sideways towards the upright line through the middle. */
		float Pull = 0.0f;
		/** Places and speeds grow with the radius. */
		bool bScale = true;
		/** Sizes and heights grow with the radius. */
		bool bScaleSize = true;
		/** Moves along with the effect instead of staying where it was made. */
		bool bFollow = false;
		bool bRandomTurn = false;
		/** Stays at its biggest until the effect stops, such as the ice of a frozen enemy. */
		bool bHoldUntilStop = false;
		/** LifeMin and LifeMax are shares of the seconds the caller gives, such as a cloud of steam. */
		bool bLifeFromSeconds = false;
		/** All of them fade out together over this many seconds at the end of their life. */
		float FadeLast = 0.0f;
		int32 Max = 24;
	};

	struct FRecipe
	{
		const TCHAR* Name = TEXT("");
		ELength Length = ELength::Once;
		/** How long Once makes parts, and the length of Seconds when the caller gives none. */
		float Seconds = 0.0f;
		/** A bright ball of this share of the radius. */
		float FlashShare = 0.0f;
		float FlashSeconds = 0.15f;
		float FlashGlow = 5.0f;
		/** A flat ring that runs out to this share of the radius. */
		float RingShare = 0.0f;
		float RingSeconds = 0.3f;
		/** Light per cm of radius. Lasting effects keep it, flickering, until they stop. */
		float LightPerCm = 0.0f;
		float LightSeconds = 0.3f;
		bool bLightLasts = false;
		/** Zigzag bolts from the place to the target. */
		int32 BoltCount = 0;
		float BoltSeconds = 0.2f;
		/** How far a bend may stray, as a share of the length. */
		float BoltJitter = 0.12f;
		int32 BoltSegments = 8;
		float BoltThicknessCm = 6.0f;
		float BoltGlow = 4.0f;
		/** A bolt from the sky somewhere in the radius every this many seconds. */
		float BoltEvery = 0.0f;
		TArray<FEmitter> Emitters;
	};

	TMap<FName, int32>& GetCounts()
	{
		static TMap<FName, int32> Counts;
		return Counts;
	}

	TSet<FString>& GetReported()
	{
		static TSet<FString> Reported;
		return Reported;
	}

	/** Each broken effect is reported once, not every time a spell is cast. */
	void ReportOnce(const FString& Path, const FString& Error)
	{
		if (!GetReported().Contains(Path))
		{
			GetReported().Add(Path);
			FEIDataFile::ReportError(LookFileName, Error);
		}
	}

	FVector RandomInBall(float Radius)
	{
		return FMath::VRand() * Radius * FMath::Pow(FMath::FRand(), 1.0f / 3.0f);
	}

	FVector RandomOnCircle(float Radius)
	{
		const float Angle = FMath::FRand() * 2.0f * PI;
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Radius;
	}

	FVector RandomInCircle(float Radius)
	{
		return RandomOnCircle(Radius * FMath::Sqrt(FMath::FRand()));
	}

	const TArray<FRecipe>& GetRecipes();

	int32 FindRecipe(FName Name)
	{
		const TArray<FRecipe>& Recipes = GetRecipes();
		for (int32 Index = 0; Index < Recipes.Num(); ++Index)
		{
			if (Name == FName(Recipes[Index].Name))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	const FLinearColor Fire(1.0f, 0.35f, 0.05f);
	const FLinearColor Ember(1.0f, 0.7f, 0.2f);
	const FLinearColor Ice(0.5f, 0.85f, 1.0f);
	const FLinearColor Frost(0.85f, 0.95f, 1.0f);
	const FLinearColor Spark(0.65f, 0.75f, 1.0f);
	const FLinearColor Dust(0.7f, 0.62f, 0.5f);
	const FLinearColor Steam(0.85f, 0.88f, 0.92f);
	const FLinearColor White(1.0f, 1.0f, 1.0f);

	// The fields of FEmitter and FRecipe must stay in the order they are declared in, since the recipes name them.
	void AddSpells(TArray<FRecipe>& Out)
	{
		Out.Add({ .Name = TEXT("feuerball"), .Length = ELength::UntilStop, .Emitters = {
			{ .Rate = 60.0f, .LifeMin = 0.25f, .LifeMax = 0.45f, .Shape = EShape::Sphere, .Spread = 15.0f,
				.SizeStart = 30.0f, .SizePeak = 36.0f, .SizeEnd = 0.0f, .PeakAt = 0.15f, .SpeedUp = 80.0f, .SpeedRandom = 60.0f,
				.bScale = false, .bScaleSize = false, .Max = 30 },
			{ .Tint = Ember, .TintShare = 1.0f, .Glow = 4.0f, .Rate = 25.0f, .LifeMin = 0.3f, .LifeMax = 0.5f,
				.SizeStart = 5.0f, .SizePeak = 6.0f, .SizeEnd = 0.0f, .Stretch = 4.0f, .SpeedRandom = 250.0f, .Gravity = 400.0f,
				.bScale = false, .bScaleSize = false, .Max = 15 } } });

		Out.Add({ .Name = TEXT("feuerball_explosion"), .Seconds = 0.25f, .FlashShare = 0.6f, .FlashSeconds = 0.18f,
			.RingShare = 1.0f, .RingSeconds = 0.35f, .LightPerCm = 200.0f, .LightSeconds = 0.35f, .Emitters = {
			{ .Glow = 3.0f, .Burst = 14, .LifeMin = 0.35f, .LifeMax = 0.6f, .Shape = EShape::Sphere, .Spread = 30.0f,
				.SizeStart = 30.0f, .SizePeak = 70.0f, .SizeEnd = 0.0f, .PeakAt = 0.25f, .SpeedOut = 350.0f, .SpeedUp = 60.0f,
				.Drag = 4.0f, .bRandomTurn = true, .Max = 14 },
			{ .Tint = Ember, .TintShare = 1.0f, .Glow = 5.0f, .Burst = 24, .LifeMin = 0.5f, .LifeMax = 0.9f, .Spread = 10.0f,
				.SizeStart = 6.0f, .SizePeak = 6.0f, .SizeEnd = 0.0f, .Stretch = 5.0f, .SpeedOut = 900.0f, .SpeedUp = 300.0f,
				.Gravity = 900.0f, .Drag = 1.0f, .Max = 24 },
			{ .Tint = Ember, .TintShare = 0.6f, .Glow = 3.0f, .Rate = 30.0f, .LifeMin = 0.8f, .LifeMax = 1.3f,
				.Shape = EShape::Sphere, .Spread = 50.0f, .SizeStart = 8.0f, .SizePeak = 10.0f, .SizeEnd = 0.0f,
				.SpeedUp = 150.0f, .SpeedRandom = 60.0f, .Max = 12 } } });

		Out.Add({ .Name = TEXT("eislanze"), .Length = ELength::UntilStop, .Emitters = {
			{ .Tint = Frost, .TintShare = 0.5f, .Glow = 1.2f, .Rate = 40.0f, .LifeMin = 0.4f, .LifeMax = 0.7f,
				.Shape = EShape::Sphere, .Spread = 12.0f, .SizeStart = 20.0f, .SizePeak = 30.0f, .SizeEnd = 0.0f, .PeakAt = 0.3f,
				.SpeedRandom = 30.0f, .bScale = false, .bScaleSize = false, .Max = 28 },
			{ .bCube = true, .Glow = 3.0f, .Rate = 20.0f, .LifeMin = 0.3f, .LifeMax = 0.5f, .SizeStart = 6.0f, .SizePeak = 6.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.1f, .SpeedRandom = 150.0f, .Gravity = 300.0f, .bScale = false, .bScaleSize = false,
				.bRandomTurn = true, .Max = 12 } } });

		Out.Add({ .Name = TEXT("eislanze_treffer"), .FlashShare = 0.4f, .FlashSeconds = 0.12f, .RingShare = 0.8f,
			.RingSeconds = 0.3f, .LightPerCm = 120.0f, .LightSeconds = 0.25f, .Emitters = {
			{ .bCube = true, .Glow = 3.0f, .Burst = 16, .LifeMin = 0.4f, .LifeMax = 0.7f, .SizeStart = 10.0f, .SizePeak = 12.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.1f, .SpeedOut = 600.0f, .SpeedUp = 200.0f, .Gravity = 900.0f, .bRandomTurn = true,
				.Max = 16 },
			{ .Tint = Frost, .TintShare = 0.5f, .Glow = 1.0f, .Burst = 6, .LifeMin = 0.5f, .LifeMax = 0.8f,
				.Shape = EShape::Sphere, .Spread = 30.0f, .SizeStart = 30.0f, .SizePeak = 60.0f, .SizeEnd = 0.0f, .PeakAt = 0.3f,
				.SpeedOut = 80.0f, .Drag = 2.0f, .Max = 6 } } });

		Out.Add({ .Name = TEXT("bolzen"), .Length = ELength::UntilStop, .Emitters = {
			{ .Glow = 3.0f, .Rate = 40.0f, .LifeMin = 0.15f, .LifeMax = 0.25f, .SizeStart = 12.0f, .SizePeak = 12.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.05f, .bScale = false, .bScaleSize = false, .Max = 12 } } });

		Out.Add({ .Name = TEXT("bolzen_treffer"), .FlashShare = 0.5f, .FlashSeconds = 0.1f, .LightPerCm = 100.0f,
			.LightSeconds = 0.15f, .Emitters = {
			{ .Glow = 4.0f, .Burst = 10, .LifeMin = 0.2f, .LifeMax = 0.4f, .SizeStart = 5.0f, .SizePeak = 5.0f, .SizeEnd = 0.0f,
				.Stretch = 4.0f, .SpeedOut = 500.0f, .Gravity = 600.0f, .Max = 10 } } });

		Out.Add({ .Name = TEXT("kettenblitz"), .LightPerCm = 60.0f, .LightSeconds = 0.2f, .BoltCount = 2, .BoltSeconds = 0.25f,
			.Emitters = {
			{ .Glow = 4.0f, .Burst = 10, .LifeMin = 0.2f, .LifeMax = 0.4f, .Shape = EShape::Target, .SizeStart = 5.0f,
				.SizePeak = 5.0f, .SizeEnd = 0.0f, .Stretch = 4.0f, .SpeedOut = 500.0f, .Gravity = 600.0f, .bScale = false,
				.bScaleSize = false, .Max = 10 } } });

		// The radius is the reach of the gust, so the streaks fly that far while their size stays.
		Out.Add({ .Name = TEXT("windstoss"), .Seconds = 0.15f, .Emitters = {
			{ .Tint = White, .TintShare = 0.5f, .Glow = 1.5f, .Burst = 18, .Rate = 60.0f, .LifeMin = 0.35f, .LifeMax = 0.5f,
				.Shape = EShape::Cone, .Spread = 5.0f, .ZFrom = -40.0f, .ZTo = 60.0f, .SizeStart = 8.0f, .SizePeak = 10.0f,
				.SizeEnd = 0.0f, .Stretch = 12.0f, .SpeedOut = 250.0f, .Drag = 1.0f, .bScaleSize = false, .Max = 30 },
			{ .Tint = Dust, .TintShare = 1.0f, .Glow = 0.6f, .Burst = 8, .LifeMin = 0.5f, .LifeMax = 0.8f, .Shape = EShape::Cone,
				.Spread = 5.0f, .ZFrom = -80.0f, .ZTo = -60.0f, .SizeStart = 30.0f, .SizePeak = 60.0f, .SizeEnd = 0.0f,
				.PeakAt = 0.3f, .SpeedOut = 150.0f, .Drag = 2.0f, .bScaleSize = false, .Max = 10 } } });
	}

	FEmitter WhirlStreaks()
	{
		return { .Tint = White, .TintShare = 0.6f, .Glow = 1.2f, .Rate = 40.0f, .LifeMin = 0.6f, .LifeMax = 1.0f,
			.Shape = EShape::Ring, .Spread = 30.0f, .ZFrom = 0.0f, .ZTo = 250.0f, .SizeStart = 8.0f, .SizePeak = 10.0f,
			.SizeEnd = 0.0f, .PeakAt = 0.3f, .Stretch = 6.0f, .SpeedOut = 30.0f, .SpeedUp = 150.0f, .Swirl = 360.0f,
			.bScaleSize = false, .bFollow = true, .Max = 40 };
	}

	FEmitter WhirlDust()
	{
		return { .Tint = Dust, .TintShare = 1.0f, .Glow = 0.6f, .Rate = 20.0f, .LifeMin = 0.7f, .LifeMax = 1.1f,
			.Shape = EShape::Ring, .Spread = 80.0f, .ZFrom = 0.0f, .ZTo = 20.0f, .SizeStart = 40.0f, .SizePeak = 70.0f,
			.SizeEnd = 0.0f, .PeakAt = 0.3f, .SpeedOut = 30.0f, .Swirl = 180.0f, .bFollow = true, .Max = 20 };
	}

	void AddRunes(TArray<FRecipe>& Out)
	{
		Out.Add({ .Name = TEXT("frostnova"), .Seconds = 0.2f, .FlashShare = 0.25f, .RingShare = 1.0f, .RingSeconds = 0.4f,
			.LightPerCm = 80.0f, .LightSeconds = 0.4f, .Emitters = {
			{ .bCube = true, .Glow = 3.0f, .Burst = 24, .LifeMin = 0.5f, .LifeMax = 0.8f, .Spread = 10.0f, .ZFrom = 20.0f,
				.ZTo = 60.0f, .SizeStart = 12.0f, .SizePeak = 14.0f, .SizeEnd = 0.0f, .PeakAt = 0.1f, .SpeedOut = 700.0f,
				.Gravity = 200.0f, .Drag = 1.5f, .bRandomTurn = true, .Max = 24 },
			{ .Tint = Frost, .TintShare = 0.5f, .Glow = 1.0f, .Burst = 16, .LifeMin = 0.8f, .LifeMax = 1.2f,
				.Shape = EShape::Ring, .Spread = 90.0f, .ZFrom = 10.0f, .ZTo = 30.0f, .SizeStart = 30.0f, .SizePeak = 60.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.3f, .SpeedOut = 40.0f, .Max = 16 },
			{ .bCube = true, .Glow = 2.5f, .Burst = 14, .LifeMin = 1.2f, .LifeMax = 1.8f, .Shape = EShape::Disc, .Spread = 85.0f,
				.ZFrom = 0.0f, .ZTo = 10.0f, .SizeStart = 10.0f, .SizePeak = 18.0f, .SizeEnd = 0.0f, .PeakAt = 0.15f,
				.bRandomTurn = true, .Max = 14 } } });

		Out.Add({ .Name = TEXT("gewitterfeld"), .Length = ELength::Seconds, .Seconds = 4.0f, .LightPerCm = 30.0f,
			.bLightLasts = true, .BoltSeconds = 0.2f, .BoltEvery = 0.45f, .Emitters = {
			{ .Glow = 4.0f, .Rate = 25.0f, .LifeMin = 0.1f, .LifeMax = 0.2f, .Shape = EShape::Disc, .Spread = 90.0f,
				.ZFrom = 5.0f, .ZTo = 40.0f, .SizeStart = 5.0f, .SizePeak = 5.0f, .SizeEnd = 0.0f, .Stretch = 5.0f,
				.SpeedRandom = 300.0f, .Max = 20 },
			{ .Glow = 2.0f, .Rate = 12.0f, .LifeMin = 0.8f, .LifeMax = 1.4f, .Shape = EShape::Disc, .Spread = 85.0f,
				.ZFrom = 10.0f, .ZTo = 120.0f, .SizeStart = 10.0f, .SizePeak = 12.0f, .SizeEnd = 0.0f, .PeakAt = 0.4f,
				.SpeedUp = 40.0f, .SpeedRandom = 20.0f, .Swirl = 30.0f, .Max = 20 },
			{ .Glow = 2.0f, .Rate = 20.0f, .LifeMin = 0.4f, .LifeMax = 0.6f, .Shape = EShape::Ring, .Spread = 100.0f,
				.ZFrom = 5.0f, .ZTo = 15.0f, .SizeStart = 12.0f, .SizePeak = 14.0f, .SizeEnd = 0.0f, .Max = 20 } } });

		// The radius is half the length of the wall, so it burns as thick on any length.
		Out.Add({ .Name = TEXT("flammenwand"), .Length = ELength::Seconds, .Seconds = 4.0f, .LightPerCm = 20.0f,
			.bLightLasts = true, .Emitters = {
			{ .Glow = 2.5f, .Rate = 30.0f, .RateExponent = 1.0f, .LifeMin = 0.5f, .LifeMax = 0.8f, .Shape = EShape::Wall,
				.Spread = 30.0f, .ZFrom = 0.0f, .ZTo = 30.0f, .SizeStart = 50.0f, .SizePeak = 70.0f, .SizeEnd = 0.0f,
				.PeakAt = 0.3f, .SpeedUp = 220.0f, .SpeedRandom = 30.0f, .bScale = false, .bScaleSize = false, .Max = 80 },
			{ .Tint = Ember, .TintShare = 0.7f, .Glow = 3.0f, .Rate = 15.0f, .RateExponent = 1.0f, .LifeMin = 0.8f,
				.LifeMax = 1.3f, .Shape = EShape::Wall, .Spread = 30.0f, .ZFrom = 50.0f, .ZTo = 150.0f, .SizeStart = 8.0f,
				.SizePeak = 8.0f, .SizeEnd = 0.0f, .SpeedUp = 200.0f, .SpeedRandom = 80.0f, .bScale = false,
				.bScaleSize = false, .Max = 40 } } });

		Out.Add({ .Name = TEXT("windhose"), .Length = ELength::Seconds, .Seconds = 4.0f,
			.Emitters = { WhirlStreaks(), WhirlDust() } });
	}

	void AddReactions(TArray<FRecipe>& Out)
	{
		Out.Add({ .Name = TEXT("feuerwirbel"), .Length = ELength::Seconds, .Seconds = 4.0f, .LightPerCm = 30.0f,
			.bLightLasts = true, .Emitters = { WhirlStreaks(), WhirlDust(),
			{ .Tint = Fire, .TintShare = 1.0f, .Glow = 2.5f, .Rate = 35.0f, .LifeMin = 0.5f, .LifeMax = 0.9f,
				.Shape = EShape::Ring, .Spread = 35.0f, .ZFrom = 0.0f, .ZTo = 150.0f, .SizeStart = 40.0f, .SizePeak = 60.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.3f, .SpeedOut = 20.0f, .SpeedUp = 200.0f, .Swirl = 300.0f, .bScaleSize = false, .bFollow = true,
				.Max = 40 } } });

		Out.Add({ .Name = TEXT("frostwirbel"), .Length = ELength::Seconds, .Seconds = 4.0f, .Emitters = { WhirlStreaks(), WhirlDust(),
			{ .bCube = true, .Tint = Ice, .TintShare = 1.0f, .Glow = 3.0f, .Rate = 30.0f, .LifeMin = 0.6f, .LifeMax = 1.0f,
				.Shape = EShape::Ring, .Spread = 45.0f, .ZFrom = 0.0f, .ZTo = 200.0f, .SizeStart = 10.0f, .SizePeak = 12.0f,
				.SizeEnd = 0.0f, .SpeedUp = 100.0f, .Swirl = 400.0f, .bScaleSize = false, .bFollow = true, .bRandomTurn = true,
				.Max = 30 },
			{ .Tint = Frost, .TintShare = 1.0f, .Glow = 0.8f, .Rate = 10.0f, .LifeMin = 1.0f, .LifeMax = 1.5f,
				.Shape = EShape::Disc, .Spread = 90.0f, .ZFrom = 0.0f, .ZTo = 15.0f, .SizeStart = 40.0f, .SizePeak = 70.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.4f, .Swirl = 60.0f, .bFollow = true, .Max = 15 } } });

		Out.Add({ .Name = TEXT("gewitterwirbel"), .Length = ELength::Seconds, .Seconds = 4.0f, .LightPerCm = 30.0f,
			.bLightLasts = true, .Emitters = { WhirlStreaks(), WhirlDust(),
			{ .Tint = Spark, .TintShare = 1.0f, .Glow = 4.0f, .Rate = 30.0f, .LifeMin = 0.1f, .LifeMax = 0.25f,
				.Shape = EShape::Ring, .Spread = 40.0f, .ZFrom = 0.0f, .ZTo = 250.0f, .SizeStart = 5.0f, .SizePeak = 5.0f,
				.SizeEnd = 0.0f, .Stretch = 5.0f, .SpeedRandom = 200.0f, .Swirl = 300.0f, .bScaleSize = false, .bFollow = true,
				.Max = 20 } } });

		// The puffs live as long as the cloud, so the steam hangs there and then fades as a whole.
		Out.Add({ .Name = TEXT("dampfexplosion"), .Length = ELength::Seconds, .Seconds = 4.0f, .FlashShare = 0.4f,
			.FlashSeconds = 0.2f, .RingShare = 1.0f, .RingSeconds = 0.4f, .LightPerCm = 80.0f, .LightSeconds = 0.3f, .Emitters = {
			{ .Tint = Steam, .TintShare = 1.0f, .Glow = 0.5f, .Burst = 18, .LifeMin = 0.85f, .LifeMax = 1.0f,
				.Shape = EShape::Sphere, .Spread = 70.0f, .ZFrom = 20.0f, .ZTo = 120.0f, .SizeStart = 80.0f, .SizePeak = 160.0f,
				.SizeEnd = 150.0f, .PeakAt = 0.15f, .SpeedOut = 120.0f, .SpeedUp = 20.0f, .Drag = 1.5f, .bRandomTurn = true,
				.bLifeFromSeconds = true, .FadeLast = 1.0f, .Max = 18 },
			{ .Tint = Steam, .TintShare = 1.0f, .Glow = 0.4f, .Rate = 10.0f, .LifeMin = 1.0f, .LifeMax = 1.6f,
				.Shape = EShape::Disc, .Spread = 80.0f, .ZFrom = 50.0f, .ZTo = 150.0f, .SizeStart = 20.0f, .SizePeak = 40.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.3f, .SpeedUp = 60.0f, .Max = 16 } } });

		Out.Add({ .Name = TEXT("funkenflug"), .BoltCount = 1, .BoltSeconds = 0.15f, .BoltThicknessCm = 3.0f, .Emitters = {
			{ .Glow = 4.0f, .Burst = 8, .LifeMin = 0.2f, .LifeMax = 0.35f, .Shape = EShape::Target, .SizeStart = 4.0f,
				.SizePeak = 4.0f, .SizeEnd = 0.0f, .Stretch = 4.0f, .SpeedOut = 400.0f, .Gravity = 600.0f, .bScale = false,
				.bScaleSize = false, .Max = 8 } } });

		Out.Add({ .Name = TEXT("splitterbruch"), .FlashShare = 0.35f, .FlashSeconds = 0.12f, .RingShare = 0.9f,
			.LightPerCm = 100.0f, .LightSeconds = 0.25f, .Emitters = {
			{ .bCube = true, .Tint = Ice, .TintShare = 1.0f, .Glow = 3.0f, .Burst = 20, .LifeMin = 0.6f, .LifeMax = 0.9f,
				.Spread = 20.0f, .ZFrom = 0.0f, .ZTo = 60.0f, .SizeStart = 14.0f, .SizePeak = 16.0f, .SizeEnd = 0.0f,
				.PeakAt = 0.1f, .SpeedOut = 600.0f, .SpeedUp = 300.0f, .Gravity = 1200.0f, .bRandomTurn = true, .Max = 20 },
			{ .Tint = White, .TintShare = 1.0f, .Glow = 5.0f, .Burst = 10, .LifeMin = 0.3f, .LifeMax = 0.5f,
				.Shape = EShape::Sphere, .Spread = 60.0f, .ZFrom = 0.0f, .ZTo = 60.0f, .SizeStart = 6.0f, .SizePeak = 10.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.3f, .Max = 10 } } });

		// It strikes at the place and comes from the target up in the sky.
		Out.Add({ .Name = TEXT("blitzschlag"), .FlashShare = 0.4f, .RingShare = 0.8f, .LightPerCm = 150.0f,
			.BoltCount = 2, .BoltSeconds = 0.3f, .BoltJitter = 0.08f, .BoltSegments = 10, .BoltThicknessCm = 10.0f,
			.BoltGlow = 5.0f, .Emitters = {
			{ .Glow = 4.0f, .Burst = 16, .LifeMin = 0.25f, .LifeMax = 0.5f, .SizeStart = 5.0f, .SizePeak = 5.0f,
				.SizeEnd = 0.0f, .Stretch = 4.0f, .SpeedOut = 600.0f, .SpeedUp = 200.0f, .Gravity = 900.0f, .Max = 16 } } });
	}

	/** Motes that circle over the head while an element sits on an enemy, so a reaction is ready. */
	FRecipe Mark(const TCHAR* Name, bool bCube, float Stretch)
	{
		return { .Name = Name, .Length = ELength::UntilStop, .Emitters = {
			{ .bCube = bCube, .Glow = 3.0f, .Burst = 3, .LifeMin = 0.6f, .LifeMax = 0.6f, .Shape = EShape::Ring, .Spread = 30.0f,
				.ZFrom = 120.0f, .ZTo = 120.0f, .SizeStart = 0.0f, .SizePeak = 14.0f, .SizeEnd = 0.0f, .PeakAt = 0.3f,
				.Stretch = Stretch, .Swirl = 180.0f, .bFollow = true, .bRandomTurn = bCube, .bHoldUntilStop = true, .Max = 3 } } };
	}

	void AddStates(TArray<FRecipe>& Out)
	{
		// The radius is 100 times the size of the enemy, and its middle is the middle of the body.
		Out.Add({ .Name = TEXT("zustand_brennend"), .Length = ELength::UntilStop, .LightPerCm = 15.0f, .bLightLasts = true,
			.Emitters = {
			{ .Tint = Fire, .TintShare = 1.0f, .Glow = 2.5f, .Rate = 25.0f, .LifeMin = 0.4f, .LifeMax = 0.6f,
				.Shape = EShape::Disc, .Spread = 30.0f, .ZFrom = -60.0f, .ZTo = 40.0f, .SizeStart = 25.0f, .SizePeak = 35.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.3f, .SpeedUp = 150.0f, .bFollow = true, .Max = 20 },
			{ .Tint = Ember, .TintShare = 1.0f, .Glow = 3.0f, .Rate = 8.0f, .LifeMin = 0.6f, .LifeMax = 1.0f,
				.Shape = EShape::Disc, .Spread = 30.0f, .ZFrom = 0.0f, .ZTo = 60.0f, .SizeStart = 6.0f, .SizePeak = 6.0f,
				.SizeEnd = 0.0f, .SpeedUp = 200.0f, .SpeedRandom = 60.0f, .Max = 10 } } });

		Out.Add({ .Name = TEXT("zustand_verlangsamt"), .Length = ELength::UntilStop, .Emitters = {
			{ .Tint = Ice, .TintShare = 1.0f, .Glow = 1.5f, .Rate = 10.0f, .LifeMin = 0.8f, .LifeMax = 1.2f,
				.Shape = EShape::Ring, .Spread = 45.0f, .ZFrom = -85.0f, .ZTo = -70.0f, .SizeStart = 12.0f, .SizePeak = 14.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.4f, .SpeedUp = 30.0f, .Swirl = 90.0f, .bFollow = true, .Max = 14 } } });

		Out.Add({ .Name = TEXT("zustand_gefroren"), .Length = ELength::UntilStop, .Emitters = {
			{ .bCube = true, .Tint = Ice, .TintShare = 1.0f, .Glow = 1.8f, .Burst = 10, .LifeMin = 0.6f, .LifeMax = 0.8f,
				.Shape = EShape::Disc, .Spread = 35.0f, .ZFrom = -80.0f, .ZTo = 60.0f, .SizeStart = 0.0f, .SizePeak = 40.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.2f, .bFollow = true, .bRandomTurn = true, .bHoldUntilStop = true, .Max = 10 },
			{ .Tint = White, .TintShare = 1.0f, .Glow = 5.0f, .Rate = 5.0f, .LifeMin = 0.3f, .LifeMax = 0.5f,
				.Shape = EShape::Sphere, .Spread = 50.0f, .ZFrom = -40.0f, .ZTo = 60.0f, .SizeStart = 0.0f, .SizePeak = 12.0f,
				.SizeEnd = 0.0f, .PeakAt = 0.5f, .bFollow = true, .Max = 5 } } });

		Out.Add(Mark(TEXT("mal_feuer"), false, 1.0f));
		Out.Add(Mark(TEXT("mal_eis"), true, 1.0f));
		Out.Add(Mark(TEXT("mal_blitz"), false, 3.0f));
		Out.Add(Mark(TEXT("mal_wind"), false, 5.0f));
	}

	void AddOthers(TArray<FRecipe>& Out)
	{
		Out.Add({ .Name = TEXT("glutschaedel_explosion"), .Seconds = 0.3f, .FlashShare = 0.7f, .FlashSeconds = 0.22f,
			.RingShare = 1.0f, .RingSeconds = 0.4f, .LightPerCm = 220.0f, .LightSeconds = 0.4f, .Emitters = {
			{ .Glow = 3.0f, .Burst = 18, .LifeMin = 0.4f, .LifeMax = 0.7f, .Shape = EShape::Sphere, .Spread = 30.0f,
				.SizeStart = 30.0f, .SizePeak = 75.0f, .SizeEnd = 0.0f, .PeakAt = 0.25f, .SpeedOut = 380.0f, .SpeedUp = 80.0f,
				.Drag = 4.0f, .bRandomTurn = true, .Max = 18 },
			{ .Tint = Ember, .TintShare = 1.0f, .Glow = 5.0f, .Burst = 30, .LifeMin = 0.5f, .LifeMax = 1.0f, .Spread = 10.0f,
				.SizeStart = 7.0f, .SizePeak = 7.0f, .SizeEnd = 0.0f, .Stretch = 5.0f, .SpeedOut = 1000.0f, .SpeedUp = 350.0f,
				.Gravity = 900.0f, .Drag = 1.0f, .Max = 30 },
			{ .Glow = 1.5f, .Rate = 40.0f, .LifeMin = 1.0f, .LifeMax = 1.6f, .Shape = EShape::Disc, .Spread = 60.0f,
				.ZFrom = 0.0f, .ZTo = 20.0f, .SizeStart = 10.0f, .SizePeak = 14.0f, .SizeEnd = 0.0f, .SpeedUp = 120.0f,
				.SpeedRandom = 40.0f, .Max = 14 } } });

		Out.Add({ .Name = TEXT("truhe_siegel"), .FlashShare = 0.5f, .FlashSeconds = 0.25f, .RingShare = 1.0f,
			.RingSeconds = 0.5f, .LightPerCm = 100.0f, .LightSeconds = 0.4f, .Emitters = {
			{ .Glow = 3.0f, .Burst = 20, .LifeMin = 0.8f, .LifeMax = 1.2f, .Shape = EShape::Sphere, .Spread = 40.0f,
				.SizeStart = 10.0f, .SizePeak = 14.0f, .SizeEnd = 0.0f, .PeakAt = 0.2f, .SpeedOut = 200.0f, .SpeedUp = 250.0f,
				.Drag = 2.0f, .Max = 20 },
			{ .Tint = White, .TintShare = 0.5f, .Glow = 5.0f, .Burst = 12, .LifeMin = 0.3f, .LifeMax = 0.5f, .SizeStart = 5.0f,
				.SizePeak = 5.0f, .SizeEnd = 0.0f, .Stretch = 4.0f, .SpeedOut = 500.0f, .Gravity = 300.0f, .Max = 12 } } });
	}

	const TArray<FRecipe>& GetRecipes()
	{
		static const TArray<FRecipe> Recipes = []
		{
			TArray<FRecipe> Out;
			AddSpells(Out);
			AddRunes(Out);
			AddReactions(Out);
			AddStates(Out);
			AddOthers(Out);
			return Out;
		}();
		return Recipes;
	}
}

AEIEffect::AEIEffect()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	InitialLifeSpan = SafetySeconds;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

const TArray<FName>& AEIEffect::GetNames()
{
	static const TArray<FName> Names = []
	{
		TArray<FName> Out;
		for (const FRecipe& Recipe : GetRecipes())
		{
			Out.Add(FName(Recipe.Name));
		}
		return Out;
	}();
	return Names;
}

int32 AEIEffect::GetPlayCount(FName Name)
{
	return GetCounts().FindRef(Name);
}

void AEIEffect::ResetPlayCounts()
{
	GetCounts().Reset();
	GetReported().Reset();
}

AEIEffect* AEIEffect::Play(const UObject* WorldContext, FName Name, const FEIEffectParams& Params)
{
	UWorld* World = WorldContext != nullptr ? WorldContext->GetWorld() : nullptr;
	if (World == nullptr || World->GetNetMode() == NM_DedicatedServer || FindRecipe(Name) == INDEX_NONE)
	{
		return nullptr;
	}
	GetCounts().FindOrAdd(Name)++;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEIEffect* Effect = World->SpawnActor<AEIEffect>(Params.Location, Params.Rotation, Spawn);
	if (Effect != nullptr)
	{
		Effect->Start(Name, Params);
	}
	return Effect;
}
void AEIEffect::Start(FName Name, const FEIEffectParams& InParams)
{
	EffectName = Name;
	RecipeIndex = FindRecipe(Name);
	const FRecipe& Recipe = GetRecipes()[RecipeIndex];
	Params = InParams;
	Params.RadiusCm = FMath::Max(Params.RadiusCm, 1.0f);
	Params.Seconds = InParams.Seconds > 0.0f ? InParams.Seconds : Recipe.Seconds;
	switch (Recipe.Length)
	{
	case ELength::Once:
		SpawnSeconds = Recipe.Seconds;
		break;
	case ELength::Seconds:
		SpawnSeconds = Params.Seconds;
		break;
	case ELength::UntilStop:
		SpawnSeconds = TNumericLimits<float>::Max();
		break;
	}
	SetLifeSpan(Recipe.Length == ELength::UntilStop ? 0.0f : SpawnSeconds + SafetySeconds);

	if (Params.AttachTo != nullptr)
	{
		AttachToComponent(Params.AttachTo, FAttachmentTransformRules::KeepWorldTransform);
		Parent = Params.AttachTo;
		bAttached = true;
	}
	Params.AttachTo = nullptr;

	const FString Key = TEXT("effekt.") + Name.ToString();
	const FEILookEntry* Entry = FEILookTable::Get().Find(Key);
	if (Entry != nullptr && Entry->HasModel())
	{
		UObject* Asset = LoadObject<UObject>(nullptr, *Entry->Model);
		if (UNiagaraSystem* System = Cast<UNiagaraSystem>(Asset))
		{
			StartModel(System, *Entry);
			bShowsPlaceholder = Entry->bKeepGlowParts;
		}
		else
		{
			ReportOnce(Entry->Model, Asset == nullptr
				? FString::Printf(TEXT("%s: Den Effekt %s gibt es nicht. Die Leuchtteile bleiben."), *Key, *Entry->Model)
				: FString::Printf(TEXT("%s: %s ist kein Niagara-Effekt, sondern %s. Die Leuchtteile bleiben."), *Key, *Entry->Model, *Asset->GetClass()->GetName()));
		}
	}
	if (!bShowsPlaceholder)
	{
		return;
	}

	const float Radius = Params.RadiusCm;
	if (Recipe.FlashShare > 0.0f)
	{
		Flash = MakePart(EIVisuals::GetSphere(), Params.Color, Recipe.FlashGlow, FlashMaterial);
	}
	if (Recipe.RingShare > 0.0f)
	{
		Ring = MakePart(EIVisuals::GetCylinder(), Params.Color, Recipe.FlashGlow * 0.6f, RingMaterial);
	}
	if (Recipe.LightPerCm > 0.0f)
	{
		Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(Root);
		Light->SetCastShadows(false);
		Light->SetLightColor(Params.Color);
		Light->SetAttenuationRadius(Radius * 3.0f);
		Light->SetIntensity(0.0f);
		Light->RegisterComponent();
		LightShare = 1.0f;
	}
	for (int32 Index = 0; Index < Recipe.Emitters.Num(); ++Index)
	{
		FEmitterState& State = Emitters.AddDefaulted_GetRef();
		State.Index = Index;
		const int32 Burst = Recipe.Emitters[Index].Burst;
		for (int32 Slot = 0; Slot < Burst; ++Slot)
		{
			SpawnParticle(State, Slot, Burst);
		}
	}
	if (FVector::Dist(Params.Location, Params.Target) >= MinBoltLengthCm)
	{
		for (int32 Count = 0; Count < Recipe.BoltCount; ++Count)
		{
			StartBolt(Params.Location, Params.Target);
		}
	}
	NextBoltIn = Recipe.BoltEvery * 0.3f;
	TickLayers(0.0f);
	for (FEmitterState& State : Emitters)
	{
		TickEmitter(State, 0.0f);
	}
}

void AEIEffect::StartModel(UNiagaraSystem* System, const FEILookEntry& Entry)
{
	Model = NewObject<UNiagaraComponent>(this);
	Model->SetupAttachment(Root);
	Model->SetRelativeTransform(Entry.GetTransform());
	Model->SetAutoActivate(false);
	Model->SetAutoDestroy(false);
	Model->SetAsset(System);
	Model->OnSystemFinished.AddDynamic(this, &AEIEffect::HandleModelFinished);
	Model->RegisterComponent();
	Model->SetVariableLinearColor(TEXT("Farbe"), Params.Color);
	Model->SetVariableFloat(TEXT("Radius"), Params.RadiusCm);
	Model->SetVariableFloat(TEXT("Dauer"), Params.Seconds);
	Model->SetVariableFloat(TEXT("Winkel"), Params.AngleDegrees);
	Model->SetVariableVec3(TEXT("Ziel"), Params.Target - Params.Location);
	Model->Activate(true);
}

void AEIEffect::HandleModelFinished(UNiagaraComponent* Finished)
{
	bModelFinished = true;
}

UStaticMeshComponent* AEIEffect::MakePart(UStaticMesh* Mesh, const FLinearColor& Color, float Glow, UMaterialInstanceDynamic*& Material)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetStaticMesh(Mesh);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetGenerateOverlapEvents(false);
	Part->SetCanEverAffectNavigation(false);
	Part->SetCastShadow(false);
	Part->bReceivesDecals = false;
	Part->SetUsingAbsoluteLocation(true);
	Part->SetUsingAbsoluteRotation(true);
	Part->SetUsingAbsoluteScale(true);
	Part->SetVisibility(false);
	Part->SetupAttachment(Root);
	Part->RegisterComponent();
	if (Material == nullptr)
	{
		Material = EIVisuals::SetGlow(Part, Color, Glow);
		Kept.Add(Material);
	}
	else
	{
		Part->SetMaterial(0, Material);
	}
	Kept.Add(Part);
	return Part;
}

FVector AEIEffect::GetCenter() const
{
	return GetActorLocation();
}

void AEIEffect::SpawnParticle(FEmitterState& State, int32 Slot, int32 Slots)
{
	const FEmitter& Emitter = GetRecipes()[RecipeIndex].Emitters[State.Index];
	if (State.Particles.Num() >= Emitter.Max)
	{
		return;
	}
	const float Scale = Params.RadiusCm / 100.0f;
	const float Place = Emitter.bScale ? Scale : 1.0f;
	const float SizeScale = Emitter.bScaleSize ? Scale : 1.0f;
	const float Spread = Emitter.Spread * Place;
	const FRotator Facing = GetActorRotation();
	const FVector Forward = Facing.Vector();
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);

	FVector Offset = FVector::ZeroVector;
	FVector Out = FMath::VRand();
	switch (Emitter.Shape)
	{
	case EShape::Point:
		Offset = RandomInBall(Spread);
		break;
	case EShape::Sphere:
		Offset = RandomInBall(Spread);
		Out = Offset.IsNearlyZero() ? Out : Offset.GetSafeNormal();
		break;
	case EShape::Ring:
	{
		// A burst spreads evenly around, so a few motes over a head do not clump.
		const float Angle = Slots > 0 ? (Slot + FMath::FRand() * 0.3f) * 2.0f * PI / Slots : FMath::FRand() * 2.0f * PI;
		Out = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		Offset = Out * Spread;
		break;
	}
	case EShape::Disc:
		Offset = RandomInCircle(Spread);
		Out = Offset.IsNearlyZero() ? RandomOnCircle(1.0f) : Offset.GetSafeNormal2D();
		break;
	case EShape::Wall:
	{
		const float Across = FMath::FRandRange(-Spread, Spread);
		Offset = Right * FMath::FRandRange(-Params.RadiusCm, Params.RadiusCm) + Forward * Across;
		Out = Across >= 0.0f ? Forward : -Forward;
		break;
	}
	case EShape::Cone:
	{
		const float HalfAngle = Params.AngleDegrees > 0.0f ? Params.AngleDegrees / 2.0f : 15.0f;
		Offset = RandomInBall(Spread);
		Out = FMath::VRandCone(Forward, FMath::DegreesToRadians(HalfAngle));
		break;
	}
	case EShape::Target:
		Offset = Params.Target - GetCenter() + RandomInBall(Spread);
		break;
	}
	Offset.Z += FMath::FRandRange(Emitter.ZFrom, Emitter.ZTo) * SizeScale;

	FParticle& Particle = State.Particles.AddDefaulted_GetRef();
	Particle.Position = Emitter.bFollow ? Offset : GetCenter() + Offset;
	Particle.Velocity = (Out * Emitter.SpeedOut + FVector::UpVector * Emitter.SpeedUp + Forward * Emitter.SpeedForward
		+ FMath::VRand() * Emitter.SpeedRandom * FMath::FRand()) * Place;
	Particle.Turn = Emitter.bRandomTurn ? FRotator(FMath::FRand() * 360.0f, FMath::FRand() * 360.0f, FMath::FRand() * 360.0f) : Facing;
	Particle.Motion = Particle.Velocity * 0.01f;
	const float Life = FMath::FRandRange(Emitter.LifeMin, Emitter.LifeMax);
	Particle.Life = FMath::Max(Emitter.bLifeFromSeconds ? Params.Seconds * Life : Life, 0.01f);
}

void AEIEffect::TickEmitter(FEmitterState& State, float DeltaSeconds)
{
	const FEmitter& Emitter = GetRecipes()[RecipeIndex].Emitters[State.Index];
	const float Scale = Params.RadiusCm / 100.0f;
	const float Place = Emitter.bScale ? Scale : 1.0f;
	const float SizeScale = Emitter.bScaleSize ? Scale : 1.0f;
	const FVector Center = GetCenter();

	if (!bStopped && Age < SpawnSeconds && Emitter.Rate > 0.0f)
	{
		State.SpawnDebt += Emitter.Rate * FMath::Pow(Scale, Emitter.RateExponent) * DeltaSeconds;
		while (State.SpawnDebt >= 1.0f)
		{
			State.SpawnDebt -= 1.0f;
			SpawnParticle(State);
		}
	}

	for (int32 Index = State.Particles.Num() - 1; Index >= 0; --Index)
	{
		FParticle& Particle = State.Particles[Index];
		Particle.Age += DeltaSeconds;
		if (Emitter.bHoldUntilStop && !bStopped)
		{
			Particle.Age = FMath::Min(Particle.Age, Emitter.PeakAt * Particle.Life);
		}
		if (Particle.Age >= Particle.Life)
		{
			State.Particles.RemoveAtSwap(Index);
			continue;
		}
		Particle.Velocity.Z -= Emitter.Gravity * Place * DeltaSeconds;
		if (Emitter.Drag > 0.0f)
		{
			Particle.Velocity *= FMath::Exp(-Emitter.Drag * DeltaSeconds);
		}
		FVector Relative = Emitter.bFollow ? Particle.Position : Particle.Position - Center;
		if (Emitter.Pull > 0.0f)
		{
			Particle.Velocity -= FVector(Relative.X, Relative.Y, 0.0f).GetSafeNormal() * Emitter.Pull * Place * DeltaSeconds;
		}
		const FVector Before = Relative;
		Relative += Particle.Velocity * DeltaSeconds;
		if (Emitter.Swirl != 0.0f)
		{
			Relative = Relative.RotateAngleAxis(Emitter.Swirl * DeltaSeconds, FVector::UpVector);
		}
		if (DeltaSeconds > 0.0f)
		{
			Particle.Motion = Relative - Before;
		}
		Particle.Position = Emitter.bFollow ? Relative : Center + Relative;
	}

	while (State.Parts.Num() < State.Particles.Num())
	{
		const FLinearColor Color = FMath::Lerp(Params.Color, Emitter.Tint, Emitter.TintShare);
		State.Parts.Add(MakePart(Emitter.bCube ? EIVisuals::GetCube() : EIVisuals::GetSphere(), Color, Emitter.Glow, State.Material));
	}
	for (int32 Index = 0; Index < State.Parts.Num(); ++Index)
	{
		UStaticMeshComponent* Part = State.Parts[Index];
		if (!State.Particles.IsValidIndex(Index))
		{
			if (Part->IsVisible())
			{
				Part->SetVisibility(false);
			}
			continue;
		}
		const FParticle& Particle = State.Particles[Index];
		const float T = Particle.Age / Particle.Life;
		const float Size = SizeScale * (T < Emitter.PeakAt
			? FMath::Lerp(Emitter.SizeStart, Emitter.SizePeak, T / FMath::Max(Emitter.PeakAt, KINDA_SMALL_NUMBER))
			: FMath::Lerp(Emitter.SizePeak, Emitter.SizeEnd, (T - Emitter.PeakAt) / FMath::Max(1.0f - Emitter.PeakAt, KINDA_SMALL_NUMBER)));
		if (Size < 0.5f)
		{
			Part->SetVisibility(false);
			continue;
		}
		const FVector Where = Emitter.bFollow ? Center + Particle.Position : Particle.Position;
		if (Emitter.Stretch != 1.0f && !Particle.Motion.IsNearlyZero())
		{
			Part->SetWorldLocationAndRotation(Where, Particle.Motion.Rotation());
			Part->SetWorldScale3D(FVector(Size * Emitter.Stretch, Size, Size) / MeshSizeCm);
		}
		else
		{
			Part->SetWorldLocationAndRotation(Where, Particle.Turn);
			Part->SetWorldScale3D(FVector(Size / MeshSizeCm));
		}
		if (!Part->IsVisible())
		{
			Part->SetVisibility(true);
		}
	}

	if (Emitter.FadeLast > 0.0f && State.Material != nullptr)
	{
		const float End = Emitter.bLifeFromSeconds ? Params.Seconds * Emitter.LifeMax : Emitter.LifeMax;
		State.Material->SetScalarParameterValue(Opacity, FMath::Clamp((End - Age) / Emitter.FadeLast, 0.0f, 1.0f));
	}
}

void AEIEffect::StartBolt(const FVector& From, const FVector& To)
{
	const FRecipe& Recipe = GetRecipes()[RecipeIndex];
	FBolt* Bolt = Bolts.FindByPredicate([&Recipe](const FBolt& Each) { return Each.Age >= Recipe.BoltSeconds; });
	if (Bolt == nullptr)
	{
		Bolt = &Bolts.AddDefaulted_GetRef();
		for (int32 Segment = 0; Segment < Recipe.BoltSegments; ++Segment)
		{
			Bolt->Parts.Add(MakePart(EIVisuals::GetCube(), Params.Color, Recipe.BoltGlow, Bolt->Material));
		}
	}
	Bolt->From = From;
	Bolt->To = To;
	Bolt->Age = 0.0f;
	Bolt->NextJitterIn = BoltRejitterSeconds;
	if (Bolt->Material != nullptr)
	{
		Bolt->Material->SetScalarParameterValue(Opacity, 1.0f);
	}
	LayOutBolt(*Bolt);
}

void AEIEffect::LayOutBolt(FBolt& Bolt) const
{
	const FRecipe& Recipe = GetRecipes()[RecipeIndex];
	const FVector Line = Bolt.To - Bolt.From;
	const float Length = Line.Size();
	const FVector Along = Line.GetSafeNormal();
	const FVector Side = FVector::CrossProduct(Along, FMath::Abs(Along.Z) > 0.9f ? FVector::ForwardVector : FVector::UpVector).GetSafeNormal();
	const FVector Up = FVector::CrossProduct(Side, Along);
	const int32 Segments = Bolt.Parts.Num();
	FVector Last = Bolt.From;
	for (int32 Segment = 0; Segment < Segments; ++Segment)
	{
		const float T = static_cast<float>(Segment + 1) / Segments;
		FVector Next = Bolt.From + Line * T;
		if (Segment + 1 < Segments)
		{
			// The bends stray most in the middle, so both ends stay on their spots.
			const float Stray = Recipe.BoltJitter * Length * FMath::Sin(T * PI);
			Next += Side * FMath::FRandRange(-Stray, Stray) + Up * FMath::FRandRange(-Stray, Stray) * 0.5f;
		}
		UStaticMeshComponent* Part = Bolt.Parts[Segment];
		const FVector Piece = Next - Last;
		Part->SetWorldLocationAndRotation((Last + Next) / 2.0, Piece.Rotation());
		Part->SetWorldScale3D(FVector(Piece.Size(), Recipe.BoltThicknessCm, Recipe.BoltThicknessCm) / MeshSizeCm);
		Part->SetVisibility(true);
		Last = Next;
	}
}

void AEIEffect::TickBolts(float DeltaSeconds)
{
	const FRecipe& Recipe = GetRecipes()[RecipeIndex];
	if (Recipe.BoltEvery > 0.0f && bShowsPlaceholder && !bStopped && Age < SpawnSeconds)
	{
		NextBoltIn -= DeltaSeconds;
		if (NextBoltIn <= 0.0f)
		{
			NextBoltIn = Recipe.BoltEvery * FMath::FRandRange(0.6f, 1.4f);
			const FVector Spot = GetCenter() + RandomInCircle(Params.RadiusCm * 0.9f);
			StartBolt(Spot, Spot + FVector(0.0f, 0.0f, SkyBoltHeightCm));
		}
	}
	for (FBolt& Bolt : Bolts)
	{
		if (Bolt.Age >= Recipe.BoltSeconds)
		{
			continue;
		}
		Bolt.Age += DeltaSeconds;
		if (Bolt.Age >= Recipe.BoltSeconds)
		{
			for (UStaticMeshComponent* Part : Bolt.Parts)
			{
				Part->SetVisibility(false);
			}
			continue;
		}
		Bolt.NextJitterIn -= DeltaSeconds;
		if (Bolt.NextJitterIn <= 0.0f)
		{
			Bolt.NextJitterIn = BoltRejitterSeconds;
			LayOutBolt(Bolt);
		}
		if (Bolt.Material != nullptr)
		{
			Bolt.Material->SetScalarParameterValue(Opacity, 1.0f - Bolt.Age / Recipe.BoltSeconds);
		}
	}
}

void AEIEffect::TickLayers(float DeltaSeconds)
{
	const FRecipe& Recipe = GetRecipes()[RecipeIndex];
	const FVector Center = GetCenter();
	if (Flash != nullptr)
	{
		const float T = Age / Recipe.FlashSeconds;
		Flash->SetVisibility(T < 1.0f);
		if (T < 1.0f)
		{
			const float Growth = 1.0f - FMath::Square(1.0f - T);
			Flash->SetWorldLocation(Center);
			Flash->SetWorldScale3D(FVector(2.0f * Params.RadiusCm * Recipe.FlashShare * FMath::Lerp(0.3f, 1.0f, Growth) / MeshSizeCm));
			if (FlashMaterial != nullptr)
			{
				FlashMaterial->SetScalarParameterValue(Opacity, 1.0f - T);
			}
		}
	}
	if (Ring != nullptr)
	{
		const float T = Age / Recipe.RingSeconds;
		Ring->SetVisibility(T < 1.0f);
		if (T < 1.0f)
		{
			const float Growth = 1.0f - FMath::Square(1.0f - T);
			const float Width = 2.0f * Params.RadiusCm * Recipe.RingShare * FMath::Lerp(0.2f, 1.0f, Growth) / MeshSizeCm;
			Ring->SetWorldLocation(Center + FVector(0.0f, 0.0f, 5.0f));
			Ring->SetWorldScale3D(FVector(Width, Width, RingThickness));
			if (RingMaterial != nullptr)
			{
				RingMaterial->SetScalarParameterValue(Opacity, 1.0f - T);
			}
		}
	}
	if (Light != nullptr)
	{
		if (Recipe.bLightLasts && !bStopped && Age < SpawnSeconds)
		{
			LightShare = FMath::FRandRange(0.7f, 1.0f);
		}
		else if (Recipe.bLightLasts)
		{
			LightShare = FMath::Max(LightShare - DeltaSeconds / LightFadeSeconds, 0.0f);
		}
		else
		{
			LightShare = FMath::Max(1.0f - Age / Recipe.LightSeconds, 0.0f);
		}
		Light->SetIntensity(Params.RadiusCm * Recipe.LightPerCm * LightShare);
	}
}

bool AEIEffect::IsLasting() const
{
	return GetRecipes()[RecipeIndex].Length != ELength::Once;
}

int32 AEIEffect::CountShownParts() const
{
	int32 Count = 0;
	for (const TObjectPtr<UObject>& Each : Kept)
	{
		const UStaticMeshComponent* Part = Cast<UStaticMeshComponent>(Each);
		Count += Part != nullptr && Part->IsVisible() ? 1 : 0;
	}
	return Count;
}

void AEIEffect::Stop()
{
	if (bStopped)
	{
		return;
	}
	bStopped = true;
	if (Model != nullptr && !bModelStopped)
	{
		bModelStopped = true;
		Model->Deactivate();
	}
	// What it made stays where it was while it fades, so a burning enemy that dies leaves its flames behind.
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

bool AEIEffect::IsDone() const
{
	const FRecipe& Recipe = GetRecipes()[RecipeIndex];
	if (!bStopped && Age < SpawnSeconds)
	{
		return false;
	}
	for (const FEmitterState& State : Emitters)
	{
		if (State.Particles.Num() > 0)
		{
			return false;
		}
	}
	for (const FBolt& Bolt : Bolts)
	{
		if (Bolt.Age < Recipe.BoltSeconds)
		{
			return false;
		}
	}
	if ((Flash != nullptr && Age < Recipe.FlashSeconds) || (Ring != nullptr && Age < Recipe.RingSeconds) || LightShare > 0.0f)
	{
		return false;
	}
	return Model == nullptr || bModelFinished;
}

void AEIEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (RecipeIndex == INDEX_NONE)
	{
		Destroy();
		return;
	}
	Age += DeltaSeconds;
	if (bAttached && !bStopped)
	{
		const USceneComponent* Holder = Parent.Get();
		const AActor* Holding = Holder != nullptr ? Holder->GetOwner() : nullptr;
		if (Holding == nullptr || !IsValid(Holding) || Holding->IsActorBeingDestroyed())
		{
			Stop();
		}
	}
	if (Model != nullptr && !bModelStopped && GetRecipes()[RecipeIndex].Length == ELength::Seconds && Age >= SpawnSeconds)
	{
		bModelStopped = true;
		Model->Deactivate();
	}
	for (FEmitterState& State : Emitters)
	{
		TickEmitter(State, DeltaSeconds);
	}
	TickBolts(DeltaSeconds);
	TickLayers(DeltaSeconds);
	if (IsDone())
	{
		Destroy();
	}
}
