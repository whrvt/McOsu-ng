//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		clickable button displaying score, grade, name, acc, mods, combo
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUUISONGBROWSERSCOREBUTTON_H
#define OSUUISONGBROWSERSCOREBUTTON_H

#include "CBaseUIButton.h"

#include "OsuScore.h"
#include "OsuDatabase.h"

class Osu;
class OsuSkinImage;

class OsuUIContextMenu;

class OsuUISongBrowserScoreButton : public CBaseUIButton
{
public:
	static OsuSkinImage *getGradeImage(OsuScore::GRADE grade);
	static UString getModsStringForDisplay(int mods);
	static UString getModsStringForConVar(int mods);

	enum class STYLE : uint8_t
	{
		SCORE_BROWSER,
		TOP_RANKS
	};

public:
	OsuUISongBrowserScoreButton(OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, STYLE style = STYLE::SCORE_BROWSER);
	~OsuUISongBrowserScoreButton() override;

	void draw(Graphics *g) override;
	void update() override;

	void highlight();
	void resetHighlight();

	void setScore(const OsuDatabase::Score &score, const OsuDatabaseBeatmap *diff2 = NULL, int index = 1, UString titleString = "", float weight = 1.0f);
	void setIndex(int index) {m_iScoreIndexNumber = index;}

	[[nodiscard]] inline OsuDatabase::Score getScore() const {return m_score;}
	[[nodiscard]] inline uint64_t getScoreUnixTimestamp() const {return m_score.unixTimestamp;}
	[[nodiscard]] inline unsigned long long getScoreScore() const {return m_score.score;}
	[[nodiscard]] inline float getScorePP() const {return m_score.pp;}

	[[nodiscard]] inline UString getDateTime() const {return m_sScoreDateTime;}
	[[nodiscard]] inline int getIndex() const {return m_iScoreIndexNumber;}

private:
	static ConVar *m_osu_scores_sort_by_pp_ref;
	static ConVar *m_osu_mods_ref;
	static ConVar *m_osu_speed_override_ref;
	static ConVar *m_osu_ar_override_ref;
	static ConVar *m_osu_cs_override_ref;
	static ConVar *m_osu_od_override_ref;
	static ConVar *m_osu_hp_override_ref;
	static ConVar *m_osu_ar_override_lock_ref;
	static ConVar *m_osu_od_override_lock_ref;
	static UString recentScoreIconString;

	void updateElapsedTimeString();

	void onClicked() override;

	void onMouseInside() override;
	void onMouseOutside() override;

	void onFocusStolen() override;

	void onRightMouseUpInside();
	void onContextMenu(UString text, int id = -1);
	void onUseModsClicked();
	void onDeleteScoreClicked();
	void onDeleteScoreConfirmed(UString text, int id);

	bool isContextMenuVisible();

	OsuUIContextMenu *m_contextMenu;
	STYLE m_style;
	float m_fIndexNumberAnim;
	bool m_bIsPulseAnim;

	bool m_bRightClick;
	bool m_bRightClickCheck;

	// score data
	OsuDatabase::Score m_score;

	int m_iScoreIndexNumber;
	uint64_t m_iScoreUnixTimestamp;

	OsuScore::GRADE m_scoreGrade;

	// STYLE::SCORE_BROWSER
	UString m_sScoreTime;
	UString m_sScoreUsername;
	UString m_sScoreScore;
	UString m_sScoreScorePP;
	UString m_sScoreAccuracy;
	UString m_sScoreAccuracyFC;
	UString m_sScoreMods;
	UString m_sCustom;

	// STYLE::TOP_RANKS
	UString m_sScoreTitle;
	UString m_sScoreScorePPWeightedPP;
	UString m_sScoreScorePPWeightedWeight;
	UString m_sScoreWeight;

	std::vector<UString> m_tooltipLines;
	UString m_sScoreDateTime;
};

#endif
