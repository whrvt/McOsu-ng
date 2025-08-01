//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score/results/ranking screen
//
// $NoKeywords: $osuss
//===============================================================================//

#include "OsuRankingScreen.h"

#include <utility>

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "ConVar.h"
#include "Keyboard.h"
#include "AnimationHandler.h"
#include "Mouse.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUIImage.h"
#include "CBaseUILabel.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuIcons.h"
#include "OsuScore.h"
#include "OsuBeatmap.h"
#include "OsuOptionsMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuTooltipOverlay.h"
#include "OsuModSelector.h"
#include "OsuGameRules.h"
#include "OsuReplay.h"

#include "OsuUIRankingScreenInfoLabel.h"
#include "OsuUIRankingScreenRankingPanel.h"
#include "OsuUISongBrowserScoreButton.h"
namespace cv::osu {
ConVar rankingscreen_topbar_height_percent("osu_rankingscreen_topbar_height_percent", 0.785f, FCVAR_NONE);
ConVar rankingscreen_pp("osu_rankingscreen_pp", true, FCVAR_NONE);
ConVar draw_rankingscreen_background_image("osu_draw_rankingscreen_background_image", true, FCVAR_NONE);
}


class OsuRankingScreenIndexLabel : public CBaseUILabel
{
public:
	OsuRankingScreenIndexLabel() : CBaseUILabel(-1, 0, 0, 0, "", "You achieved the #1 score on local rankings!")
	{
		m_bVisible2 = false;
	}

	void draw() override
	{
		if (!m_bVisible || !m_bVisible2) return;

		// draw background gradient
		const Color topColor = 0xdd634e13;
		const Color bottomColor = 0xdd785f15;
		g->fillGradient(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y, topColor, topColor, bottomColor, bottomColor);

		// draw ranking index text
		const float textScale = 0.45f;
		g->pushTransform();
		{
			const float scale = (m_vSize.y / m_fStringHeight)*textScale;

			g->scale(scale, scale);
			g->translate((int)(m_vPos.x + m_vSize.x/2 - m_fStringWidth*scale/2), (int)(m_vPos.y + m_vSize.y/2 + m_font->getHeight()*scale/2));
			g->translate(1, 1);
			g->setColor(0xff000000);
			g->drawString(m_font, m_sText);
			g->translate(-1, -1);
			g->setColor(m_textColor);
			g->drawString(m_font, m_sText);
		}
		g->popTransform();
	}

	inline void setVisible2(bool visible2) {m_bVisible2 = visible2;}

	[[nodiscard]] inline bool isVisible2() const {return m_bVisible2;}

private:
	bool m_bVisible2;
};

class OsuRankingScreenBottomElement : public CBaseUILabel
{
public:
	OsuRankingScreenBottomElement() : CBaseUILabel(-1, 0, 0, 0, "", "")
	{
		m_bVisible2 = false;
	}

	void draw() override
	{
		if (!m_bVisible || !m_bVisible2) return;

		// draw background gradient
		const Color topColor = 0xdd3a2e0c;
		const Color bottomColor = 0xdd493a0f;
		g->fillGradient(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y, topColor, topColor, bottomColor, bottomColor);
	}

	inline void setVisible2(bool visible2) {m_bVisible2 = visible2;}

	[[nodiscard]] bool isVisible2() const {return m_bVisible2;}

private:
	bool m_bVisible2;
};

class OsuRankingScreenScrollDownInfoButton : public CBaseUIButton
{
public:
	OsuRankingScreenScrollDownInfoButton() : CBaseUIButton(0, 0, 0, 0, "")
	{
		m_bVisible2 = false;
		m_fAlpha = 1.0f;
	}

	void draw() override
	{
		if (!m_bVisible || !m_bVisible2) return;

		const float textScale = 0.45f;
		g->pushTransform();
		{
			const float scale = (m_vSize.y / m_fStringHeight)*textScale;

			float animation = fmod((float)(engine->getTime()-0.0f)*3.2f, 2.0f);
			if (animation > 1.0f)
				animation = 2.0f - animation;

			animation =  -animation*(animation-2); // quad out
			const float offset = -m_fStringHeight*scale*0.25f + animation*m_fStringHeight*scale*0.25f;

			g->scale(scale, scale);
			g->translate((int)(m_vPos.x + m_vSize.x/2 - m_fStringWidth*scale/2), (int)(m_vPos.y + m_vSize.y/2 + m_fStringHeight*scale/2 - offset));
			g->translate(2, 2);
			g->setColor(0xff000000);
			g->setAlpha(m_fAlpha);
			g->drawString(m_font, m_sText);
			g->translate(-2, -2);
			g->setColor(0xffffffff);
			g->setAlpha(m_fAlpha);
			g->drawString(m_font, m_sText);
		}
		g->popTransform();

		/*
		g->setColor(0xffffffff);
		g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
		*/
	}

