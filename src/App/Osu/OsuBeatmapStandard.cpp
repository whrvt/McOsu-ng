//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		osu!standard circle clicking
//
// $NoKeywords: $osustd
//===============================================================================//

#include "OsuBeatmapStandard.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "Environment.h"
#include "SoundEngine.h"
#include "AnimationHandler.h"
#include "Mouse.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuMultiplayer.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuGameRules.h"
#include "OsuModFPoSu.h"
#include "OsuDatabase.h"
#include "OsuSongBrowser2.h"
#include "OsuNotificationOverlay.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuDifficultyCalculator.h"
#include "OsuBackgroundStarCacheLoader.h"
#include "OsuReplay.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"
#include "OsuSpinner.h"

#include <cstring>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <utility>
namespace cv::osu {
ConVar draw_followpoints("osu_draw_followpoints", true, FCVAR_NONE);
ConVar draw_reverse_order("osu_draw_reverse_order", false, FCVAR_NONE);
ConVar draw_playfield_border("osu_draw_playfield_border", true, FCVAR_NONE);

ConVar stacking("osu_stacking", true, FCVAR_NONE, "Whether to use stacking calculations or not");
ConVar stacking_leniency_override("osu_stacking_leniency_override", -1.0f, FCVAR_NONE);

ConVar auto_snapping_strength("osu_auto_snapping_strength", 1.0f, FCVAR_NONE, "How many iterations of quadratic interpolation to use, more = snappier, 0 = linear");
ConVar auto_cursordance("osu_auto_cursordance", false, FCVAR_NONE);
ConVar autopilot_snapping_strength("osu_autopilot_snapping_strength", 2.0f, FCVAR_NONE, "How many iterations of quadratic interpolation to use, more = snappier, 0 = linear");
ConVar autopilot_lenience("osu_autopilot_lenience", 0.75f, FCVAR_NONE);

ConVar followpoints_clamp("osu_followpoints_clamp", false, FCVAR_NONE, "clamp followpoint approach time to current circle approach time (instead of using the hardcoded default 800 ms raw)");
ConVar followpoints_anim("osu_followpoints_anim", false, FCVAR_NONE, "scale + move animation while fading in followpoints (osu only does this when its internal default skin is being used)");
ConVar followpoints_connect_combos("osu_followpoints_connect_combos", false, FCVAR_NONE, "connect followpoints even if a new combo has started");
ConVar followpoints_connect_spinners("osu_followpoints_connect_spinners", false, FCVAR_NONE, "connect followpoints even through spinners");
ConVar followpoints_approachtime("osu_followpoints_approachtime", 800.0f, FCVAR_NONE);
ConVar followpoints_scale_multiplier("osu_followpoints_scale_multiplier", 1.0f, FCVAR_NONE);
ConVar followpoints_separation_multiplier("osu_followpoints_separation_multiplier", 1.0f, FCVAR_NONE);

ConVar number_scale_multiplier("osu_number_scale_multiplier", 1.0f, FCVAR_NONE);

ConVar playfield_mirror_horizontal("osu_playfield_mirror_horizontal", false, FCVAR_NONE);
ConVar playfield_mirror_vertical("osu_playfield_mirror_vertical", false, FCVAR_NONE);
ConVar playfield_rotation("osu_playfield_rotation", 0.0f, FCVAR_NONE, "rotates the entire playfield by this many degrees");
ConVar playfield_stretch_x("osu_playfield_stretch_x", 0.0f, FCVAR_NONE, "offsets/multiplies all hitobject coordinates by it (0 = default 1x playfield size, -1 = on a line, -0.5 = 0.5x playfield size, 0.5 = 1.5x playfield size)");
ConVar playfield_stretch_y("osu_playfield_stretch_y", 0.0f, FCVAR_NONE, "offsets/multiplies all hitobject coordinates by it (0 = default 1x playfield size, -1 = on a line, -0.5 = 0.5x playfield size, 0.5 = 1.5x playfield size)");
ConVar playfield_circular("osu_playfield_circular", false, FCVAR_NONE, "whether the playfield area should be transformed from a rectangle into a circle/disc/oval");

ConVar drain_lazer_health_min("osu_drain_lazer_health_min", 0.95f, FCVAR_NONE);
ConVar drain_lazer_health_mid("osu_drain_lazer_health_mid", 0.70f, FCVAR_NONE);
ConVar drain_lazer_health_max("osu_drain_lazer_health_max", 0.30f, FCVAR_NONE);

ConVar mod_wobble("osu_mod_wobble", false, FCVAR_NONE);
ConVar mod_wobble2("osu_mod_wobble2", false, FCVAR_NONE);
ConVar mod_wobble_strength("osu_mod_wobble_strength", 25.0f, FCVAR_NONE);
ConVar mod_wobble_frequency("osu_mod_wobble_frequency", 1.0f, FCVAR_NONE);
ConVar mod_wobble_rotation_speed("osu_mod_wobble_rotation_speed", 1.0f, FCVAR_NONE);
ConVar mod_jigsaw2("osu_mod_jigsaw2", false, FCVAR_NONE);
ConVar mod_jigsaw_followcircle_radius_factor("osu_mod_jigsaw_followcircle_radius_factor", 0.0f, FCVAR_NONE);
ConVar mod_shirone("osu_mod_shirone", false, FCVAR_NONE);
ConVar mod_shirone_combo("osu_mod_shirone_combo", 20.0f, FCVAR_NONE);
ConVar mod_mafham_render_chunksize("osu_mod_mafham_render_chunksize", 15, FCVAR_NONE, "render this many hitobjects per frame chunk into the scene buffer (spreads rendering across many frames to minimize lag)");

ConVar mandala("osu_mandala", false, FCVAR_NONE);
ConVar mandala_num("osu_mandala_num", 7, FCVAR_NONE);

ConVar debug_hiterrorbar_misaims("osu_debug_hiterrorbar_misaims", false, FCVAR_NONE);

ConVar pp_live_timeout("osu_pp_live_timeout", 1.0f, FCVAR_NONE, "show message that we're still calculating stars after this many seconds, on the first start of the beatmap");
}

















OsuBeatmapStandard::OsuBeatmapStandard() : OsuBeatmap()
{
	m_bIsSpinnerActive = false;

	m_fPlayfieldRotation = 0.0f;
	m_fScaleFactor = 1.0f;

	m_fXMultiplier = 1.0f;
	m_fNumberScale = 1.0f;
	m_fHitcircleOverlapScale = 1.0f;
	m_fRawHitcircleDiameter = 27.35f * 2.0f;
	m_fHitcircleDiameter = 0.0f;
	m_fSliderFollowCircleDiameter = 0.0f;
	m_fRawSliderFollowCircleDiameter = 0.0f;

	m_iAutoCursorDanceIndex = 0;

	m_fAimStars = 0.0f;
	m_fAimSliderFactor = 0.0f;
	m_fAimDifficultSliders = 0.0f;
	m_fAimDifficultStrains = 0.0f;
	m_fSpeedStars = 0.0f;
	m_fSpeedNotes = 0.0f;
	m_fSpeedDifficultStrains = 0.0f;
	m_starCacheLoader = new OsuBackgroundStarCacheLoader(this);
	m_fStarCacheTime = 0.0f;

	m_bWasHREnabled = false;
	m_fPrevHitCircleDiameter = 0.0f;
	m_bWasHorizontalMirrorEnabled = false;
	m_bWasVerticalMirrorEnabled = false;
	m_bWasEZEnabled = false;
	m_bWasMafhamEnabled = false;
	m_fPrevPlayfieldRotationFromConVar = 0.0f;
	m_fPrevPlayfieldStretchX = 0.0f;
	m_fPrevPlayfieldStretchY = 0.0f;
	m_fPrevHitCircleDiameterForStarCache = 1.0f;
	m_fPrevSpeedForStarCache = 1.0f;

	m_bIsPreLoading = true;
	m_iPreLoadingIndex = 0;

	m_mafhamActiveRenderTarget = NULL;
	m_mafhamFinishedRenderTarget = NULL;
	m_bMafhamRenderScheduled = true;
	m_iMafhamHitObjectRenderIndex = 0;
	m_iMafhamPrevHitObjectIndex = 0;
	m_iMafhamActiveRenderHitObjectIndex = 0;
	m_iMafhamFinishedRenderHitObjectIndex = 0;
	m_bInMafhamRenderChunk = false;

	m_iMandalaIndex = 0;

	// convar refs















}

OsuBeatmapStandard::~OsuBeatmapStandard()
{
	m_starCacheLoader->kill();

	// spec: resourceManager->destroyResource should take care of this? why do we have to manually block here
	// if (resourceManager->isLoadingResource(m_starCacheLoader))
	// 	while (!m_starCacheLoader->isAsyncReady()) {;}

	resourceManager->destroyResource(m_starCacheLoader);
}

void OsuBeatmapStandard::draw()
{
	OsuBeatmap::draw();
	if (!canDraw()) return;
	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	// draw playfield border
	if (cv::osu::draw_playfield_border.getBool() && !cv::osu::stdrules::mod_fps.getBool())
		osu->getHUD()->drawPlayfieldBorder(m_vPlayfieldCenter, m_vPlayfieldSize, m_fHitcircleDiameter);

	// draw hiterrorbar
	if (!cv::osu::fposu::mod_fposu.getBool())
		osu->getHUD()->drawHitErrorBar(this);

	// draw first person crosshair
	if (cv::osu::stdrules::mod_fps.getBool())
	{
		const int length = 15;
		Vector2 center = osuCoords2Pixels(Vector2(OsuGameRules::OSU_COORD_WIDTH/2, OsuGameRules::OSU_COORD_HEIGHT/2));
		g->setColor(0xff777777);
		g->drawLine(center.x, (int)(center.y - length), center.x, (int)(center.y + length + 1));
		g->drawLine((int)(center.x - length), center.y, (int)(center.x + length + 1), center.y);
	}

	// draw followpoints
	if (cv::osu::draw_followpoints.getBool() && !cv::osu::stdrules::mod_mafham.getBool())
		drawFollowPoints();

	// draw all hitobjects in reverse
	if (cv::osu::draw_hitobjects.getBool())
		drawHitObjects();

	if (cv::osu::mandala.getBool())
	{
		for (int i=0; i<cv::osu::mandala_num.getInt(); i++)
		{
			m_iMandalaIndex = i;
			drawHitObjects();
		}
	}

	/*
	if (m_bFailed)
	{
		const float failTimePercentInv = 1.0f - m_fFailAnim; // goes from 0 to 1 over the duration of osu_fail_time

		Vector2 playfieldBorderTopLeft = Vector2((int)(m_vPlayfieldCenter.x - m_vPlayfieldSize.x/2 - m_fHitcircleDiameter/2), (int)(m_vPlayfieldCenter.y - m_vPlayfieldSize.y/2 - m_fHitcircleDiameter/2));
		Vector2 playfieldBorderSize = Vector2((int)(m_vPlayfieldSize.x + m_fHitcircleDiameter), (int)(m_vPlayfieldSize.y + m_fHitcircleDiameter));

		g->setColor(0xff000000);
		g->setAlpha(failTimePercentInv);
		g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y, playfieldBorderSize.x, playfieldBorderSize.y);
	}
	*/

	// debug stuff
	if (cv::osu::debug_hiterrorbar_misaims.getBool())
	{
		for (int i=0; i<m_misaimObjects.size(); i++)
		{
			g->setColor(0xbb00ff00);
			Vector2 pos = osuCoords2Pixels(m_misaimObjects[i]->getRawPosAt(0));
			g->fillRect(pos.x - 50, pos.y - 50, 100, 100);
		}
	}
}

