//================ Copyright (c) 2019, PG & Francesco149, All rights reserved. =================//
//
// Purpose:		star rating + pp calculation, based on https://github.com/Francesco149/oppai/
//
// $NoKeywords: $tomstarspp
//==============================================================================================//

#pragma once
#ifndef OSUDIFFICULTYCALCULATOR_H
#define OSUDIFFICULTYCALCULATOR_H

#include "cbase.h"

class Osu;
class OsuBeatmap;

class OsuSliderCurve;

class ConVar;



class OsuDifficultyHitObject
{
public:
	enum class TYPE : char
	{
		INVALID = 0,
		CIRCLE,
		SPINNER,
		SLIDER,
	};

	struct SLIDER_SCORING_TIME
	{
		enum class TYPE : uint8_t
		{
			TICK,
			REPEAT,
			END,
		};

		TYPE type;
		float time;

		unsigned long long sortHack;
	};

	struct SliderScoringTimeComparator
	{
		bool operator() (const SLIDER_SCORING_TIME &a, const SLIDER_SCORING_TIME &b) const
		{
			// strict weak ordering!
			if (a.time == b.time)
				return a.sortHack < b.sortHack;
			else
				return a.time < b.time;
		}
	};

public:
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time); // circle
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime); // spinner
	OsuDifficultyHitObject(TYPE type, Vector2 pos, long time, long endTime, float spanDuration, char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength, std::vector<SLIDER_SCORING_TIME> scoringTimes, int repeats, bool calculateSliderCurveInConstructor); // slider
	~OsuDifficultyHitObject();

	OsuDifficultyHitObject(const OsuDifficultyHitObject&) = delete;
	OsuDifficultyHitObject(OsuDifficultyHitObject &&dobj) noexcept;

	OsuDifficultyHitObject& operator = (OsuDifficultyHitObject &&dobj) noexcept;

	void updateStackPosition(float stackOffset);
	void updateCurveStackPosition(float stackOffset);

	[[nodiscard]] Vector2 getOriginalRawPosAt(long pos) const; // for stacking calculations, always returns the unstacked original position at that point in time
	[[nodiscard]] float getT(long pos, bool raw) const;

	[[nodiscard]] inline long getDuration() const {return endTime - time;}

public:
	// circles (base)
	TYPE type;
	Vector2 pos;
	long time;

	// spinners + sliders
	long endTime;

	// sliders
	float spanDuration; // i.e. sliderTimeWithoutRepeats
	char osuSliderCurveType;
	float pixelLength;
	std::vector<SLIDER_SCORING_TIME> scoringTimes;
	int repeats;

	// custom
	OsuSliderCurve *curve;
	bool scheduledCurveAlloc;
	std::vector<Vector2> scheduledCurveAllocControlPoints;
	float scheduledCurveAllocStackOffset;

	int stack;
	Vector2 originalPos;

	unsigned long long sortHack;

private:
	static unsigned long long sortHackCounter;
};



class OsuDifficultyCalculator
{
public:
	static constexpr const int PP_ALGORITHM_VERSION = 20250306;

public:
	class Skills
	{
	public:
		static constexpr const int NUM_SKILLS = 3;

	public:
		enum class Skill : uint8_t
		{
			SPEED,
			AIM_SLIDERS,
			AIM_NO_SLIDERS,
		};

		static int skillToIndex(const Skill skill)
		{
			switch (skill)
			{
			case Skill::SPEED:
				return 0;
			case Skill::AIM_SLIDERS:
				return 1;
			case Skill::AIM_NO_SLIDERS:
				return 2;
			}

			return 0;
		}
	};

	// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
	// see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Aim.cs

	static constexpr const double decay_base[Skills::NUM_SKILLS] = {0.3, 0.15, 0.15};			// how much strains decay per interval (if the previous interval's peak strains after applying decay are still higher than the current one's, they will be used as the peak strains).
	static constexpr const double weight_scaling[Skills::NUM_SKILLS] = {1.46, 25.6, 25.6};		// used to keep speed and aim balanced between eachother