	void setAlpha(float alpha) {m_fAlpha = alpha;}

	inline void setVisible2(bool visible2) {m_bVisible2 = visible2;}

	[[nodiscard]] inline bool isVisible2() const {return m_bVisible2;}

private:
	void onMouseDownInside() override
	{
		CBaseUIButton::onMouseDownInside();
		onClicked();
	}

	bool m_bVisible2;
	float m_fAlpha;
};



OsuRankingScreen::OsuRankingScreen() : OsuScreenBackable()
{


	m_container = new CBaseUIContainer(0, 0, 0, 0, "");
	m_rankings = new CBaseUIScrollView(-1, 0, 0, 0, "");
	m_rankings->setHorizontalScrolling(false);
	m_rankings->setVerticalScrolling(true);
	m_rankings->setDrawFrame(false);
	m_rankings->setDrawBackground(false);
	m_rankings->setDrawScrollbars(true);
	m_container->addBaseUIElement(m_rankings);

	m_songInfo = new OsuUIRankingScreenInfoLabel(5, 5, 0, 0, "");
	m_container->addBaseUIElement(m_songInfo);

	m_rankingTitle = new CBaseUIImage(osu->getSkin()->getRankingTitle()->getName(), 0, 0, 0, 0, "");
	m_rankingTitle->setDrawBackground(false);
	m_rankingTitle->setDrawFrame(false);
	m_container->addBaseUIElement(m_rankingTitle);

	m_rankingPanel = new OsuUIRankingScreenRankingPanel();
	m_rankingPanel->setDrawBackground(false);
	m_rankingPanel->setDrawFrame(false);
	m_rankings->getContainer()->addBaseUIElement(m_rankingPanel);

	m_rankingGrade = new CBaseUIImage(osu->getSkin()->getRankingA()->getName(), 0, 0, 0, 0, "");
	m_rankingGrade->setDrawBackground(false);
	m_rankingGrade->setDrawFrame(false);
	m_rankings->getContainer()->addBaseUIElement(m_rankingGrade);

	m_rankingBottom = new OsuRankingScreenBottomElement();
	m_rankings->getContainer()->addBaseUIElement(m_rankingBottom);

	m_rankingIndex = new OsuRankingScreenIndexLabel();
	m_rankingIndex->setDrawFrame(false);
	m_rankingIndex->setCenterText(true);
	m_rankingIndex->setFont(osu->getSongBrowserFont());
	m_rankingIndex->setTextColor(0xffffcb21);
	m_rankings->getContainer()->addBaseUIElement(m_rankingIndex);

	m_rankingScrollDownInfoButton = new OsuRankingScreenScrollDownInfoButton();
	m_rankingScrollDownInfoButton->setFont(osu->getFontIcons());
	m_rankingScrollDownInfoButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuRankingScreen::onScrollDownClicked) );
	UString iconString;
	iconString.insert(0, OsuIcons::ARROW_DOWN);
	iconString.append("   ");
	iconString.insert(iconString.length(), OsuIcons::ARROW_DOWN);
	iconString.append("   ");
	iconString.insert(iconString.length(), OsuIcons::ARROW_DOWN);
	m_rankingScrollDownInfoButton->setText(iconString);
	m_container->addBaseUIElement(m_rankingScrollDownInfoButton);
	m_fRankingScrollDownInfoButtonAlphaAnim = 1.0f;

	setGrade(OsuScore::GRADE::GRADE_D);
	setIndex(0); // TEMP

	m_fUnstableRate = 0.0f;
	m_fHitErrorAvgMin = 0.0f;
	m_fHitErrorAvgMax = 0.0f;
	m_fStarsTomTotal = 0.0f;
	m_fStarsTomAim = 0.0f;
	m_fStarsTomSpeed = 0.0f;
	m_fPPv2 = 0.0f;

	m_fSpeedMultiplier = 0.0f;
	m_fCS = 0.0f;
	m_fAR = 0.0f;
	m_fOD = 0.0f;
	m_fHP = 0.0f;

	m_bModSS = false;
	m_bModSD = false;
	m_bModEZ = false;
	m_bModHD = false;
	m_bModHR = false;
	m_bModNC = false;
	m_bModDT = false;
	m_bModNM = false;
	m_bModScorev2 = false;
	m_bModTarget = false;
	m_bModSpunout = false;
	m_bModRelax = false;
	m_bModNF = false;
	m_bModHT = false;
	m_bModAutopilot = false;
	m_bModAuto = false;
	m_bModTD = false;

	m_bIsLegacyScore = false;
	m_bIsImportedLegacyScore = false;
	m_bIsUnranked = false;
}

