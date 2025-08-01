//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		example usage of the beatmap class for a custom gamemode
//
// $NoKeywords: $osubmex
//===============================================================================//

#include "OsuBeatmapExample.h"

#include "Engine.h"
#include "ResourceManager.h"

OsuBeatmapExample::OsuBeatmapExample() : OsuBeatmap()
{
	m_bFakeExtraLoading = true;
	m_fFakeExtraLoadingTime = 0.0f;
}

void OsuBeatmapExample::draw()
{
	OsuBeatmap::draw();
	if (!canDraw()) return;

	if (m_bFakeExtraLoading)
	{
		g->pushTransform();
		g->translate(200, 200);
		g->drawString(resourceManager->getFont("FONT_DEFAULT"), UString::format("Fake Loading Time: %f", m_fFakeExtraLoadingTime - engine->getTime()));
		g->popTransform();
	}

	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	// draw all hitobjects and stuff
}

void OsuBeatmapExample::draw3D()
{
	OsuBeatmap::draw3D();
	if (!canDraw()) return;

	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	// draw all hitobjects and stuff
}

void OsuBeatmapExample::update()
{
	if (!canUpdate())
	{
		OsuBeatmap::update();
		return;
	}

	// baseclass call (does the actual hitobject updates among other things)
	OsuBeatmap::update();

	if (engine->getTime() > m_fFakeExtraLoadingTime)
	{
		m_bFakeExtraLoading = false;
	}

	if (isLoading()) return; // only continue if we have loaded everything

	// update everything else here
}

void OsuBeatmapExample::onModUpdate()
{
	debugLog("\n");
}

bool OsuBeatmapExample::isLoading() const
{
	return OsuBeatmap::isLoading() || m_bFakeExtraLoading;
}

void OsuBeatmapExample::onBeforeLoad()
{
	// called before any hitobjects are loaded from disk
	debugLog("\n");

	m_bFakeExtraLoading = false;
}

void OsuBeatmapExample::onLoad()
{
	// called after all hitobjects have been loaded + created (m_hitobjects is now filled!)

	// this allows beatmaps to load extra things, like custom skin elements (see isLoading())
	// OsuBeatmap will show the loading spinner as long as isLoading() returns true, and delay the play start
	m_bFakeExtraLoading = true;
	m_fFakeExtraLoadingTime = engine->getTime() + 5.0f;
}

void OsuBeatmapExample::onPlayStart()
{
	// called at the exact moment when the player starts playing (after the potential wait-time due to early hitobjects!)
	// (is NOT called immediately when you click on a beatmap diff)
	debugLog("\n");
}

void OsuBeatmapExample::onBeforeStop(bool  /*quit*/)
{
	// called before unloading all hitobjects, when the player stops playing
	debugLog("\n");
}

void OsuBeatmapExample::onStop(bool  /*quit*/)
{
	// called after unloading all hitobjects, when the player stops playing this beatmap and returns to the songbrowser
	debugLog("\n");
}

void OsuBeatmapExample::onPaused(bool  /*first*/)
{
	// called when the player pauses the game
	debugLog("\n");
}