	static constexpr const double DIFFCALC_EPSILON = 1e-32;

	struct IncrementalState
	{
		double interval_end;
		double max_strain;
		double max_object_strain;
		double relevant_note_sum; // speed only
		double consistent_top_strain;
		double difficult_strains;
		double max_slider_strain;
		double difficult_sliders;
		std::vector<double> highest_strains;
		std::vector<double> slider_strains;
	};

	class DiffObject
	{
	public:
		OsuDifficultyHitObject *ho;

		double strains[Skills::NUM_SKILLS];

		// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
		// needed because raw speed strain and rhythm strain are combined in different ways
		double raw_speed_strain;
		double rhythm;

		Vector2 norm_start;		// start position normalized on radius

		double angle;			// precalc

		double jumpDistance;	// precalc
		double minJumpDistance;	// precalc
		double minJumpTime;		// precalc
		double travelDistance;	// precalc

		double delta_time;		// strain temp
		double strain_time;		// strain temp

		bool lazyCalcFinished;	// precalc temp
		Vector2 lazyEndPos;		// precalc temp
		double lazyTravelDist;	// precalc temp
		double lazyTravelTime;	// precalc temp
		double travelTime;		// precalc temp

		const std::vector<DiffObject> &objects;	// NOTE: McOsu stores the first object in this array while lazer doesn't. newer lazer algorithms require referencing objects "randomly", so we just keep the entire vector around.
		int prevObjectIndex;					// WARNING: this will be -1 for the first object (as the name implies), see note above

		DiffObject(OsuDifficultyHitObject *base_object, float radius_scaling_factor, std::vector<DiffObject> &diff_objects, int prevObjectIdx);

		[[nodiscard]] inline const DiffObject *get_previous(int backwardsIdx) const {return (objects.size() > 0 && prevObjectIndex - backwardsIdx < (int)objects.size() ? &objects[std::max(0, prevObjectIndex - backwardsIdx)] : NULL);}
		[[nodiscard]] inline double get_strain(Skills::Skill type) const {return strains[Skills::skillToIndex(type)] * (type == Skills::Skill::SPEED ? rhythm : 1.0);}
		[[nodiscard]] inline double get_slider_aim_strain() const {return ho->type == OsuDifficultyHitObject::TYPE::SLIDER ? strains[Skills::skillToIndex(Skills::Skill::AIM_SLIDERS)] : -1.0;}
		inline static double applyDiminishingExp(double val) {return std::pow(val, 0.99);}
		inline static double strainDecay(Skills::Skill type, double ms) {return std::pow(decay_base[Skills::skillToIndex(type)], ms / 1000.0);}

