//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		screen + back button
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUSCREENBACKABLE_H
#define OSUSCREENBACKABLE_H

#include "OsuScreen.h"
#include "OsuUIBackButton.h"

class Osu;

class OsuScreenBackable : public OsuScreen
{
public:
	OsuScreenBackable();
	~OsuScreenBackable();

	void draw() override;
	void update() override;

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	void onResolutionChange(Vector2 newResolution) override;

	virtual void stealFocus();

protected:
	virtual void onBack() = 0;

	virtual void updateLayout();

	OsuUIBackButton *m_backButton;
};

#endif
