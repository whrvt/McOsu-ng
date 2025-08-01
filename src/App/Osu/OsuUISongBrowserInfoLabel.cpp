//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser beatmap info (top left)
//
// $NoKeywords: $osusbil
//===============================================================================//

#include "OsuUISongBrowserInfoLabel.h"

#include <utility>

#include "Engine.h"
#include "ResourceManager.h"
#include "Environment.h"
#include "ConVar.h"
#include "Mouse.h"

#include "Osu.h"
#include "OsuBeatmap.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuNotificationOverlay.h"
#include "OsuTooltipOverlay.h"
#include "OsuSongBrowser2.h"
#include "OsuGameRules.h"
#include "OsuMultiplayer.h"

#include "OsuOptionsMenu.h"

OsuUISongBrowserInfoLabel::OsuUISongBrowserInfoLabel(float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), "")
{
	
	m_font = osu->getSubTitleFont();




	m_iMargin = 10;

	const float globalScaler = 1.3f;

	m_fTitleScale = 0.77f*globalScaler;
	m_fSubTitleScale = 0.6f*globalScaler;
	m_fSongInfoScale = 0.7f*globalScaler;
	m_fDiffInfoScale = 0.65f*globalScaler;
	m_fOffsetInfoScale = 0.6f*globalScaler;

	m_sArtist = "Artist";
	m_sTitle = "Title";
	m_sDiff = "Difficulty";
	m_sMapper = "Mapper";

	m_iLengthMS = 0;
	m_iMinBPM = 0;
	m_iMaxBPM = 0;
	m_iMostCommonBPM = 0;
	m_iNumObjects = 0;

	m_fCS = 5.0f;
	m_fAR = 5.0f;
	m_fOD = 5.0f;
	m_fHP = 5.0f;
	m_fStars = 5.0f;

	m_iLocalOffset = 0;
	m_iOnlineOffset = 0;

	m_iBeatmapId = -1;
}