void OsuBeatmapStandard::drawInt()
{
	OsuBeatmap::drawInt();
	if (!canDraw()) return;

	if (isLoadingStarCache() && engine->getTime() > m_fStarCacheTime)
	{
		float progressPercent = 0.0f;
		if (m_hitobjects.size() > 0)
			progressPercent = (float)m_starCacheLoader->getProgress() / (float)m_hitobjects.size();

		g->setColor(0x44ffffff);
		UString loadingMessage = UString::format("Calculating stars for realtime pp/stars (%i%%) ...", (int)(progressPercent*100.0f));
		UString loadingMessage2 = "(To get rid of this delay, disable [Draw Statistics: pp/Stars***])";
		g->pushTransform();
		{
			g->translate((int)(osu->getVirtScreenWidth()/2 - osu->getSubTitleFont()->getStringWidth(loadingMessage)/2), osu->getVirtScreenHeight() - osu->getSubTitleFont()->getHeight() - 25);
			g->drawString(osu->getSubTitleFont(), loadingMessage);
		}
		g->popTransform();
		g->pushTransform();
		{
			g->translate((int)(osu->getVirtScreenWidth()/2 - osu->getSubTitleFont()->getStringWidth(loadingMessage2)/2), osu->getVirtScreenHeight() - 15);
			g->drawString(osu->getSubTitleFont(), loadingMessage2);
		}
		g->popTransform();
	}
	else if (osu->isInMultiplayer() && osu->getMultiplayer()->isWaitingForPlayers())
	{
		if (!m_bIsPreLoading && !isLoadingStarCache()) // usability
		{
			g->setColor(0x44ffffff);
			UString loadingMessage = "Waiting for players ...";
			g->pushTransform();
			{
				g->translate((int)(osu->getVirtScreenWidth()/2 - osu->getSubTitleFont()->getStringWidth(loadingMessage)/2), osu->getVirtScreenHeight() - osu->getSubTitleFont()->getHeight() - 15);
				g->drawString(osu->getSubTitleFont(), loadingMessage);
			}
			g->popTransform();
		}
	}
}

void OsuBeatmapStandard::draw3D()
{
	OsuBeatmap::draw3D();
	if (!canDraw()) return;
	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	updateHitobjectMetrics(); // needed for raw hitcircleDiameter

	// draw all hitobjects in reverse
	if (cv::osu::draw_hitobjects.getBool())
	{
		const long curPos = m_iCurMusicPosWithOffsets;
		const long pvs = getPVS();
		const bool usePVS = cv::osu::pvs.getBool();

		if (!cv::osu::draw_reverse_order.getBool())
		{
			for (int i=(int)m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
			{
				// PVS optimization (reversed)
				if (usePVS)
				{
					if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
						break;
					if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
						continue;
				}

				m_hitobjectsSortedByEndTime[i]->draw3D();
			}
		}
		else
		{
			for (int i=0; std::cmp_less(i,m_hitobjectsSortedByEndTime.size()); i++)
			{
				// PVS optimization
				if (usePVS)
				{
					if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
						continue;
					if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
						break;
				}

				m_hitobjectsSortedByEndTime[i]->draw3D();
			}
		}
	}
}

void OsuBeatmapStandard::draw3D2()
{
	OsuBeatmap::draw3D2();
	if (!canDraw()) return;
	if (isLoading()) return; // only start drawing the rest of the playfield if everything has loaded

	updateHitobjectMetrics(); // needed for raw hitcircleDiameter

	// TODO: draw followpoints
	{
		/*
		if (cv::osu::draw_followpoints.getBool() && !cv::osu::stdrules::mod_mafham.getBool())
			drawFollowPoints();
		*/
	}

	// draw all hitobjects in reverse
	if (cv::osu::draw_hitobjects.getBool())
	{
		const long curPos = m_iCurMusicPosWithOffsets;
		const long pvs = getPVS();
		const bool usePVS = cv::osu::pvs.getBool();

		for (int i=0; std::cmp_less(i,m_hitobjectsSortedByEndTime.size()); i++)
		{
			// PVS optimization
			if (usePVS)
			{
				if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
					continue;
				if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
					break;
			}

			m_hitobjectsSortedByEndTime[i]->draw3D2();
		}
	}
}

void OsuBeatmapStandard::drawFollowPoints()
{
	OsuSkin *skin = osu->getSkin();

	const long curPos = m_iCurMusicPosWithOffsets;

	// I absolutely hate this, followpoints can be abused for cheesing high AR reading since they always fade in with a fixed 800 ms custom approach time
	// capping it at the current approach rate seems sensible, but unfortunately that's not what osu is doing
	// it was non-osu-compliant-clamped since this client existed, but let's see how many people notice a change after all this time (26.02.2020)
	const long followPointApproachTime = cv::osu::followpoints_clamp.getBool() ? std::min((long)OsuGameRules::getApproachTime(this), (long)cv::osu::followpoints_approachtime.getFloat()) : (long)cv::osu::followpoints_approachtime.getFloat();

	const bool followPointsConnectCombos = cv::osu::followpoints_connect_combos.getBool();
	const bool followPointsConnectSpinners = cv::osu::followpoints_connect_spinners.getBool();
	const float followPointSeparationMultiplier = std::max(cv::osu::followpoints_separation_multiplier.getFloat(), 0.1f);
	const float followPointPrevFadeTime = cv::osu::followpoints_prevfadetime.getFloat();
	const float followPointScaleMultiplier = cv::osu::followpoints_scale_multiplier.getFloat();

	// include previous object in followpoints
	int lastObjectIndex = -1;

	for (int index=m_iPreviousFollowPointObjectIndex; index<m_hitobjects.size(); index++)
	{
		lastObjectIndex = index-1;

		// ignore future spinners
		if ((m_hitobjects[index]->getType() == OsuHitObject::SPINNER) && !followPointsConnectSpinners) // if this is a spinner
		{
			lastObjectIndex = -1;
			continue;
		}

		// NOTE: "m_hitobjects[index]->getComboNumber() != 1" breaks (not literally) on new combos
		// NOTE: the "getComboNumber()" call has been replaced with isEndOfCombo() because of osu_ignore_beatmap_combo_numbers and osu_number_max
		const bool isCurrentHitObjectNewCombo = (lastObjectIndex >= 0 ? m_hitobjects[lastObjectIndex]->isEndOfCombo() : false);
		const bool isCurrentHitObjectSpinner = (lastObjectIndex >= 0 && followPointsConnectSpinners ? (m_hitobjects[lastObjectIndex]->getType() == OsuHitObject::SPINNER) : false);
		if (lastObjectIndex >= 0 && (!isCurrentHitObjectNewCombo || followPointsConnectCombos || (isCurrentHitObjectSpinner && followPointsConnectSpinners)))
		{
			// ignore previous spinners
			if ((m_hitobjects[lastObjectIndex]->getType() == OsuHitObject::SPINNER) && !followPointsConnectSpinners) // if this is a spinner
			{
				lastObjectIndex = -1;
				continue;
			}

			// get time & pos of the last and current object
			const long lastObjectEndTime = m_hitobjects[lastObjectIndex]->getTime() + m_hitobjects[lastObjectIndex]->getDuration() + 1;
			const long objectStartTime = m_hitobjects[index]->getTime();
			const long timeDiff = objectStartTime - lastObjectEndTime;

			const Vector2 startPoint = osuCoords2Pixels(m_hitobjects[lastObjectIndex]->getRawPosAt(lastObjectEndTime));
			const Vector2 endPoint = osuCoords2Pixels(m_hitobjects[index]->getRawPosAt(objectStartTime));

			const float xDiff = endPoint.x - startPoint.x;
			const float yDiff = endPoint.y - startPoint.y;
			const Vector2 diff = endPoint - startPoint;
			const float dist = std::round(diff.length() * 100.0f) / 100.0f; // rounded to avoid flicker with playfield rotations

			// draw all points between the two objects
			const int followPointSeparation = Osu::getUIScale(32) * followPointSeparationMultiplier;
			for (int j=(int)(followPointSeparation * 1.5f); j<(dist - followPointSeparation); j+=followPointSeparation)
			{
				const float animRatio = ((float)j / dist);

				const Vector2 animPosStart = startPoint + (animRatio - 0.1f) * diff;
				const Vector2 finalPos = startPoint + animRatio * diff;

				const long fadeInTime = (long)(lastObjectEndTime + animRatio * timeDiff) - followPointApproachTime;
				const long fadeOutTime = (long)(lastObjectEndTime + animRatio * timeDiff);

				// draw
				float alpha = 1.0f;
				float followAnimPercent = std::clamp<float>((float)(curPos - fadeInTime) / (float)followPointPrevFadeTime, 0.0f, 1.0f);
				followAnimPercent = -followAnimPercent*(followAnimPercent - 2.0f); // quad out

				// NOTE: only internal osu default skin uses scale + move transforms here, it is impossible to achieve this effect with user skins
				const float scale = cv::osu::followpoints_anim.getBool() ? 1.5f - 0.5f*followAnimPercent : 1.0f;
				const Vector2 followPos = cv::osu::followpoints_anim.getBool() ? animPosStart + (finalPos - animPosStart)*followAnimPercent : finalPos;

				// bullshit performance optimization: only draw followpoints if within screen bounds (plus a bit of a margin)
				// there is only one beatmap where this matters currently: https://osu.ppy.sh/b/1145513
				if (followPos.x < -osu->getVirtScreenWidth() || followPos.x > osu->getVirtScreenWidth()*2 || followPos.y < -osu->getVirtScreenHeight() || followPos.y > osu->getVirtScreenHeight()*2)
					continue;

				// calculate trail alpha
				if (curPos >= fadeInTime && curPos < fadeOutTime)
				{
					// future trail
					const float delta = curPos - fadeInTime;
					alpha = (float)delta / (float)followPointApproachTime;
				}
				else if (curPos >= fadeOutTime && curPos < (fadeOutTime + (long)followPointPrevFadeTime))
				{
					// previous trail
					const long delta = curPos - fadeOutTime;
					alpha = 1.0f - (float)delta / (float)(followPointPrevFadeTime);
				}
				else
					alpha = 0.0f;

				// draw it
				g->setColor(0xffffffff);
				g->setAlpha(alpha);
				g->pushTransform();
				{
					g->rotate(glm::degrees(glm::atan2(yDiff, xDiff)));

					skin->getFollowPoint2()->setAnimationTimeOffset(fadeInTime);

					// NOTE: getSizeBaseRaw() depends on the current animation time being set correctly beforehand! (otherwise you get incorrect scales, e.g. for animated elements with inconsistent @2x mixed in)
					// the followpoints are scaled by one eighth of the hitcirclediameter (not the raw diameter, but the scaled diameter)
					const float followPointImageScale = ((m_fHitcircleDiameter / 8.0f) / skin->getFollowPoint2()->getSizeBaseRaw().x) * followPointScaleMultiplier;

					skin->getFollowPoint2()->drawRaw(followPos, followPointImageScale*scale);
				}
				g->popTransform();
			}
		}

		// store current index as previous index
		lastObjectIndex = index;

		// iterate up until the "nextest" element
		if (m_hitobjects[index]->getTime() >= curPos + followPointApproachTime)
			break;
	}
}

void OsuBeatmapStandard::drawHitObjects()
{
	const long curPos = m_iCurMusicPosWithOffsets;
	const long pvs = getPVS();
	const bool usePVS = cv::osu::pvs.getBool();

	if (!cv::osu::stdrules::mod_mafham.getBool())
	{
		if (!cv::osu::draw_reverse_order.getBool())
		{
			for (int i=m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
			{
				// PVS optimization (reversed)
				if (usePVS)
				{
					if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
						break;
					if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
						continue;
				}

				m_hitobjectsSortedByEndTime[i]->draw();
			}
		}
		else
		{
			for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
			{
				// PVS optimization
				if (usePVS)
				{
					if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
						continue;
					if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
						break;
				}

				m_hitobjectsSortedByEndTime[i]->draw();
			}
		}
		for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
		{
			// NOTE: to fix mayday simultaneous sliders with increasing endtime getting culled here, would have to switch from m_hitobjectsSortedByEndTime to m_hitobjects
			// PVS optimization
			if (usePVS)
			{
				if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
					continue;
				if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
					break;
			}

			m_hitobjectsSortedByEndTime[i]->draw2();
		}
	}
	else
	{
		const int mafhamRenderLiveSize = cv::osu::stdrules::mod_mafham_render_livesize.getInt();

		if (m_mafhamActiveRenderTarget == NULL)
			m_mafhamActiveRenderTarget = osu->getFrameBuffer();

		if (m_mafhamFinishedRenderTarget == NULL)
			m_mafhamFinishedRenderTarget = osu->getFrameBuffer2();

		// if we have a chunk to render into the scene buffer
		const bool shouldDrawBuffer = (m_hitobjectsSortedByEndTime.size() - m_iCurrentHitObjectIndex) > mafhamRenderLiveSize;
		bool shouldRenderChunk = m_iMafhamHitObjectRenderIndex < m_hitobjectsSortedByEndTime.size() && shouldDrawBuffer;
		if (shouldRenderChunk)
		{
			m_bInMafhamRenderChunk = true;

			m_mafhamActiveRenderTarget->setClearColorOnDraw(m_iMafhamHitObjectRenderIndex == 0);
			m_mafhamActiveRenderTarget->setClearDepthOnDraw(m_iMafhamHitObjectRenderIndex == 0);

			m_mafhamActiveRenderTarget->enable();
			{
				g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_ALPHA);
				{
					int chunkCounter = 0;
					for (int i=m_hitobjectsSortedByEndTime.size()-1 - m_iMafhamHitObjectRenderIndex; i>=0; i--, m_iMafhamHitObjectRenderIndex++)
					{
						chunkCounter++;
						if (chunkCounter > cv::osu::mod_mafham_render_chunksize.getInt())
							break; // continue chunk render in next frame

						if (i <= m_iCurrentHitObjectIndex + mafhamRenderLiveSize) // skip live objects
						{
							m_iMafhamHitObjectRenderIndex = m_hitobjectsSortedByEndTime.size(); // stop chunk render
							break;
						}

						// PVS optimization (reversed)
						if (usePVS)
						{
							if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
							{
								m_iMafhamHitObjectRenderIndex = m_hitobjectsSortedByEndTime.size(); // stop chunk render
								break;
							}
							if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
								continue;
						}

						m_hitobjectsSortedByEndTime[i]->draw();

						m_iMafhamActiveRenderHitObjectIndex = i;
					}
				}
				g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
			}
			m_mafhamActiveRenderTarget->disable();

			m_bInMafhamRenderChunk = false;
		}
		shouldRenderChunk = m_iMafhamHitObjectRenderIndex < m_hitobjectsSortedByEndTime.size() && shouldDrawBuffer;
		if (!shouldRenderChunk && m_bMafhamRenderScheduled)
		{
			// finished, we can now swap the active framebuffer with the one we just finished
			m_bMafhamRenderScheduled = false;

			RenderTarget *temp = m_mafhamFinishedRenderTarget;
			m_mafhamFinishedRenderTarget = m_mafhamActiveRenderTarget;
			m_mafhamActiveRenderTarget = temp;

			m_iMafhamFinishedRenderHitObjectIndex = m_iMafhamActiveRenderHitObjectIndex;
			m_iMafhamActiveRenderHitObjectIndex = m_hitobjectsSortedByEndTime.size(); // reset
		}

		// draw scene buffer
		if (shouldDrawBuffer)
		{
			g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_COLOR);
			{
				m_mafhamFinishedRenderTarget->draw(0, 0);
			}
			g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
		}

		// draw followpoints
		if (cv::osu::draw_followpoints.getBool())
			drawFollowPoints();

		// draw live hitobjects (also, code duplication yay)
		{
			for (int i=m_hitobjectsSortedByEndTime.size()-1; i>=0; i--)
			{
				// PVS optimization (reversed)
				if (usePVS)
				{
					if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
						break;
					if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
						continue;
				}

				if (i > m_iCurrentHitObjectIndex + mafhamRenderLiveSize || (i > m_iMafhamFinishedRenderHitObjectIndex-1 && shouldDrawBuffer)) // skip non-live objects
					continue;

				m_hitobjectsSortedByEndTime[i]->draw();
			}

			for (int i=0; i<m_hitobjectsSortedByEndTime.size(); i++)
			{
				// PVS optimization
				if (usePVS)
				{
					if (m_hitobjectsSortedByEndTime[i]->isFinished() && (curPos - pvs > m_hitobjectsSortedByEndTime[i]->getTime() + m_hitobjectsSortedByEndTime[i]->getDuration())) // past objects
						continue;
					if (m_hitobjectsSortedByEndTime[i]->getTime() > curPos + pvs) // future objects
						break;
				}

				if (i >= m_iCurrentHitObjectIndex + mafhamRenderLiveSize || (i >= m_iMafhamFinishedRenderHitObjectIndex-1 && shouldDrawBuffer)) // skip non-live objects
					break;

				m_hitobjectsSortedByEndTime[i]->draw2();
			}
		}
	}
}

