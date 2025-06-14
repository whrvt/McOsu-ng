//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		text bar overlay which can eat inputs (also used for key bindings)
//
// $NoKeywords: $osunot
//===============================================================================//

#pragma once
#ifndef OSUNOTIFICATIONOVERLAY_H
#define OSUNOTIFICATIONOVERLAY_H

#include "OsuScreen.h"
#include "KeyboardEvent.h"

class Osu;

class OsuNotificationOverlayKeyListener
{
public:
	virtual ~OsuNotificationOverlayKeyListener() {;}
	virtual void onKey(KeyboardEvent &e) = 0;
};

class OsuNotificationOverlay : public OsuScreen
{
public:
	OsuNotificationOverlay();
	~OsuNotificationOverlay() override {;}

	void draw() override;

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	void addNotification(UString text, Color textColor = 0xffffffff, bool waitForKey = false, float duration = -1.0f);
	void setDisallowWaitForKeyLeftClick(bool disallowWaitForKeyLeftClick) {m_bWaitForKeyDisallowsLeftClick = disallowWaitForKeyLeftClick;}

	void stopWaitingForKey(bool stillConsumeNextChar = false);

	void addKeyListener(OsuNotificationOverlayKeyListener *keyListener) {m_keyListener = keyListener;}

	virtual bool isVisible();

	inline bool isWaitingForKey() {return m_bWaitForKey || m_bConsumeNextChar;}

private:
	struct NOTIFICATION
	{
		UString text;
		Color textColor;

		float time;
		float alpha;
		float backgroundAnim;
		float fallAnim;
	};

	void drawNotificationText(NOTIFICATION &n);
	void drawNotificationBackground(NOTIFICATION &n);

	NOTIFICATION m_notification1;
	NOTIFICATION m_notification2;

	bool m_bWaitForKey;
	bool m_bWaitForKeyDisallowsLeftClick;
	bool m_bConsumeNextChar;
	OsuNotificationOverlayKeyListener *m_keyListener;
};

#endif
