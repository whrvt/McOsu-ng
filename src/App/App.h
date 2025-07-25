//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		app base class (v3)
//
// $NoKeywords: $appb
//===============================================================================//

#pragma once
#ifndef APP_H
#define APP_H

#include "KeyboardListener.h"
#include "Rect.h"

class Engine;

class AppBase : public KeyboardListener
{
public:
	AppBase() { ; }
	~AppBase() override { ; }

	virtual void draw() { ; }
	virtual void update() { ; }
	virtual bool isInCriticalInteractiveSession() { return false; }

	[[nodiscard]] virtual const Vector2 &getVirtScreenSize() const {static Vector2 dummyres{1920,1080}; return dummyres;}
	[[nodiscard]] virtual McRect getVirtScreenRectWithinEngineRect() const {return {};};
	[[nodiscard]] virtual int getVirtScreenWidth() const {return 1920;}
	[[nodiscard]] virtual int getVirtScreenHeight() const {return 1080;}

	void onKeyDown(KeyboardEvent & /*ev*/) override { ; }
	void onKeyUp(KeyboardEvent & /*ev*/) override { ; }
	void onChar(KeyboardEvent & /*ev*/) override { ; }

	virtual void onResolutionChanged(Vector2 /*newResolution*/) { ; }
	virtual void onDPIChanged() { ; }

	virtual void onFocusGained() { ; }
	virtual void onFocusLost() { ; }

	virtual void onMinimized() { ; }
	virtual void onRestored() { ; }

	virtual bool onShutdown() { return true; }
};

#include "Osu.h" // do "using App = Osu;"

#endif