void OsuUISongBrowserInfoLabel::draw()
{
	// debug bounding box
	if (cv::osu::debug.getBool())
	{
		g->setColor(0xffff0000);
		g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y);
		g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x, m_vPos.y+m_vSize.y);
		g->drawLine(m_vPos.x, m_vPos.y+m_vSize.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
		g->drawLine(m_vPos.x+m_vSize.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
	}

	// build strings
	const UString titleText = (osu->getMultiplayer()->isDownloadingBeatmap() ? UString::format("DOWNLOADING: %i %%", (int)(osu->getMultiplayer()->getDownloadBeatmapPercentage() * 100.0f)) : buildTitleString());
	const UString subTitleText = buildSubTitleString();
	const UString songInfoText = buildSongInfoString();
	const UString diffInfoText = buildDiffInfoString();
	const UString offsetInfoText = buildOffsetInfoString();

	const float globalScale = std::max((m_vSize.y / getMinimumHeight())*0.91f, 1.0f);

	const int shadowOffset = std::round(1.0f * ((float)m_font->getDPI() / 96.0f)); // NOTE: abusing font dpi

	int yCounter = m_vPos.y;

	// draw title
	g->pushTransform();
	{
		const float scale = m_fTitleScale*globalScale;

		yCounter += m_font->getHeight()*scale;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x), yCounter);

		g->translate(shadowOffset, shadowOffset);
		g->setColor(0xff000000);
		g->drawString(m_font, titleText);
		g->translate(-shadowOffset, -shadowOffset);
		g->setColor(0xffffffff);
		g->drawString(m_font, titleText);
	}
	g->popTransform();

	// draw subtitle (mapped by)
	g->pushTransform();
	{
		const float scale = m_fSubTitleScale*globalScale;

		yCounter += m_font->getHeight()*scale + m_iMargin*globalScale*1.0f;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x), yCounter);

		g->translate(shadowOffset, shadowOffset);
		g->setColor(0xff000000);
		g->drawString(m_font, subTitleText);
		g->translate(-shadowOffset, -shadowOffset);
		g->setColor(0xffffffff);
		g->drawString(m_font, subTitleText);
	}
	g->popTransform();

	// draw song info (length, bpm, objects)
	const Color songInfoColor = (osu->getSpeedMultiplier() != 1.0f ? (osu->getSpeedMultiplier() > 1.0f ? 0xffff7f7f : 0xffadd8e6) : 0xffffffff);
	g->pushTransform();
	{
		const float scale = m_fSongInfoScale*globalScale*0.9f;

		yCounter += m_font->getHeight()*scale + m_iMargin*globalScale*1.0f;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x), yCounter);

		g->translate(shadowOffset, shadowOffset);
		g->setColor(0xff000000);
		g->drawString(m_font, songInfoText);
		g->translate(-shadowOffset, -shadowOffset);
		g->setColor(songInfoColor);
		g->drawString(m_font, songInfoText);
	}
	g->popTransform();

	// draw diff info (CS, AR, OD, HP, Stars)
	const Color diffInfoColor = osu->getModEZ() ? 0xffadd8e6 : (osu->getModHR() ? 0xffff7f7f : 0xffffffff);
	g->pushTransform();
	{
		const float scale = m_fDiffInfoScale*globalScale*0.9f;

		yCounter += m_font->getHeight()*scale + m_iMargin*globalScale*0.85f;

		g->scale(scale, scale);
		g->translate((int)(m_vPos.x), yCounter);

		g->translate(shadowOffset, shadowOffset);
		g->setColor(0xff000000);
		g->drawString(m_font, diffInfoText);
		g->translate(-shadowOffset, -shadowOffset);
		g->setColor(diffInfoColor);
		g->drawString(m_font, diffInfoText);
	}
	g->popTransform();

	// draw offset (local, online)
	if (m_iLocalOffset != 0 || m_iOnlineOffset != 0)
	{
		g->pushTransform();
		{
			const float scale = m_fOffsetInfoScale*globalScale*0.8f;

			yCounter += m_font->getHeight()*scale + m_iMargin*globalScale*0.85f;

			g->scale(scale, scale);
			g->translate((int)(m_vPos.x), yCounter);

			g->translate(shadowOffset, shadowOffset);
			g->setColor(0xff000000);
			g->drawString(m_font, offsetInfoText);
			g->translate(-shadowOffset, -shadowOffset);
			g->setColor(0xffffffff);
			g->drawString(m_font, offsetInfoText);
		}
		g->popTransform();
	}
}

