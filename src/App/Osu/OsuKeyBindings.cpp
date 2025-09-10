//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		key bindings container
//
// $NoKeywords: $osukey
//===============================================================================//

#include "OsuKeyBindings.h"

#include "Keyboard.h"
namespace cv::osu::keybinds {
ConVar LEFT_CLICK("osu_key_left_click", (int)KEY_Z, FCVAR_NONE);
ConVar RIGHT_CLICK("osu_key_right_click", (int)KEY_X, FCVAR_NONE);
ConVar LEFT_CLICK_2("osu_key_left_click_2", 0, FCVAR_NONE);
ConVar RIGHT_CLICK_2("osu_key_right_click_2", 0, FCVAR_NONE);

ConVar FPOSU_ZOOM("osu_key_fposu_zoom", 0, FCVAR_NONE);

ConVar INCREASE_SPEED("osu_key_mania_increase_speed", (int)KEY_RIGHT, FCVAR_NONE);
ConVar DECREASE_SPEED("osu_key_mania_decrease_speed", (int)KEY_LEFT, FCVAR_NONE);

ConVar INCREASE_VOLUME("osu_key_increase_volume", (int)KEY_UP, FCVAR_NONE);
ConVar DECREASE_VOLUME("osu_key_decrease_volume", (int)KEY_DOWN, FCVAR_NONE);

ConVar INCREASE_LOCAL_OFFSET("osu_key_increase_local_offset", (int)KEY_ADD, FCVAR_NONE);
ConVar DECREASE_LOCAL_OFFSET("osu_key_decrease_local_offset", (int)KEY_SUBTRACT, FCVAR_NONE);

ConVar GAME_PAUSE("osu_key_game_pause", (int)(Env::cfg(OS::WASM) ? KEY_TILDE : KEY_ESCAPE), FCVAR_NONE);
ConVar SKIP_CUTSCENE("osu_key_skip_cutscene", (int)KEY_SPACE, FCVAR_NONE);
ConVar TOGGLE_SCOREBOARD("osu_key_toggle_scoreboard", (int)KEY_TAB, FCVAR_NONE);
ConVar SEEK_TIME("osu_key_seek_time", (int)KEY_LSHIFT, FCVAR_NONE);
ConVar SEEK_TIME_BACKWARD("osu_key_seek_time_backward", (int)KEY_LEFT, FCVAR_NONE);
ConVar SEEK_TIME_FORWARD("osu_key_seek_time_forward", (int)KEY_RIGHT, FCVAR_NONE);
ConVar QUICK_RETRY("osu_key_quick_retry", (int)KEY_BACKSPACE, FCVAR_NONE);
ConVar QUICK_SAVE("osu_key_quick_save", (int)KEY_F6, FCVAR_NONE);
ConVar QUICK_LOAD("osu_key_quick_load", (int)KEY_F7, FCVAR_NONE);
ConVar SAVE_SCREENSHOT("osu_key_save_screenshot", (int)KEY_F12, FCVAR_NONE);
ConVar DISABLE_MOUSE_BUTTONS("osu_key_disable_mouse_buttons", (int)KEY_F10, FCVAR_NONE);
ConVar BOSS_KEY("osu_key_boss", (int)KEY_INSERT, FCVAR_NONE);

ConVar TOGGLE_MODSELECT("osu_key_toggle_modselect", (int)KEY_F1, FCVAR_NONE);
ConVar RANDOM_BEATMAP("osu_key_random_beatmap", (int)KEY_F2, FCVAR_NONE);

ConVar MOD_EASY("osu_key_mod_easy", (int)KEY_Q, FCVAR_NONE);
ConVar MOD_NOFAIL("osu_key_mod_nofail", (int)KEY_W, FCVAR_NONE);
ConVar MOD_HALFTIME("osu_key_mod_halftime", (int)KEY_E, FCVAR_NONE);
ConVar MOD_HARDROCK("osu_key_mod_hardrock", (int)KEY_A, FCVAR_NONE);
ConVar MOD_SUDDENDEATH("osu_key_mod_suddendeath", (int)KEY_S, FCVAR_NONE);
ConVar MOD_DOUBLETIME("osu_key_mod_doubletime", (int)KEY_D, FCVAR_NONE);
ConVar MOD_HIDDEN("osu_key_mod_hidden", (int)KEY_F, FCVAR_NONE);
ConVar MOD_FLASHLIGHT("osu_key_mod_flashlight", (int)KEY_G, FCVAR_NONE);
ConVar MOD_RELAX("osu_key_mod_relax", (int)KEY_Z, FCVAR_NONE);
ConVar MOD_AUTOPILOT("osu_key_mod_autopilot", (int)KEY_X, FCVAR_NONE);
ConVar MOD_SPUNOUT("osu_key_mod_spunout", (int)KEY_C, FCVAR_NONE);
ConVar MOD_AUTO("osu_key_mod_auto", (int)KEY_V, FCVAR_NONE);
ConVar MOD_SCOREV2("osu_key_mod_scorev2", (int)KEY_B, FCVAR_NONE);
}