OsuRankingScreen::~OsuRankingScreen()
{
	SAFE_DELETE(m_container);
}

void OsuRankingScreen::draw()
{
	if (!m_bVisible) return;

	// draw background image
	if (cv::osu::draw_rankingscreen_background_image.getBool())
		OsuSongBrowser2::drawSelectedBeatmapBackgroundImage(osu);

	m_rankings->draw();

	// draw active mods
	const Vector2 modPosStart = Vector2(m_rankings->getSize().x - osu->getUIScale(20), m_rankings->getScrollPosY() + osu->getUIScale(260));
	Vector2 modPos = modPosStart;
	Vector2 modPosMax;
	if (m_bModTD)
		drawModImage(osu->getSkin()->getSelectionModTD(), modPos, modPosMax);
	if (m_bModSS)
		drawModImage(osu->getSkin()->getSelectionModPerfect(), modPos, modPosMax);
	else if (m_bModSD)
		drawModImage(osu->getSkin()->getSelectionModSuddenDeath(), modPos, modPosMax);
	if (m_bModEZ)
		drawModImage(osu->getSkin()->getSelectionModEasy(), modPos, modPosMax);
	if (m_bModHD)
		drawModImage(osu->getSkin()->getSelectionModHidden(), modPos, modPosMax);
	if (m_bModHR)
		drawModImage(osu->getSkin()->getSelectionModHardRock(), modPos, modPosMax);
	if (m_bModNC)
		drawModImage(osu->getSkin()->getSelectionModNightCore(), modPos, modPosMax);
	else if (m_bModDT)
		drawModImage(osu->getSkin()->getSelectionModDoubleTime(), modPos, modPosMax);
	if (m_bModNM)
		drawModImage(osu->getSkin()->getSelectionModNightmare(), modPos, modPosMax);
	if (m_bModScorev2)
		drawModImage(osu->getSkin()->getSelectionModScorev2(), modPos, modPosMax);
	if (m_bModTarget)
		drawModImage(osu->getSkin()->getSelectionModTarget(), modPos, modPosMax);
	if (m_bModSpunout)
		drawModImage(osu->getSkin()->getSelectionModSpunOut(), modPos, modPosMax);
	if (m_bModRelax)
		drawModImage(osu->getSkin()->getSelectionModRelax(), modPos, modPosMax);
	if (m_bModNF)
		drawModImage(osu->getSkin()->getSelectionModNoFail(), modPos, modPosMax);
	if (m_bModHT)
		drawModImage(osu->getSkin()->getSelectionModHalfTime(), modPos, modPosMax);
	if (m_bModAutopilot)
		drawModImage(osu->getSkin()->getSelectionModAutopilot(), modPos, modPosMax);
	if (m_bModAuto)
		drawModImage(osu->getSkin()->getSelectionModAutoplay(), modPos, modPosMax);

	// draw experimental mods
	if (m_enabledExperimentalMods.size() > 0)
	{
		McFont *experimentalModFont = osu->getSubTitleFont();
		const UString prefix = "+ ";

		float maxStringWidth = 0.0f;
		for (int i=0; i<m_enabledExperimentalMods.size(); i++)
		{
			UString experimentalModName = m_enabledExperimentalMods[i]->getName();
			experimentalModName.insert(0, prefix);
			const float width = experimentalModFont->getStringWidth(experimentalModName);
			if (width > maxStringWidth)
				maxStringWidth = width;
		}

		const int backgroundMargin = 6;
		const float heightMultiplier = 1.25f;
		const int experimentalModHeight = (experimentalModFont->getHeight() * heightMultiplier);
		const Vector2 experimentalModPos = Vector2(modPosStart.x - maxStringWidth - backgroundMargin, std::max(modPosStart.y, modPosMax.y) + osu->getUIScale(10) + experimentalModFont->getHeight()*heightMultiplier);
		const int backgroundWidth = maxStringWidth + 2*backgroundMargin;
		const int backgroundHeight = experimentalModHeight*m_enabledExperimentalMods.size() + 2*backgroundMargin;

		// draw background
		g->setColor(0x77000000);
		g->fillRect((int)experimentalModPos.x - backgroundMargin, (int)experimentalModPos.y - experimentalModFont->getHeight() - backgroundMargin, backgroundWidth, backgroundHeight);

		// draw mods
		g->pushTransform();
		{
			g->translate((int)experimentalModPos.x, (int)experimentalModPos.y);
			for (int i=0; i<m_enabledExperimentalMods.size(); i++)
			{
				UString experimentalModName = m_enabledExperimentalMods[i]->getName();
				experimentalModName.insert(0, prefix);

				g->translate(1.5f, 1.5f);
				g->setColor(0xff000000);
				g->drawString(experimentalModFont, experimentalModName);
				g->translate(-1.5f, -1.5f);
				g->setColor(0xffffffff);
				g->drawString(experimentalModFont, experimentalModName);
				g->translate(0, experimentalModHeight);
			}
		}
		g->popTransform();
	}

	// draw pp
	if (cv::osu::rankingscreen_pp.getBool() && !m_bIsLegacyScore)
	{
		const UString ppString = getPPString();
		const Vector2 ppPos = getPPPosRaw() + m_vPPCursorMagnetAnimation;

		g->pushTransform();
		{
			g->translate((int)ppPos.x + 2, (int)ppPos.y + 2);
			g->setColor(0xff000000);
			g->drawString(osu->getTitleFont(), ppString);
			g->translate(-2, -2);
			g->setColor(0xffffffff);
			g->drawString(osu->getTitleFont(), ppString);
		}
		g->popTransform();
	}

	if (cv::osu::scores_enabled.getBool())
		m_rankingScrollDownInfoButton->draw();

	// draw top black bar
	g->setColor(0xff000000);
	g->fillRect(0, 0, osu->getVirtScreenWidth(), m_rankingTitle->getSize().y*cv::osu::rankingscreen_topbar_height_percent.getFloat());

	m_rankingTitle->draw();
	m_songInfo->draw();

	OsuScreenBackable::draw();
}

