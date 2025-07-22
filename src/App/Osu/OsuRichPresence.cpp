//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		generalized rich presence handler
//
// $NoKeywords: $rpt
//===============================================================================//

#include "OsuRichPresence.h"

#include "Engine.h"
#include "ConVar.h"
#include "Environment.h"
#include "SteamworksInterface.h"
#include "DiscordInterface.h"

#include "Osu.h"
#include "OsuScore.h"
#include "OsuSongBrowser2.h"
#include "OsuDatabase.h"

#include "OsuBeatmap.h"
#include "OsuDatabaseBeatmap.h"
namespace cv::osu {
ConVar rich_presence("osu_rich_presence", true, FCVAR_NONE, CFUNC(OsuRichPresence::onRichPresenceChange));
ConVar rich_presence_dynamic_windowtitle("osu_rich_presence_dynamic_windowtitle", true, FCVAR_NONE, "should the window title show the currently playing beatmap Artist - Title and [Difficulty] name");
ConVar rich_presence_show_recentplaystats("osu_rich_presence_show_recentplaystats", true, FCVAR_NONE);
ConVar rich_presence_discord_show_totalpp("osu_rich_presence_discord_show_totalpp", true, FCVAR_NONE);
}



const UString OsuRichPresence::KEY_STEAM_STATUS = "status";
const UString OsuRichPresence::KEY_DISCORD_STATUS = "state";
const UString OsuRichPresence::KEY_DISCORD_DETAILS = "details";

void OsuRichPresence::onMainMenu()
{
	setStatus("Main Menu");
}

void OsuRichPresence::onSongBrowser()
{
	setStatus("Song Selection");

	// also update window title
	if (cv::osu::rich_presence_dynamic_windowtitle.getBool())
		env->setWindowTitle(PACKAGE_NAME);
}

void OsuRichPresence::onPlayStart()
{
	UString playingInfo /*= "Playing "*/;
	playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty2()->getArtist());
	playingInfo.append(" - ");
	playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty2()->getTitle());
	playingInfo.append(" [");
	playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty2()->getDifficultyName());
	playingInfo.append("]");

	setStatus(playingInfo);

	// also update window title
	if (cv::osu::rich_presence_dynamic_windowtitle.getBool())
	{
		UString windowTitle = UString(playingInfo);
		windowTitle.insert(0, PACKAGE_NAME " - ");
		env->setWindowTitle(windowTitle);
	}
}

void OsuRichPresence::onPlayEnd(bool quit)
{
	if (!quit && cv::osu::rich_presence_show_recentplaystats.getBool())
	{
		const bool isUnranked = (osu->getModAuto() || (osu->getModAutopilot() && osu->getModRelax()));

		if (!isUnranked)
		{
			// e.g.: 230pp 900x 95.50% HDHRDT 6*

			// pp
			UString scoreInfo = UString::format("%ipp", (int)(std::round(osu->getScore()->getPPv2())));

			// max combo
			scoreInfo.append(UString::format(" %ix", osu->getScore()->getComboMax()));

			// accuracy
			scoreInfo.append(UString::format(" %.2f%%", osu->getScore()->getAccuracy()*100.0f));

			// mods
			UString mods = osu->getScore()->getModsStringForRichPresence();
			if (mods.length() > 0)
			{
				scoreInfo.append(" ");
				scoreInfo.append(mods);
			}

			// stars
			scoreInfo.append(UString::format(" %.2f*", osu->getScore()->getStarsTomTotal()));

			setStatus(scoreInfo);
		}
	}
}

void OsuRichPresence::setStatus(const UString& status, bool force)
{
	if (!cv::osu::rich_presence.getBool() && !force) return;

	// steam
	if constexpr (Env::cfg(FEAT::STEAM))
		steam->setRichPresence(KEY_STEAM_STATUS, status);

	// discord
	discord->setRichPresence("largeImageKey", "logo_512", true);
	discord->setRichPresence("smallImageKey", "logo_discord_512_blackfill", true);
	discord->setRichPresence("largeImageText", cv::osu::rich_presence_discord_show_totalpp.getBool() ? "Top = Status / Recent Play; Bottom = Total weighted pp (McOsu scores only!)" : "", true);
	discord->setRichPresence("smallImageText", cv::osu::rich_presence_discord_show_totalpp.getBool() ? "Total weighted pp only work after the database has been loaded!" : "", true);
	discord->setRichPresence(KEY_DISCORD_DETAILS, status);

	if (osu != NULL && osu->getSongBrowser() != NULL)
	{
		if (cv::osu::rich_presence_discord_show_totalpp.getBool())
		{


			const int ppRounded = (int)(std::round(osu->getSongBrowser()->getDatabase()->calculatePlayerStats(cv::name.getString()).pp));
			if (ppRounded > 0)
				discord->setRichPresence(KEY_DISCORD_STATUS, UString::format("%ipp (Mc)", ppRounded));
		}
	}
	else if (force && status.length() < 1)
		discord->setRichPresence(KEY_DISCORD_STATUS, "");
}

void OsuRichPresence::onRichPresenceChange(const UString & /*oldValue*/, const UString & /*newValue*/)
{
	if (osu)
	{
		if (!cv::osu::rich_presence.getBool())
			onRichPresenceDisable();
		else
			onRichPresenceEnable();
	}
}

void OsuRichPresence::onRichPresenceEnable()
{
	setStatus("...");
}

void OsuRichPresence::onRichPresenceDisable()
{
	setStatus("", true);
}