void OsuBeatmapStandard::update()
{
	if (!canUpdate())
	{
		OsuBeatmap::update();
		return;
	}

	// the order of execution in here is a bit specific, since some things do need to be updated before loading has finished
	// note that the baseclass call to update() is NOT the first call, but comes after updating the metrics and wobble

	// live update hitobject and playfield metrics
	updateHitobjectMetrics();
	updatePlayfieldMetrics();

	// wobble mod
	if (cv::osu::mod_wobble.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		m_fPlayfieldRotation = (m_iCurMusicPos / 1000.0f) * 30.0f * speedMultiplierCompensation * cv::osu::mod_wobble_rotation_speed.getFloat();
		m_fPlayfieldRotation = std::fmod(m_fPlayfieldRotation, 360.0f);
	}
	else
		m_fPlayfieldRotation = 0.0f;

	// baseclass call (does the actual hitobject updates among other things)
	OsuBeatmap::update();

	// handle preloading (only for distributed slider vertexbuffer generation atm)
	if (m_bIsPreLoading)
	{
		if (cv::osu::debug.getBool() && m_iPreLoadingIndex == 0)
			debugLog("OsuBeatmapStandard: Preloading slider vertexbuffers ...\n");

		double startTime = engine->getLiveElapsedEngineTime();
		double delta = 0.0;
		while (delta < 0.010 && m_bIsPreLoading) // hardcoded VR deadline of 10 ms (11 but sanity), will temporarily bring us down to 45 fps on average (better than freezing). works fine for desktop gameplay too
		{
			if (m_iPreLoadingIndex >= m_hitobjects.size())
			{
				m_bIsPreLoading = false;
				debugLog("OsuBeatmapStandard: Preloading done.\n");
				break;
			}
			else
			{
				auto *sliderPointer = m_hitobjects[m_iPreLoadingIndex]->asSlider();
				if (sliderPointer != NULL)
					sliderPointer->rebuildVertexBuffer();
			}

			m_iPreLoadingIndex++;
			delta = engine->getLiveElapsedEngineTime() - startTime;
		}
	}

	// notify all other players (including ourself) once we've finished loading
	if (osu->isInMultiplayer())
	{
		if (!isLoadingInt()) // if local loading has finished
		{
			if (osu->getMultiplayer()->isWaitingForClient()) // if we are still in the waiting state
				osu->getMultiplayer()->onClientStatusUpdate(false, false);
		}
	}

	if (isLoading()) return; // only continue if we have loaded everything

	// update auto (after having updated the hitobjects)
	if (osu->getModAuto() || osu->getModAutopilot())
		updateAutoCursorPos();

	// spinner detection (used by osu!stable drain, and by OsuHUD for not drawing the hiterrorbar)
	if (m_currentHitObject != NULL)
	{
		if ((m_currentHitObject->getType() == OsuHitObject::SPINNER) && m_iCurMusicPosWithOffsets > m_currentHitObject->getTime() && m_iCurMusicPosWithOffsets < m_currentHitObject->getTime() + m_currentHitObject->getDuration())
			m_bIsSpinnerActive = true;
		else
			m_bIsSpinnerActive = false;
	}

	// scene buffering logic
	if (cv::osu::stdrules::mod_mafham.getBool())
	{
		if (!m_bMafhamRenderScheduled && m_iCurrentHitObjectIndex != m_iMafhamPrevHitObjectIndex) // if we are not already rendering and the index changed
		{
			m_iMafhamPrevHitObjectIndex = m_iCurrentHitObjectIndex;
			m_iMafhamHitObjectRenderIndex = 0;
			m_bMafhamRenderScheduled = true;
		}
	}

	// full alternate mod lenience
	if (cv::osu::mod_fullalternate.getBool())
	{
		if (m_bInBreak || m_bIsInSkippableSection || m_bIsSpinnerActive || m_iCurrentHitObjectIndex < 1)
			m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = m_iCurrentHitObjectIndex + 1;
	}
}