OsuKeyBindings::OsuKeyBindings()
{
	ALL = {&cv::osu::keybinds::LEFT_CLICK,
	       &cv::osu::keybinds::RIGHT_CLICK,
	       &cv::osu::keybinds::LEFT_CLICK_2,
	       &cv::osu::keybinds::RIGHT_CLICK_2,

	       &cv::osu::keybinds::FPOSU_ZOOM,

	       &cv::osu::keybinds::INCREASE_SPEED,
	       &cv::osu::keybinds::DECREASE_SPEED,

	       &cv::osu::keybinds::INCREASE_VOLUME,
	       &cv::osu::keybinds::DECREASE_VOLUME,

	       &cv::osu::keybinds::INCREASE_LOCAL_OFFSET,
	       &cv::osu::keybinds::DECREASE_LOCAL_OFFSET,

	       &cv::osu::keybinds::GAME_PAUSE,
	       &cv::osu::keybinds::SKIP_CUTSCENE,
	       &cv::osu::keybinds::TOGGLE_SCOREBOARD,
	       &cv::osu::keybinds::SEEK_TIME,
	       &cv::osu::keybinds::SEEK_TIME_BACKWARD,
	       &cv::osu::keybinds::SEEK_TIME_FORWARD,
	       &cv::osu::keybinds::QUICK_RETRY,
	       &cv::osu::keybinds::QUICK_SAVE,
	       &cv::osu::keybinds::QUICK_LOAD,
	       &cv::osu::keybinds::SAVE_SCREENSHOT,
	       &cv::osu::keybinds::DISABLE_MOUSE_BUTTONS,
	       &cv::osu::keybinds::BOSS_KEY,

	       &cv::osu::keybinds::TOGGLE_MODSELECT,
	       &cv::osu::keybinds::RANDOM_BEATMAP,

	       &cv::osu::keybinds::MOD_EASY,
	       &cv::osu::keybinds::MOD_NOFAIL,
	       &cv::osu::keybinds::MOD_HALFTIME,
	       &cv::osu::keybinds::MOD_HARDROCK,
	       &cv::osu::keybinds::MOD_SUDDENDEATH,
	       &cv::osu::keybinds::MOD_DOUBLETIME,
	       &cv::osu::keybinds::MOD_HIDDEN,
	       &cv::osu::keybinds::MOD_FLASHLIGHT,
	       &cv::osu::keybinds::MOD_RELAX,
	       &cv::osu::keybinds::MOD_AUTOPILOT,
	       &cv::osu::keybinds::MOD_SPUNOUT,
	       &cv::osu::keybinds::MOD_AUTO,
	       &cv::osu::keybinds::MOD_SCOREV2};

	MANIA = OsuKeyBindings::createManiaConVarSets();
}

OsuKeyBindings::~OsuKeyBindings()
{
	for (auto &maniaSet : MANIA)
	{
		for (auto *conVar : maniaSet)
			delete conVar;
		maniaSet.clear();
	}
	MANIA.clear();
	MANIA.shrink_to_fit();

	ALL.clear();
	ALL.shrink_to_fit();
}

