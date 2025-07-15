//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		single note
//
// $NoKeywords: $osumanian
//===============================================================================//

#pragma once
#ifndef OSU_OSUMANIANOTE_H
#define OSU_OSUMANIANOTE_H

#include "OsuHitObject.h"

class OsuBeatmapMania;

class OsuManiaNote final : public OsuHitObject
{
public:
	OsuManiaNote(int column, long sliderTime, long time, int sampleType, int comboNumber, int colorCounter, OsuBeatmapMania *beatmap);

	void draw() override;
	void update(long curPos) override;

	void updateStackPosition(float  /*stackOffset*/) override {;}
	void miss(long  /*curPos*/) override {;}

	void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks) override;
	void onKeyUpEvent(std::vector<OsuBeatmap::CLICK> &keyUps) override;
	void onReset(long curPos) override;

	[[nodiscard]] constexpr Type getType() const override { return NOTE; }

	[[nodiscard]] OsuManiaNote* asNote() override { return this; }
	[[nodiscard]] const OsuManiaNote* asNote() const override { return this; }

	[[nodiscard]] Vector2 getRawPosAt(long  /*pos*/) const override {return Vector2(0,0);}
	[[nodiscard]] Vector2 getOriginalRawPosAt(long  /*pos*/) const override {return Vector2(0,0);}
	[[nodiscard]] Vector2 getAutoCursorPos(long  /*curPos*/) const override {return Vector2(0,0);}

private:
	inline bool isHoldNote() {return m_iObjectDuration > 0;}

	void onHit(OsuScore::HIT result, long delta, bool start = false, bool ignoreOnHitErrorBar = false);

	OsuBeatmapMania *m_beatmap;

	int m_iColumn;

	bool m_bStartFinished;
	OsuScore::HIT m_startResult;
	OsuScore::HIT m_endResult;
};

#endif