void OsuBeatmapStandard::onModUpdate(bool rebuildSliderVertexBuffers, bool recomputeDrainRate)
{
	if (cv::osu::debug.getBool())
		debugLog("@ {:f}\n", engine->getTime());

	osu->getMultiplayer()->onServerModUpdate();

	updatePlayfieldMetrics();
	updateHitobjectMetrics();

	if (recomputeDrainRate)
		computeDrainRate();

	if (m_music != NULL)
	{
		m_music->setSpeed(osu->getSpeedMultiplier());
		m_music->setPitch(osu->getPitchMultiplier());
	}

	// recalculate slider vertexbuffers
	if (osu->getModHR() != m_bWasHREnabled || cv::osu::playfield_mirror_horizontal.getBool() != m_bWasHorizontalMirrorEnabled || cv::osu::playfield_mirror_vertical.getBool() != m_bWasVerticalMirrorEnabled)
	{
		m_bWasHREnabled = osu->getModHR();
		m_bWasHorizontalMirrorEnabled = cv::osu::playfield_mirror_horizontal.getBool();
		m_bWasVerticalMirrorEnabled = cv::osu::playfield_mirror_vertical.getBool();

		calculateStacks();

		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (osu->getModEZ() != m_bWasEZEnabled)
	{
		calculateStacks();

		m_bWasEZEnabled = osu->getModEZ();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (getHitcircleDiameter() != m_fPrevHitCircleDiameter && m_hitobjects.size() > 0)
	{
		calculateStacks();

		m_fPrevHitCircleDiameter = getHitcircleDiameter();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (cv::osu::playfield_rotation.getFloat() != m_fPrevPlayfieldRotationFromConVar)
	{
		m_fPrevPlayfieldRotationFromConVar = cv::osu::playfield_rotation.getFloat();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (cv::osu::playfield_stretch_x.getFloat() != m_fPrevPlayfieldStretchX)
	{
		calculateStacks();

		m_fPrevPlayfieldStretchX = cv::osu::playfield_stretch_x.getFloat();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (cv::osu::playfield_stretch_y.getFloat() != m_fPrevPlayfieldStretchY)
	{
		calculateStacks();

		m_fPrevPlayfieldStretchY = cv::osu::playfield_stretch_y.getFloat();
		if (rebuildSliderVertexBuffers)
			updateSliderVertexBuffers();
	}
	if (cv::osu::stdrules::mod_mafham.getBool() != m_bWasMafhamEnabled)
	{
		m_bWasMafhamEnabled = cv::osu::stdrules::mod_mafham.getBool();
		for (int i=0; i<m_hitobjects.size(); i++)
		{
			m_hitobjects[i]->update(m_iCurMusicPosWithOffsets);
		}
	}

	// recalculate star cache for live pp
	if (cv::osu::draw_statistics_pp.getBool() || cv::osu::draw_statistics_livestars.getBool()) // sanity + performance/usability
	{
		bool didCSChange = false;
		if (getHitcircleDiameter() != m_fPrevHitCircleDiameterForStarCache && m_hitobjects.size() > 0)
		{
			m_fPrevHitCircleDiameterForStarCache = getHitcircleDiameter();
			didCSChange = true;
		}

		bool didSpeedChange = false;
		if (osu->getSpeedMultiplier() != m_fPrevSpeedForStarCache && m_hitobjects.size() > 0)
		{
			m_fPrevSpeedForStarCache = osu->getSpeedMultiplier(); // this is not using the beatmap function for speed on purpose, because that wouldn't work while the music is still loading
			didSpeedChange = true;
		}

		if (didCSChange || didSpeedChange)
		{
			if (m_selectedDifficulty2 != NULL)
				updateStarCache();
		}
	}
}

bool OsuBeatmapStandard::isLoading() const
{
	return (isLoadingInt() || (osu->isInMultiplayer() && osu->getMultiplayer()->isWaitingForPlayers()));
}

Vector2 OsuBeatmapStandard::pixels2OsuCoords(Vector2 pixelCoords) const
{
	// un-first-person
	if (cv::osu::stdrules::mod_fps.getBool())
	{
		// HACKHACK: this is the worst hack possible (engine->isDrawing()), but it works
		// the problem is that this same function is called while draw()ing and update()ing
		if (!((engine->isDrawing() && (osu->getModAuto() || osu->getModAutopilot())) || !(osu->getModAuto() || osu->getModAutopilot())))
			pixelCoords += getFirstPersonCursorDelta();
	}

	// un-offset and un-scale, reverse order
	pixelCoords -= m_vPlayfieldOffset;
	pixelCoords /= m_fScaleFactor;

	return pixelCoords;
}

Vector2 OsuBeatmapStandard::osuCoords2Pixels(Vector2 coords) const
{
	if (osu->getModHR())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (cv::osu::playfield_mirror_horizontal.getBool())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (cv::osu::playfield_mirror_vertical.getBool())
		coords.x = OsuGameRules::OSU_COORD_WIDTH - coords.x;

	// wobble
	if (cv::osu::mod_wobble.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		coords.x += std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::mod_wobble_frequency.getFloat())*cv::osu::mod_wobble_strength.getFloat();
		coords.y += std::sin((m_iCurMusicPos/1000.0f)*4*speedMultiplierCompensation*cv::osu::mod_wobble_frequency.getFloat())*cv::osu::mod_wobble_strength.getFloat();
	}

	// wobble2
	if (cv::osu::mod_wobble2.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		Vector2 centerDelta = coords - Vector2(OsuGameRules::OSU_COORD_WIDTH, OsuGameRules::OSU_COORD_HEIGHT)/2.0f;
		coords.x += centerDelta.x*0.25f*std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::mod_wobble_frequency.getFloat())*cv::osu::mod_wobble_strength.getFloat();
		coords.y += centerDelta.y*0.25f*std::sin((m_iCurMusicPos/1000.0f)*3*speedMultiplierCompensation*cv::osu::mod_wobble_frequency.getFloat())*cv::osu::mod_wobble_strength.getFloat();
	}

	// rotation
	if (m_fPlayfieldRotation + cv::osu::playfield_rotation.getFloat() != 0.0f)
	{
		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(m_fPlayfieldRotation + cv::osu::playfield_rotation.getFloat()); // (m_iCurMusicPos/1000.0f)*30

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x;
		coords.y = coords3.y;
	}

	if (cv::osu::mandala.getBool())
	{
		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ((360.0f / cv::osu::mandala_num.getInt()) * (m_iMandalaIndex + 1)); // (m_iCurMusicPos/1000.0f)*30

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x;
		coords.y = coords3.y;
	}

	// if wobble, clamp coordinates
	if (cv::osu::mod_wobble.getBool() || cv::osu::mod_wobble2.getBool())
	{
		coords.x = std::clamp<float>(coords.x, 0.0f, OsuGameRules::OSU_COORD_WIDTH);
		coords.y = std::clamp<float>(coords.y, 0.0f, OsuGameRules::OSU_COORD_HEIGHT);
	}

	if (m_bFailed)
	{
		float failTimePercentInv = 1.0f - m_fFailAnim; // goes from 0 to 1 over the duration of osu_fail_time
		failTimePercentInv *= failTimePercentInv;

		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(failTimePercentInv*60.0f);

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x + failTimePercentInv*OsuGameRules::OSU_COORD_WIDTH*0.25f;
		coords.y = coords3.y + failTimePercentInv*OsuGameRules::OSU_COORD_HEIGHT*1.25f;
	}

	// playfield stretching/transforming
	coords.x -= OsuGameRules::OSU_COORD_WIDTH/2; // center
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;
	{
		if (cv::osu::playfield_circular.getBool())
		{
			// normalize to -1 +1
			coords.x /= (float)OsuGameRules::OSU_COORD_WIDTH / 2.0f;
			coords.y /= (float)OsuGameRules::OSU_COORD_HEIGHT / 2.0f;

			// clamp (for sqrt) and transform
			coords.x = std::clamp<float>(coords.x, -1.0f, 1.0f);
			coords.y = std::clamp<float>(coords.y, -1.0f, 1.0f);
			coords = mapNormalizedCoordsOntoUnitCircle(coords);

			// and scale back up
			coords.x *= (float)OsuGameRules::OSU_COORD_WIDTH / 2.0f;
			coords.y *= (float)OsuGameRules::OSU_COORD_HEIGHT / 2.0f;
		}

		// stretch
		coords.x *= 1.0f + cv::osu::playfield_stretch_x.getFloat();
		coords.y *= 1.0f + cv::osu::playfield_stretch_y.getFloat();
	}
	coords.x += OsuGameRules::OSU_COORD_WIDTH/2; // undo center
	coords.y += OsuGameRules::OSU_COORD_HEIGHT/2;

	// scale and offset
	coords *= m_fScaleFactor;
	coords += m_vPlayfieldOffset; // the offset is already scaled, just add it

	// first person mod, centered cursor
	if (cv::osu::stdrules::mod_fps.getBool())
	{
		// this is the worst hack possible (engine->isDrawing()), but it works
		// the problem is that this same function is called while draw()ing and update()ing
		if ((engine->isDrawing() && (osu->getModAuto() || osu->getModAutopilot())) || !(osu->getModAuto() || osu->getModAutopilot()))
			coords += getFirstPersonCursorDelta();
	}

	return coords;
}

Vector2 OsuBeatmapStandard::osuCoords2RawPixels(Vector2 coords) const
{
	// scale and offset
	coords *= m_fScaleFactor;
	coords += m_vPlayfieldOffset; // the offset is already scaled, just add it

	return coords;
}

Vector3 OsuBeatmapStandard::osuCoordsTo3D(Vector2 coords, const OsuHitObject * /*hitObject*/) const
{
	if (osu->getModHR())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (cv::osu::playfield_mirror_horizontal.getBool())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (cv::osu::playfield_mirror_vertical.getBool())
		coords.x = OsuGameRules::OSU_COORD_WIDTH - coords.x;

	// wobble
	if (cv::osu::mod_wobble.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		coords.x += std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::mod_wobble_frequency.getFloat())*cv::osu::mod_wobble_strength.getFloat();
		coords.y += std::sin((m_iCurMusicPos/1000.0f)*4*speedMultiplierCompensation*cv::osu::mod_wobble_frequency.getFloat())*cv::osu::mod_wobble_strength.getFloat();
	}

	// wobble2
	if (cv::osu::mod_wobble2.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();
		Vector2 centerDelta = coords - Vector2(OsuGameRules::OSU_COORD_WIDTH, OsuGameRules::OSU_COORD_HEIGHT)/2.0f;
		coords.x += centerDelta.x*0.25f*std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::mod_wobble_frequency.getFloat())*cv::osu::mod_wobble_strength.getFloat();
		coords.y += centerDelta.y*0.25f*std::sin((m_iCurMusicPos/1000.0f)*3*speedMultiplierCompensation*cv::osu::mod_wobble_frequency.getFloat())*cv::osu::mod_wobble_strength.getFloat();
	}

	// rotation
	if (m_fPlayfieldRotation + cv::osu::playfield_rotation.getFloat() != 0.0f)
	{
		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(m_fPlayfieldRotation + cv::osu::playfield_rotation.getFloat());

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x;
		coords.y = coords3.y;
	}

	// if wobble, clamp coordinates
	if (cv::osu::mod_wobble.getBool() || cv::osu::mod_wobble2.getBool())
	{
		coords.x = std::clamp<float>(coords.x, 0.0f, OsuGameRules::OSU_COORD_WIDTH);
		coords.y = std::clamp<float>(coords.y, 0.0f, OsuGameRules::OSU_COORD_HEIGHT);
	}

	if (m_bFailed)
	{
		float failTimePercentInv = 1.0f - m_fFailAnim; // goes from 0 to 1 over the duration of osu_fail_time
		failTimePercentInv *= failTimePercentInv;

		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(failTimePercentInv*60.0f);

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x + failTimePercentInv*OsuGameRules::OSU_COORD_WIDTH*0.25f;
		coords.y = coords3.y + failTimePercentInv*OsuGameRules::OSU_COORD_HEIGHT*1.25f;
	}

	// 3d center
	coords.x -= OsuGameRules::OSU_COORD_WIDTH / 2;
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT / 2;

	if (cv::osu::playfield_circular.getBool())
	{
		// normalize to -1 +1
		coords.x /= (float)OsuGameRules::OSU_COORD_WIDTH / 2.0f;
		coords.y /= (float)OsuGameRules::OSU_COORD_HEIGHT / 2.0f;

		// clamp (for sqrt) and transform
		coords.x = std::clamp<float>(coords.x, -1.0f, 1.0f);
		coords.y = std::clamp<float>(coords.y, -1.0f, 1.0f);
		coords = mapNormalizedCoordsOntoUnitCircle(coords);

		// and scale back up
		coords.x *= (float)OsuGameRules::OSU_COORD_WIDTH / 2.0f;
		coords.y *= (float)OsuGameRules::OSU_COORD_HEIGHT / 2.0f;
	}

	// 3d scale
	coords.x *= 1.0f + cv::osu::playfield_stretch_x.getFloat();
	coords.y *= 1.0f + cv::osu::playfield_stretch_y.getFloat();

	const float xCurvePercent = (1.0f + ((coords.x / ((float)OsuGameRules::OSU_COORD_WIDTH / 2.0f)) * cv::osu::fposu::threeD_curve_multiplier.getFloat())) / 2.0f;

	Vector3 coords3d = Vector3(coords.x, -coords.y, 0) * OsuModFPoSu::SIZEDIV3D * osu->getFPoSu()->get3DPlayfieldScale();

	if (cv::osu::fposu::mod_strafing.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();

		const float x = std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::fposu::mod_strafing_frequency_x.getFloat())*cv::osu::fposu::mod_strafing_strength_x.getFloat();
		const float y = std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::fposu::mod_strafing_frequency_y.getFloat())*cv::osu::fposu::mod_strafing_strength_y.getFloat();
		const float z = std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::fposu::mod_strafing_frequency_z.getFloat())*cv::osu::fposu::mod_strafing_strength_z.getFloat();

		coords3d += Vector3(x, y, z);
	}

	// TODO: make this extra mod separate from depth wobble
	/*
	{
		float depthMultiplier = (float)-hitObject->getDelta() / (float)hitObject->getApproachTime(); // ... -1 0 1

		if (depthMultiplier >= -1.0f)
		{
			const float spawnDistance = -0.5f;
			const float overshootDistance = 0.2f;

			const float stopOvershootPercent = 0.5f;

			float depthMultiplierClamped = std::clamp<float>(depthMultiplier, -1.0f, stopOvershootPercent); // -1 0 stopOvershootPercent
			depthMultiplierClamped = (depthMultiplierClamped + 1.0f) / (1.0f); // 0 1+stopOvershootPercent

			if (depthMultiplierClamped < 1.0f)
			{
				//depthMultiplierClamped = 1.0f - (1.0f - depthMultiplierClamped)*(1.0f - depthMultiplierClamped);
				//depthMultiplierClamped = 1.0f - (1.0f - depthMultiplierClamped)*(1.0f - depthMultiplierClamped);
				//depthMultiplierClamped = 1.0f - (1.0f - depthMultiplierClamped)*(1.0f - depthMultiplierClamped);
			}
			else
			{
				//depthMultiplierClamped = 1.0f - (1.0f - (depthMultiplierClamped - 1.0f)) * (1.0f - (depthMultiplierClamped - 1.0f));
			}

			coords3d.z += std::lerp(spawnDistance, overshootDistance, depthMultiplierClamped);
		}
	}
	*/
	//if (cv::osu::fposu::mod_3d_depthwobble.getBool())
	/*
	{
		// TODO: try coords.y multiplier such that we get 3d multiplied sines with random mountains sprinkled regularly, should be more enjoyable to have more height differences like that
		coords3d.z += std::sin(m_iCurMusicPos/1000.0f*3 + (coords.x / (float)OsuGameRules::OSU_COORD_WIDTH)*15)*0.2f;
	}
	*/

	if (cv::osu::fposu::curved.getBool())
		return (coords3d + Vector3(0, 0, -quadLerp3f(cv::osu::fposu::distance.getFloat(), osu->getFPoSu()->getEdgeDistance(), cv::osu::fposu::distance.getFloat(), xCurvePercent)));
	else
		return (coords3d + Vector3(0, 0, -cv::osu::fposu::distance.getFloat()));
}

