//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score/results/ranking screen
//
// $NoKeywords: $osuss
//===============================================================================//

#include "OsuRankingScreen.h"

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

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuScore.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuSongBrowser2.h"
#include "OsuTooltipOverlay.h"
#include "OsuGameRules.h"

#include "OsuUIRankingScreenInfoLabel.h"
#include "OsuUIRankingScreenRankingPanel.h"

ConVar osu_rankingscreen_topbar_height_percent("osu_rankingscreen_topbar_height_percent", 0.785f);
ConVar osu_rankingscreen_pp("osu_rankingscreen_pp", true);
ConVar osu_draw_rankingscreen_background_image("osu_draw_rankingscreen_background_image", true);

OsuRankingScreen::OsuRankingScreen(Osu *osu) : OsuScreenBackable(osu)
{
	m_container = new CBaseUIContainer(0, 0, 0, 0, "");
	m_rankings = new CBaseUIScrollView(0, 0, 0, 0, "");
	m_rankings->setHorizontalScrolling(false);
	m_rankings->setVerticalScrolling(true);
	m_rankings->setDrawFrame(false);
	m_rankings->setDrawBackground(false);
	m_rankings->setDrawScrollbars(false);
	m_container->addBaseUIElement(m_rankings);

	m_songInfo = new OsuUIRankingScreenInfoLabel(m_osu, 5, 5, 0, 0, "");
	m_container->addBaseUIElement(m_songInfo);

	m_rankingTitle = new CBaseUIImage(m_osu->getSkin()->getRankingTitle()->getName(), 0, 0, 0, 0, "");
	m_rankingTitle->setDrawBackground(false);
	m_rankingTitle->setDrawFrame(false);
	m_container->addBaseUIElement(m_rankingTitle);

	m_rankingPanel = new OsuUIRankingScreenRankingPanel(osu);
	m_rankingPanel->setDrawBackground(false);
	m_rankingPanel->setDrawFrame(false);
	m_rankings->getContainer()->addBaseUIElement(m_rankingPanel);

	m_rankingGrade = new CBaseUIImage(m_osu->getSkin()->getRankingA()->getName(), 0, 0, 0, 0, "");
	m_rankingGrade->setDrawBackground(false);
	m_rankingGrade->setDrawFrame(false);
	m_rankings->getContainer()->addBaseUIElement(m_rankingGrade);

	setGrade(OsuScore::GRADE::GRADE_D);

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
}

OsuRankingScreen::~OsuRankingScreen()
{
	SAFE_DELETE(m_container);
}

void OsuRankingScreen::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// draw background image
	if (osu_draw_rankingscreen_background_image.getBool())
		OsuSongBrowser2::drawSelectedBeatmapBackgroundImage(g, m_osu);

	m_rankings->draw(g);

	// draw active mods
	Vector2 modPos = m_rankingGrade->getPos() + Vector2(m_rankingGrade->getSize().x*0.925f, m_rankingGrade->getSize().y*0.7f);
	if (m_osu->getModSS())
		drawModImage(g, m_osu->getSkin()->getSelectionModPerfect(), modPos);
	else if (m_osu->getModSD())
		drawModImage(g, m_osu->getSkin()->getSelectionModSuddenDeath(), modPos);
	if (m_osu->getModEZ())
		drawModImage(g, m_osu->getSkin()->getSelectionModEasy(), modPos);
	if (m_osu->getModHD())
		drawModImage(g, m_osu->getSkin()->getSelectionModHidden(), modPos);
	if (m_osu->getModHR())
		drawModImage(g, m_osu->getSkin()->getSelectionModHardRock(), modPos);
	if (m_osu->getModNC())
		drawModImage(g, m_osu->getSkin()->getSelectionModNightCore(), modPos);
	else if (m_osu->getModDT())
		drawModImage(g, m_osu->getSkin()->getSelectionModDoubleTime(), modPos);
	if (m_osu->getModNM())
		drawModImage(g, m_osu->getSkin()->getSelectionModNightmare(), modPos);
	if (m_osu->getModTarget())
		drawModImage(g, m_osu->getSkin()->getSelectionModTarget(), modPos);
	if (m_osu->getModSpunout())
		drawModImage(g, m_osu->getSkin()->getSelectionModSpunOut(), modPos);
	if (m_osu->getModRelax())
		drawModImage(g, m_osu->getSkin()->getSelectionModRelax(), modPos);
	if (m_osu->getModNF())
		drawModImage(g, m_osu->getSkin()->getSelectionModNoFail(), modPos);
	if (m_osu->getModHT())
		drawModImage(g, m_osu->getSkin()->getSelectionModHalfTime(), modPos);
	if (m_osu->getModAutopilot())
		drawModImage(g, m_osu->getSkin()->getSelectionModAutopilot(), modPos);
	if (m_osu->getModAuto())
		drawModImage(g, m_osu->getSkin()->getSelectionModAutoplay(), modPos);

	// draw pp
	if (osu_rankingscreen_pp.getBool())
	{
		UString ppString = getPPString();
		Vector2 ppPos = getPPPosRaw() + m_vPPCursorMagnetAnimation;
		g->pushTransform();
		g->translate((int)ppPos.x + 2, (int)ppPos.y + 2);
		g->setColor(0xff000000);
		g->drawString(m_osu->getTitleFont(), ppString);
		g->translate(-2, -2);
		g->setColor(0xffffffff);
		g->drawString(m_osu->getTitleFont(), ppString);
		g->popTransform();
	}

	// draw top black bar
	g->setColor(0xff000000);
	g->fillRect(0, 0, m_osu->getScreenWidth(), m_rankingTitle->getSize().y*osu_rankingscreen_topbar_height_percent.getFloat());

	m_rankingTitle->draw(g);
	m_songInfo->draw(g);

	OsuScreenBackable::draw(g);
}