void OsuRankingScreen::drawModImage(OsuSkinImage *image, Vector2 &pos, Vector2 &max)
{
	g->setColor(0xffffffff);
	image->draw(Vector2(pos.x - image->getSize().x/2.0f, pos.y));

	pos.x -= osu->getUIScale(20);

	if (pos.y + image->getSize().y/2 > max.y)
		max.y = pos.y + image->getSize().y/2;
}

void OsuRankingScreen::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// HACKHACK:
	if (osu->getOptionsMenu()->isMouseInside())
		mouse->resetWheelDelta();

	// update and focus handling
	m_container->update();

	if (osu->getOptionsMenu()->isMouseInside())
		stealFocus();

	if (osu->getOptionsMenu()->isBusy() || m_backButton->isActive())
		m_container->stealFocus();

	// tooltip (pp + accuracy + unstable rate)
	if (!osu->getOptionsMenu()->isMouseInside() && !m_bIsLegacyScore && mouse->getPos().x < osu->getVirtScreenWidth() * 0.5f)
	{
		osu->getTooltipOverlay()->begin();
		{
			osu->getTooltipOverlay()->addLine(UString::format("%.2fpp", m_fPPv2));
			osu->getTooltipOverlay()->addLine("Difficulty:");
			osu->getTooltipOverlay()->addLine(UString::format("Stars: %.2f (%.2f aim, %.2f speed)", m_fStarsTomTotal, m_fStarsTomAim, m_fStarsTomSpeed));
			osu->getTooltipOverlay()->addLine(UString::format("Speed: %.3gx", m_fSpeedMultiplier));
			osu->getTooltipOverlay()->addLine(UString::format("CS:%.4g AR:%.4g OD:%.4g HP:%.4g", m_fCS, m_fAR, m_fOD, m_fHP));

			if (m_sMods.length() > 0)
				osu->getTooltipOverlay()->addLine(m_sMods);

			if (!m_bIsImportedLegacyScore)
			{
				osu->getTooltipOverlay()->addLine("Accuracy:");
				osu->getTooltipOverlay()->addLine(UString::format("Error: %.2fms - %.2fms avg", m_fHitErrorAvgMin, m_fHitErrorAvgMax));
				osu->getTooltipOverlay()->addLine(UString::format("Unstable Rate: %.2f", m_fUnstableRate));
			}
			else
				osu->getTooltipOverlay()->addLine("This score was imported from osu!");
		}
		osu->getTooltipOverlay()->end();
	}

	// frustration multiplier
	Vector2 cursorDelta = getPPPosCenterRaw() - mouse->getPos();
	Vector2 norm;
	const float dist = 150.0f;
	if (cursorDelta.length() > dist)
	{
		cursorDelta.x = 0;
		cursorDelta.y = 0;
	}
	else
	{
		norm = cursorDelta;
		norm.normalize();
	}
	float percent = 1.0f - (cursorDelta.length() / dist);
	Vector2 target = norm*percent*percent*(dist + 50);
	anim->moveQuadOut(&m_vPPCursorMagnetAnimation.x, target.x, 0.20f, true);
	anim->moveQuadOut(&m_vPPCursorMagnetAnimation.y, target.y, 0.20f, true);

	// button transparency
	const float transparencyStart = m_container->getPos().y + m_container->getSize().y;
	const float transparencyEnd = m_container->getPos().y + m_container->getSize().y + m_rankingIndex->getSize().y/2;
	const float alpha = std::clamp<float>((m_rankingIndex->getPos().y + (transparencyEnd - transparencyStart) - transparencyStart)/(transparencyEnd - transparencyStart), 0.0f, 1.0f);
	anim->moveLinear(&m_fRankingScrollDownInfoButtonAlphaAnim, alpha, 0.075*m_fRankingScrollDownInfoButtonAlphaAnim, true);
	m_rankingScrollDownInfoButton->setAlpha(m_fRankingScrollDownInfoButtonAlphaAnim);
}

