//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		baseclass for any drawable screen state object of the game
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUSCREEN_H
#define OSUSCREEN_H

#include "cbase.h"
#include "KeyboardListener.h"

class Osu;

class OsuScreen : public KeyboardListener
{
public:
	OsuScreen();
	~OsuScreen() override {;}

	virtual void draw() {;}
	virtual void update() {;}

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override {;}
	void onChar(KeyboardEvent &e) override {;}

	virtual void onResolutionChange(Vector2 newResolution) {;}

	virtual void setVisible(bool visible) {m_bVisible = visible;}

	[[nodiscard]] inline bool isVisible() const {return m_bVisible;}

protected:
	bool m_bVisible;
};

#endif