Vector3 OsuBeatmapStandard::osuCoordsToRaw3D(Vector2 coords) const
{
	// 3d center
	coords.x -= OsuGameRules::OSU_COORD_WIDTH / 2;
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT / 2;

	const float xCurvePercent = (1.0f + ((coords.x / ((float)OsuGameRules::OSU_COORD_WIDTH / 2.0f)) * cv::osu::fposu::threeD_curve_multiplier.getFloat())) / 2.0f;

	Vector3 coords3d = Vector3(coords.x, -coords.y, 0) * OsuModFPoSu::SIZEDIV3D * osu->getFPoSu()->get3DPlayfieldScale();

	if (cv::osu::fposu::mod_strafing.getBool())
	{
		const float speedMultiplierCompensation = 1.0f / getSpeedMultiplier();

		const float x = std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::fposu::mod_strafing_frequency_x.getFloat())*cv::osu::fposu::mod_strafing_strength_x.getFloat();
		const float y = std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::fposu::mod_strafing_frequency_y.getFloat())*cv::osu::fposu::mod_strafing_strength_y.getFloat();
		const float z = std::sin((m_iCurMusicPos/1000.0f)*5*speedMultiplierCompensation*cv::osu::fposu::mod_strafing_frequency_z.getFloat())*cv::osu::fposu::mod_strafing_strength_z.getFloat();

		coords3d += Vector3(x, y, z);
	}

	if (cv::osu::fposu::mod_3d_depthwobble.getBool())
	{
		// TODO: implement (all 3d transforms need to match osuCoordsTo3D(), otherwise auto and traceLine() would break)
	}

	if (cv::osu::fposu::curved.getBool())
		return (coords3d + Vector3(0, 0, -quadLerp3f(cv::osu::fposu::distance.getFloat(), osu->getFPoSu()->getEdgeDistance(), cv::osu::fposu::distance.getFloat(), xCurvePercent)));
	else
		return (coords3d + Vector3(0, 0, -cv::osu::fposu::distance.getFloat()));
}

Vector2 OsuBeatmapStandard::osuCoords2LegacyPixels(Vector2 coords) const
{
	if (osu->getModHR())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (cv::osu::playfield_mirror_horizontal.getBool())
		coords.y = OsuGameRules::OSU_COORD_HEIGHT - coords.y;
	if (cv::osu::playfield_mirror_vertical.getBool())
		coords.x = OsuGameRules::OSU_COORD_WIDTH - coords.x;

	// rotation
	if (m_fPlayfieldRotation + cv::osu::playfield_rotation.getFloat() != 0.0f)
	{
		coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
		coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

		Vector3 coords3 = Vector3(coords.x, coords.y, 0);
		Matrix4 rot;
		rot.rotateZ(m_fPlayfieldRotation + cv::osu::playfield_rotation.getFloat());

		coords3 = coords3 * rot;
		coords3.x += OsuGameRules::OSU_COORD_WIDTH/2;
		coords3.y += OsuGameRules::OSU_COORD_HEIGHT/2;

		coords.x = coords3.x;
		coords.y = coords3.y;
	}

	// VR center
	coords.x -= OsuGameRules::OSU_COORD_WIDTH/2;
	coords.y -= OsuGameRules::OSU_COORD_HEIGHT/2;

	if (cv::osu::playfield_circular.getBool())
	{
		// normalize to -1 +1
		coords.x /= (float)OsuGameRules::OSU_COORD_WIDTH / 2.0f;
		coords.y /= (float)OsuGameRules::OSU_COORD_HEIGHT / 2.0f;

		// clamp (for sqrt) and transform
		coords.x = std::clamp<float>(coords.x, -1.0f, 1.0f);
		coords.y = std::clamp<float>(coords.y, -1.0f, 1.0f);
		coords = mapNormalizedCoordsOntoUnitCircle(coords);

		// and scale back up
		coords.x *= (float)OsuGameRules::OSU_COORD_WIDTH / 2.0f;
		coords.y *= (float)OsuGameRules::OSU_COORD_HEIGHT / 2.0f;
	}

	// VR scale
	coords.x *= 1.0f + cv::osu::playfield_stretch_x.getFloat();
	coords.y *= 1.0f + cv::osu::playfield_stretch_y.getFloat();

	return coords;
}

Vector2 OsuBeatmapStandard::getCursorPos() const
{
	if (cv::osu::stdrules::mod_fps.getBool() && !m_bIsPaused)
	{
		if (osu->getModAuto() || osu->getModAutopilot())
			return m_vAutoCursorPos;
		else
			return m_vPlayfieldCenter;
	}
	else if (osu->getModAuto() || osu->getModAutopilot())
		return m_vAutoCursorPos;
	else
	{
		Vector2 pos = mouse->getPos();
		if (cv::osu::mod_shirone.getBool() && osu->getScore()->getCombo() > 0) // <3
			return pos + Vector2(std::sin((m_iCurMusicPos/20.0f)*1.15f)*((float)osu->getScore()->getCombo()/cv::osu::mod_shirone_combo.getFloat()), std::cos((m_iCurMusicPos/20.0f)*1.3f)*((float)osu->getScore()->getCombo()/cv::osu::mod_shirone_combo.getFloat()));
		else
			return pos;
	}
}

Vector2 OsuBeatmapStandard::getFirstPersonCursorDelta() const
{
	return m_vPlayfieldCenter - (osu->getModAuto() || osu->getModAutopilot() ? m_vAutoCursorPos : mouse->getPos());
}

float OsuBeatmapStandard::getHitcircleDiameter() const
{
	return m_fHitcircleDiameter;
}

void OsuBeatmapStandard::onBeforeLoad()
{
	debugLog("\n");

	osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::START, 0, false, m_selectedDifficulty2);

	// some hitobjects already need this information to be up-to-date before their constructor is called
	updatePlayfieldMetrics();
	updateHitobjectMetrics();

	m_bIsPreLoading = false;
}

void OsuBeatmapStandard::onLoad()
{
	debugLog("\n");

	// after the hitobjects have been loaded we can calculate the stacks
	calculateStacks();
	computeDrainRate();

	// start preloading (delays the play start until it's set to false, see isLoading())
	m_bIsPreLoading = true;
	m_iPreLoadingIndex = 0;

	// build stars
	m_fStarCacheTime = engine->getTime() + cv::osu::pp_live_timeout.getFloat(); // first time delay only. subsequent updates should immediately show the loading spinner
	updateStarCache();
}

void OsuBeatmapStandard::onPlayStart()
{
	debugLog("\n");

	onModUpdate(false, false); // if there are calculations in there that need the hitobjects to be loaded, also applies speed/pitch
}

