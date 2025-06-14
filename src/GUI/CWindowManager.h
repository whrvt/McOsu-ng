//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		handles multiple window interactions
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef CWINDOWMANAGER_H
#define CWINDOWMANAGER_H

#include "cbase.h"
#include "KeyboardListener.h"

class CBaseUIWindow;

class CWindowManager : public KeyboardListener
{
public:
	CWindowManager();
	~CWindowManager() override;

	void draw();
	void update();

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	void onResolutionChange(Vector2 newResolution);

	void openAll();
	void closeAll();

	void addWindow(CBaseUIWindow *window);

	void setVisible(bool visible) {m_bVisible = visible;}
	void setEnabled(bool enabled);
	void setFocus(CBaseUIWindow *window);

	bool isMouseInside();
	bool isVisible();
	bool isActive();

	std::vector<CBaseUIWindow*> *getAllWindowsPointer() {return &m_windows;}

private:
	int getTopMouseWindowIndex();

	bool m_bVisible;
	bool m_bEnabled;

	int m_iLastEnabledWindow;
	int m_iCurrentEnabledWindow;

	std::vector<CBaseUIWindow*> m_windows;
};

#endif
