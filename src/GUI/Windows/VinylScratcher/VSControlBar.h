//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		play/pause/forward/shuffle/repeat/eq/settings/volume/time bar
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef VSCONTROLBAR_H
#define VSCONTROLBAR_H

#include "WindowUIElement.h"

class McFont;

class CBaseUIContainer;
class CBaseUIButton;
class CBaseUICheckbox;
class CBaseUISlider;

class VSControlBar : public WindowUIElement
{
public:
	VSControlBar(int x, int y, int xSize, int ySize, McFont *font);
	~VSControlBar() override;

	void draw() override;
	void update() override;

	[[nodiscard]] inline CBaseUISlider *getVolumeSlider() const {return m_volume;}
	[[nodiscard]] inline CBaseUIButton *getPlayButton() const {return m_play;}
	[[nodiscard]] inline CBaseUIButton *getPrevButton() const {return m_prev;}
	[[nodiscard]] inline CBaseUIButton *getNextButton() const {return m_next;}
	[[nodiscard]] inline CBaseUIButton *getInfoButton() const {return m_info;}

	// inspection
	CBASE_UI_TYPE(VSControlBar, VSCONTROLBAR, WindowUIElement)
protected:
	void onResized() override;
	void onMoved() override;
	void onFocusStolen() override;
	void onEnabled() override;
	void onDisabled() override;

private:
	void onRepeatCheckboxChanged(CBaseUICheckbox *box);
	void onShuffleCheckboxChanged(CBaseUICheckbox *box);
	void onVolumeChanged(UString oldValue, UString newValue);

	CBaseUIContainer *m_container;

	CBaseUISlider *m_volume;
	CBaseUIButton *m_play;
	CBaseUIButton *m_prev;
	CBaseUIButton *m_next;

	CBaseUIButton *m_info;

	CBaseUIButton *m_settings;
	CBaseUICheckbox *m_shuffle;
	CBaseUICheckbox *m_eq;
	CBaseUICheckbox *m_repeat;
};

#endif