void OsuBeatmapStandard::onBeforeStop(bool quit)
{
	debugLog("\n");

	// kill any running star cache loader
	stopStarCacheLoader();

	if (!quit) // if the ranking screen is going to be shown
	{
		// calculate final pp
		debugLog(" calculating pp ...\n");
		double aim = 0.0;
		double aimSliderFactor = 0.0;
		double aimDifficultSliders = 0.0;
		double aimDifficultStrains = 0.0;
		double speed = 0.0;
		double speedNotes = 0.0;
		double speedDifficultStrains = 0.0;

		// calculate stars
		const UString &osuFilePath = m_selectedDifficulty2->getFilePath();
		const Osu::GAMEMODE gameMode = Osu::GAMEMODE::STD;
		const float AR = getAR();
		const float CS = getCS();
		const float OD = getOD();
		const float speedMultiplier = osu->getSpeedMultiplier(); // NOTE: not this->getSpeedMultiplier()!
		const bool relax = osu->getModRelax();
		const bool autopilot = osu->getModAutopilot();
		const bool touchDevice = osu->getModTD();

		OsuDatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres = OsuDatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, gameMode, AR, CS, speedMultiplier);
		const double totalStars = OsuDifficultyCalculator::calculateStarDiffForHitObjects(diffres.diffobjects, CS, OD, speedMultiplier, relax, autopilot, touchDevice, &aim, &aimSliderFactor, &aimDifficultSliders, &aimDifficultStrains, &speed, &speedNotes, &speedDifficultStrains);

		m_fAimStars = (float)aim;
		m_fSpeedStars = (float)speed;

		const int numHitObjects = m_hitobjects.size();
		const int numCircles = m_selectedDifficulty2->getNumCircles();
		const int numSliders = m_selectedDifficulty2->getNumSliders();
		const int numSpinners = m_selectedDifficulty2->getNumSpinners();
		const int maxPossibleCombo = m_iMaxPossibleCombo;
		const int highestCombo = osu->getScore()->getComboMax();
		const int numMisses = osu->getScore()->getNumMisses();
		const int num300s = osu->getScore()->getNum300s();
		const int num100s = osu->getScore()->getNum100s();
		const int num50s = osu->getScore()->getNum50s();
		const float pp = OsuDifficultyCalculator::calculatePPv2(this, aim, aimSliderFactor, aimDifficultSliders, aimDifficultStrains, speed, speedNotes, speedDifficultStrains, numHitObjects, numCircles, numSliders, numSpinners, maxPossibleCombo, highestCombo, numMisses, num300s, num100s, num50s);
		osu->getScore()->setStarsTomTotal(totalStars);
		osu->getScore()->setStarsTomAim(m_fAimStars);
		osu->getScore()->setStarsTomSpeed(m_fSpeedStars);
		osu->getScore()->setPPv2(pp);
		debugLog(" done.\n");

		// save local score, but only under certain conditions
		const bool isComplete = (num300s + num100s + num50s + numMisses >= numHitObjects);
		const bool isZero = (osu->getScore()->getScore() < 1);
		const bool isUnranked = (osu->getModAuto() || (osu->getModAutopilot() && osu->getModRelax())) || osu->getScore()->isUnranked();

		//debugLog("isComplete = {}, isZero = {}, isUnranked = {}\n", (int)isComplete, (int)isZero, (int)isUnranked);

		int scoreIndex = -1;
		if (isComplete && !isZero && !isUnranked && !osu->getScore()->hasDied())
		{
			const int scoreVersion = OsuScore::VERSION;

			debugLog(" saving score ...\n");
			{
				OsuDatabase::Score score;

				score.isLegacyScore = false;

				//if (scoreVersion > 20190103)
				{
					score.isImportedLegacyScore = false;
				}

				score.version = scoreVersion;
				score.unixTimestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

				// default
				score.playerName = cv::name.getString();

				score.num300s = osu->getScore()->getNum300s();
				score.num100s = osu->getScore()->getNum100s();
				score.num50s = osu->getScore()->getNum50s();
				score.numGekis = osu->getScore()->getNum300gs();
				score.numKatus = osu->getScore()->getNum100ks();
				score.numMisses = osu->getScore()->getNumMisses();
				score.score = osu->getScore()->getScore();
				score.comboMax = osu->getScore()->getComboMax();
				score.perfect = (maxPossibleCombo > 0 && score.comboMax > 0 && score.comboMax >= maxPossibleCombo);
				score.modsLegacy = osu->getScore()->getModsLegacy();
				{
					// special case: manual slider accuracy has been enabled (affects pp but not score), so force scorev2 for potential future score recalculations
					// NOTE: I forgot to add this before 20210103, so all old scores which were played without scorev2 but with osu_slider_scorev2 will get downgraded slighly :(
					score.modsLegacy |= (cv::osu::slider_scorev2.getBool() ? OsuReplay::Mods::ScoreV2 : 0);
				}

				// custom
				score.numSliderBreaks = osu->getScore()->getNumSliderBreaks();
				score.pp = pp;
				score.unstableRate = osu->getScore()->getUnstableRate();
				score.hitErrorAvgMin = osu->getScore()->getHitErrorAvgMin();
				score.hitErrorAvgMax = osu->getScore()->getHitErrorAvgMax();
				score.starsTomTotal = totalStars;
				score.starsTomAim = aim;
				score.starsTomSpeed = speed;
				score.speedMultiplier = osu->getSpeedMultiplier();
				score.CS = CS;
				score.AR = AR;
				score.OD = getOD();
				score.HP = getHP();

				//if (scoreVersion > 20180722)
				{
					score.maxPossibleCombo = maxPossibleCombo;
					score.numHitObjects = numHitObjects;
					score.numCircles = numCircles;
				}

				std::vector<ConVar*> allExperimentalMods = osu->getExperimentalMods();
				for (int i=0; i<allExperimentalMods.size(); i++)
				{
					if (allExperimentalMods[i]->getBool())
					{
						score.experimentalModsConVars.append(allExperimentalMods[i]->getName());
						score.experimentalModsConVars.append(";");
					}
				}

				score.md5hash = m_selectedDifficulty2->getMD5Hash(); // NOTE: necessary for "Use Mods"

				// save it
				scoreIndex = osu->getSongBrowser()->getDatabase()->addScore(m_selectedDifficulty2->getMD5Hash(), score);
				if (scoreIndex == -1)
					osu->getNotificationOverlay()->addNotification(UString::format("Failed saving score! md5hash.length() = %i", m_selectedDifficulty2->getMD5Hash().length()), 0xffff0000, false, 3.0f);
			}
			debugLog(" done.\n");
		}
		osu->getScore()->setIndex(scoreIndex);
		osu->getScore()->setComboFull(maxPossibleCombo); // used in OsuRankingScreen/OsuUIRankingScreenRankingPanel

		// special case: incomplete scores should NEVER show pp, even if auto
		if (!isComplete)
			osu->getScore()->setPPv2(0.0f);
	}
}

void OsuBeatmapStandard::onStop(bool quit)
{
	debugLog("\n");

	if (quit)
		osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::STOP);
}

void OsuBeatmapStandard::onPaused(bool first)
{
	debugLog("\n");

	osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::PAUSE);

	if (first)
	{
		m_vContinueCursorPoint = mouse->getPos();

		if (cv::osu::stdrules::mod_fps.getBool())
			m_vContinueCursorPoint = OsuGameRules::getPlayfieldCenter();
	}
}

void OsuBeatmapStandard::onUnpaused()
{
	debugLog("\n");

	osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::UNPAUSE);
}

void OsuBeatmapStandard::onRestart(bool quick)
{
	debugLog("\n");

	osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::STATE::RESTART, 0, quick);
}

void OsuBeatmapStandard::updateAutoCursorPos()
{
	m_vAutoCursorPos = m_vPlayfieldCenter;
	m_vAutoCursorPos.y *= 2.5f; // start moving in offscreen from bottom

	if (!m_bIsPlaying && !m_bIsPaused)
	{
		m_vAutoCursorPos = m_vPlayfieldCenter;
		return;
	}
	if (m_hitobjects.size() < 1)
	{
		m_vAutoCursorPos = m_vPlayfieldCenter;
		return;
	}

	const long curMusicPos = m_iCurMusicPosWithOffsets;

	// general
	long prevTime = 0;
	long nextTime = m_hitobjects[0]->getTime();
	Vector2 prevPos = m_vAutoCursorPos;
	Vector2 curPos = m_vAutoCursorPos;
	Vector2 nextPos = m_vAutoCursorPos;
	bool haveCurPos = false;

	// dance
	int nextPosIndex = 0;

	if (m_hitobjects[0]->getTime() < (long)cv::osu::early_note_time.getInt())
		prevTime = -(long)cv::osu::early_note_time.getInt() * getSpeedMultiplier();

	if (osu->getModAuto())
	{
		bool autoDanceOverride = false;
		for (int i=0; i<m_hitobjects.size(); i++)
		{
			OsuHitObject *o = m_hitobjects[i];

			// get previous object
			if (o->getTime() <= curMusicPos)
			{
				prevTime = o->getTime() + o->getDuration();
				prevPos = o->getAutoCursorPos(curMusicPos);
				if (o->getDuration() > 0 && curMusicPos - o->getTime() <= o->getDuration())
				{
					if (cv::osu::auto_cursordance.getBool())
					{
						const auto *sliderPointer = o->asSlider();
						if (sliderPointer != NULL)
						{
							const std::vector<OsuSlider::SLIDERCLICK> &clicks = sliderPointer->getClicks();

							// start
							prevTime = o->getTime();
							prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));

							long biggestPrevious = 0;
							long smallestNext = std::numeric_limits<long>::max();
							bool allFinished = true;
							long endTime = 0;

							// middle clicks
							for (int c=0; c<clicks.size(); c++)
							{
								// get previous click
								if (clicks[c].time <= curMusicPos && clicks[c].time > biggestPrevious)
								{
									biggestPrevious = clicks[c].time;
									prevTime = clicks[c].time;
									prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));
								}

								// get next click
								if (clicks[c].time > curMusicPos && clicks[c].time < smallestNext)
								{
									smallestNext = clicks[c].time;
									nextTime = clicks[c].time;
									nextPos = osuCoords2Pixels(o->getRawPosAt(nextTime));
								}

								// end hack
								if (!clicks[c].finished)
									allFinished = false;
								else if (clicks[c].time > endTime)
									endTime = clicks[c].time;
							}

							// end
							if (allFinished)
							{
								// hack for slider without middle clicks
								if (endTime == 0)
									endTime = o->getTime();

								prevTime = endTime;
								prevPos = osuCoords2Pixels(o->getRawPosAt(prevTime));
								nextTime = o->getTime() + o->getDuration();
								nextPos = osuCoords2Pixels(o->getRawPosAt(nextTime));
							}

							haveCurPos = false;
							autoDanceOverride = true;
							break;
						}
					}

					haveCurPos = true;
					curPos = prevPos;
					break;
				}
			}

			// get next object
			if (o->getTime() > curMusicPos)
			{
				nextPosIndex = i;
				if (!autoDanceOverride)
				{
					nextPos = o->getAutoCursorPos(curMusicPos);
					nextTime = o->getTime();
				}
				break;
			}
		}
	}
	else if (osu->getModAutopilot())
	{
		for (int i=0; i<m_hitobjects.size(); i++)
		{
			OsuHitObject *o = m_hitobjects[i];

			// get previous object
			if (o->isFinished() || (curMusicPos > o->getTime() + o->getDuration() + (long)(OsuGameRules::getHitWindow50(this)*cv::osu::autopilot_lenience.getFloat())))
			{
				prevTime = o->getTime() + o->getDuration() + o->getAutopilotDelta();
				prevPos = o->getAutoCursorPos(curMusicPos);
			}
			else if (!o->isFinished()) // get next object
			{
				nextPosIndex = i;
				nextPos = o->getAutoCursorPos(curMusicPos);
				nextTime = o->getTime();

				// wait for the user to click
				if (curMusicPos >= nextTime + o->getDuration())
				{
					haveCurPos = true;
					curPos = nextPos;

					//long delta = curMusicPos - (nextTime + o->getDuration());
					o->setAutopilotDelta(curMusicPos - (nextTime + o->getDuration()));
				}
				else if (o->getDuration() > 0 && curMusicPos >= nextTime) // handle objects with duration
				{
					haveCurPos = true;
					curPos = nextPos;
					o->setAutopilotDelta(0);
				}

				break;
			}
		}
	}

	if (haveCurPos) // in active hitObject
		m_vAutoCursorPos = curPos;
	else
	{
		// interpolation
		float percent = 1.0f;
		if ((nextTime == 0 && prevTime == 0) || (nextTime - prevTime) == 0)
			percent = 1.0f;
		else
			percent = (float)((long)curMusicPos - prevTime) / (float)(nextTime - prevTime);

		percent = std::clamp<float>(percent, 0.0f, 1.0f);

		// scaled distance (not osucoords)
		float distance = (nextPos-prevPos).length();
		if (distance > m_fHitcircleDiameter*1.05f) // snap only if not in a stream (heuristic)
		{
			int numIterations = std::clamp<int>(osu->getModAutopilot() ? cv::osu::autopilot_snapping_strength.getInt() : cv::osu::auto_snapping_strength.getInt(), 0, 42);
			for (int i=0; i<numIterations; i++)
			{
				percent = (-percent)*(percent-2.0f);
			}
		}
		else // in a stream
		{
			m_iAutoCursorDanceIndex = nextPosIndex;
		}

		m_vAutoCursorPos = prevPos + (nextPos - prevPos)*percent;

		if (cv::osu::auto_cursordance.getBool() && !osu->getModAutopilot())
		{
			Vector3 dir = Vector3(nextPos.x, nextPos.y, 0) - Vector3(prevPos.x, prevPos.y, 0);
			Vector3 center = dir*0.5f;
			Matrix4 worldMatrix;
			worldMatrix.translate(center);
			worldMatrix.rotate((1.0f-percent) * 180.0f * (m_iAutoCursorDanceIndex % 2 == 0 ? 1 : -1), 0, 0, 1);
			Vector3 fancyAutoCursorPos = worldMatrix*center;
			m_vAutoCursorPos = prevPos + (nextPos-prevPos)*0.5f + Vector2(fancyAutoCursorPos.x, fancyAutoCursorPos.y);
		}
	}
}

