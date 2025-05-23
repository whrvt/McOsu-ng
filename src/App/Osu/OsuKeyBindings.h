//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		key bindings container
//
// $NoKeywords: $osukey
//===============================================================================//

#pragma once
#ifndef OSUKEYBINDINGS_H
#define OSUKEYBINDINGS_H

#include "ConVar.h"

class OsuKeyBindings
{
public:
	OsuKeyBindings();
	~OsuKeyBindings();
	static ConVar LEFT_CLICK;
	static ConVar RIGHT_CLICK;
	static ConVar LEFT_CLICK_2;
	static ConVar RIGHT_CLICK_2;

	static ConVar FPOSU_ZOOM;

	static ConVar INCREASE_SPEED;
	static ConVar DECREASE_SPEED;

	static ConVar INCREASE_VOLUME;
	static ConVar DECREASE_VOLUME;

	static ConVar INCREASE_LOCAL_OFFSET;
	static ConVar DECREASE_LOCAL_OFFSET;

	static ConVar GAME_PAUSE;
	static ConVar SKIP_CUTSCENE;
	static ConVar TOGGLE_SCOREBOARD;
	static ConVar SEEK_TIME;
	static ConVar SEEK_TIME_BACKWARD;
	static ConVar SEEK_TIME_FORWARD;
	static ConVar QUICK_RETRY;
	static ConVar QUICK_SAVE;
	static ConVar QUICK_LOAD;
	static ConVar SAVE_SCREENSHOT;
	static ConVar DISABLE_MOUSE_BUTTONS;
	static ConVar BOSS_KEY;

	static ConVar TOGGLE_MODSELECT;
	static ConVar RANDOM_BEATMAP;

	static ConVar MOD_EASY;
	static ConVar MOD_NOFAIL;
	static ConVar MOD_HALFTIME;
	static ConVar MOD_HARDROCK;
	static ConVar MOD_SUDDENDEATH;
	static ConVar MOD_DOUBLETIME;
	static ConVar MOD_HIDDEN;
	static ConVar MOD_FLASHLIGHT;
	static ConVar MOD_RELAX;
	static ConVar MOD_AUTOPILOT;
	static ConVar MOD_SPUNOUT;
	static ConVar MOD_AUTO;
	static ConVar MOD_SCOREV2;

	[[nodiscard]] constexpr const std::vector<ConVar*>* get() const {return &ALL;}
	[[nodiscard]] constexpr const std::vector<std::vector<ConVar*>>* getMania() const {return &MANIA;}

private:

	std::vector<ConVar*> ALL;
	std::vector<std::vector<ConVar*>> MANIA;

	static std::vector<ConVar*> createManiaConVarSet(int k);
	static std::vector<std::vector<ConVar*>> createManiaConVarSets();
	static void setDefaultManiaKeys(std::vector<std::vector<ConVar*>> mania);
};

#endif
