//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		textbox + scrollview command suggestion list
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef CONSOLEBOX_H
#define CONSOLEBOX_H

#include "WindowUIElement.h"

#include <atomic>
#include <mutex>

class CBaseUITextbox;
class CBaseUIButton;
class CBaseUIScrollView;
class CBaseUIBoxShadow;

class ConsoleBoxTextbox;

class ConsoleBox : public WindowUIElement
{
public:
	ConsoleBox();
	virtual ~ConsoleBox();

	void draw(Graphics *g) override;
	void drawLogOverlay(Graphics *g);
	void update() override;

	void onKeyDown(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	void onResolutionChange(Vector2 newResolution);

	void processCommand(UString command);
	void execConfigFile(UString filename);

	void log(UString text, Color textColor = 0xffffffff);

	// set
	void setRequireShiftToActivate(bool requireShiftToActivate) { m_bRequireShiftToActivate = requireShiftToActivate; }

	// get
	bool isBusy() override;
	bool isActive() override;

	// ILLEGAL:
	[[nodiscard]] inline ConsoleBoxTextbox *getTextbox() const { return m_textbox; }

	// inspection
	CBASE_UI_TYPE(ConsoleBox, CONSOLEBOX, WindowUIElement)
private:
	struct LOG_ENTRY
	{
		UString text;
		Color textColor;
	};

private:
	void onSuggestionClicked(CBaseUIButton *suggestion);

	void addSuggestion(const UString &text, const UString &helpText, const UString &command);
	void clearSuggestions();

	void show();
	void toggle(KeyboardEvent &e);

	float getAnimTargetY();

	float getDPIScale();

	// handle pending animation operations from logging thread
	void processPendingLogAnimations();

	int m_iSuggestionCount;
	int m_iSelectedSuggestion; // for up/down buttons

	ConsoleBoxTextbox *m_textbox;
	CBaseUIScrollView *m_suggestion;
	std::vector<CBaseUIButton *> m_vSuggestionButtons;
	float m_fSuggestionY;

	bool m_bRequireShiftToActivate;
	bool m_bConsoleAnimateOnce;
	float m_fConsoleDelay;
	float m_fConsoleAnimation;
	bool m_bConsoleAnimateIn;
	bool m_bConsoleAnimateOut;

	bool m_bSuggestionAnimateIn;
	bool m_bSuggestionAnimateOut;
	float m_fSuggestionAnimation;

	float m_fLogTime;
	float m_fLogYPos;
	std::vector<LOG_ENTRY> m_log;
	McFont *m_logFont;

	std::vector<UString> m_commandHistory;
	int m_iSelectedHistory;

	std::mutex m_logMutex;

	// thread-safe log animation state
	std::atomic<bool> m_bLogAnimationResetPending;
	std::atomic<float> m_fPendingLogTime;
};

#endif
