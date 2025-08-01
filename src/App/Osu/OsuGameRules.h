//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		difficulty & playfield behaviour
//
// $NoKeywords: $osugr
//===============================================================================//

#pragma once
#ifndef OSUGAMERULES_H
#define OSUGAMERULES_H

#include "Osu.h"
#include "OsuBeatmap.h"

#include "ConVar.h"

class OsuGameRules
{
public:

	//********************//
	//  Positional Audio  //
	//********************//

	static float osuCoords2Pan(float x)
	{
		return (x / (float)OsuGameRules::OSU_COORD_WIDTH - 0.5f) * 0.8f;
	}


	//************************//
	//	Hitobject Animations  //
	//************************//

	static float getFadeOutTime(OsuBeatmap *beatmap) // this scales the fadeout duration with the current speed multiplier
	{
		return cv::osu::stdrules::hitobject_fade_out_time.getFloat() * (1.0f / std::max(beatmap->getSpeedMultiplier(), cv::osu::stdrules::hitobject_fade_out_time_speed_multiplier_min.getFloat()));
	}

	static inline long getFadeInTime() {return (long)cv::osu::stdrules::hitobject_fade_in_time.getInt();}

	//********************//
	//	Hitobject Timing  //
	//********************//


	// ignore all mods and overrides
	static inline float getRawMinApproachTime()
	{
		return cv::osu::stdrules::approachtime_min.getFloat();
	}
	static inline float getRawMidApproachTime()
	{
		return cv::osu::stdrules::approachtime_mid.getFloat();
	}
	static inline float getRawMaxApproachTime()
	{
		return cv::osu::stdrules::approachtime_max.getFloat();
	}

	// respect mods and overrides
	static inline float getMinApproachTime()
	{
		return getRawMinApproachTime() * (cv::osu::stdrules::mod_millhioref.getBool() ? cv::osu::stdrules::mod_millhioref_multiplier.getFloat() : 1.0f);
	}
	static inline float getMidApproachTime()
	{
		return getRawMidApproachTime() * (cv::osu::stdrules::mod_millhioref.getBool() ? cv::osu::stdrules::mod_millhioref_multiplier.getFloat() : 1.0f);
	}
	static inline float getMaxApproachTime()
	{
		return getRawMaxApproachTime() * (cv::osu::stdrules::mod_millhioref.getBool() ? cv::osu::stdrules::mod_millhioref_multiplier.getFloat() : 1.0f);
	}

	static inline float getMinHitWindow300() {return cv::osu::stdrules::hitwindow_300_min.getFloat();}
	static inline float getMidHitWindow300() {return cv::osu::stdrules::hitwindow_300_mid.getFloat();}
	static inline float getMaxHitWindow300() {return cv::osu::stdrules::hitwindow_300_max.getFloat();}

	static inline float getMinHitWindow100() {return cv::osu::stdrules::hitwindow_100_min.getFloat();}
	static inline float getMidHitWindow100() {return cv::osu::stdrules::hitwindow_100_mid.getFloat();}
	static inline float getMaxHitWindow100() {return cv::osu::stdrules::hitwindow_100_max.getFloat();}

	static inline float getMinHitWindow50() {return cv::osu::stdrules::hitwindow_50_min.getFloat();}
	static inline float getMidHitWindow50() {return cv::osu::stdrules::hitwindow_50_mid.getFloat();}
	static inline float getMaxHitWindow50() {return cv::osu::stdrules::hitwindow_50_max.getFloat();}

	// AR 5 -> 1200 ms
	static float mapDifficultyRange(float scaledDiff, float min, float mid, float max)
	{
	    if (scaledDiff > 5.0f)
	        return mid + (max - mid) * (scaledDiff - 5.0f) / 5.0f;

	    if (scaledDiff < 5.0f)
	        return mid - (mid - min) * (5.0f - scaledDiff) / 5.0f;

	    return mid;
	}

	static double mapDifficultyRangeDouble(double scaledDiff, double min, double mid, double max)
	{
	    if (scaledDiff > 5.0)
	        return mid + (max - mid) * (scaledDiff - 5.0) / 5.0;

	    if (scaledDiff < 5.0)
	        return mid - (mid - min) * (5.0 - scaledDiff) / 5.0;

	    return mid;
	}

	// 1200 ms -> AR 5
	static float mapDifficultyRangeInv(float val, float min, float mid, float max)
	{
		if (val < mid) // > 5.0f (inverted)
			return ((val*5.0f - mid*5.0f) / (max - mid)) + 5.0f;

		if (val > mid) // < 5.0f (inverted)
			return 5.0f - ((mid*5.0f - val*5.0f) / (mid - min));

	    return 5.0f;
	}

