//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		analog to OsuUISongBrowserInfoLabel, but for the ranking screen
//
// $NoKeywords: $osursil
//===============================================================================//

#pragma once
#ifndef OSUUIRANKINGSCREENINFOLABEL_H
#define OSUUIRANKINGSCREENINFOLABEL_H

#include <utility>

#include "OsuUIElement.h"

class McFont;

class Osu;
class OsuBeatmap;
class OsuDatabaseBeatmap;

class OsuUIRankingScreenInfoLabel final : public OsuUIElement
{
public:
	OsuUIRankingScreenInfoLabel(float xPos, float yPos, float xSize, float ySize, UString name);

	void draw() override;

	void setFromBeatmap(OsuBeatmap *beatmap, OsuDatabaseBeatmap *diff2);

	void setArtist(UString artist) {m_sArtist = std::move(artist);}
	void setTitle(UString title) {m_sTitle = std::move(title);}
	void setDiff(UString diff) {m_sDiff = std::move(diff);}
	void setMapper(UString mapper) {m_sMapper = std::move(mapper);}
	void setPlayer(UString player) {m_sPlayer = std::move(player);}
	void setDate(UString date) {m_sDate = std::move(date);}

	float getMinimumWidth();
	float getMinimumHeight();

	// inspection
	CBASE_UI_TYPE(OsuUIRankingScreenInfoLabel, RANKINGSCREENINFOLABEL, OsuUIElement)
private:

	UString buildTitleString();
	UString buildSubTitleString();
	UString buildPlayerString();

	McFont *m_font;

	int m_iMargin;
	float m_fSubTitleScale;

	UString m_sArtist;
	UString m_sTitle;
	UString m_sDiff;
	UString m_sMapper;
	UString m_sPlayer;
	UString m_sDate;
};

#endif
