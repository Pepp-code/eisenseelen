#include "EIHeroValues.h"

#include "Dom/JsonObject.h"
#include "EIDataFile.h"

namespace
{
	bool ReadValues(const FJsonObject& Root, FEIHeroValues& OutValues, FString& OutError)
	{
		return FEIDataFile::ReadNumber(Root, TEXT("lauftempo_m_pro_s"), 1.0, 30.0, OutValues.RunSpeedMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("sprunghoehe_m"), 0.2, 10.0, OutValues.JumpHeightMeters, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("schwerkraft_faktor"), 0.2, 5.0, OutValues.GravityFactor, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("luftkontrolle_0_bis_1"), 0.0, 1.0, OutValues.AirControl, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("maus_faktor"), 0.1, 5.0, OutValues.MouseFactor, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("dash_tempo_bonus_m_pro_s"), 0.0, 30.0, OutValues.DashSpeedBonusMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("dash_hopser_m_pro_s"), 0.0, 20.0, OutValues.DashHopMetersPerSecond, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("dash_abklingzeit_s"), 0.0, 30.0, OutValues.DashCooldownSeconds, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("tempo_kette_fenster_s"), 0.0, 1.0, OutValues.SpeedChainWindowSeconds, OutError)
			&& FEIDataFile::ReadNumber(Root, TEXT("hoechsttempo_m_pro_s"), 1.0, 100.0, OutValues.MaxSpeedMetersPerSecond, OutError);
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