void OsuRankingScreen::setVisible(bool visible)
{
	OsuScreenBackable::setVisible(visible);

	if (m_bVisible)
	{
		m_backButton->resetAnimation();
		m_rankings->scrollToY(0, false);

		updateLayout();
	}
}

void OsuRankingScreen::setScore(OsuScore *score)
{
	m_rankingPanel->setScore(score);
	setGrade(score->getGrade());
	setIndex(score->getIndex());

	m_fUnstableRate = score->getUnstableRate();
	m_fHitErrorAvgMin = score->getHitErrorAvgMin();
	m_fHitErrorAvgMax = score->getHitErrorAvgMax();
	m_fStarsTomTotal = score->getStarsTomTotal();
	m_fStarsTomAim = score->getStarsTomAim();
	m_fStarsTomSpeed = score->getStarsTomSpeed();
	m_fPPv2 = score->getPPv2();

	const UString modsString = OsuUISongBrowserScoreButton::getModsStringForDisplay(score->getModsLegacy());
	if (modsString.length() > 0)
	{
		m_sMods = "Mods: ";
		m_sMods.append(modsString);
	}
	else
		m_sMods = "";

	m_bModSS = osu->getModSS();
	m_bModSD = osu->getModSD();
	m_bModEZ = osu->getModEZ();
	m_bModHD = osu->getModHD();
	m_bModHR = osu->getModHR();
	m_bModNC = osu->getModNC();
	m_bModDT = osu->getModDT();
	m_bModNM = osu->getModNM();
	m_bModScorev2 = osu->getModScorev2();
	m_bModTarget = osu->getModTarget();
	m_bModSpunout = osu->getModSpunout();
	m_bModRelax = osu->getModRelax();
	m_bModNF = osu->getModNF();
	m_bModHT = osu->getModHT();
	m_bModAutopilot = osu->getModAutopilot();
	m_bModAuto = osu->getModAuto();
	m_bModTD = osu->getModTD();

	m_enabledExperimentalMods.clear();
	std::vector<ConVar*> allExperimentalMods = osu->getExperimentalMods();
	for (int i=0; i<allExperimentalMods.size(); i++)
	{
		if (allExperimentalMods[i]->getBool())
			m_enabledExperimentalMods.push_back(allExperimentalMods[i]);
	}

	m_bIsLegacyScore = false;
	m_bIsImportedLegacyScore = false;
	m_bIsUnranked = score->isUnranked();
}