void OsuUISongBrowserInfoLabel::update()
{
	CBaseUIButton::update();
	if (!m_bVisible) return;

	// detail info tooltip when hovering over diff info
	if (isMouseInside() && !osu->getOptionsMenu()->isMouseInside())
	{
		OsuBeatmap *beatmap = osu->getSelectedBeatmap();
		if (beatmap != NULL)
		{
			const float speedMultiplierInv = (1.0f / osu->getSpeedMultiplier());

			const float approachTimeRoundedCompensated = ((int)OsuGameRules::getApproachTime(beatmap)) * speedMultiplierInv;
			const float hitWindow300RoundedCompensated = ((int)OsuGameRules::getHitWindow300(beatmap) - 0.5f) * speedMultiplierInv;
			const float hitWindow100RoundedCompensated = ((int)OsuGameRules::getHitWindow100(beatmap) - 0.5f) * speedMultiplierInv;
			const float hitWindow50RoundedCompensated  = ((int)OsuGameRules::getHitWindow50(beatmap)  - 0.5f) * speedMultiplierInv;
			const float hitobjectRadiusRoundedCompensated = (OsuGameRules::getRawHitCircleDiameter(beatmap->getCS()) / 2.0f);

			osu->getTooltipOverlay()->begin();
			{
				osu->getTooltipOverlay()->addLine(UString::format("Approach time: %.2fms", approachTimeRoundedCompensated));
				osu->getTooltipOverlay()->addLine(UString::format("300: +-%.2fms", hitWindow300RoundedCompensated));
				osu->getTooltipOverlay()->addLine(UString::format("100: +-%.2fms", hitWindow100RoundedCompensated));
				osu->getTooltipOverlay()->addLine(UString::format(" 50: +-%.2fms", hitWindow50RoundedCompensated));
				osu->getTooltipOverlay()->addLine(UString::format("Spinner difficulty: %.2f", OsuGameRules::getSpinnerSpinsPerSecond(beatmap)));
				osu->getTooltipOverlay()->addLine(UString::format("Hit object radius: %.2f", hitobjectRadiusRoundedCompensated));

				if (beatmap->getSelectedDifficulty2() != NULL)
				{
					int numObjects = beatmap->getSelectedDifficulty2()->getNumObjects();
					int numCircles = beatmap->getSelectedDifficulty2()->getNumCircles();
					int numSliders = beatmap->getSelectedDifficulty2()->getNumSliders();
					unsigned long lengthMS = beatmap->getSelectedDifficulty2()->getLengthMS();

					const bool areStarsInaccurate = (osu->getSongBrowser()->getDynamicStarCalculator()->isDead() || !osu->getSongBrowser()->getDynamicStarCalculator()->isAsyncReady());
					if (!areStarsInaccurate)
					{
						numObjects = osu->getSongBrowser()->getDynamicStarCalculator()->getNumObjects();
						numCircles = osu->getSongBrowser()->getDynamicStarCalculator()->getNumCircles();
						numSliders = std::max(0, osu->getSongBrowser()->getDynamicStarCalculator()->getNumObjects() - osu->getSongBrowser()->getDynamicStarCalculator()->getNumCircles() - osu->getSongBrowser()->getDynamicStarCalculator()->getNumSpinners());
						lengthMS = osu->getSongBrowser()->getDynamicStarCalculator()->getLengthMS();
					}

					const float opm = (lengthMS > 0 ? ((float)numObjects / (float)(lengthMS / 1000.0f / 60.0f)) : 0.0f) * osu->getSpeedMultiplier();
					const float cpm = (lengthMS > 0 ? ((float)numCircles / (float)(lengthMS / 1000.0f / 60.0f)) : 0.0f) * osu->getSpeedMultiplier();
					const float spm = (lengthMS > 0 ? ((float)numSliders / (float)(lengthMS / 1000.0f / 60.0f)) : 0.0f) * osu->getSpeedMultiplier();

					osu->getTooltipOverlay()->addLine(UString::format("Circles: %i, Sliders: %i, Spinners: %i", numCircles, numSliders, std::max(0, numObjects - numCircles - numSliders)));
					osu->getTooltipOverlay()->addLine(UString::format("OPM: %i, CPM: %i, SPM: %i", (int)opm, (int)cpm, (int)spm));
					osu->getTooltipOverlay()->addLine(UString::format("ID: %i, SetID: %i", beatmap->getSelectedDifficulty2()->getID(), beatmap->getSelectedDifficulty2()->getSetID()));
					osu->getTooltipOverlay()->addLine(UString::format("MD5: %s", beatmap->getSelectedDifficulty2()->getMD5Hash().c_str()));
				}
			}
			osu->getTooltipOverlay()->end();
		}
	}
}

void OsuUISongBrowserInfoLabel::setFromBeatmap(OsuBeatmap * /*beatmap*/, OsuDatabaseBeatmap *diff2)
{
	m_iBeatmapId = diff2->getID();

	setArtist(diff2->getArtist());
	setTitle(diff2->getTitle());
	setDiff(diff2->getDifficultyName());
	setMapper(diff2->getCreator());

	setLengthMS(diff2->getLengthMS());
	setBPM(diff2->getMinBPM(), diff2->getMaxBPM(), diff2->getMostCommonBPM());
	setNumObjects(diff2->getNumObjects());

	setCS(diff2->getCS());
	setAR(diff2->getAR());
	setOD(diff2->getOD());
	setHP(diff2->getHP());
	setStars(diff2->getStarsNomod());

	setLocalOffset(diff2->getLocalOffset());
	setOnlineOffset(diff2->getOnlineOffset());
}

