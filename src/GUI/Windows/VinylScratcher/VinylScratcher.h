//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		a music player with style
//
// $NoKeywords: $vs
//===============================================================================//

#pragma once
#ifndef VINYLSCRATCHER_H
#define VINYLSCRATCHER_H

#include "WindowUIElement.h"

class Sound;
class ConVar;

class CBaseUISlider;

class VSTitleBar;
class VSControlBar;
class VSMusicBrowser;

class VinylScratcher : public CBaseUIWindow
{
public:
	static bool tryPlayFile(UString filepath);

public:
	VinylScratcher();
	~VinylScratcher() override {;}

	void update() override;

	void onKeyDown(KeyboardEvent &e) override;

	// inspection
	CBASE_UI_TYPE(VinylScratcher, WindowUIElement::VINYLSCRATCHERWINDOW, CBaseUIWindow)
protected:
	void onResized() override;

private:
	void onFinished();
	void onFileClicked(UString filepath, bool reverse);
	void onVolumeChanged(CBaseUISlider *slider);
	void onSeek();
	void onPlayClicked();
	void onNextClicked();
	void onPrevClicked();

	static Sound *m_stream2;

	VSTitleBar *m_titleBar;
	VSControlBar *m_controlBar;
	VSMusicBrowser *m_musicBrowser;

	Sound *m_stream;
	float m_fReverseMessageTimer;

	;
};

#endif