std::vector<ConVar*> OsuKeyBindings::createManiaConVarSet(int k)
{
	std::vector<ConVar*> convars;
	for (int i=1; i<=k; i++)
	{
		convars.push_back(MAKENEWCONVAR(fmt::format("osu_key_mania_{:d}k_{:d}", k, i), 0));
	}
	return convars;
}

std::vector<std::vector<ConVar*>> OsuKeyBindings::createManiaConVarSets()
{
	std::vector<std::vector<ConVar*>> sets;
	for (int i=1; i<=10; i++)
	{
		sets.push_back(createManiaConVarSet(i));
	}
	setDefaultManiaKeys(sets);
	return sets;
}

void OsuKeyBindings::setDefaultManiaKeys(std::vector<std::vector<ConVar*>> mania)
{
	mania[0][0]->setValue((int)KEY_F);

	mania[1][0]->setValue((int)KEY_F);
	mania[1][1]->setValue((int)KEY_J);

	mania[2][0]->setValue((int)KEY_F);
	mania[2][1]->setValue((int)KEY_SPACE);
	mania[2][2]->setValue((int)KEY_J);

	mania[3][0]->setValue((int)KEY_D);
	mania[3][1]->setValue((int)KEY_F);
	mania[3][2]->setValue((int)KEY_J);
	mania[3][3]->setValue((int)KEY_K);

	mania[4][0]->setValue((int)KEY_D);
	mania[4][1]->setValue((int)KEY_F);
	mania[4][2]->setValue((int)KEY_SPACE);
	mania[4][3]->setValue((int)KEY_J);
	mania[4][4]->setValue((int)KEY_K);

	mania[5][0]->setValue((int)KEY_S);
	mania[5][1]->setValue((int)KEY_D);
	mania[5][2]->setValue((int)KEY_F);
	mania[5][3]->setValue((int)KEY_J);
	mania[5][4]->setValue((int)KEY_K);
	mania[5][5]->setValue((int)KEY_L);

	mania[6][0]->setValue((int)KEY_S);
	mania[6][1]->setValue((int)KEY_D);
	mania[6][2]->setValue((int)KEY_F);
	mania[6][3]->setValue((int)KEY_SPACE);
	mania[6][4]->setValue((int)KEY_J);
	mania[6][5]->setValue((int)KEY_K);
	mania[6][6]->setValue((int)KEY_L);

	mania[7][0]->setValue((int)KEY_A);
	mania[7][1]->setValue((int)KEY_S);
	mania[7][2]->setValue((int)KEY_D);
	mania[7][3]->setValue((int)KEY_F);
	mania[7][4]->setValue((int)KEY_J);
	mania[7][5]->setValue((int)KEY_K);
	mania[7][6]->setValue((int)KEY_L);
	mania[7][7]->setValue((int)KEY_N); // TODO

	mania[8][0]->setValue((int)KEY_A);
	mania[8][1]->setValue((int)KEY_S);
	mania[8][2]->setValue((int)KEY_D);
	mania[8][3]->setValue((int)KEY_F);
	mania[8][4]->setValue((int)KEY_SPACE);
	mania[8][5]->setValue((int)KEY_J);
	mania[8][6]->setValue((int)KEY_K);
	mania[8][7]->setValue((int)KEY_L);
	mania[8][8]->setValue((int)KEY_N); // TODO

	mania[9][0]->setValue((int)KEY_A);
	mania[9][1]->setValue((int)KEY_S);
	mania[9][2]->setValue((int)KEY_D);
	mania[9][3]->setValue((int)KEY_F);
	mania[9][4]->setValue((int)KEY_SPACE);
	mania[9][5]->setValue((int)KEY_N); // TODO
	mania[9][6]->setValue((int)KEY_O);
	mania[9][7]->setValue((int)KEY_P);
	mania[9][8]->setValue((int)KEY_N); // TODO
	mania[9][9]->setValue((int)KEY_N); // TODO
}