void OsuUISongBrowserInfoLabel::setFromMissingBeatmap(long beatmapId)
{
	m_iBeatmapId = beatmapId;

	setArtist(m_iBeatmapId > 0 ? "CLICK HERE TO DOWNLOAD!" : "MISSING BEATMAP!");
	setTitle("");
	setDiff("no map");
	setMapper("MISSING BEATMAP!");

	setLengthMS(0);
	setBPM(0, 0, 0);
	setNumObjects(0);

	setCS(0);
	setAR(0);
	setOD(0);
	setHP(0);
	setStars(0);

	setLocalOffset(0);
	setOnlineOffset(0);
}

void OsuUISongBrowserInfoLabel::onClicked()
{
	// deprecated due to "Web" button
	/*
	if (m_iBeatmapId > 0)
	{
		env->openURLInDefaultBrowser(UString::format("https://osu.ppy.sh/b/%ld", m_iBeatmapId));
		osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	}
	*/

	if (osu->getMultiplayer()->isInMultiplayer() && osu->getMultiplayer()->isMissingBeatmap() && !osu->getMultiplayer()->isDownloadingBeatmap())
	{
		osu->getNotificationOverlay()->addNotification("Requesting download from peer, please wait ...", 0xffffffff, false, 0.75f);
		osu->getMultiplayer()->onClientBeatmapDownloadRequest();
	}

	CBaseUIButton::onClicked();
}

UString OsuUISongBrowserInfoLabel::buildTitleString()
{
	UString titleString = m_sArtist;
	titleString.append(" - ");
	titleString.append(m_sTitle);
	titleString.append(" [");
	titleString.append(m_sDiff);
	titleString.append("]");

	return titleString;
}

UString OsuUISongBrowserInfoLabel::buildSubTitleString()
{
	UString subTitleString = "Mapped by ";
	subTitleString.append(m_sMapper);

	return subTitleString;
}

UString OsuUISongBrowserInfoLabel::buildSongInfoString()
{
	unsigned long lengthMS = m_iLengthMS;

	const bool areStarsInaccurate = (osu->getSongBrowser()->getDynamicStarCalculator()->isDead() || !osu->getSongBrowser()->getDynamicStarCalculator()->isAsyncReady());

	if (!areStarsInaccurate)
		lengthMS = std::max(lengthMS, (unsigned long)osu->getSongBrowser()->getDynamicStarCalculator()->getLengthMS());

	const unsigned long fullSeconds = (lengthMS*(1.0 / osu->getSpeedMultiplier())) / 1000.0;
	const int minutes = fullSeconds / 60;
	const int seconds = fullSeconds % 60;

	const int minBPM = m_iMinBPM * osu->getSpeedMultiplier();
	const int maxBPM = m_iMaxBPM * osu->getSpeedMultiplier();
	const int mostCommonBPM = m_iMostCommonBPM * osu->getSpeedMultiplier();

	int numObjects = m_iNumObjects;

	if (!areStarsInaccurate)
		numObjects = osu->getSongBrowser()->getDynamicStarCalculator()->getNumObjects();

	if (m_iMinBPM == m_iMaxBPM)
		return UString::format("Length: %02i:%02i BPM: %i Objects: %i", minutes, seconds, maxBPM, numObjects);
	else
		return UString::format("Length: %02i:%02i BPM: %i-%i (%i) Objects: %i", minutes, seconds, minBPM, maxBPM, mostCommonBPM, numObjects);
}

