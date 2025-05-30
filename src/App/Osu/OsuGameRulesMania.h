//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		difficulty & plafield behaviour
//
// $NoKeywords: $osugrm
//===============================================================================//

#pragma once
#ifndef OSUGAMERULESMANIA_H
#define OSUGAMERULESMANIA_H

#include "Osu.h"
#include "OsuBeatmap.h"

#include "ConVar.h"

class OsuGameRulesMania
{
public:

	//********************//
	//	Hitobject Timing  //
	//********************//

	
	
	
	
	
	
	

	static inline float getHitWindow300r(const OsuBeatmap *beatmap)	{return cv::osu::maniarules::mania_hitwindow_300r.getFloat() + cv::osu::maniarules::mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindow300(const OsuBeatmap *beatmap)	{return cv::osu::maniarules::mania_hitwindow_300.getFloat() + cv::osu::maniarules::mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindow200(const OsuBeatmap *beatmap)	{return cv::osu::maniarules::mania_hitwindow_200.getFloat() + cv::osu::maniarules::mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindow100(const OsuBeatmap *beatmap)	{return cv::osu::maniarules::mania_hitwindow_100.getFloat() + cv::osu::maniarules::mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindow50(const OsuBeatmap *beatmap)		{return cv::osu::maniarules::mania_hitwindow_50.getFloat() + cv::osu::maniarules::mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}
	static inline float getHitWindowMiss(const OsuBeatmap *beatmap)	{return cv::osu::maniarules::mania_hitwindow_miss.getFloat() + cv::osu::maniarules::mania_hitwindow_od_add_multiplier.getFloat()*(std::min(10.0f, std::max(0.0f, 10.0f - beatmap->getOD())));}

	static OsuScore::HIT getHitResult(long delta, OsuBeatmap *beatmap)
	{
		delta = std::abs(delta);

		OsuScore::HIT result = OsuScore::HIT::HIT_NULL;

		// TODO: rainbow 300s and 200s

		if (delta <= (long)getHitWindow300(beatmap))
			result = OsuScore::HIT::HIT_300;
		else if (delta <= (long)getHitWindow100(beatmap))
			result = OsuScore::HIT::HIT_100;
		else if (delta <= (long)getHitWindow50(beatmap))
			result = OsuScore::HIT::HIT_50;
		else if (delta <= (long)getHitWindowMiss(beatmap))
			result = OsuScore::HIT::HIT_MISS;

		return result;
	}
};

#endif
