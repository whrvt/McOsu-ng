//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		3D flip bar used for music scrolling/searching/play history
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef VSTITLEBAR_H
#define VSTITLEBAR_H

#include "WindowUIElement.h"

class McFont;

class CBaseUIContainer;
class CBaseUIButton;

class VSTitleBar : public WindowUIElement
{
public:
	using SeekCallback = SA::delegate<void()>;

public:
	VSTitleBar(int x, int y, int xSize, McFont *font);
	~VSTitleBar() override;

	void draw() override;
	void update() override;

	void setSeekCallback(const SeekCallback& callback) {m_seekCallback = callback;}
	void setTitle(const UString& title, bool reverse = false);

	[[nodiscard]] inline bool isSeeking() const {return m_bIsSeeking;}

	// inspection
	CBASE_UI_TYPE(VSTitleBar, VSTITLEBAR, WindowUIElement)
protected:
	void onResized() override;
	void onMoved() override;
	void onFocusStolen() override;

private:
	void drawTitle1();
	void drawTitle2();

	SeekCallback m_seekCallback;

	McFont *m_font;

	CBaseUIContainer *m_container;

	CBaseUIButton *m_title;
	CBaseUIButton *m_title2;

	float m_fRot;

	int m_iFlip;

	bool m_bIsSeeking;
};

#endif