void OsuRankingScreen::drawModImage(Graphics *g, OsuSkinImage *image, Vector2 &pos)
{
	g->setColor(0xffffffff);
	image->draw(g, Vector2(pos.x - image->getSize().x/2.0f, pos.y));

	pos.x -= image->getSize().x/2.0f;
}

void OsuRankingScreen::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	m_container->update();

	// tooltip (pp + accuracy + unstable rate)
	if (m_rankingPanel->isMouseInside())
	{
		m_osu->getTooltipOverlay()->begin();
		m_osu->getTooltipOverlay()->addLine(UString::format("%.2fpp", m_fPPv2));
		m_osu->getTooltipOverlay()->addLine("Difficulty:");
		m_osu->getTooltipOverlay()->addLine(UString::format("Stars: %.2f (%.2f aim, %.2f speed)", m_fStarsTomTotal, m_fStarsTomAim, m_fStarsTomSpeed));
		m_osu->getTooltipOverlay()->addLine(UString::format("Speed: %.3gx", m_fSpeedMultiplier));
		m_osu->getTooltipOverlay()->addLine(UString::format("CS:%.4g AR:%.4g OD:%.4g HP:%.4g", m_fCS, m_fAR, m_fOD, m_fHP));
		m_osu->getTooltipOverlay()->addLine("Accuracy:");
		m_osu->getTooltipOverlay()->addLine(UString::format("Error: %.2fms - %.2fms avg", m_fHitErrorAvgMin, m_fHitErrorAvgMax));
		m_osu->getTooltipOverlay()->addLine(UString::format("Unstable Rate: %.2f", m_fUnstableRate));
		m_osu->getTooltipOverlay()->end();
	}

	// frustration multiplier
	Vector2 cursorDelta = getPPPosCenterRaw() - engine->getMouse()->getPos();
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
}

void OsuRankingScreen::setVisible(bool visible)
{
	OsuScreenBackable::setVisible(visible);

	if (m_bVisible)
	{
		m_rankings->scrollToTop();
		updateLayout();
	}
}

void OsuRankingScreen::setScore(OsuScore *score)
{
	m_rankingPanel->setScore(score);
	setGrade(score->getGrade());

	m_fUnstableRate = score->getUnstableRate();
	m_fHitErrorAvgMin = score->getHitErrorAvgMin();
	m_fHitErrorAvgMax = score->getHitErrorAvgMax();
	m_fStarsTomTotal = score->getStarsTomTotal();
	m_fStarsTomAim = score->getStarsTomAim();
	m_fStarsTomSpeed = score->getStarsTomSpeed();
	m_fPPv2 = score->getPPv2();
}

void OsuRankingScreen::setBeatmapInfo(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff)
{
	m_songInfo->setFromBeatmap(beatmap, diff);
	m_songInfo->setPlayer(convar->getConVarByName("name")->getString());

	// round all here to 2 decimal places
	m_fSpeedMultiplier = std::round(m_osu->getSpeedMultiplier() * 100.0f) / 100.0f;
	m_fCS = std::round(beatmap->getCS() * 100.0f) / 100.0f;
	m_fAR = std::round(OsuGameRules::getApproachRateForSpeedMultiplier(beatmap) * 100.0f) / 100.0f;
	m_fOD = std::round(OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap) * 100.0f) / 100.0f;
	m_fHP = std::round(beatmap->getHP() * 100.0f) / 100.0f;
}

