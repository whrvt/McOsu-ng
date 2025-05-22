//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		used by OsuBeatmapStandard for populating the live pp star cache
//
// $NoKeywords: $osubgscache
//===============================================================================//

#pragma once
#ifndef OSUBACKGROUNDSTARCACHELOADER_H
#define OSUBACKGROUNDSTARCACHELOADER_H

#include "Resource.h"

class OsuBeatmap;

class OsuBackgroundStarCacheLoader : public Resource
{
public:
	OsuBackgroundStarCacheLoader(OsuBeatmap *beatmap);

	bool isDead() {return m_bDead.load();}
	void kill() {m_bDead = true; m_iProgress = 0;}
	void revive() {m_bDead = false; m_iProgress = 0;}

	[[nodiscard]] inline int getProgress() const {return m_iProgress.load();}

	[[nodiscard]] Type getResType() const override { return APPDEFINED; } // TODO: handle this better?
protected:
	void init() override;
	void initAsync() override;
	void destroy() override {;}

private:
	OsuBeatmap *m_beatmap;

	std::atomic<bool> m_bDead;
	std::atomic<int> m_iProgress;
};

#endif
