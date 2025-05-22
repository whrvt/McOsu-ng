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
	typedef fastdelegate::FastDelegate0<> SeekCallback;

public:
	VSTitleBar(int x, int y, int xSize, McFont *font);
	~VSTitleBar() override;

	void draw(Graphics *g) override;
	void update() override;

	void setSeekCallback(SeekCallback callback) {m_seekCallback = callback;}
	void setTitle(UString title, bool reverse = false);

	[[nodiscard]] inline bool isSeeking() const {return m_bIsSeeking;}

	// inspection
	CBASE_UI_TYPE(VSTitleBar, VSTITLEBAR, WindowUIElement)
protected:
	void onResized() override;
	void onMoved() override;
	void onFocusStolen() override;

private:
	void drawTitle1(Graphics *g);
	void drawTitle2(Graphics *g);

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