void OsuBeatmapStandard::updatePlayfieldMetrics()
{
	m_fScaleFactor = OsuGameRules::getPlayfieldScaleFactor();
	m_vPlayfieldSize = OsuGameRules::getPlayfieldSize();
	m_vPlayfieldOffset = OsuGameRules::getPlayfieldOffset();
	m_vPlayfieldCenter = OsuGameRules::getPlayfieldCenter();
}

void OsuBeatmapStandard::updateHitobjectMetrics()
{
	OsuSkin *skin = osu->getSkin();

	m_fRawHitcircleDiameter = OsuGameRules::getRawHitCircleDiameter(getCS());
	m_fXMultiplier = OsuGameRules::getHitCircleXMultiplier();
	m_fHitcircleDiameter = OsuGameRules::getHitCircleDiameter(this);

	const float osuCoordScaleMultiplier = (getHitcircleDiameter() / m_fRawHitcircleDiameter);
	m_fNumberScale = (m_fRawHitcircleDiameter / (160.0f * (skin->isDefault12x() ? 2.0f : 1.0f))) * osuCoordScaleMultiplier * cv::osu::number_scale_multiplier.getFloat();
	m_fHitcircleOverlapScale = (m_fRawHitcircleDiameter / (160.0f)) * osuCoordScaleMultiplier * cv::osu::number_scale_multiplier.getFloat();

	const float sliderFollowCircleDiameterMultiplier = (osu->getModNM() || cv::osu::mod_jigsaw2.getBool() ? (1.0f*(1.0f - cv::osu::mod_jigsaw_followcircle_radius_factor.getFloat()) + cv::osu::mod_jigsaw_followcircle_radius_factor.getFloat()*cv::osu::stdrules::slider_followcircle_size_multiplier.getFloat()) : cv::osu::stdrules::slider_followcircle_size_multiplier.getFloat());
	m_fRawSliderFollowCircleDiameter = m_fRawHitcircleDiameter * sliderFollowCircleDiameterMultiplier;
	m_fSliderFollowCircleDiameter = getHitcircleDiameter() * sliderFollowCircleDiameterMultiplier;
}

void OsuBeatmapStandard::updateSliderVertexBuffers()
{
	updatePlayfieldMetrics();
	updateHitobjectMetrics();

	m_bWasEZEnabled = osu->getModEZ(); // to avoid useless double updates in onModUpdate()
	m_fPrevHitCircleDiameter = getHitcircleDiameter(); // same here
	m_fPrevPlayfieldRotationFromConVar = cv::osu::playfield_rotation.getFloat(); // same here
	m_fPrevPlayfieldStretchX = cv::osu::playfield_stretch_x.getFloat(); // same here
	m_fPrevPlayfieldStretchY = cv::osu::playfield_stretch_y.getFloat(); // same here

	debugLog(" for {} hitobjects ...\n", m_hitobjects.size());

	for (int i=0; i<m_hitobjects.size(); i++)
	{
		auto *sliderPointer = m_hitobjects[i]->asSlider();
		if (sliderPointer != NULL)
			sliderPointer->rebuildVertexBuffer();
	}
}

void OsuBeatmapStandard::calculateStacks()
{
	if (!cv::osu::stacking.getBool()) return;

	updateHitobjectMetrics();

	debugLog("OsuBeatmapStandard: Calculating stacks ...\n");

	// reset
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		m_hitobjects[i]->setStack(0);
	}

	const float STACK_LENIENCE = 3.0f;
	const float STACK_OFFSET = 0.05f;

	const float approachTime = OsuGameRules::getApproachTimeForStacking(this);

	const float stackLeniency = (cv::osu::stacking_leniency_override.getFloat() >= 0.0f ? cv::osu::stacking_leniency_override.getFloat() : m_selectedDifficulty2->getStackLeniency());

	if (getSelectedDifficulty2()->getVersion() > 5)
	{
		// peppy's algorithm
		// https://gist.github.com/peppy/1167470

		for (int i=m_hitobjects.size()-1; i>=0; i--)
		{
			int n = i;

			OsuHitObject *objectI = m_hitobjects[i];
			bool isSpinner = objectI->getType() == OsuHitObject::SPINNER;

			if (objectI->getStack() != 0 || isSpinner)
				continue;

			bool isHitCircle = objectI->getType() == OsuHitObject::CIRCLE;
			bool isSlider = objectI->getType() == OsuHitObject::SLIDER;

			if (isHitCircle)
			{
				while (--n >= 0)
				{
					OsuHitObject *objectN = m_hitobjects[n];

					if (objectN->getType() == OsuHitObject::SPINNER)
						continue;

					if (objectI->getTime() - (approachTime * stackLeniency) > (objectN->getTime() + objectN->getDuration()))
						break;

					Vector2 objectNEndPosition = objectN->getOriginalRawPosAt(objectN->getTime() + objectN->getDuration());
					if (objectN->getDuration() != 0 && (objectNEndPosition - objectI->getOriginalRawPosAt(objectI->getTime())).length() < STACK_LENIENCE)
					{
						int offset = objectI->getStack() - objectN->getStack() + 1;
						for (int j=n+1; j<=i; j++)
						{
							if ((objectNEndPosition - m_hitobjects[j]->getOriginalRawPosAt(m_hitobjects[j]->getTime())).length() < STACK_LENIENCE)
								m_hitobjects[j]->setStack(m_hitobjects[j]->getStack() - offset);
						}

						break;
					}

					if ((objectN->getOriginalRawPosAt(objectN->getTime()) - objectI->getOriginalRawPosAt(objectI->getTime())).length() < STACK_LENIENCE)
					{
						objectN->setStack(objectI->getStack() + 1);
						objectI = objectN;
					}
				}
			}
			else if (isSlider)
			{
				while (--n >= 0)
				{
					OsuHitObject *objectN = m_hitobjects[n];

					if (objectN->getType() == OsuHitObject::SPINNER)
						continue;

					if (objectI->getTime() - (approachTime * stackLeniency) > objectN->getTime())
						break;

					if (((objectN->getDuration() != 0 ? objectN->getOriginalRawPosAt(objectN->getTime() + objectN->getDuration()) : objectN->getOriginalRawPosAt(objectN->getTime())) - objectI->getOriginalRawPosAt(objectI->getTime())).length() < STACK_LENIENCE)
					{
						objectN->setStack(objectI->getStack() + 1);
						objectI = objectN;
					}
				}
			}
		}
	}
	else // getSelectedDifficulty()->version < 6
	{
		// old stacking algorithm for old beatmaps
		// https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Beatmaps/OsuBeatmapProcessor.cs

		for (int i=0; i<m_hitobjects.size(); i++)
		{
			OsuHitObject *currHitObject = m_hitobjects[i];
			const auto *sliderPointer = currHitObject->asSlider();

			const bool isSlider = (sliderPointer != NULL);

			if (currHitObject->getStack() != 0 && !isSlider)
				continue;

			long startTime = currHitObject->getTime() + currHitObject->getDuration();
			int sliderStack = 0;

			for (int j=i+1; j<m_hitobjects.size(); j++)
			{
				OsuHitObject *objectJ = m_hitobjects[j];

				if (objectJ->getTime() - (approachTime * stackLeniency) > startTime)
					break;

				// "The start position of the hitobject, or the position at the end of the path if the hitobject is a slider"
				Vector2 position2 = isSlider
					? sliderPointer->getOriginalRawPosAt(sliderPointer->getTime() + sliderPointer->getDuration())
					: currHitObject->getOriginalRawPosAt(currHitObject->getTime());

				if ((objectJ->getOriginalRawPosAt(objectJ->getTime()) - currHitObject->getOriginalRawPosAt(currHitObject->getTime())).length() < 3)
				{
					currHitObject->setStack(currHitObject->getStack() + 1);
					startTime = objectJ->getTime() + objectJ->getDuration();
				}
				else if ((objectJ->getOriginalRawPosAt(objectJ->getTime()) - position2).length() < 3)
				{
					// "Case for sliders - bump notes down and right, rather than up and left."
					sliderStack++;
					objectJ->setStack(objectJ->getStack() - sliderStack);
					startTime = objectJ->getTime() + objectJ->getDuration();
				}
			}
		}
	}

	// update hitobject positions
	float stackOffset = m_fRawHitcircleDiameter * STACK_OFFSET;
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		if (m_hitobjects[i]->getStack() != 0)
			m_hitobjects[i]->updateStackPosition(stackOffset);
	}
}