void OsuRankingScreen::setScore(const OsuDatabase::Score& score, UString dateTime)
{
	m_bIsLegacyScore = score.isLegacyScore;
	m_bIsImportedLegacyScore = score.isImportedLegacyScore;
	m_bIsUnranked = false;

	m_songInfo->setDate(std::move(dateTime));
	m_songInfo->setPlayer(score.playerName);

	m_rankingPanel->setScore(score);
	setGrade(OsuScore::calculateGrade(score.num300s, score.num100s, score.num50s, score.numMisses, score.modsLegacy & OsuReplay::Mods::Hidden, score.modsLegacy & OsuReplay::Mods::Flashlight));
	setIndex(-1);

	m_fUnstableRate = score.unstableRate;
	m_fHitErrorAvgMin = score.hitErrorAvgMin;
	m_fHitErrorAvgMax = score.hitErrorAvgMax;
	m_fStarsTomTotal = score.starsTomTotal;
	m_fStarsTomAim = score.starsTomAim;
	m_fStarsTomSpeed = score.starsTomSpeed;
	m_fPPv2 = score.pp;

	m_fSpeedMultiplier = std::round(score.speedMultiplier * 100.0f) / 100.0f;
	m_fCS = std::round(score.CS * 100.0f) / 100.0f;
	m_fAR = std::round(OsuGameRules::getRawApproachRateForSpeedMultiplier(OsuGameRules::getRawApproachTime(score.AR), score.speedMultiplier) * 100.0f) / 100.0f;
	m_fOD = std::round(OsuGameRules::getRawOverallDifficultyForSpeedMultiplier(OsuGameRules::getRawHitWindow300(score.OD), score.speedMultiplier) * 100.0f) / 100.0f;
	m_fHP = std::round(score.HP * 100.0f) / 100.0f;

	const UString modsString = OsuUISongBrowserScoreButton::getModsStringForDisplay(score.modsLegacy);
	if (modsString.length() > 0)
	{
		m_sMods = "Mods: ";
		m_sMods.append(modsString);
	}
	else
		m_sMods = "";

	m_bModSS = score.modsLegacy & OsuReplay::Mods::Perfect;
	m_bModSD = score.modsLegacy & OsuReplay::Mods::SuddenDeath;
	m_bModEZ = score.modsLegacy & OsuReplay::Mods::Easy;
	m_bModHD = score.modsLegacy & OsuReplay::Mods::Hidden;
	m_bModHR = score.modsLegacy & OsuReplay::Mods::HardRock;
	m_bModNC = score.modsLegacy & OsuReplay::Mods::Nightcore;
	m_bModDT = score.modsLegacy & OsuReplay::Mods::DoubleTime;
	m_bModNM = score.modsLegacy & OsuReplay::Mods::Nightmare;
	m_bModScorev2 = score.modsLegacy & OsuReplay::Mods::ScoreV2;
	m_bModTarget = score.modsLegacy & OsuReplay::Mods::Target;
	m_bModSpunout = score.modsLegacy & OsuReplay::Mods::SpunOut;
	m_bModRelax = score.modsLegacy & OsuReplay::Mods::Relax;
	m_bModNF = score.modsLegacy & OsuReplay::Mods::NoFail;
	m_bModHT = score.modsLegacy & OsuReplay::Mods::HalfTime;
	m_bModAutopilot = score.modsLegacy & OsuReplay::Mods::Relax2;
	m_bModAuto = score.modsLegacy & OsuReplay::Mods::Autoplay;
	m_bModTD = score.modsLegacy & OsuReplay::Mods::TouchDevice;

	m_enabledExperimentalMods.clear();
	if (score.experimentalModsConVars.length() > 0)
	{
		std::vector<UString> experimentalMods = score.experimentalModsConVars.split(";");
		for (int i=0; i<experimentalMods.size(); i++)
		{
			if (experimentalMods[i].length() > 0)
			{
				ConVar *cvar = convar->getConVarByName(experimentalMods[i], false);
				if (cvar != NULL)
					m_enabledExperimentalMods.push_back(cvar);
			}
		}
	}
}