void OsuRankingScreen::updateLayout()
{
	OsuScreenBackable::updateLayout();

	m_container->setSize(m_osu->getScreenSize());
	m_rankings->setSize(m_osu->getScreenSize());

	m_songInfo->setSize(m_osu->getScreenWidth(), m_rankingTitle->getSize().y*osu_rankingscreen_topbar_height_percent.getFloat());

	m_rankingTitle->setImage(m_osu->getSkin()->getRankingTitle());
	m_rankingTitle->setScale(Osu::getImageScale(m_osu, m_rankingTitle->getImage(), 75.0f), Osu::getImageScale(m_osu, m_rankingTitle->getImage(), 75.0f));
	m_rankingTitle->setSize(m_rankingTitle->getImage()->getWidth()*m_rankingTitle->getScale().x, m_rankingTitle->getImage()->getHeight()*m_rankingTitle->getScale().y);
	m_rankingTitle->setRelPos(m_container->getSize().x - m_rankingTitle->getSize().x - m_osu->getUIScale(m_osu, 20.0f), 0);

	Vector2 hardcodedOsuRankingPanelImageSize = Vector2(622, 505) * (m_osu->getSkin()->isRankingPanel2x() ? 2.0f : 1.0f);
	m_rankingPanel->setImage(m_osu->getSkin()->getRankingPanel());
	m_rankingPanel->setScale(Osu::getImageScale(m_osu, hardcodedOsuRankingPanelImageSize, 317.0f), Osu::getImageScale(m_osu, hardcodedOsuRankingPanelImageSize, 317.0f));
	m_rankingPanel->setSize(m_rankingPanel->getImage()->getWidth()*m_rankingPanel->getScale().x, m_rankingPanel->getImage()->getHeight()*m_rankingPanel->getScale().y);
	m_rankingPanel->setRelPosY(m_rankingTitle->getSize().y*0.825f);

	setGrade(m_grade);

	m_container->update_pos();
	m_rankings->getContainer()->update_pos();
	m_rankings->setScrollSizeToContent();
}

void OsuRankingScreen::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	// stop applause sound
	if (m_osu->getSkin()->getApplause() != NULL && m_osu->getSkin()->getApplause()->isPlaying())
		engine->getSound()->stop(m_osu->getSkin()->getApplause());

	m_osu->toggleRankingScreen();
}

void OsuRankingScreen::setGrade(OsuScore::GRADE grade)
{
	m_grade = grade;

	Vector2 hardcodedOsuRankingGradeImageSize = Vector2(369, 422);
	switch (grade)
	{
	case OsuScore::GRADE::GRADE_XH:
		hardcodedOsuRankingGradeImageSize *= (m_osu->getSkin()->isRankingXH2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(m_osu->getSkin()->getRankingXH());
		break;
	case OsuScore::GRADE::GRADE_SH:
		hardcodedOsuRankingGradeImageSize *= (m_osu->getSkin()->isRankingSH2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(m_osu->getSkin()->getRankingSH());
		break;
	case OsuScore::GRADE::GRADE_X:
		hardcodedOsuRankingGradeImageSize *= (m_osu->getSkin()->isRankingX2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(m_osu->getSkin()->getRankingX());
		break;
	case OsuScore::GRADE::GRADE_S:
		hardcodedOsuRankingGradeImageSize *= (m_osu->getSkin()->isRankingS2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(m_osu->getSkin()->getRankingS());
		break;
	case OsuScore::GRADE::GRADE_A:
		hardcodedOsuRankingGradeImageSize *= (m_osu->getSkin()->isRankingA2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(m_osu->getSkin()->getRankingA());
		break;
	case OsuScore::GRADE::GRADE_B:
		hardcodedOsuRankingGradeImageSize *= (m_osu->getSkin()->isRankingB2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(m_osu->getSkin()->getRankingB());
		break;
	case OsuScore::GRADE::GRADE_C:
		hardcodedOsuRankingGradeImageSize *= (m_osu->getSkin()->isRankingC2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(m_osu->getSkin()->getRankingC());
		break;
	default:
		hardcodedOsuRankingGradeImageSize *= (m_osu->getSkin()->isRankingD2x() ? 2.0f : 1.0f);
		m_rankingGrade->setImage(m_osu->getSkin()->getRankingD());
		break;
	}
	m_rankingGrade->setScale(Osu::getImageScale(m_osu, hardcodedOsuRankingGradeImageSize, 230.0f), Osu::getImageScale(m_osu, hardcodedOsuRankingGradeImageSize, 230.0f));
	m_rankingGrade->setSize(m_rankingGrade->getImage()->getWidth()*m_rankingGrade->getScale().x, m_rankingGrade->getImage()->getHeight()*m_rankingGrade->getScale().y);
	m_rankingGrade->setRelPos(m_rankings->getSize().x - m_rankingGrade->getSize().x - m_osu->getUIScale(m_osu, 5), m_rankingPanel->getRelPos().y + m_osu->getUIScale(m_osu, 5));
}

UString OsuRankingScreen::getPPString()
{
	return UString::format("%ipp", (int)(std::round(m_fPPv2)));
}

Vector2 OsuRankingScreen::getPPPosRaw()
{
	UString ppString = getPPString();
	float ppStringWidth = m_osu->getTitleFont()->getStringWidth(ppString);
	return m_rankingGrade->getPos() + Vector2(m_rankingGrade->getSize().x/2 - ppStringWidth/2, m_rankingGrade->getSize().y + m_osu->getTitleFont()->getHeight() + 25);
}

Vector2 OsuRankingScreen::getPPPosCenterRaw()
{
	return m_rankingGrade->getPos() + Vector2(m_rankingGrade->getSize().x/2, m_rankingGrade->getSize().y + m_osu->getTitleFont()->getHeight()/2 + 25);
}
