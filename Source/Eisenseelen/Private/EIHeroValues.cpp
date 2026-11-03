#include "EIHeroValues.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"

namespace
{
	bool ReadValues(const FJsonObject& Root, FEIHeroValues& OutValues, FString& OutError)
	{
		return FEIDataFile::ReadNumber(Root, TEXT("lauftempo_m_pro_s"), 1.0, 30.0, OutValues.RunSpeedMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("sprint_tempo_m_pro_s"), 1.0, 30.0, OutValues.SprintSpeedMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("blocken_tempo_m_pro_s"), 0.1, 30.0, OutValues.BlockSpeedMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("sprunghoehe_m"), 0.2, 10.0, OutValues.JumpHeightMeters, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("schwerkraft_faktor"), 0.2, 5.0, OutValues.GravityFactor, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("luftkontrolle_0_bis_1"), 0.0, 1.0, OutValues.AirControl, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("luft_lenkung_grad_pro_s"), 0.0, 2000.0, OutValues.AirTurnDegreesPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("maus_faktor"), 0.1, 5.0, OutValues.MouseFactor, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("leben_erholung_nach_s"), 0.0, 600.0, OutValues.LifeBackAfterSeconds, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("leben_erholung_anteil_pro_s"), 0.0, 10.0, OutValues.LifeBackSharePerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("sprung_puffer_s"), 0.0, 1.0, OutValues.JumpBufferSeconds, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("absprung_ab_hoehe_m"), 1.0, 1000.0, OutValues.DiveFromHeightMeters, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("absprung_tempo_m_pro_s"), 1.0, 100.0, OutValues.DiveSpeedMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("absprung_fall_tempo_m_pro_s"), 1.0, 100.0, OutValues.DiveFallSpeedMetersPerSecond, OutError);
	}
}

bool FEIHeroValues::LoadFromString(const FString& Json, FEIHeroValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Parse(Json, OutError);
	FEIHeroValues Loaded;
	if (!Root.IsValid() || !ReadValues(*Root, Loaded, OutError))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}

bool FEIHeroValues::LoadFromProjectFile(FEIHeroValues& OutValues, FString& OutError)
{
	const TSharedPtr<FJsonObject> Root = FEIDataFile::Load(TEXT("held.json"), OutError);
	FEIHeroValues Loaded;
	if (!Root.IsValid() || !ReadValues(*Root, Loaded, OutError))
	{
		return false;
	}
	OutValues = Loaded;
	return true;
}