void OsuRankingScreen::setBeatmapInfo(OsuBeatmap *beatmap, OsuDatabaseBeatmap *diff2)
{
	m_songInfo->setFromBeatmap(beatmap, diff2);
	m_songInfo->setPlayer(m_bIsUnranked ? "McOsu" : cv::name.getString());

	// round all here to 2 decimal places
	m_fSpeedMultiplier = std::round(osu->getSpeedMultiplier() * 100.0f) / 100.0f;
	m_fCS = std::round(beatmap->getCS() * 100.0f) / 100.0f;
	m_fAR = std::round(OsuGameRules::getApproachRateForSpeedMultiplier(beatmap) * 100.0f) / 100.0f;
	m_fOD = std::round(OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap) * 100.0f) / 100.0f;
	m_fHP = std::round(beatmap->getHP() * 100.0f) / 100.0f;
}

void OsuRankingScreen::updateLayout()
{
	OsuScreenBackable::updateLayout();

	const float uiScale = cv::osu::ui_scale.getFloat();

	m_container->setSize(osu->getVirtScreenSize());

	m_rankingTitle->setImage(osu->getSkin()->getRankingTitle());
	m_rankingTitle->setScale(Osu::getImageScale(m_rankingTitle->getImage(), 75.0f) * uiScale, Osu::getImageScale(m_rankingTitle->getImage(), 75.0f) * uiScale);
	m_rankingTitle->setSize(m_rankingTitle->getImage()->getWidth()*m_rankingTitle->getScale().x, m_rankingTitle->getImage()->getHeight()*m_rankingTitle->getScale().y);
	m_rankingTitle->setRelPos(m_container->getSize().x - m_rankingTitle->getSize().x - osu->getUIScale(20.0f), 0);

	m_songInfo->setSize(osu->getVirtScreenWidth(), std::max(m_songInfo->getMinimumHeight(), m_rankingTitle->getSize().y*cv::osu::rankingscreen_topbar_height_percent.getFloat()));

	m_rankings->setSize(osu->getVirtScreenSize().x + 2, osu->getVirtScreenSize().y - m_songInfo->getSize().y + 3);
	m_rankings->setRelPosY(m_songInfo->getSize().y - 1);
	m_container->update_pos();

	// NOTE: no uiScale for rankingPanel and rankingGrade, doesn't really work due to legacy layout expectations
	const Vector2 hardcodedOsuRankingPanelImageSize = Vector2(622, 505) * (osu->getSkin()->isRankingPanel2x() ? 2.0f : 1.0f);
	m_rankingPanel->setImage(osu->getSkin()->getRankingPanel());
	m_rankingPanel->setScale(Osu::getImageScale(hardcodedOsuRankingPanelImageSize, 317.0f), Osu::getImageScale(hardcodedOsuRankingPanelImageSize, 317.0f));
	m_rankingPanel->setSize(std::max(hardcodedOsuRankingPanelImageSize.x*m_rankingPanel->getScale().x, m_rankingPanel->getImage()->getWidth()*m_rankingPanel->getScale().x), std::max(hardcodedOsuRankingPanelImageSize.y*m_rankingPanel->getScale().y, m_rankingPanel->getImage()->getHeight()*m_rankingPanel->getScale().y));

	m_rankingIndex->setSize(m_rankings->getSize().x + 2, osu->getVirtScreenHeight()*0.07f * uiScale);
	m_rankingIndex->setBackgroundColor(0xff745e13);
	m_rankingIndex->setRelPosY(m_rankings->getSize().y + 1);

	m_rankingBottom->setSize(m_rankings->getSize().x + 2, osu->getVirtScreenHeight()*0.2f);
	m_rankingBottom->setRelPosY(m_rankingIndex->getRelPos().y + m_rankingIndex->getSize().y);

	m_rankingScrollDownInfoButton->setSize(m_container->getSize().x*0.2f * uiScale, m_container->getSize().y*0.1f * uiScale);
	m_rankingScrollDownInfoButton->setRelPos(m_container->getPos().x + m_container->getSize().x/2 - m_rankingScrollDownInfoButton->getSize().x/2, m_container->getSize().y - m_rankingScrollDownInfoButton->getSize().y);

	setGrade(m_grade);

	m_container->update_pos();
	m_rankings->getContainer()->update_pos();
	m_rankings->setScrollSizeToContent(0);
}