UString OsuUISongBrowserInfoLabel::buildDiffInfoString()
{
	float CS = m_fCS;
	float AR = m_fAR;
	float OD = m_fOD;
	float HP = m_fHP;
	float stars = m_fStars;

	const float modStars = (float)osu->getSongBrowser()->getDynamicStarCalculator()->getTotalStars();
	const float modPp = (float)osu->getSongBrowser()->getDynamicStarCalculator()->getPPv2();
	const bool areStarsInaccurate = (osu->getSongBrowser()->getDynamicStarCalculator()->isDead() || !osu->getSongBrowser()->getDynamicStarCalculator()->isAsyncReady());

	OsuBeatmap *beatmap = osu->getSelectedBeatmap();
	if (beatmap != NULL)
	{
		CS = beatmap->getCS();
		AR = OsuGameRules::getApproachRateForSpeedMultiplier(beatmap);
		OD = OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap);
		HP = beatmap->getHP();
	}

	const float starComparisonEpsilon = 0.01f;
	const bool starsAndModStarsAreEqual = (std::abs(stars - modStars) < starComparisonEpsilon);

	UString finalString;
	if (areStarsInaccurate && cv::osu::songbrowser_dynamic_star_recalc.getBool())
		finalString = UString::format("CS:%.3g AR:%.3g OD:%.3g HP:%.3g Stars:%.3g *", CS, AR, OD, HP, stars);
	else if (!starsAndModStarsAreEqual && cv::osu::songbrowser_dynamic_star_recalc.getBool())
		finalString = UString::format("CS:%.3g AR:%.3g OD:%.3g HP:%.3g Stars:%.3g -> %.3g (%ipp)", CS, AR, OD, HP, stars, modStars, (int)(std::round(modPp)));
	else if (cv::osu::songbrowser_dynamic_star_recalc.getBool())
		finalString = UString::format("CS:%.3g AR:%.3g OD:%.3g HP:%.3g Stars:%.3g (%ipp)", CS, AR, OD, HP, stars, (int)(std::round(modPp)));
	else
		finalString = UString::format("CS:%.3g AR:%.3g OD:%.3g HP:%.3g Stars:%.3g", CS, AR, OD, HP, stars);

	return finalString;
}

UString OsuUISongBrowserInfoLabel::buildOffsetInfoString()
{
	return UString::format("Your Offset: %ld ms / Online Offset: %ld ms", m_iLocalOffset, m_iOnlineOffset);
}

float OsuUISongBrowserInfoLabel::getMinimumWidth()
{
	/*
	float titleWidth = m_font->getStringWidth(buildTitleString());
	float subTitleWidth = m_font->getStringWidth(buildSubTitleString()) * m_fSubTitleScale;
	*/
	float titleWidth = 0;
	float subTitleWidth = 0;
	float songInfoWidth = m_font->getStringWidth(buildSongInfoString()) * m_fSongInfoScale;
	float diffInfoWidth = m_font->getStringWidth(buildDiffInfoString()) * m_fDiffInfoScale;
	float offsetInfoWidth = m_font->getStringWidth(buildOffsetInfoString()) * m_fOffsetInfoScale;

	return std::max(std::max(std::max(std::max(titleWidth, subTitleWidth), songInfoWidth), diffInfoWidth), offsetInfoWidth);
}

float OsuUISongBrowserInfoLabel::getMinimumHeight()
{
	float titleHeight = m_font->getHeight() * m_fTitleScale;
	float subTitleHeight = m_font->getHeight() * m_fSubTitleScale;
	float songInfoHeight = m_font->getHeight() * m_fSongInfoScale;
	float diffInfoHeight = m_font->getHeight() * m_fDiffInfoScale;
	///float offsetInfoHeight = m_font->getHeight() * m_fOffsetInfoScale;

	return titleHeight + subTitleHeight + songInfoHeight + diffInfoHeight/* + offsetInfoHeight*/ + m_iMargin*3; // this is commented on purpose (also, it should be m_iMargin*4 but the 3 is also on purpose)
}
