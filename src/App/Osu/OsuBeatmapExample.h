//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		example usage of the beatmap class for a custom gamemode
//
// $NoKeywords: $osubmex
//===============================================================================//

#pragma once
#ifndef OSUBEATMAPEXAMPLE_H
#define OSUBEATMAPEXAMPLE_H

#include "OsuBeatmap.h"

class OsuBeatmapExample final : public OsuBeatmap
{
public:
	OsuBeatmapExample();

	void draw() override;
	void draw3D() override;
	void update() override;

	void onModUpdate() override;
	[[nodiscard]] bool isLoading() const override;

	[[nodiscard]] Type getType() const override { return EXAMPLE; }

	OsuBeatmapExample* asExample() override { return this; }
	[[nodiscard]] const OsuBeatmapExample* asExample() const override { return this;  }

private:
	void onBeforeLoad() override;
	void onLoad() override;
	void onPlayStart() override;
	void onBeforeStop(bool quit) override;
	void onStop(bool quit) override;
	void onPaused(bool first) override;

	bool m_bFakeExtraLoading;
	float m_fFakeExtraLoadingTime;
};

#endif