void OsuRankingScreen::onBack()
{
	soundEngine->play(osu->getSkin()->getMenuClick());

	// stop applause sound
	if (osu->getSkin()->getApplause() != NULL && osu->getSkin()->getApplause()->isPlaying())
		soundEngine->stop(osu->getSkin()->getApplause());

	osu->toggleRankingScreen();
}

void OsuRankingScreen::onScrollDownClicked()
{
	m_rankings->scrollToBottom();
}

void OsuRankingScreen::setGrade(OsuScore::GRADE grade)
{
	m_grade = grade;

	Vector2 hardcodedOsuRankingGradeImageSize = Vector2(369, 422);
	switch (grade)
	{
	case OsuScore::GRADE::GRADE_XH:
		hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingXH2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(osu->getSkin()->getRankingXH());
		break;
	case OsuScore::GRADE::GRADE_SH:
		hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingSH2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(osu->getSkin()->getRankingSH());
		break;
	case OsuScore::GRADE::GRADE_X:
		hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingX2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(osu->getSkin()->getRankingX());
		break;
	case OsuScore::GRADE::GRADE_S:
		hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingS2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(osu->getSkin()->getRankingS());
		break;
	case OsuScore::GRADE::GRADE_A:
		hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingA2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(osu->getSkin()->getRankingA());
		break;
	case OsuScore::GRADE::GRADE_B:
		hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingB2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(osu->getSkin()->getRankingB());
		break;
	case OsuScore::GRADE::GRADE_C:
		hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingC2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(osu->getSkin()->getRankingC());
		break;
	default:
		hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingD2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(osu->getSkin()->getRankingD());
		break;
	}

	const float uiScale = /*cv::osu::ui_scale.getFloat()*/1.0f; // NOTE: no uiScale for rankingPanel and rankingGrade, doesn't really work due to legacy layout expectations

	const float rankingGradeImageScale = Osu::getImageScale(hardcodedOsuRankingGradeImageSize, 230.0f) * uiScale;
	m_rankingGrade->setScale(rankingGradeImageScale, rankingGradeImageScale);
	m_rankingGrade->setSize(m_rankingGrade->getImage()->getWidth()*m_rankingGrade->getScale().x, m_rankingGrade->getImage()->getHeight()*m_rankingGrade->getScale().y);
	m_rankingGrade->setRelPos(
		m_rankings->getSize().x - osu->getUIScale(120) - m_rankingGrade->getImage()->getWidth()*m_rankingGrade->getScale().x/2.0f,
		-m_rankings->getRelPos().y + osu->getUIScale(osu->getSkin()->getVersion() > 1.0f ? 200 : 170) - m_rankingGrade->getImage()->getHeight()*m_rankingGrade->getScale().x/2.0f
	);
}

void OsuRankingScreen::setIndex(int index)
{
	if (!cv::osu::scores_enabled.getBool())
		index = -1;

	if (index > -1)
	{
		m_rankingIndex->setText(UString::format("You achieved the #%i score on local rankings!", (index+1)));
		m_rankingIndex->setVisible2(true);
		m_rankingBottom->setVisible2(true);
		m_rankingScrollDownInfoButton->setVisible2(index < 1); // only show button if we made a new highscore
	}
	else
	{
		m_rankingIndex->setVisible2(false);
		m_rankingBottom->setVisible2(false);
		m_rankingScrollDownInfoButton->setVisible2(false);
	}
}

UString OsuRankingScreen::getPPString()
{
	return UString::format("%ipp", (int)(std::round(m_fPPv2)));
}

Vector2 OsuRankingScreen::getPPPosRaw()
{
	const UString ppString = getPPString();
	float ppStringWidth = osu->getTitleFont()->getStringWidth(ppString);
	return Vector2(m_rankingGrade->getPos().x, 0) + Vector2(m_rankingGrade->getSize().x/2 - ppStringWidth/2, m_rankings->getScrollPosY() + osu->getUIScale(400) + osu->getTitleFont()->getHeight()/2);
}

Vector2 OsuRankingScreen::getPPPosCenterRaw()
{
	return Vector2(m_rankingGrade->getPos().x, 0) + Vector2(m_rankingGrade->getSize().x/2, m_rankings->getScrollPosY() + osu->getUIScale(400) + osu->getTitleFont()->getHeight()/2);
}
