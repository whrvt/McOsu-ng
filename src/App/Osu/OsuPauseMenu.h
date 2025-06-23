//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		pause menu (while playing)
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUPAUSEMENU_H
#define OSUPAUSEMENU_H

#include "OsuScreen.h"

class Osu;
class OsuSongBrowser;
class CBaseUIContainer;
class OsuUIPauseMenuButton;

class OsuPauseMenu : public OsuScreen
{
public:
	OsuPauseMenu();
	~OsuPauseMenu() override;

	void draw() override;
	void update() override;

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	void onResolutionChange(Vector2 newResolution) override;

	void setVisible(bool visible) override;

	void setContinueEnabled(bool continueEnabled);

private:
	void updateLayout();

	void onContinueClicked();
	void onRetryClicked();
	void onBackClicked();

	void onSelectionChange();

	void scheduleVisibilityChange(bool visible);

	OsuUIPauseMenuButton *addButton(std::function<Image*()> getImageFunc);

	CBaseUIContainer *m_container;
	bool m_bScheduledVisibilityChange;
	bool m_bScheduledVisibility;

	std::vector<OsuUIPauseMenuButton*> m_buttons;
	OsuUIPauseMenuButton *m_selectedButton;
	float m_fWarningArrowsAnimStartTime;
	float m_fWarningArrowsAnimAlpha;
	float m_fWarningArrowsAnimX;
	float m_fWarningArrowsAnimY;
	bool m_bInitialWarningArrowFlyIn;

	bool m_bContinueEnabled;
	bool m_bClick1Down;
	bool m_bClick2Down;

	float m_fDimAnim;
};

#endif
