//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		score handler (calculation, storage, hud)
//
// $NoKeywords: $osu
//===============================================================================//

#pragma once
#ifndef OSUSCORE_H
#define OSUSCORE_H

#include "cbase.h"

class ConVar;

class Osu;
class OsuBeatmap;
class OsuHitObject;

class OsuScore
{
public:
	static constexpr const int VERSION = 20241007;

	enum class HIT : uint8_t
	{
		// score
		HIT_NULL,
		HIT_MISS,
		HIT_50,
		HIT_100,
		HIT_300,

		// only used for health + SS/PF mods
		HIT_MISS_SLIDERBREAK,
		HIT_MU,
		HIT_100K,
		HIT_300K,
		HIT_300G,
		HIT_SLIDER10,	// tick
		HIT_SLIDER30,	// repeat
		HIT_SPINNERSPIN,
		HIT_SPINNERBONUS
	};

	enum class GRADE : uint8_t
	{
		GRADE_XH,
		GRADE_SH,
		GRADE_X,
		GRADE_S,
		GRADE_A,
		GRADE_B,
		GRADE_C,
		GRADE_D,
		GRADE_F,
		GRADE_N
	};

	static float calculateAccuracy(int num300s, int num100s, int num50s, int numMisses);
	static GRADE calculateGrade(int num300s, int num100s, int num50s, int numMisses, bool modHidden, bool modFlashlight);

public:
	OsuScore();

	void reset(); // only OsuBeatmap may call this function!

	void addHitResult(OsuBeatmap *beatmap, OsuHitObject *hitObject, OsuScore::HIT hit, long delta, bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore); // only OsuBeatmap may call this function!
	void addHitResultComboEnd(OsuScore::HIT hit);
	void addSliderBreak(); // only OsuBeatmap may call this function!
	void addPoints(int points, bool isSpinner);
	void setComboFull(int comboFull) {m_iComboFull = comboFull;}
	void setComboEndBitmask(int comboEndBitmask) {m_iComboEndBitmask = comboEndBitmask;}
	void setDead(bool dead);

	void addKeyCount(int key);

	void setStarsTomTotal(float starsTomTotal) {m_fStarsTomTotal = starsTomTotal;}
	void setStarsTomAim(float starsTomAim) {m_fStarsTomAim = starsTomAim;}
	void setStarsTomSpeed(float starsTomSpeed) {m_fStarsTomSpeed = starsTomSpeed;}
	void setPPv2(float ppv2) {m_fPPv2 = ppv2;}
	void setIndex(int index) {m_iIndex = index;}

	void setNumEZRetries(int numEZRetries) {m_iNumEZRetries = numEZRetries;}

	[[nodiscard]] inline float getStarsTomTotal() const {return m_fStarsTomTotal;}
	[[nodiscard]] inline float getStarsTomAim() const {return m_fStarsTomAim;}
	[[nodiscard]] inline float getStarsTomSpeed() const {return m_fStarsTomSpeed;}
	[[nodiscard]] inline float getPPv2() const {return m_fPPv2;}
	[[nodiscard]] inline int getIndex() const {return m_iIndex;}

	unsigned long long getScore();
	[[nodiscard]] inline GRADE getGrade() const {return m_grade;}
	[[nodiscard]] inline int getCombo() const {return m_iCombo;}
	[[nodiscard]] inline int getComboMax() const {return m_iComboMax;}
	[[nodiscard]] inline int getComboFull() const {return m_iComboFull;}
	[[nodiscard]] inline int getComboEndBitmask() const {return m_iComboEndBitmask;}
	[[nodiscard]] inline float getAccuracy() const {return m_fAccuracy;}
	[[nodiscard]] inline float getUnstableRate() const {return m_fUnstableRate;}
	[[nodiscard]] inline float getHitErrorAvgMin() const {return m_fHitErrorAvgMin;}
	[[nodiscard]] inline float getHitErrorAvgMax() const {return m_fHitErrorAvgMax;}
	[[nodiscard]] inline float getHitErrorAvgCustomMin() const {return m_fHitErrorAvgCustomMin;}
	[[nodiscard]] inline float getHitErrorAvgCustomMax() const {return m_fHitErrorAvgCustomMax;}
	[[nodiscard]] inline int getNumMisses() const {return m_iNumMisses;}
	[[nodiscard]] inline int getNumSliderBreaks() const {return m_iNumSliderBreaks;}
	[[nodiscard]] inline int getNum50s() const {return m_iNum50s;}
	[[nodiscard]] inline int getNum100s() const {return m_iNum100s;}
	[[nodiscard]] inline int getNum100ks() const {return m_iNum100ks;}
	[[nodiscard]] inline int getNum300s() const {return m_iNum300s;}
	[[nodiscard]] inline int getNum300gs() const {return m_iNum300gs;}

	[[nodiscard]] inline int getNumEZRetries() const {return m_iNumEZRetries;}

	[[nodiscard]] inline bool isDead() const {return m_bDead;}
	[[nodiscard]] inline bool hasDied() const {return m_bDied;}

	[[nodiscard]] inline bool isUnranked() const {return m_bIsUnranked;}

	static double getHealthIncrease(OsuBeatmap *beatmap, OsuScore::HIT hit);
	static double getHealthIncrease(OsuScore::HIT hit, double HP = 5.0f, double hpMultiplierNormal = 1.0f, double hpMultiplierComboEnd = 1.0f, double hpBarMaximumForNormalization = 200.0f);

	int getKeyCount(int key);
	int getModsLegacy();
	UString getModsStringForRichPresence();

private:
	
	

	void onScoreChange();

	std::vector<HIT> m_hitresults;
	std::vector<int> m_hitdeltas;

	GRADE m_grade;

	float m_fStarsTomTotal;
	float m_fStarsTomAim;
	float m_fStarsTomSpeed;
	float m_fPPv2;
	int m_iIndex;

	unsigned long long m_iScoreV1;
	unsigned long long m_iScoreV2;
	unsigned long long m_iScoreV2ComboPortion;
	unsigned long long m_iBonusPoints;
	int m_iCombo;
	int m_iComboMax;
	int m_iComboFull;
	int m_iComboEndBitmask;
	float m_fAccuracy;
	float m_fHitErrorAvgMin;
	float m_fHitErrorAvgMax;
	float m_fHitErrorAvgCustomMin;
	float m_fHitErrorAvgCustomMax;
	float m_fUnstableRate;

	int m_iNumMisses;
	int m_iNumSliderBreaks;
	int m_iNum50s;
	int m_iNum100s;
	int m_iNum100ks;
	int m_iNum300s;
	int m_iNum300gs;

	bool m_bDead;
	bool m_bDied;

	int m_iNumK1;
	int m_iNumK2;
	int m_iNumM1;
	int m_iNumM2;

	// custom
	int m_iNumEZRetries;
	bool m_bIsUnranked;
};

#endif