	// 1200 ms -> AR 5
	static float getRawApproachRateForSpeedMultiplier(float approachTime, float speedMultiplier) // ignore all mods and overrides
	{
		return mapDifficultyRangeInv(approachTime * (1.0f / speedMultiplier), getRawMinApproachTime(), getRawMidApproachTime(), getRawMaxApproachTime());
	}
	static float getApproachRateForSpeedMultiplier(const OsuBeatmap *beatmap, float speedMultiplier)
	{
		return mapDifficultyRangeInv((float)getApproachTime(beatmap) * (1.0f / speedMultiplier), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getApproachRateForSpeedMultiplier(const OsuBeatmap *beatmap) // respect all mods and overrides
	{
		return getApproachRateForSpeedMultiplier(beatmap, osu->getSpeedMultiplier());
	}
	static float getRawApproachRateForSpeedMultiplier(const OsuBeatmap *beatmap) // ignore AR override
	{
		return mapDifficultyRangeInv((float)getRawApproachTime(beatmap) * (1.0f / osu->getSpeedMultiplier()), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getConstantApproachRateForSpeedMultiplier(const OsuBeatmap *beatmap) // ignore AR override, keep AR consistent through speed changes
	{
		return mapDifficultyRangeInv((float)getRawApproachTime(beatmap) * osu->getSpeedMultiplier(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getRawConstantApproachRateForSpeedMultiplier(float approachTime, float speedMultiplier) // ignore all mods and overrides, keep AR consistent through speed changes
	{
		return mapDifficultyRangeInv(approachTime * speedMultiplier, getRawMinApproachTime(), getRawMidApproachTime(), getRawMaxApproachTime());
	}

	// 50 ms -> OD 5
	static float getRawOverallDifficultyForSpeedMultiplier(float hitWindow300, float speedMultiplier) // ignore all mods and overrides
	{
		return mapDifficultyRangeInv(hitWindow300 * (1.0f / speedMultiplier), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getOverallDifficultyForSpeedMultiplier(const OsuBeatmap *beatmap, float speedMultiplier) // respect all mods and overrides
	{
		return mapDifficultyRangeInv((float)getHitWindow300(beatmap) * (1.0f / speedMultiplier), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getOverallDifficultyForSpeedMultiplier(const OsuBeatmap *beatmap) // respect all mods and overrides
	{
		return getOverallDifficultyForSpeedMultiplier(beatmap, osu->getSpeedMultiplier());
	}
	static float getRawOverallDifficultyForSpeedMultiplier(const OsuBeatmap *beatmap) // ignore OD override
	{
		return mapDifficultyRangeInv((float)getRawHitWindow300(beatmap) * (1.0f / osu->getSpeedMultiplier()), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getConstantOverallDifficultyForSpeedMultiplier(const OsuBeatmap *beatmap) // ignore OD override, keep OD consistent through speed changes
	{
		return mapDifficultyRangeInv((float)getRawHitWindow300(beatmap) * osu->getSpeedMultiplier(), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getRawConstantOverallDifficultyForSpeedMultiplier(float hitWindow300, float speedMultiplier) // ignore all mods and overrides, keep OD consistent through speed changes
	{
		return mapDifficultyRangeInv(hitWindow300 * speedMultiplier, getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}

	static float getRawApproachTime(float AR) // ignore all mods and overrides
	{
		return mapDifficultyRange(AR, getRawMinApproachTime(), getRawMidApproachTime(), getRawMaxApproachTime());
	}
	static float getApproachTime(const OsuBeatmap *beatmap)
	{
		return cv::osu::stdrules::mod_mafham.getBool() ? beatmap->getLength()*2 : mapDifficultyRange(beatmap->getAR(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getRawApproachTime(const OsuBeatmap *beatmap) // ignore AR override
	{
		return cv::osu::stdrules::mod_mafham.getBool() ? beatmap->getLength()*2 : mapDifficultyRange(beatmap->getRawAR(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getApproachTimeForStacking(float AR)
	{
		return mapDifficultyRange(cv::osu::stdrules::stacking_ar_override.getFloat() < 0.0f ? AR : cv::osu::stdrules::stacking_ar_override.getFloat(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}
	static float getApproachTimeForStacking(const OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(cv::osu::stdrules::stacking_ar_override.getFloat() < 0.0f ? beatmap->getAR() : cv::osu::stdrules::stacking_ar_override.getFloat(), getMinApproachTime(), getMidApproachTime(), getMaxApproachTime());
	}

	static float getRawHitWindow300(float OD) // ignore all mods and overrides
	{
		return mapDifficultyRange(OD, getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getHitWindow300(const OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(beatmap->getOD(), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}
	static float getRawHitWindow300(const OsuBeatmap *beatmap) // ignore OD override
	{
		return mapDifficultyRange(beatmap->getRawOD(), getMinHitWindow300(), getMidHitWindow300(), getMaxHitWindow300());
	}

	static float getRawHitWindow100(float OD) // ignore all mods and overrides
	{
		return mapDifficultyRange(OD, getMinHitWindow100(), getMidHitWindow100(), getMaxHitWindow100());
	}
	static float getHitWindow100(const OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(beatmap->getOD(), getMinHitWindow100(), getMidHitWindow100(), getMaxHitWindow100());
	}

	static float getRawHitWindow50(float OD) // ignore all mods and overrides
	{
		return mapDifficultyRange(OD, getMinHitWindow50(), getMidHitWindow50(), getMaxHitWindow50());
	}
	static float getHitWindow50(const OsuBeatmap *beatmap)
	{
		return mapDifficultyRange(beatmap->getOD(), getMinHitWindow50(), getMidHitWindow50(), getMaxHitWindow50());
	}
	static inline float getHitWindowMiss(const OsuBeatmap * /*beatmap*/)
	{
		return cv::osu::stdrules::hitwindow_miss.getFloat(); // opsu is using this here: (500.0f - (beatmap->getOD() * 10.0f)), while osu is just using 400 absolute ms hardcoded, not sure why
	}

	static float getSpinnerSpinsPerSecond(const OsuBeatmap *beatmap) // raw spins required per second
	{
		return mapDifficultyRange(beatmap->getOD(), 3.0f, 5.0f, 7.5f);
	}
	static float getSpinnerRotationsForSpeedMultiplier(const OsuBeatmap *beatmap, long spinnerDuration, float speedMultiplier)
	{
		///return (int)((float)spinnerDuration / 1000.0f * getSpinnerSpinsPerSecond(beatmap)); // actual
		return (int)((((float)spinnerDuration / 1000.0f * getSpinnerSpinsPerSecond(beatmap)) * 0.5f) * (std::min(1.0f / speedMultiplier, 1.0f))); // Mc
	}
	static float getSpinnerRotationsForSpeedMultiplier(const OsuBeatmap *beatmap, long spinnerDuration) // spinner length compensated rotations // respect all mods and overrides
	{
		return getSpinnerRotationsForSpeedMultiplier(beatmap, spinnerDuration, osu->getSpeedMultiplier());
	}

	static OsuScore::HIT getHitResult(long delta, const OsuBeatmap *beatmap)
	{
		if (cv::osu::stdrules::mod_halfwindow.getBool() && delta > 0 && delta <= (long)getHitWindowMiss(beatmap))
		{
			if (!cv::osu::stdrules::mod_halfwindow_allow_300s.getBool())
				return OsuScore::HIT::HIT_MISS;
			else if (delta > (long)getHitWindow300(beatmap))
				return OsuScore::HIT::HIT_MISS;
		}

		delta = std::abs(delta);

		OsuScore::HIT result = OsuScore::HIT::HIT_NULL;

		if (!cv::osu::stdrules::mod_ming3012.getBool() && !cv::osu::stdrules::mod_no100s.getBool() && !cv::osu::stdrules::mod_no50s.getBool())
		{
			if (delta <= (long)getHitWindow300(beatmap))
				result = OsuScore::HIT::HIT_300;
			else if (delta <= (long)getHitWindow100(beatmap))
				result = OsuScore::HIT::HIT_100;
			else if (delta <= (long)getHitWindow50(beatmap))
				result = OsuScore::HIT::HIT_50;
			else if (delta <= (long)getHitWindowMiss(beatmap))
				result = OsuScore::HIT::HIT_MISS;
		}
		else if (cv::osu::stdrules::mod_ming3012.getBool())
		{
			if (delta <= (long)getHitWindow300(beatmap))
				result = OsuScore::HIT::HIT_300;
			else if (delta <= (long)getHitWindow50(beatmap))
				result = OsuScore::HIT::HIT_50;
			else if (delta <= (long)getHitWindowMiss(beatmap))
				result = OsuScore::HIT::HIT_MISS;
		}
		else if (cv::osu::stdrules::mod_no100s.getBool())
		{
			if (delta <= (long)getHitWindow300(beatmap))
				result = OsuScore::HIT::HIT_300;
			else if (delta <= (long)getHitWindowMiss(beatmap))
				result = OsuScore::HIT::HIT_MISS;
		}
		else if (cv::osu::stdrules::mod_no50s.getBool())
		{
			if (delta <= (long)getHitWindow300(beatmap))
				result = OsuScore::HIT::HIT_300;
			else if (delta <= (long)getHitWindow100(beatmap))
				result = OsuScore::HIT::HIT_100;
			else if (delta <= (long)getHitWindowMiss(beatmap))
				result = OsuScore::HIT::HIT_MISS;
		}

		return result;
	}



	//*********************//
	//	Hitobject Scaling  //
	//*********************//

	

	// "Builds of osu! up to 2013-05-04 had the gamefield being rounded down, which caused incorrect radius calculations
	// in widescreen cases. This ratio adjusts to allow for old replays to work post-fix, which in turn increases the lenience
	// for all plays, but by an amount so small it should only be effective in replays."
	static constexpr const float broken_gamefield_rounding_allowance = 1.00041f;

	static float getRawHitCircleScale(float CS)
	{
		return std::max(0.0f, ((1.0f - 0.7f * (CS - 5.0f) / 5.0f) / 2.0f) * broken_gamefield_rounding_allowance);
	}

	static float getRawHitCircleDiameter(float CS)
	{
		return getRawHitCircleScale(CS) * 128.0f; // gives the circle diameter in osu!pixels, goes negative above CS 12.1429
	}

	static float getHitCircleXMultiplier()
	{
		return getPlayfieldSize().x / OSU_COORD_WIDTH; // scales osu!pixels to the actual playfield size
	}

	static float getHitCircleDiameter(const OsuBeatmap *beatmap)
	{
		return getRawHitCircleDiameter(beatmap->getCS()) * getHitCircleXMultiplier();
	}



	//*************//
	//	Playfield  //
	//*************//

	
	

	static constexpr int OSU_COORD_WIDTH = 512;
	static constexpr int OSU_COORD_HEIGHT = 384;

	static float getPlayfieldScaleFactor()
	{
		const int engineScreenWidth = osu->getVirtScreenWidth();
		const int topBorderSize = cv::osu::stdrules::playfield_border_top_percent.getFloat()*osu->getVirtScreenHeight();
		const int bottomBorderSize = cv::osu::stdrules::playfield_border_bottom_percent.getFloat()*osu->getVirtScreenHeight();
		const int engineScreenHeight = osu->getVirtScreenHeight() - bottomBorderSize - topBorderSize;

		return osu->getVirtScreenWidth()/(float)OSU_COORD_WIDTH > engineScreenHeight/(float)OSU_COORD_HEIGHT ? engineScreenHeight/(float)OSU_COORD_HEIGHT : engineScreenWidth/(float)OSU_COORD_WIDTH;
	}

	static Vector2 getPlayfieldSize()
	{
		const float scaleFactor = getPlayfieldScaleFactor();

		return Vector2(OSU_COORD_WIDTH*scaleFactor, OSU_COORD_HEIGHT*scaleFactor);
	}

	static Vector2 getPlayfieldOffset()
	{
		const Vector2 playfieldSize = getPlayfieldSize();
		const int bottomBorderSize = cv::osu::stdrules::playfield_border_bottom_percent.getFloat()*osu->getVirtScreenHeight();
		int playfieldYOffset = (osu->getVirtScreenHeight()/2.0f - (playfieldSize.y/2.0f)) - bottomBorderSize;

		if (cv::osu::stdrules::mod_fps.getBool())
			playfieldYOffset = 0; // first person mode doesn't need any offsets, cursor/crosshair should be centered on screen

		return Vector2((osu->getVirtScreenWidth()-playfieldSize.x)/2.0f, (osu->getVirtScreenHeight()-playfieldSize.y)/2.0f + playfieldYOffset);
	}

	static Vector2 getPlayfieldCenter()
	{
		const float scaleFactor = getPlayfieldScaleFactor();
		const Vector2 playfieldOffset = getPlayfieldOffset();

		return Vector2((OSU_COORD_WIDTH/2)*scaleFactor + playfieldOffset.x, (OSU_COORD_HEIGHT/2)*scaleFactor + playfieldOffset.y);
	}
};

#endif
