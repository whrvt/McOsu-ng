//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		osu!mania gamemode
//
// $NoKeywords: $osumania
//===============================================================================//

#pragma once
#ifndef OSU_OSUBEATMAPMANIA_H
#define OSU_OSUBEATMAPMANIA_H

#include "OsuBeatmap.h"

class OsuBeatmapMania final : public OsuBeatmap
{
public:
	OsuBeatmapMania();

	void draw(Graphics *g) override;
	void update() override;

	void onModUpdate() override;

	void onKeyDown(KeyboardEvent &key) override;
	void onKeyUp(KeyboardEvent &key) override;

	[[nodiscard]] Type getType() const override{ return MANIA; }

	OsuBeatmapMania* asMania() override { return this; }
	[[nodiscard]] const OsuBeatmapMania* asMania() const override { return this; }

	[[nodiscard]] inline Vector2 getPlayfieldSize() const {return m_vPlayfieldSize;}
	[[nodiscard]] inline Vector2 getPlayfieldCenter() const {return m_vPlayfieldCenter;}

	[[nodiscard]] int getNumColumns() const;

private:
	int getColumnForKey(int numColumns, KeyboardEvent &key);

	void onPlayStart() override;

	Vector2 m_vPlayfieldSize;
	Vector2 m_vPlayfieldCenter;

	bool m_bColumnKeyDown[128];

	Vector2 m_vRotation;
	Vector2 m_vMouseBackup;
	float m_fZoom;
};

#endif
