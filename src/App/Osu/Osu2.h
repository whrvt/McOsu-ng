//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		unfinished spectator client
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSU2_H
#define OSU2_H

#include "App.h"
#include "Osu.h"

class OsuBeatmap;
class OsuDatabaseBeatmap;

class Osu2 final : public App
{
public:
	Osu2();
	~Osu2() override;

	void draw(Graphics *g) override;
	void update() override;

	void onKeyDown(KeyboardEvent &key) override;
	void onKeyUp(KeyboardEvent &key) override;
	void onChar(KeyboardEvent &charCode) override;

	void onResolutionChanged(Vector2 newResolution) override;

	void onFocusGained() override;
	void onFocusLost() override;

	void onMinimized() override;
	void onRestored() override;

	bool onShutdown() override;

	[[nodiscard]] inline int getNumInstances() const {return m_instances.size();}

private:
	void onSkinChange(UString oldValue, UString newValue);

	std::vector<Osu*> m_slaves;
	std::vector<Osu*> m_instances;

	OsuDatabaseBeatmap *m_prevBeatmapDifficulty;
	bool m_bSlavesLoaded;

	// hack
	bool m_bPrevPlayingState;
};

extern Osu2 *osu2;

#endif
