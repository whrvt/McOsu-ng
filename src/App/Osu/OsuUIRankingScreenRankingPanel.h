//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score + results panel (300, 100, 50, miss, combo, accuracy, score)
//
// $NoKeywords: $osursp
//===============================================================================//

#pragma once
#ifndef OSUUIRANKINGSCREENRANKINGPANEL_H
#define OSUUIRANKINGSCREENRANKINGPANEL_H

#include "CBaseUIImage.h"

#include "OsuDatabase.h"

class Osu;
class OsuScore;
class OsuSkinImage;

class OsuUIRankingScreenRankingPanel : public CBaseUIImage
{
public:
	OsuUIRankingScreenRankingPanel();

	void draw() override;

	void setScore(OsuScore *score);
	void setScore(const OsuDatabase::Score& score);

private:
	void drawHitImage(OsuSkinImage *img, float scale, Vector2 pos);
	void drawNumHits(int numHits, float scale, Vector2 pos);

	unsigned long long m_iScore;
	int m_iNum300s;
	int m_iNum300gs;
	int m_iNum100s;
	int m_iNum100ks;
	int m_iNum50s;
	int m_iNumMisses;
	int m_iCombo;
	float m_fAccuracy;
	bool m_bPerfect;
};

#endif
