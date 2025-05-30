//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		generalized rich presence handler
//
// $NoKeywords: $rpt
//===============================================================================//

#pragma once
#ifndef OSURICHPRESENCE_H
#define OSURICHPRESENCE_H

#include "cbase.h"

class ConVar;

class Osu;

class OsuRichPresence
{
public:
	static void onMainMenu();
	static void onSongBrowser();
	static void onPlayStart();
	static void onPlayEnd(bool quit);

	static void onRichPresenceChange(UString oldValue, UString newValue);

private:
	static const UString KEY_STEAM_STATUS;
	static const UString KEY_DISCORD_STATUS;
	static const UString KEY_DISCORD_DETAILS;

	

	static void setStatus(UString status, bool force = false);

	static void onRichPresenceEnable();
	static void onRichPresenceDisable();
};

#endif
