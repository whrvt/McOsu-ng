//================ Copyright (c) 2015, PG & Jeffrey Han (opsu!), All rights reserved. =================//
//
// Purpose:		spinner. spin logic has been taken from opsu!, I didn't have time to rewrite it yet
//
// $NoKeywords: $spin
//=====================================================================================================//

#pragma once
#ifndef OSUSPINNER_H
#define OSUSPINNER_H

#include "OsuHitObject.h"

class OsuSpinner : public OsuHitObject
{
public:
	OsuSpinner(int x, int y, long time, int sampleType, bool isEndOfCombo, long endTime, OsuBeatmapStandard *beatmap);
	~OsuSpinner() override;

	void draw(Graphics *g) override;
	void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr) override;
	void draw3D(Graphics *g) override;
	void update(long curPos) override;

	[[nodiscard]] constexpr Type getType() const override { return SPINNER; }

	[[nodiscard]] OsuSpinner* asSpinner() override { return this; }
	[[nodiscard]] const OsuSpinner* asSpinner() const override { return this; }

	void updateStackPosition(float stackOffset) override {;}
	void miss(long curPos) override {;}

	[[nodiscard]] Vector2 getRawPosAt(long pos) const override {return m_vRawPos;}
	[[nodiscard]] Vector2 getOriginalRawPosAt(long pos) const override {return m_vOriginalRawPos;}
	[[nodiscard]] Vector2 getAutoCursorPos(long curPos) const override;

	void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks) override;
	void onReset(long curPos) override;

private:
	void onHit();
	void rotate(float rad);

	OsuBeatmapStandard *m_beatmap;

	Vector2 m_vRawPos;
	Vector2 m_vOriginalRawPos;

	bool m_bClickedOnce;
	float m_fPercent;

	float m_fDrawRot;
	float m_fRotations;
	float m_fRotationsNeeded;
	float m_fDeltaOverflow;
	float m_fSumDeltaAngle;

	int m_iMaxStoredDeltaAngles;
	float *m_storedDeltaAngles;
	int m_iDeltaAngleIndex;
	float m_fDeltaAngleOverflow;

	float m_fRPM;

	float m_fLastMouseAngle;
	float m_fLastVRCursorAngle1;
	float m_fLastVRCursorAngle2;
	float m_fRatio;
};

#endif