void OsuBeatmapStandard::computeDrainRate()
{
	m_fDrainRate = 0.0;
	m_fHpMultiplierNormal = 1.0;
	m_fHpMultiplierComboEnd = 1.0;

	if (m_hitobjects.size() < 1 || m_selectedDifficulty2 == NULL) return;

	debugLog("OsuBeatmapStandard: Calculating drain ...\n");

	const int drainType = cv::osu::drain_type.getInt();

	if (drainType == 1) // osu!stable
	{
		// see https://github.com/ppy/osu-iPhone/blob/master/Classes/OsuPlayer.m
		// see calcHPDropRate() @ https://github.com/ppy/osu-iPhone/blob/master/Classes/OsuFiletype.m#L661

		// NOTE: all drain changes between 2014 and today have been fixed here (the link points to an old version of the algorithm!)
		// these changes include: passive spinner nerf (drain * 0.25 while spinner is active), and clamping the object length drain to 0 + an extra check for that (see maxLongObjectDrop)
		// see https://osu.ppy.sh/home/changelog/stable40/20190513.2

		struct TestPlayer
		{
			TestPlayer(double hpBarMaximum)
			{
				this->hpBarMaximum = hpBarMaximum;

				hpMultiplierNormal = 1.0;
				hpMultiplierComboEnd = 1.0;

				resetHealth();
			}

			void resetHealth()
			{
				health = hpBarMaximum;
				healthUncapped = hpBarMaximum;
			}

			void increaseHealth(double amount)
			{
				healthUncapped += amount;
				health += amount;

				if (health > hpBarMaximum)
					health = hpBarMaximum;

				if (health < 0.0)
					health = 0.0;

				if (healthUncapped < 0.0)
					healthUncapped = 0.0;
			}

			void decreaseHealth(double amount)
			{
				health -= amount;

				if (health < 0.0)
					health = 0.0;

				if (health > hpBarMaximum)
					health = hpBarMaximum;

				healthUncapped -= amount;

				if (healthUncapped < 0.0)
					healthUncapped = 0.0;
			}

			double hpBarMaximum;

			double health;
			double healthUncapped;

			double hpMultiplierNormal;
			double hpMultiplierComboEnd;
		};
		TestPlayer testPlayer((double)cv::osu::drain_stable_hpbar_maximum.getFloat());

		const double HP = getHP();
		const int version = m_selectedDifficulty2->getVersion();

		double testDrop = 0.05;

		const double lowestHpEver = OsuGameRules::mapDifficultyRangeDouble(HP, 195.0, 160.0, 60.0);
		const double lowestHpComboEnd = OsuGameRules::mapDifficultyRangeDouble(HP, 198.0, 170.0, 80.0);
		const double lowestHpEnd = OsuGameRules::mapDifficultyRangeDouble(HP, 198.0, 180.0, 80.0);
		const double HpRecoveryAvailable = OsuGameRules::mapDifficultyRangeDouble(HP, 8.0, 4.0, 0.0);

		bool fail = false;

		do
		{
			testPlayer.resetHealth();

			double lowestHp = testPlayer.health;
			int lastTime = (int)(m_hitobjects[0]->getTime() - (long)OsuGameRules::getApproachTime(this));
			fail = false;

			const int breakCount = m_breaks.size();
			int breakNumber = 0;

			int comboTooLowCount = 0;

			for (int i=0; i<m_hitobjects.size(); i++)
			{
				const OsuHitObject *h = m_hitobjects[i];
				const auto *sliderPointer = h->asSlider();
				const auto *spinnerPointer = h->asSpinner();

				const int localLastTime = lastTime;

				int breakTime = 0;
				if (breakCount > 0 && breakNumber < breakCount)
				{
					const OsuDatabaseBeatmap::BREAK &e = m_breaks[breakNumber];
					if (e.startTime >= localLastTime && e.endTime <= h->getTime())
					{
						// consider break start equal to object end time for version 8+ since drain stops during this time
						breakTime = (version < 8) ? (e.endTime - e.startTime) : (e.endTime - localLastTime);
						breakNumber++;
					}
				}

				testPlayer.decreaseHealth(testDrop*(h->getTime() - lastTime - breakTime));

				lastTime = (int)(h->getTime() + h->getDuration());

				if (testPlayer.health < lowestHp)
					lowestHp = testPlayer.health;

				if (testPlayer.health > lowestHpEver)
				{
					const double longObjectDrop = testDrop * (double)h->getDuration();
					const double maxLongObjectDrop = std::max(0.0, longObjectDrop - testPlayer.health);

					testPlayer.decreaseHealth(longObjectDrop);

					// nested hitobjects
					if (sliderPointer != NULL)
					{
						// startcircle
						testPlayer.increaseHealth(OsuScore::getHealthIncrease(OsuScore::HIT::HIT_SLIDER30, HP, testPlayer.hpMultiplierNormal, testPlayer.hpMultiplierComboEnd, 1.0)); // slider30

						// ticks + repeats + repeat ticks
						const std::vector<OsuSlider::SLIDERCLICK> &clicks = sliderPointer->getClicks();
						for (int c=0; c<clicks.size(); c++)
						{
							switch (clicks[c].type)
							{
							case 0: // repeat
								testPlayer.increaseHealth(OsuScore::getHealthIncrease(OsuScore::HIT::HIT_SLIDER30, HP, testPlayer.hpMultiplierNormal, testPlayer.hpMultiplierComboEnd, 1.0)); // slider30
								break;
							case 1: // tick
								testPlayer.increaseHealth(OsuScore::getHealthIncrease(OsuScore::HIT::HIT_SLIDER10, HP, testPlayer.hpMultiplierNormal, testPlayer.hpMultiplierComboEnd, 1.0)); // slider10
								break;
							}
						}

						// endcircle
						testPlayer.increaseHealth(OsuScore::getHealthIncrease(OsuScore::HIT::HIT_SLIDER30, HP, testPlayer.hpMultiplierNormal, testPlayer.hpMultiplierComboEnd, 1.0)); // slider30
					}
					else if (spinnerPointer != NULL)
					{
						const int rotationsNeeded = (int)((float)spinnerPointer->getDuration() / 1000.0f * OsuGameRules::getSpinnerSpinsPerSecond(this));
						for (int r=0; r<rotationsNeeded; r++)
						{
							testPlayer.increaseHealth(OsuScore::getHealthIncrease(OsuScore::HIT::HIT_SPINNERSPIN, HP, testPlayer.hpMultiplierNormal, testPlayer.hpMultiplierComboEnd, 1.0)); // spinnerspin
						}
					}

					if (!(maxLongObjectDrop > 0.0) || (testPlayer.health - maxLongObjectDrop) > lowestHpEver)
					{
						// regular hit (for every hitobject)
						testPlayer.increaseHealth(OsuScore::getHealthIncrease(OsuScore::HIT::HIT_300, HP, testPlayer.hpMultiplierNormal, testPlayer.hpMultiplierComboEnd, 1.0)); // 300

						// end of combo (new combo starts at next hitobject)
						if ((i == m_hitobjects.size() - 1) || m_hitobjects[i]->isEndOfCombo())
						{
							testPlayer.increaseHealth(OsuScore::getHealthIncrease(OsuScore::HIT::HIT_300G, HP, testPlayer.hpMultiplierNormal, testPlayer.hpMultiplierComboEnd, 1.0)); // geki

							if (testPlayer.health < lowestHpComboEnd)
							{
								if (++comboTooLowCount > 2)
								{
									testPlayer.hpMultiplierComboEnd *= 1.07;
									testPlayer.hpMultiplierNormal *= 1.03;
									fail = true;
									break;
								}
							}
						}

						continue;
					}

					fail = true;
					testDrop *= 0.96;
					break;
				}

				fail = true;
				testDrop *= 0.96;
				break;
			}

			if (!fail && testPlayer.health < lowestHpEnd)
			{
				fail = true;
				testDrop *= 0.94;
				testPlayer.hpMultiplierComboEnd *= 1.01;
				testPlayer.hpMultiplierNormal *= 1.01;
			}

			const double recovery = (testPlayer.healthUncapped - testPlayer.hpBarMaximum) / (double)m_hitobjects.size();
			if (!fail && recovery < HpRecoveryAvailable)
			{
				fail = true;
				testDrop *= 0.96;
				testPlayer.hpMultiplierComboEnd *= 1.02;
				testPlayer.hpMultiplierNormal *= 1.01;
			}
		}
		while (fail);

		m_fDrainRate = (testDrop / testPlayer.hpBarMaximum) * 1000.0; // from [0, 200] to [0, 1], and from ms to seconds
		m_fHpMultiplierComboEnd = testPlayer.hpMultiplierComboEnd;
		m_fHpMultiplierNormal = testPlayer.hpMultiplierNormal;
	}
	else if (drainType == 2) // osu!lazer 2020
	{
		// build healthIncreases
		std::vector<std::pair<double, double>> healthIncreases; // [first = time, second = health]
		healthIncreases.reserve(m_hitobjects.size());
		const double healthIncreaseForHit300 = OsuScore::getHealthIncrease(OsuScore::HIT::HIT_300);
		for (int i=0; i<m_hitobjects.size(); i++)
		{
			// nested hitobjects
			const auto *sliderPointer = m_hitobjects[i]->asSlider();
			if (sliderPointer != NULL)
			{
				// startcircle
				healthIncreases.emplace_back((double)m_hitobjects[i]->getTime(), healthIncreaseForHit300);

				// ticks + repeats + repeat ticks
				const std::vector<OsuSlider::SLIDERCLICK> &clicks = sliderPointer->getClicks();
				for (int c=0; c<clicks.size(); c++)
				{
					healthIncreases.emplace_back((double)clicks[c].time, healthIncreaseForHit300);
				}
			}

			// regular hitobject
			healthIncreases.emplace_back(m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration(), healthIncreaseForHit300);
		}

		const int numHealthIncreases = healthIncreases.size();
		const int numBreaks = m_breaks.size();
		const double drainStartTime = m_hitobjects[0]->getTime();

		// see computeDrainRate() & https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Scoring/DrainingHealthProcessor.cs

		const double minimum_health_error = 0.01;

		const double min_health_target = cv::osu::drain_lazer_health_min.getFloat();
		const double mid_health_target = cv::osu::drain_lazer_health_mid.getFloat();
		const double max_health_target = cv::osu::drain_lazer_health_max.getFloat();

		const double targetMinimumHealth = OsuGameRules::mapDifficultyRange(getHP(), min_health_target, mid_health_target, max_health_target);

		int adjustment = 1;
		double result = 1.0;

		// Although we expect the following loop to converge within 30 iterations (health within 1/2^31 accuracy of the target),
		// we'll still keep a safety measure to avoid infinite loops by detecting overflows.
		while (adjustment > 0)
		{
			double currentHealth = 1.0;
			double lowestHealth = 1.0;
			int currentBreak = -1;

			for (int i=0; i<numHealthIncreases; i++)
			{
				double currentTime = healthIncreases[i].first;
				double lastTime = i > 0 ? healthIncreases[i - 1].first : drainStartTime;

				// Subtract any break time from the duration since the last object
				if (numBreaks > 0)
				{
					// Advance the last break occuring before the current time
					while (currentBreak + 1 < numBreaks && (double)m_breaks[currentBreak + 1].endTime < currentTime)
					{
						currentBreak++;
					}

					if (currentBreak >= 0)
						lastTime = std::max(lastTime, (double)m_breaks[currentBreak].endTime);
				}

				// Apply health adjustments
				currentHealth -= (healthIncreases[i].first - lastTime) * result;
				lowestHealth = std::min(lowestHealth, currentHealth);
				currentHealth = std::min(1.0, currentHealth + healthIncreases[i].second);

				// Common scenario for when the drain rate is definitely too harsh
				if (lowestHealth < 0)
					break;
			}

			// Stop if the resulting health is within a reasonable offset from the target
			if (std::abs(lowestHealth - targetMinimumHealth) <= minimum_health_error)
				break;

			// This effectively works like a binary search - each iteration the search space moves closer to the target, but may exceed it.
			adjustment *= 2;
			result += 1.0 / adjustment * (std::signbit(lowestHealth - targetMinimumHealth) ? -1.0 : 1.0);
		}

		m_fDrainRate = result * 1000.0; // from ms to seconds
	}
}

void OsuBeatmapStandard::updateStarCache()
{
	if (cv::osu::draw_statistics_pp.getBool() || cv::osu::draw_statistics_livestars.getBool())
	{
		// so we don't get a useless double load inside onModUpdate()
		m_fPrevHitCircleDiameterForStarCache = getHitcircleDiameter();
		m_fPrevSpeedForStarCache = osu->getSpeedMultiplier();

		// kill any running loader, so we get to a clean state
		stopStarCacheLoader();
		resourceManager->destroyResource(m_starCacheLoader);

		// create new loader
		m_starCacheLoader = new OsuBackgroundStarCacheLoader(this);
		m_starCacheLoader->revive(); // activate it
		resourceManager->requestNextLoadAsync();
		resourceManager->loadResource(m_starCacheLoader);
	}
}

void OsuBeatmapStandard::stopStarCacheLoader()
{
	if (!m_starCacheLoader->isDead())
	{
		m_starCacheLoader->kill();
		double startTime = engine->getLiveElapsedEngineTime();
		while (!m_starCacheLoader->isAsyncReady()) // stall main thread until it's killed (this should be very quick, around max 1 ms, as the kill flag is checked in every iteration)
		{
			if (engine->getLiveElapsedEngineTime() - startTime > 2)
			{
				debugLog("WARNING: Ignoring stuck StarCacheLoader thread!\n");
				break;
			}
		}

		// NOTE: this only works safely because OsuBackgroundStarCacheLoader does no work in load(), because it might still be in the ResourceManager's sync load() queue, so future loadAsync() could crash with the old pending load()
	}
}

bool OsuBeatmapStandard::isLoadingStarCache() const
{
	return ((cv::osu::draw_statistics_pp.getBool() || cv::osu::draw_statistics_livestars.getBool()) && !m_starCacheLoader->isReady());
}

bool OsuBeatmapStandard::isLoadingInt() const
{
	return (OsuBeatmap::isLoading() || m_bIsPreLoading || isLoadingStarCache());
}
