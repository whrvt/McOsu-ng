//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		slider
//
// $NoKeywords: $slider
//===============================================================================//

#pragma once
#ifndef OSUSLIDER_H
#define OSUSLIDER_H

#include "OsuHitObject.h"

class OsuSliderCurve;

class VertexArrayObject;

class OsuSlider final : public OsuHitObject
{
public:
	struct SLIDERCLICK
	{
		long time;
		bool finished;
		bool successful;
		bool sliderend;
		int type;
		int tickIndex;
	};

public:
	OsuSlider(char type, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<int> hitSounds, std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, OsuBeatmapStandard *beatmap);
	~OsuSlider() override;

	void draw() override;
	void draw2() override;
	void draw2(bool drawApproachCircle, bool drawOnlyApproachCircle);
	void draw3D() override;
	void draw3D2() override;
	void update(long curPos) override;

	[[nodiscard]] constexpr Type getType() const override { return SLIDER; }

	[[nodiscard]] OsuSlider* asSlider() override { return this; }
	[[nodiscard]] const OsuSlider* asSlider() const override { return this; }

	void updateStackPosition(float stackOffset) override;
	void miss(long curPos) override;

	[[nodiscard]] constexpr forceinline int getCombo() const override {return 2 + std::max((m_iRepeat - 1), 0) + (std::max((m_iRepeat - 1), 0)+1)*m_ticks.size();}

	[[nodiscard]] Vector2 getRawPosAt(long pos) const override;
	[[nodiscard]] Vector2 getOriginalRawPosAt(long pos) const override;
	[[nodiscard]] inline Vector2 getAutoCursorPos(long curPos) const override {return m_vCurPoint;}

	void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks) override;
	void onReset(long curPos) override;

	void rebuildVertexBuffer(bool useRawCoords = false);

	[[nodiscard]] inline bool isStartCircleFinished() const {return m_bStartFinished;}
	[[nodiscard]] inline int getRepeat() const {return m_iRepeat;}
	[[nodiscard]] inline std::vector<Vector2> getRawPoints() const {return m_points;}
	[[nodiscard]] inline float getPixelLength() const {return m_fPixelLength;}
	[[nodiscard]] inline const std::vector<SLIDERCLICK> &getClicks() const {return m_clicks;}

	// ILLEGAL:
	[[nodiscard]] inline VertexArrayObject *getVAO() const {return m_vao;}
	[[nodiscard]] inline OsuSliderCurve *getCurve() const {return m_curve;}

private:
	
	
	
	
	
	
	
	
	

	void drawStartCircle(float alpha);
	void draw3DStartCircle(const Matrix4 &baseScale, float alpha);
	void drawEndCircle(float alpha, float sliderSnake = 1.0f);
	void draw3DEndCircle(const Matrix4 &baseScale, float alpha, float sliderSnake = 1.0f);
	void drawBody(float alpha, float from, float to);

	void updateAnimations(long curPos);

	void onHit(OsuScore::HIT result, long delta, bool startOrEnd, float targetDelta = 0.0f, float targetAngle = 0.0f, bool isEndResultFromStrictTrackingMod = false);
	void onRepeatHit(bool successful, bool sliderend);
	void onTickHit(bool successful, int tickIndex);
	void onSliderBreak();

	[[nodiscard]] float getT(long pos, bool raw) const;

	bool isClickHeldSlider(); // special logic to disallow hold tapping

	OsuBeatmapStandard *m_beatmap;

	OsuSliderCurve *m_curve;

	char m_cType;
	int m_iRepeat;
	float m_fPixelLength;
	std::vector<Vector2> m_points;
	std::vector<int> m_hitSounds;
	float m_fSliderTime;
	float m_fSliderTimeWithoutRepeats;

	struct SLIDERTICK
	{
		float percent;
		bool finished;
	};
	std::vector<SLIDERTICK> m_ticks; // ticks (drawing)

	// TEMP: auto cursordance
	std::vector<SLIDERCLICK> m_clicks; // repeats (type 0) + ticks (type 1)

	float m_fSlidePercent;			// 0.0f - 1.0f - 0.0f - 1.0f - etc.
	float m_fActualSlidePercent;	// 0.0f - 1.0f
	float m_fSliderSnakePercent;
	float m_fReverseArrowAlpha;
	float m_fBodyAlpha;

	Vector2 m_vCurPoint;
	Vector2 m_vCurPointRaw;

	OsuScore::HIT m_startResult;
	OsuScore::HIT m_endResult;
	bool m_bStartFinished;
	float m_fStartHitAnimation;
	bool m_bEndFinished;
	float m_fEndHitAnimation;
	float m_fEndSliderBodyFadeAnimation;
	long m_iStrictTrackingModLastClickHeldTime;
	int m_iDownKey;
	int m_iPrevSliderSlideSoundSampleSet;
	bool m_bCursorLeft;
	bool m_bCursorInside;
	bool m_bHeldTillEnd;
	bool m_bHeldTillEndForLenienceHack;
	bool m_bHeldTillEndForLenienceHackCheck;
	float m_fFollowCircleTickAnimationScale;
	float m_fFollowCircleAnimationScale;
	float m_fFollowCircleAnimationAlpha;

	int m_iReverseArrowPos;
	int m_iCurRepeat;
	int m_iCurRepeatCounterForHitSounds;
	bool m_bInReverse;
	bool m_bHideNumberAfterFirstRepeatHit;

	float m_fSliderBreakRapeTime;

	VertexArrayObject *m_vao;
};

#endif
