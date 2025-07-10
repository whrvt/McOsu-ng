//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		top plays list for weighted pp/acc
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUUSERSTATSSCREEN_H
#define OSUUSERSTATSSCREEN_H

#include "OsuScreenBackable.h"

class ConVar;

class CBaseUIContainer;
class CBaseUIScrollView;

class OsuUIContextMenu;
class OsuUISongBrowserUserButton;
class OsuUISongBrowserScoreButton;
class OsuUIUserStatsScreenLabel;

class OsuUserStatsScreenBackgroundPPRecalculator;

class OsuUserStatsScreen : public OsuScreenBackable
{
public:
	OsuUserStatsScreen();
	~OsuUserStatsScreen() override;

	void draw() override;
	void update() override;

	void setVisible(bool visible) override;

	void onScoreContextMenu(OsuUISongBrowserScoreButton *scoreButton, int id);

private:
	void updateLayout() override;

	void onBack() override;

	void rebuildScoreButtons(const UString &playerName);

	void onUserClicked(CBaseUIButton *button);
	void onUserButtonChange(const UString& text, int id);
	void onScoreClicked(CBaseUIButton *button);
	void onMenuClicked(CBaseUIButton *button);
	void onMenuSelected(const UString& text, int id);
	void onRecalculatePPImportLegacyScoresClicked();
	void onRecalculatePPImportLegacyScoresConfirmed(const UString& text, int id);
	void onRecalculatePP(bool importLegacyScores);
	void onCopyAllScoresClicked();
	void onCopyAllScoresUserSelected(const UString& text, int id);
	void onCopyAllScoresConfirmed(const UString& text, int id);
	void onDeleteAllScoresClicked();
	void onDeleteAllScoresConfirmed(const UString& text, int id);

	;

	CBaseUIContainer *m_container;

	OsuUIContextMenu *m_contextMenu;

	OsuUIUserStatsScreenLabel *m_ppVersionInfoLabel;

	OsuUISongBrowserUserButton *m_userButton;

	CBaseUIScrollView *m_scores;
	std::vector<OsuUISongBrowserScoreButton*> m_scoreButtons;

	CBaseUIButton *m_menuButton;

	UString m_sCopyAllScoresFromUser;

	bool m_bRecalculatingPP;
	OsuUserStatsScreenBackgroundPPRecalculator *m_backgroundPPRecalculator;
};

#endif
