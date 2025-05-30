//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		base class for all gameplay objects
//
// $NoKeywords: $hitobj
//===============================================================================//

#pragma once
#ifndef OSUHITOBJECT_H
#define OSUHITOBJECT_H

#include "OsuBeatmap.h"

class ConVar;

class OsuModFPoSu;
class OsuBeatmapStandard;

class OsuCircle;
class OsuSlider;
class OsuSpinner;
class OsuManiaNote;

class OsuHitObject
{
public:
	static void drawHitResult(const OsuBeatmapStandard *beatmap, Vector2 rawPos, OsuScore::HIT result, float animPercentInv, float hitDeltaRangePercent);
	static void draw3DHitResult(const OsuBeatmapStandard *beatmap, Vector2 rawPos, OsuScore::HIT result, float animPercentInv, float hitDeltaRangePercent);
	static void drawHitResult(const OsuSkin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos, OsuScore::HIT result, float animPercentInv, float hitDeltaRangePercent);
	static void draw3DHitResult(const OsuModFPoSu *fposu, const OsuSkin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos, OsuScore::HIT result, float animPercentInv, float hitDeltaRangePercent);

	
	
	

	

	
	
	

public:
	enum Type : uint8_t { CIRCLE, SLIDER, SPINNER, NOTE };

	OsuHitObject(long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, OsuBeatmap *beatmap);
	virtual ~OsuHitObject() {;}

	virtual void draw() {;}
	virtual void draw2();
	virtual void draw3D() {;}
	virtual void draw3D2();
	virtual void update(long curPos);

	virtual void updateStackPosition(float stackOffset) = 0;
	virtual void miss(long curPos) = 0; // only used by notelock

	[[nodiscard]] inline OsuBeatmap *getBeatmap() const {return m_beatmap;}

	[[nodiscard]] virtual constexpr forceinline int getCombo() const {return 1;} // how much combo this hitobject is "worth"

	[[nodiscard]] virtual Type getType() const = 0;

	virtual OsuCircle* asCircle() { return nullptr; }
    virtual OsuSlider* asSlider() { return nullptr; }
    virtual OsuSpinner* asSpinner() { return nullptr; }
	virtual OsuManiaNote* asNote() { return nullptr; }
	[[nodiscard]] const virtual OsuCircle* asCircle() const { return nullptr; }
    [[nodiscard]] const virtual OsuSlider* asSlider() const { return nullptr; }
    [[nodiscard]] const virtual OsuSpinner* asSpinner() const { return nullptr; }
	[[nodiscard]] const virtual OsuManiaNote* asNote() const { return nullptr; }

	void addHitResult(OsuScore::HIT result, long delta, bool isEndOfCombo, Vector2 posRaw, float targetDelta = 0.0f, float targetAngle = 0.0f, bool ignoreOnHitErrorBar = false, bool ignoreCombo = false, bool ignoreHealth = false, bool addObjectDurationToSkinAnimationTimeStartOffset = true);
	void misAimed() {m_bMisAim = true;}

	void setIsEndOfCombo(bool isEndOfCombo) {m_bIsEndOfCombo = isEndOfCombo;}
	void setStack(int stack) {m_iStack = stack;}
	void setForceDrawApproachCircle(bool firstNote) {m_bOverrideHDApproachCircle = firstNote;}
	void setAutopilotDelta(long delta) {m_iAutopilotDelta = delta;}
	void setBlocked(bool blocked) {m_bBlocked = blocked;}
	void setComboNumber(int comboNumber) {m_iComboNumber = comboNumber;}

	[[nodiscard]] virtual Vector2 getRawPosAt(long pos) const = 0; // with stack calculation modifications
	[[nodiscard]] virtual Vector2 getOriginalRawPosAt(long pos) const = 0; // without stack calculations
	[[nodiscard]] virtual Vector2 getAutoCursorPos(long curPos) const = 0;

	[[nodiscard]] inline long getTime() const {return m_iTime;}
	[[nodiscard]] inline long getDuration() const {return m_iObjectDuration;}
	[[nodiscard]] inline int getStack() const {return m_iStack;}
	[[nodiscard]] inline int getComboNumber() const {return m_iComboNumber;}
	[[nodiscard]] inline bool isEndOfCombo() const {return m_bIsEndOfCombo;}
	[[nodiscard]] inline int getColorCounter() const {return m_iColorCounter;}
	[[nodiscard]] inline int getColorOffset() const {return m_iColorOffset;}
	[[nodiscard]] inline float getApproachScale() const {return m_fApproachScale;}
	[[nodiscard]] inline long getDelta() const {return m_iDelta;}
	[[nodiscard]] inline long getApproachTime() const {return m_iApproachTime;}
	[[nodiscard]] inline long getAutopilotDelta() const {return m_iAutopilotDelta;}
	[[nodiscard]] inline unsigned long long getSortHack() const {return m_iSortHack;}

	[[nodiscard]] inline bool isVisible() const {return m_bVisible;}
	[[nodiscard]] inline bool isFinished() const {return m_bFinished;}
	[[nodiscard]] inline bool isBlocked() const {return m_bBlocked;}
	[[nodiscard]] inline bool hasMisAimed() const {return m_bMisAim;}

	virtual void onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks) {;}
	virtual void onKeyUpEvent(std::vector<OsuBeatmap::CLICK> &keyUps) {;}
	virtual void onReset(long curPos);

protected:
	OsuBeatmap *m_beatmap;

	bool m_bVisible;
	bool m_bFinished;

	long m_iTime; // the time at which this object must be clicked
	int m_iSampleType;
	int m_iComboNumber;
	bool m_bIsEndOfCombo;
	int m_iColorCounter;
	int m_iColorOffset;

	float m_fAlpha;
	float m_fAlphaWithoutHidden;
	float m_fAlphaForApproachCircle;
	float m_fApproachScale;
	float m_fHittableDimRGBColorMultiplierPercent;
	long m_iDelta; // this must be signed
	long m_iApproachTime;
	long m_iFadeInTime;		// extra time added before the approachTime to let the object smoothly become visible
	long m_iObjectDuration; // how long this object takes to click (circle = 0, slider = sliderTime, spinner = spinnerTime etc.), the object will stay visible this long extra after m_iTime;
							// it should be set by the actual object inheriting from this class

	int m_iStack;

	bool m_bBlocked;
	bool m_bOverrideHDApproachCircle;
	bool m_bMisAim;
	long m_iAutopilotDelta;
	bool m_bUseFadeInTimeAsApproachTime;

private:
	static unsigned long long sortHackCounter;

	static float lerp3f(float a, float b, float c, float percent);

	struct HITRESULTANIM
	{
		float time;
		Vector2 rawPos;
		OsuScore::HIT result;
		long delta;
		bool addObjectDurationToSkinAnimationTimeStartOffset;
	};

	void drawHitResultAnim(const HITRESULTANIM &hitresultanim);
	void draw3DHitResultAnim(const HITRESULTANIM &hitresultanim);

	HITRESULTANIM m_hitresultanim1;
	HITRESULTANIM m_hitresultanim2;

	unsigned long long m_iSortHack;
};

#endif
