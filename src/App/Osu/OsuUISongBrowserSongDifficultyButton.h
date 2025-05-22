//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap difficulty button (child of OsuUISongBrowserSongButton)
//
// $NoKeywords: $osusbsdb
//===============================================================================//

#pragma once
#ifndef OSUUISONGBROWSERSONGDIFFICULTYBUTTON_H
#define OSUUISONGBROWSERSONGDIFFICULTYBUTTON_H

#include "OsuUISongBrowserSongButton.h"

class ConVar;

class OsuUISongBrowserSongDifficultyButton final : public OsuUISongBrowserSongButton
{
public:
	OsuUISongBrowserSongDifficultyButton(OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, OsuDatabaseBeatmap *diff2, OsuUISongBrowserSongButton *parentSongButton);
	~OsuUISongBrowserSongDifficultyButton() override;

	void draw(Graphics *g) override;
	void update() override;

	void updateGrade() override;

	[[nodiscard]] Color getInactiveBackgroundColor() const override;

	[[nodiscard]] inline OsuUISongBrowserSongButton *getParentSongButton() const {return m_parentSongButton;}

	[[nodiscard]] bool isIndependentDiffButton() const;

	CBASE_UI_TYPE(OsuUISongBrowserSongDifficultyButton, OsuUIElement::UISONGBROWSERDIFFICULTYBUTTON, OsuUISongBrowserSongButton)
private:
	static ConVar *m_osu_scores_enabled;
	static ConVar *m_osu_songbrowser_dynamic_star_recalc_ref;

	void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) override;

	UString buildDiffString() {return m_sDiff;}

	UString m_sDiff;

	float m_fDiffScale;
	float m_fOffsetPercentAnim;

	OsuUISongBrowserSongButton *m_parentSongButton;

	bool m_bUpdateGradeScheduled;
	bool m_bPrevOffsetPercentSelectionState;
};

#endif
