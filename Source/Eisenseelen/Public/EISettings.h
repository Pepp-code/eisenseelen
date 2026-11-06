#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/**
 * The settings of the player, from Data/einstellungen.json as the defaults and from
 * Saved/Einstellungen.json as what the player chose in the pause menu. Only the local player has them.
 * Every knob has a range and a step, so the menu can only turn it as far as it makes sense.
 */
struct EISENSEELEN_API FEISettings
{
	/** How fast the view turns, multiplied onto the hero's mouse factor. */
	float MouseFactor = 1.0f;
	/** Whether pushing the mouse up looks down instead of up. */
	bool bInvertLook = false;
	/** Whether the HUD draws its bars and hints at all. The pause menu always draws. */
	bool bHudOn = true;
	/** Whether hits and shots show their damage as numbers. Ready for when they exist, off shows none of the player's own. */
	bool bDamageNumbers = true;
	/** How loud effects are played, from silent to full. */
	float EffectsVolume = 1.0f;
	/** How loud music will be played, from silent to full. There is no music yet. */
	float MusicVolume = 1.0f;

	static constexpr float MouseFactorMin = 0.2f;
	static constexpr float MouseFactorMax = 3.0f;
	static constexpr float MouseFactorStep = 0.1f;
	static constexpr float VolumeMin = 0.0f;
	static constexpr float VolumeMax = 1.0f;
	static constexpr float VolumeStep = 0.05f;

	/** Reads the defaults from Data/einstellungen.json. False, with a German error, if it cannot be read. */
	static bool LoadFromProjectFile(FEISettings& OutSettings, FString& OutError);

	/** What the player chose, as far as the file says. What it does not say stays at the default. */
	FString ToJson() const;
	bool FromJson(const FString& Json, FString& OutError);

	/** Where the chosen settings live. Like the save game, only written when a real player plays. */
	static bool UsesDisk();
	static FString GetPath();
	bool SaveToFile(const FString& Path) const;

	/** Turns a knob by so many steps, and holds the range. */
	void AddMouseFactor(int32 Steps);
	void AddEffectsVolume(int32 Steps);
	void AddMusicVolume(int32 Steps);
};
