//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		slider used for the volume overlay HUD
//
// $NoKeywords: $osuvolsl
//===============================================================================//

#pragma once
#ifndef OSUUIVOLUMESLIDER_H
#define OSUUIVOLUMESLIDER_H

#include "CBaseUISlider.h"

class Osu;

class McFont;

class OsuUIVolumeSlider : public CBaseUISlider
{
public:
	enum class TYPE : uint8_t
	{
		MASTER,
		MUSIC,
		EFFECTS
	};

public:
	OsuUIVolumeSlider(float xPos, float yPos, float xSize, float ySize, UString name);

	void setType(TYPE type) {m_type = type;}
	void setSelected(bool selected);

	bool checkWentMouseInside();

	float getMinimumExtraTextWidth();

	[[nodiscard]] inline bool isSelected() const {return m_bSelected;}

private:
	virtual void drawBlock();

	virtual void onMouseInside();

	TYPE m_type;
	bool m_bSelected;

	bool m_bWentMouseInside;
	float m_fSelectionAnim;

	McFont *m_font;
};

#endif