		void calculate_strains(const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf);
		void calculate_strain(const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf, const Skills::Skill dtype);
		static double calculate_difficulty(const Skills::Skill type, const DiffObject *dobjects, size_t dobjectCount, IncrementalState *incremental, std::vector<double> *outStrains = NULL, double *outDifficultStrains = NULL, double *outSkillSpecificAttrib = NULL);
		static double spacing_weight1(const double distance, const Skills::Skill diff_type);
		double spacing_weight2(const Skills::Skill diff_type, const DiffObject &prev, const DiffObject *next, double hitWindow300, bool autopilotNerf);
		double get_doubletapness(const DiffObject *next, double hitWindow300) const;
	};

public:
	// stars, fully static
	static double calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float OD, float speedMultiplier, bool relax, bool autopilot, bool touchDevice, double *aim, double *aimSliderFactor, double *aimDifficultSliders, double *difficultAimStrains, double *speed, double *speedNotes, double *difficultSpeedStrains, int upToObjectIndex = -1, std::vector<double> *outAimStrains = NULL, std::vector<double> *outSpeedStrains = NULL);
	static double calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float OD, float speedMultiplier, bool relax, bool autopilot, bool touchDevice, double *aim, double *aimSliderFactor, double *aimDifficultSliders, double *difficultAimStrains, double *speed, double *speedNotes, double *difficultSpeedStrains, int upToObjectIndex, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead);
	static double calculateStarDiffForHitObjectsInt(std::vector<DiffObject> &cachedDiffObjects, std::vector<OsuDifficultyHitObject> &sortedHitObjects, float CS, float OD, float speedMultiplier, bool relax, bool autopilot, bool touchDevice, double *aim, double *aimSliderFactor, double *aimDifficultSliders, double *difficultAimStrains, double *speed, double *speedNotes, double *difficultSpeedStrains, int upToObjectIndex, IncrementalState *incremental, std::vector<double> *outAimStrains, std::vector<double> *outSpeedStrains, const std::atomic<bool> &dead);

	// pp, use runtime mods (convenience)
	static double calculatePPv2(OsuBeatmap *beatmap, double aim, double aimSliderFactor, double aimDifficultSliders, double difficultAimStrains, double speed, double speedNotes, double difficultSpeedStrains, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo = -1, int misses = 0, int c300 = -1, int c100 = 0, int c50 = 0);

	// pp, fully static
	static double calculatePPv2(int modsLegacy, double timescale, double ar, double od, double aim, double aimSliderFactor, double aimDifficultSliders, double difficultAimStrains, double speed, double speedNotes, double difficultSpeedStrains, int numHitObjects, int numCircles, int numSliders, int numSpinners, int maxPossibleCombo, int combo, int misses, int c300, int c100, int c50);

	// helper functions
	static double calculateTotalStarsFromSkills(double aim, double speed);

private:
	static ConVar *m_osu_slider_scorev2_ref;
	static ConVar *m_osu_slider_end_inside_check_offset_ref;
	static ConVar *m_osu_slider_curve_max_length_ref;

private:
	struct Attributes
	{
		double AimStrain;
		double SliderFactor;
		double AimDifficultSliderCount;
		double AimDifficultStrainCount;
		double SpeedStrain;
		double SpeedNoteCount;
		double SpeedDifficultStrainCount;
		double ApproachRate;
		double OverallDifficulty;
		int SliderCount;
	};

	struct ScoreData
	{
		double accuracy;
		int modsLegacy;
		int countGreat;
		int countGood;
		int countMeh;
		int countMiss;
		int totalHits;
		int totalSuccessfulHits;
		int beatmapMaxCombo;
		int scoreMaxCombo;
		int amountHitObjectsWithAccuracy;
	};

	struct RhythmIsland
	{
		// NOTE: lazer stores "deltaDifferenceEpsilon" (hitWindow300 * 0.3) in this struct, but OD is constant here
		int delta;
		int deltaCount;

		inline bool equals(RhythmIsland &other, double deltaDifferenceEpsilon) const {return std::abs(delta - other.delta) < deltaDifferenceEpsilon && deltaCount == other.deltaCount;}
	};

private:
	static double computeAimValue(const ScoreData &score, const Attributes &attributes, double effectiveMissCount);
	static double computeSpeedValue(const ScoreData &score, const Attributes &attributes, double effectiveMissCount, double speedDeviation);
	static double computeAccuracyValue(const ScoreData &score, const Attributes &attributes);

	static double calculateSpeedDeviation(const ScoreData& score, const Attributes& attributes, double timescale);
	static double calculateDeviation(const Attributes &attributes, double timescale, double relevantCountGreat, double relevantCountOk, double relevantCountMeh, double relevantCountMiss);
	static double calculateSpeedHighDeviationNerf(const Attributes &attributes, double speedDeviation);

	static double erf(double x);
	static double erfInv(double z);
	static double erfImp(double x, bool invert);
	static double erfInvImp(double p, double q, double s);

	template<size_t N>
	static forceinline double evaluatePolynomial(double z, const double (&coefficients)[N])
	{
		double sum = coefficients[N - 1];
		for (int i = N - 2; i >= 0; --i)
		{
			sum *= z;
			sum += coefficients[i];
		}
		return sum;
	}
};

#endif
