//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		circle
//
// $NoKeywords: $circle
//===============================================================================//

#pragma once
#ifndef OSUCIRCLE_H
#define OSUCIRCLE_H

#include "OsuHitObject.h"

class OsuModFPoSu;
class OsuSkinImage;

class OsuCircle final : public OsuHitObject
{
public:
	// main
	static void drawApproachCircle(OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, bool overrideHDApproachCircle = false);
	static void draw3DApproachCircle(OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, bool overrideHDApproachCircle = false);
	static void drawCircle(OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void draw3DCircle(OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawCircle(OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void draw3DCircle(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float rawHitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawCircle(OsuSkin *skin, Vector2 pos, float hitcircleDiameter, Color color, float alpha = 1.0f);
	static void drawSliderStartCircle(OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void draw3DSliderStartCircle(OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawSliderStartCircle(OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float hitcircleOverlapScale, int number, int colorCounter = 0, int colorOffset = 0, float colorRGBMultiplier = 1.0f, float approachScale = 1.0f, float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void draw3DSliderStartCircle(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float rawHitcircleDiameter, float numberScale, float hitcircleOverlapScale, int number, int colorCounter = 0, int colorOffset = 0, float colorRGBMultiplier = 1.0f, float approachScale = 1.0f, float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawSliderEndCircle(OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void draw3DSliderEndCircle(OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void drawSliderEndCircle(OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number = 0, int colorCounter = 0, int colorOffset = 0, float colorRGBMultiplier = 1.0f, float approachScale = 1.0f, float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true, bool overrideHDApproachCircle = false);
	static void draw3DSliderEndCircle(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float rawHitcircleDiameter, float numberScale, float overlapScale, int number = 0, int colorCounter = 0, int colorOffset = 0, float colorRGBMultiplier = 1.0f, float approachScale = 1.0f, float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true, bool overrideHDApproachCircle = false);

	// split helper functions
	static void drawApproachCircle(OsuSkin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle);
	static void draw3DApproachCircle(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, Color comboColor, float rawHitcircleDiameter, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle);
	static void drawHitCircleOverlay(OsuSkinImage *hitCircleOverlayImage, Vector2 pos, float circleOverlayImageScale, float alpha, float colorRGBMultiplier);
	static void draw3DHitCircleOverlay(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkinImage *hitCircleOverlayImage, Vector3 pos, float alpha, float colorRGBMultiplier);
	static void drawHitCircle(Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale, float alpha);
	static void draw3DHitCircle(const OsuModFPoSu *fposu, OsuSkin *skin, const Matrix4 &baseScale, Image *hitCircleImage, Vector3 pos, Color comboColor, float alpha);
	static void drawHitCircleNumber(OsuSkin *skin, float numberScale, float overlapScale, Vector2 pos, int number, float numberAlpha, float colorRGBMultiplier);
	static void draw3DHitCircleNumber(OsuSkin *skin, float numberScale, float overlapScale, Vector3 pos, int number, float numberAlpha, float colorRGBMultiplier);

public:
	OsuCircle(int x, int y, long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, OsuBeatmapStandard *beatmap);
	~OsuCircle() override;

	void draw() override;
	void draw2() override;
	void draw3D() override;
	void draw3D2() override;
	void update(long curPos) override;

	[[nodiscard]] constexpr Type getType() const override { return CIRCLE; }

	[[nodiscard]] OsuCircle* asCircle() override { return this; }
	[[nodiscard]] const OsuCircle* asCircle() const override { return this; }

	void updateStackPosition(float stackOffset) override;
	void miss(long curPos) override;

	[[nodiscard]] Vector2 getRawPosAt(long  /*pos*/) const override {return m_vRawPos;}
	[[nodiscard]] Vector2 getOriginalRawPosAt(long  /*pos*/) const override {return m_vOriginalRawPos;}
	[[nodiscard]] Vector2 getAutoCursorPos(long curPos) const override;

	void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks) override;
	void onReset(long curPos) override;

private:
	// necessary due to the static draw functions
	static int rainbowNumber;
	static int rainbowColorCounter;

	void onHit(OsuScore::HIT result, long delta, float targetDelta = 0.0f, float targetAngle = 0.0f);

	OsuBeatmapStandard *m_beatmap;

	Vector2 m_vRawPos;
	Vector2 m_vOriginalRawPos; // for live mod changing

	bool m_bWaiting;
	float m_fHitAnimation;
	float m_fShakeAnimation;
};

#endif
