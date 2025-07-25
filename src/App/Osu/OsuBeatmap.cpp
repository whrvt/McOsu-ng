//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		beatmap loader
//
// $NoKeywords: $osubm
//===============================================================================//

#include "OsuBeatmap.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "Environment.h"
#include "SoundEngine.h"
#include "AnimationHandler.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Timing.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuMultiplayer.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuBackgroundImageHandler.h"
#include "OsuPauseMenu.h"
#include "OsuGameRules.h"
#include "OsuNotificationOverlay.h"
#include "OsuModSelector.h"
#include "OsuMainMenu.h"

#include "OsuDatabaseBeatmap.h"

#include "OsuBeatmapStandard.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"
#include "OsuSlider.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <cctype>
#include <algorithm>

static constexpr float unioffset = 0.0f +
	(Env::cfg(AUD::WASAPI)	? -25.0f  :
	 Env::cfg(AUD::BASS)	?  15.0f  : // see https://github.com/ppy/osu/blob/6d8c457c81e40cf438c69a1e6c5f02347333dfc0/osu.Game/Beatmaps/FramedBeatmapClock.cs#L68
	 Env::cfg(AUD::SDL)		? -110.0f :
	 Env::cfg(AUD::SOLOUD)	?  18.0f  : 0.0f) +
	(Env::cfg(OS::WASM)		? -75.0f  : 0.0f);

// BASS: starting with bass 2020 2.4.15.2 which has all offset problems fixed, this is the non-dsound backend compensation
// NOTE: this depends on BASS_CONFIG_UPDATEPERIOD/BASS_CONFIG_DEV_BUFFER

// WASAPI: since we use the newer bass/fx dlls for wasapi builds anyway (which have different time handling)

// SDL_mixer: it really needs that much

// SoLoud: im not sure yet, but it's dynamically adjusted in SoLoudFX

// And WASM... well, it needs work.
namespace cv::osu {
ConVar universal_offset_hardcoded("osu_universal_offset_hardcoded", unioffset, FCVAR_NONE);

ConVar pvs("osu_pvs", true, FCVAR_NONE, "optimizes all loops over all hitobjects by clamping the range to the Potentially Visible Set");
ConVar draw_hitobjects("osu_draw_hitobjects", true, FCVAR_NONE);
ConVar draw_beatmap_background_image("osu_draw_beatmap_background_image", true, FCVAR_NONE);

ConVar universal_offset("osu_universal_offset", 0.0f, FCVAR_NONE);
ConVar universal_offset_hardcoded_fallback_dsound("osu_universal_offset_hardcoded_fallback_dsound", -15.0f, FCVAR_NONE);
ConVar old_beatmap_offset("osu_old_beatmap_offset", 24.0f, FCVAR_NONE, "offset in ms which is added to beatmap versions < 5 (default value is hardcoded 24 ms in stable)");
ConVar timingpoints_offset("osu_timingpoints_offset", 5.0f, FCVAR_NONE, "Offset in ms which is added before determining the active timingpoint for the sample type and sample volume (hitsounds) of the current frame");
ConVar interpolate_music_pos("osu_interpolate_music_pos", true, FCVAR_NONE, "Interpolate song position with engine time if the audio library reports the same position more than once");
ConVar compensate_music_speed("osu_compensate_music_speed", true, FCVAR_NONE, "compensates speeds slower than 1x a little bit, by adding an offset depending on the slowness");
ConVar combobreak_sound_combo("osu_combobreak_sound_combo", 20, FCVAR_NONE, "Only play the combobreak sound if the combo is higher than this");
ConVar beatmap_preview_mods_live("osu_beatmap_preview_mods_live", false, FCVAR_NONE, "whether to immediately apply all currently selected mods while browsing beatmaps (e.g. speed/pitch)");
ConVar beatmap_preview_music_loop("osu_beatmap_preview_music_loop", true, FCVAR_NONE);

ConVar ar_override("osu_ar_override", -1.0f, FCVAR_NONE, "use this to override between AR 0 and AR 12.5+. active if value is more than or equal to 0.");
ConVar ar_overridenegative("osu_ar_overridenegative", 0.0f, FCVAR_NONE, "use this to override below AR 0. active if value is less than 0, disabled otherwise. this override always overrides the other override.");
ConVar cs_override("osu_cs_override", -1.0f, FCVAR_NONE, "use this to override between CS 0 and CS 12.1429. active if value is more than or equal to 0.");
ConVar cs_overridenegative("osu_cs_overridenegative", 0.0f, FCVAR_NONE, "use this to override below CS 0. active if value is less than 0, disabled otherwise. this override always overrides the other override.");
ConVar cs_cap_sanity("osu_cs_cap_sanity", true, FCVAR_NONE);
ConVar hp_override("osu_hp_override", -1.0f, FCVAR_NONE);
ConVar od_override("osu_od_override", -1.0f, FCVAR_NONE);
ConVar ar_override_lock("osu_ar_override_lock", false, FCVAR_NONE, "always force constant AR even through speed changes");
ConVar od_override_lock("osu_od_override_lock", false, FCVAR_NONE, "always force constant OD even through speed changes");

ConVar background_dim("osu_background_dim", 0.9f, FCVAR_NONE);
ConVar background_alpha("osu_background_alpha", 1.0f, FCVAR_NONE, "transparency of all background layers at once, only useful for FPoSu");
ConVar background_fade_after_load("osu_background_fade_after_load", true, FCVAR_NONE);
ConVar background_dont_fade_during_breaks("osu_background_dont_fade_during_breaks", false, FCVAR_NONE);
ConVar background_fade_min_duration("osu_background_fade_min_duration", 1.4f, FCVAR_NONE, "Only fade if the break is longer than this (in seconds)");
ConVar background_fade_in_duration("osu_background_fade_in_duration", 0.85f, FCVAR_NONE);
ConVar background_fade_out_duration("osu_background_fade_out_duration", 0.25f, FCVAR_NONE);
ConVar background_brightness("osu_background_brightness", 0.0f, FCVAR_NONE, "0 to 1, if this is larger than 0 then it will replace the entire beatmap background image with a solid color (see osu_background_color_r/g/b)");
ConVar background_color_r("osu_background_color_r", 255.0f, FCVAR_NONE, "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar background_color_g("osu_background_color_g", 255.0f, FCVAR_NONE, "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar background_color_b("osu_background_color_b", 255.0f, FCVAR_NONE, "0 to 255, only relevant if osu_background_brightness is larger than 0");
ConVar hiterrorbar_misaims("osu_hiterrorbar_misaims", true, FCVAR_NONE);

ConVar followpoints_prevfadetime("osu_followpoints_prevfadetime", 400.0f, FCVAR_NONE); // TODO: this shouldn't be in this class

ConVar auto_and_relax_block_user_input("osu_auto_and_relax_block_user_input", true, FCVAR_NONE);

ConVar mod_timewarp("osu_mod_timewarp", false, FCVAR_NONE);
ConVar mod_timewarp_multiplier("osu_mod_timewarp_multiplier", 1.5f, FCVAR_NONE);
ConVar mod_minimize("osu_mod_minimize", false, FCVAR_NONE);
ConVar mod_minimize_multiplier("osu_mod_minimize_multiplier", 0.5f, FCVAR_NONE);
ConVar mod_jigsaw1("osu_mod_jigsaw1", false, FCVAR_NONE);
ConVar mod_artimewarp("osu_mod_artimewarp", false, FCVAR_NONE);
ConVar mod_artimewarp_multiplier("osu_mod_artimewarp_multiplier", 0.5f, FCVAR_NONE);
ConVar mod_arwobble("osu_mod_arwobble", false, FCVAR_NONE);
ConVar mod_arwobble_strength("osu_mod_arwobble_strength", 1.0f, FCVAR_NONE);
ConVar mod_arwobble_interval("osu_mod_arwobble_interval", 7.0f, FCVAR_NONE);
ConVar mod_fullalternate("osu_mod_fullalternate", false, FCVAR_NONE);

ConVar early_note_time("osu_early_note_time", 1500.0f, FCVAR_NONE, "Timeframe in ms at the beginning of a beatmap which triggers a starting delay for easier reading");
ConVar quick_retry_time("osu_quick_retry_time", 2000.0f, FCVAR_NONE, "Timeframe in ms subtracted from the first hitobject when quick retrying (not regular retry)");
ConVar end_delay_time("osu_end_delay_time", 750.0f, FCVAR_NONE, "Duration in ms which is added at the end of a beatmap after the last hitobject is finished but before the ranking screen is automatically shown");
ConVar end_skip("osu_end_skip", true, FCVAR_NONE, "whether the beatmap jumps to the ranking screen as soon as the last hitobject plus lenience has passed");
ConVar end_skip_time("osu_end_skip_time", 400.0f, FCVAR_NONE, "Duration in ms which is added to the endTime of the last hitobject, after which pausing the game will immediately jump to the ranking screen");
ConVar skip_time("osu_skip_time", 5000.0f, FCVAR_NONE, "Timeframe in ms within a beatmap which allows skipping if it doesn't contain any hitobjects");
ConVar fail_time("osu_fail_time", 2.25f, FCVAR_NONE, "Timeframe in s for the slowdown effect after failing, before the pause menu is shown");
ConVar notelock_type("osu_notelock_type", 2, FCVAR_NONE, "which notelock algorithm to use (0 = None, 1 = McOsu, 2 = osu!stable, 3 = osu!lazer 2020)");
ConVar notelock_stable_tolerance2b("osu_notelock_stable_tolerance2b", 3, FCVAR_NONE, "time tolerance in milliseconds to allow hitting simultaneous objects close together (e.g. circle at end of slider)");
ConVar mod_suddendeath_restart("osu_mod_suddendeath_restart", false, FCVAR_NONE, "osu! has this set to false (i.e. you fail after missing). if set to true, then behave like SS/PF, instantly restarting the map");
ConVar unpause_continue_delay("osu_unpause_continue_delay", 0.15f, FCVAR_NONE, "when unpausing, wait for this many seconds before allowing \"click to continue\" to be actually clicked (to avoid instantly triggering accidentally)");

ConVar drain_type("osu_drain_type", 1, FCVAR_NONE, "which hp drain algorithm to use (0 = None, 1 = osu!stable, 2 = osu!lazer 2020, 3 = osu!lazer 2018)");
ConVar drain_kill("osu_drain_kill", true, FCVAR_NONE, "whether to kill the player upon failing");
ConVar drain_kill_notification_duration("osu_drain_kill_notification_duration", 1.0f, FCVAR_NONE, "how long to display the \"You have failed, but you can keep playing!\" notification (0 = disabled)");

ConVar drain_stable_passive_fail("osu_drain_stable_passive_fail", false, FCVAR_NONE, "whether to fail the player instantly if health = 0, or only once a negative judgement occurs");
ConVar drain_stable_break_before("osu_drain_stable_break_before", false, FCVAR_NONE, "drain after last hitobject before a break actually starts");
ConVar drain_stable_break_before_old("osu_drain_stable_break_before_old", true, FCVAR_NONE, "for beatmap versions < 8, drain after last hitobject before a break actually starts");
ConVar drain_stable_break_after("osu_drain_stable_break_after", false, FCVAR_NONE, "drain after a break before the next hitobject can be clicked");
ConVar drain_lazer_passive_fail("osu_drain_lazer_passive_fail", false, FCVAR_NONE, "whether to fail the player instantly if health = 0, or only once a negative judgement occurs");
ConVar drain_lazer_break_before("osu_drain_lazer_break_before", false, FCVAR_NONE, "drain after last hitobject before a break actually starts");
ConVar drain_lazer_break_after("osu_drain_lazer_break_after", false, FCVAR_NONE, "drain after a break before the next hitobject can be clicked");
ConVar drain_stable_spinner_nerf("osu_drain_stable_spinner_nerf", 0.25f, FCVAR_NONE, "drain gets multiplied with this while a spinner is active");
ConVar drain_stable_hpbar_recovery("osu_drain_stable_hpbar_recovery", 160.0f, FCVAR_NONE, "hp gets set to this value when failing with ez and causing a recovery");

ConVar play_hitsound_on_click_while_playing("osu_play_hitsound_on_click_while_playing", false, FCVAR_NONE);

ConVar debug_draw_timingpoints("osu_debug_draw_timingpoints", false, FCVAR_NONE);
}

OsuBeatmap::OsuBeatmap()
{
	// convar refs












	// vars
	

	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_bIsWaiting = false;
	m_bIsRestartScheduled = false;
	m_bIsRestartScheduledQuick = false;

	m_bIsInSkippableSection = false;
	m_bShouldFlashWarningArrows = false;
	m_fShouldFlashSectionPass = 0.0f;
	m_fShouldFlashSectionFail = 0.0f;
	m_bContinueScheduled = false;
	m_iContinueMusicPos = 0;
	m_fWaitTime = 0.0f;
	m_fPrevUnpauseTime = 0.0f;

	m_selectedDifficulty2 = NULL;

	m_music = NULL;

	m_fMusicFrequencyBackup = cv::snd_freq.getFloat();
	m_iCurMusicPos = 0;
	m_iCurMusicPosWithOffsets = 0;
	m_bWasSeekFrame = false;
	m_fInterpolatedMusicPos = 0.0;
	m_fLastAudioTimeAccurateSet = 0.0;
	m_fLastRealTimeForInterpolationDelta = 0.0;
	m_iResourceLoadUpdateDelayHack = 0;
	m_bForceStreamPlayback = true; // if this is set to true here, then the music will always be loaded as a stream (meaning slow disk access could cause audio stalling/stuttering)
	m_fAfterMusicIsFinishedVirtualAudioTimeStart = -1.0;
	m_bIsFirstMissSound = true;

	m_bFailed = false;
	m_fFailAnim = 1.0f;
	m_fHealth = 1.0;
	m_fHealth2 = 1.0f;

	m_fDrainRate = 0.0;
	m_fHpMultiplierNormal = 1.0;
	m_fHpMultiplierComboEnd = 1.0;

	m_fBreakBackgroundFade = 0.0f;
	m_bInBreak = false;
	m_currentHitObject = NULL;
	m_iNextHitObjectTime = 0;
	m_iPreviousHitObjectTime = 0;
	m_iPreviousSectionPassFailTime = -1;

	m_bClick1Held = false;
	m_bClick2Held = false;
	m_bClickedContinue = false;
	m_bPrevKeyWasKey1 = false;
	m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = 0;

	m_iRandomSeed = 0;

	m_iNPS = 0;
	m_iND = 0;
	m_iCurrentHitObjectIndex = 0;
	m_iCurrentNumCircles = 0;
	m_iCurrentNumSliders = 0;
	m_iCurrentNumSpinners = 0;
	m_iMaxPossibleCombo = 0;
	m_iScoreV2ComboPortionMaximum = 0;

	m_iPreviousFollowPointObjectIndex = -1;
}

OsuBeatmap::~OsuBeatmap()
{
	anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
	anim->deleteExistingAnimation(&m_fHealth2);
	anim->deleteExistingAnimation(&m_fFailAnim);

	unloadObjects();
}

void OsuBeatmap::draw()
{
	drawInt();
}

void OsuBeatmap::drawInt()
{
	if (!canDraw()) return;

	// draw background
	drawBackground();

	// draw loading circle
	if (isLoading())
		osu->getHUD()->drawLoadingSmall();
}

void OsuBeatmap::draw3D()
{
	if (!canDraw()) return;

	// empty atm
}

void OsuBeatmap::draw3D2()
{
	if (!canDraw()) return;

	// empty atm
}

void OsuBeatmap::drawDebug()
{
	if (cv::osu::debug_draw_timingpoints.getBool())
	{
		McFont *debugFont = resourceManager->getFont("FONT_DEFAULT");
		g->setColor(0xffffffff);
		g->pushTransform();
		g->translate(5, debugFont->getHeight() + 5 - mouse->getPos().y);
		{
			for (const OsuDatabaseBeatmap::TIMINGPOINT &t : m_selectedDifficulty2->getTimingpoints())
			{
				g->drawString(debugFont, UString::format("%li,%f,%i,%i,%i", t.offset, t.msPerBeat, t.sampleType, t.sampleSet, t.volume));
				g->translate(0, debugFont->getHeight());
			}
		}
		g->popTransform();
	}
}

void OsuBeatmap::drawBackground()
{
	if (!canDraw()) return;

	// draw beatmap background image
	if (cv::osu::background_brightness.getFloat() <= 0.0f)
	{
		Image *backgroundImage = osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_selectedDifficulty2);
		if (cv::osu::draw_beatmap_background_image.getBool() && backgroundImage != NULL && (cv::osu::background_dim.getFloat() < 1.0f || m_fBreakBackgroundFade > 0.0f))
		{
			const float scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getVirtScreenSize());
			const Vector2 centerTrans = (osu->getVirtScreenSize()/2.0f);

			const float backgroundFadeDimMultiplier = 1.0f - (cv::osu::background_dim.getFloat() - 0.3f);
			const auto dim = (1.0f - cv::osu::background_dim.getFloat()) + m_fBreakBackgroundFade*backgroundFadeDimMultiplier;
			const auto alpha = cv::osu::fposu::mod_fposu.getBool() ? cv::osu::background_alpha.getFloat() : 1.0f;

			g->setColor(argb(alpha, dim, dim, dim));
			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate((int)centerTrans.x, (int)centerTrans.y);
				g->drawImage(backgroundImage);
			}
			g->popTransform();
		}
	}

	// draw background
	if (cv::osu::background_brightness.getFloat() > 0.0f)
	{
		const auto brightness = cv::osu::background_brightness.getFloat();

		const auto red = brightness * cv::osu::background_color_r.getFloat();
		const auto green = brightness * cv::osu::background_color_g.getFloat();
		const auto blue = brightness * cv::osu::background_color_b.getFloat();
		const auto alpha = (1.0f - m_fBreakBackgroundFade) * (cv::osu::fposu::mod_fposu.getBool() ? cv::osu::background_alpha.getFloat() : 1.0f);

		g->setColor(argb(alpha, red, green, blue));
		g->fillRect(0, 0, osu->getVirtScreenWidth(), osu->getVirtScreenHeight());
	}

	// draw scorebar-bg
	if (cv::osu::draw_hud.getBool() && cv::osu::draw_scorebarbg.getBool() && (!cv::osu::fposu::mod_fposu.getBool() || (!cv::osu::fposu::threeD.getBool() && !cv::osu::fposu::draw_scorebarbg_on_top.getBool()))) // NOTE: special case for FPoSu
		osu->getHUD()->drawScorebarBg(cv::osu::hud_scorebar_hide_during_breaks.getBool() ? (1.0f - m_fBreakBackgroundFade) : 1.0f, osu->getHUD()->getScoreBarBreakAnim());

	if (cv::osu::debug.getBool())
	{
		int y = 50;

		if (m_bIsPaused)
		{
			g->setColor(0xffffffff);
			g->fillRect(50, y, 15, 50);
			g->fillRect(50 + 50 - 15, y, 15, 50);
		}

		y += 100;

		if (m_bIsWaiting)
		{
			g->setColor(0xff00ff00);
			g->fillRect(50, y, 50, 50);
		}

		y += 100;

		if (m_bIsPlaying)
		{
			g->setColor(0xff0000ff);
			g->fillRect(50, y, 50, 50);
		}

		y += 100;

		if (m_bForceStreamPlayback)
		{
			g->setColor(0xffff0000);
			g->fillRect(50, y, 50, 50);
		}

		y += 100;

		if (m_hitobjectsSortedByEndTime.size() > 0)
		{
			OsuHitObject *lastHitObject = m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1];
			if (lastHitObject->isFinished() && m_iCurMusicPos > lastHitObject->getTime() + lastHitObject->getDuration() + (long)cv::osu::end_skip_time.getInt())
			{
				g->setColor(0xff00ffff);
				g->fillRect(50, y, 50, 50);
			}

			y += 100;
		}
	}
}

void OsuBeatmap::update()
{
	if (!canUpdate()) return;

	if (m_bContinueScheduled)
	{
		bool isEarlyNoteContinue = (!m_bIsPaused && m_bIsWaiting); // if we paused while m_bIsWaiting (green progressbar), then we have to let the 'if (m_bIsWaiting)' block handle the sound play() call
		if (m_bClickedContinue || isEarlyNoteContinue) // originally was (m_bClick1Held || m_bClick2Held || isEarlyNoteContinue), replaced first two m_bClickedContinue
		{
			m_bClickedContinue = false;
			m_bContinueScheduled = false;
			m_bIsPaused = false;

			if (!isEarlyNoteContinue)
				soundEngine->play(m_music);

			m_bIsPlaying = true; // usually this should be checked with the result of the above play() call, but since we are continuing we can assume that everything works

			// for nightmare mod, to avoid a miss because of the continue click
			{
				m_clicks.clear();
				m_keyUps.clear();
			}
		}
	}

	// handle restarts
	if (m_bIsRestartScheduled)
	{
		m_bIsRestartScheduled = false;
		actualRestart();
		return;
	}

	// update current music position (this variable does not include any offsets!)
	m_iCurMusicPos = getMusicPositionMSInterpolated();
	m_iContinueMusicPos = m_music->getPositionMS();
	const bool wasSeekFrame = m_bWasSeekFrame;
	m_bWasSeekFrame = false;

	// handle timewarp
	if (cv::osu::mod_timewarp.getBool())
	{
		if (m_hitobjects.size() > 0 && m_iCurMusicPos > m_hitobjects[0]->getTime())
		{
			const float percentFinished = ((double)(m_iCurMusicPos - m_hitobjects[0]->getTime()) / (double)(m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() - m_hitobjects[0]->getTime()));
			const float speed = osu->getSpeedMultiplier() + percentFinished*osu->getSpeedMultiplier()*(cv::osu::mod_timewarp_multiplier.getFloat()-1.0f);
			m_music->setSpeed(speed);
		}
	}

	// HACKHACK: clean this mess up
	// waiting to start (file loading, retry)
	// NOTE: this is dependent on being here AFTER m_iCurMusicPos has been set above, because it modifies it to fake a negative start (else everything would just freeze for the waiting period)
	if (m_bIsWaiting)
	{
		if (isLoading())
		{
			m_fWaitTime = Timing::getTimeReal();

			// if the first hitobject starts immediately, add artificial wait time before starting the music
			if (!m_bIsRestartScheduledQuick && m_hitobjects.size() > 0)
			{
				if (m_hitobjects[0]->getTime() < (long)cv::osu::early_note_time.getInt())
					m_fWaitTime = Timing::getTimeReal() + cv::osu::early_note_time.getFloat()/1000.0f;
			}
		}
		else
		{
			if (Timing::getTimeReal() > m_fWaitTime)
			{
				if (!m_bIsPaused)
				{
					m_bIsWaiting = false;
					m_bIsPlaying = true;

					soundEngine->play(m_music);
					m_music->setPosition(0.0);
					m_music->setVolume(cv::osu::volume_music.getFloat());

					// if we are quick restarting, jump just before the first hitobject (even if there is a long waiting period at the beginning with nothing etc.)
					if (m_bIsRestartScheduledQuick && m_hitobjects.size() > 0 && m_hitobjects[0]->getTime() > (long)cv::osu::quick_retry_time.getInt())
						m_music->setPositionMS(std::max((long)0, m_hitobjects[0]->getTime() - (long)cv::osu::quick_retry_time.getInt()));

					m_bIsRestartScheduledQuick = false;

					onPlayStart();
				}
			}
			else
				m_iCurMusicPos = static_cast<long>((Timing::getTimeReal() - m_fWaitTime) * 1000.0f * osu->getSpeedMultiplier());
		}

		// ugh. force update all hitobjects while waiting (necessary because of pvs optimization)
		long curPos = m_iCurMusicPos
			+ (long)(cv::osu::universal_offset.getFloat() * osu->getSpeedMultiplier())
			+ (long)cv::osu::universal_offset_hardcoded.getInt()
			+ (cv::win_snd_fallback_dsound.getBool() ? (long)cv::osu::universal_offset_hardcoded_fallback_dsound.getInt() : 0)
			- m_selectedDifficulty2->getLocalOffset()
			- m_selectedDifficulty2->getOnlineOffset()
			- (m_selectedDifficulty2->getVersion() < 5 ? cv::osu::old_beatmap_offset.getInt() : 0);
		if (curPos > -1) // otherwise auto would already click elements that start at exactly 0 (while the map has not even started)
			curPos = -1;

		for (int i=0; i<m_hitobjects.size(); i++)
		{
			m_hitobjects[i]->update(curPos);
		}
	}

	// only continue updating hitobjects etc. if we have loaded everything
	if (isLoading()) return;

	// handle music loading fail
	if (!m_music->isReady())
	{
		m_iResourceLoadUpdateDelayHack++; // HACKHACK: async loading takes 1 additional engine update() until both isAsyncReady() and isReady() return true
		if (m_iResourceLoadUpdateDelayHack > 1 && !m_bForceStreamPlayback) // first: try loading a stream version of the music file
		{
			m_bForceStreamPlayback = true;
			unloadMusicInt();
			loadMusic(true, m_bForceStreamPlayback);

			// we are waiting for an asynchronous start of the beatmap in the next update()
			m_bIsWaiting = true;
			m_fWaitTime = Timing::getTimeReal();
		}
		else if (m_iResourceLoadUpdateDelayHack > 3) // second: if that still doesn't work, stop and display an error message
		{
			osu->getNotificationOverlay()->addNotification("Couldn't load music file :(", 0xffff0000);
			stop(true);
		}
	}

	// detect and handle music end
	if (!m_bIsWaiting && m_music->isReady())
	{
		const bool isMusicFinished = m_music->isFinished();

		// trigger virtual audio time after music finishes
		if (!isMusicFinished)
			m_fAfterMusicIsFinishedVirtualAudioTimeStart = -1.0;
		else if (m_fAfterMusicIsFinishedVirtualAudioTimeStart < 0.0)
			m_fAfterMusicIsFinishedVirtualAudioTimeStart = Timing::getTimeReal();

		if (isMusicFinished)
		{
			// continue with virtual audio time until the last hitobject is done (plus sanity offset given via osu_end_delay_time)
			// because some beatmaps have hitobjects going until >= the exact end of the music ffs
			// NOTE: this overwrites m_iCurMusicPos for the rest of the update loop
			m_iCurMusicPos = (long)m_music->getLengthMS() + (long)((Timing::getTimeReal() - m_fAfterMusicIsFinishedVirtualAudioTimeStart)*1000.0);
		}

		const bool hasAnyHitObjects = (m_hitobjects.size() > 0);
		const bool isTimePastLastHitObjectPlusLenience = (m_iCurMusicPos > (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size()-1]->getTime() + m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size()-1]->getDuration() + (long)cv::osu::end_delay_time.getInt()));
		if (!hasAnyHitObjects || (cv::osu::end_skip.getBool() && isTimePastLastHitObjectPlusLenience) || (!cv::osu::end_skip.getBool() && isMusicFinished))
		{
			if (!m_bFailed)
			{
				stop(false);
				return;
			}
		}
	}

	// update timing (points)
	m_iCurMusicPosWithOffsets = m_iCurMusicPos
		+ (long)(cv::osu::universal_offset.getFloat() * osu->getSpeedMultiplier())
		+ (long)cv::osu::universal_offset_hardcoded.getInt()
		+ (cv::win_snd_fallback_dsound.getBool() ? (long)cv::osu::universal_offset_hardcoded_fallback_dsound.getInt() : 0)
		- m_selectedDifficulty2->getLocalOffset()
		- m_selectedDifficulty2->getOnlineOffset()
		- (m_selectedDifficulty2->getVersion() < 5 ? cv::osu::old_beatmap_offset.getInt() : 0);
	updateTimingPoints(m_iCurMusicPosWithOffsets);

	// for performance reasons, a lot of operations are crammed into 1 loop over all hitobjects:
	// update all hitobjects,
	// handle click events,
	// also get the time of the next/previous hitobject and their indices for later,
	// and get the current hitobject,
	// also handle miss hiterrorbar slots,
	// also calculate nps and nd,
	// also handle note blocking
	m_currentHitObject = NULL;
	m_iNextHitObjectTime = 0;
	m_iPreviousHitObjectTime = 0;
	m_iPreviousFollowPointObjectIndex = 0;
	m_iNPS = 0;
	m_iND = 0;
	m_iCurrentNumCircles = 0;
	m_iCurrentNumSliders = 0;
	m_iCurrentNumSpinners = 0;
	{
		bool blockNextNotes = false;

		const long pvs = !cv::osu::stdrules::mod_mafham.getBool() ? getPVS() : (m_hitobjects.size() > 0 ? (m_hitobjects[std::clamp<int>(m_iCurrentHitObjectIndex + cv::osu::stdrules::mod_mafham_render_livesize.getInt() + 1, 0, m_hitobjects.size()-1)]->getTime() - m_iCurMusicPosWithOffsets + 1500) : getPVS());
		const bool usePVS = cv::osu::pvs.getBool();

		const int notelockType = cv::osu::notelock_type.getInt();
		const long tolerance2B = (long)cv::osu::notelock_stable_tolerance2b.getInt();

		m_iCurrentHitObjectIndex = 0; // reset below here, since it's needed for mafham pvs

		for (int i=0; i<m_hitobjects.size(); i++)
		{
			// the order must be like this:
			// 0) miscellaneous stuff (minimal performance impact)
			// 1) prev + next time vars
			// 2) PVS optimization
			// 3) main hitobject update
			// 4) note blocking
			// 5) click events
			//
			// (because the hitobjects need to know about note blocking before handling the click events)

			// ************ live pp block start ************ //
			const auto type = m_hitobjects[i]->getType();
			const bool isCircle = (type == OsuHitObject::CIRCLE);
			const bool isSlider = (type == OsuHitObject::SLIDER);
			const bool isSpinner = (type == OsuHitObject::SPINNER);
			// ************ live pp block end ************** //

			// determine previous & next object time, used for auto + followpoints + warning arrows + empty section skipping
			if (m_iNextHitObjectTime == 0)
			{
				if (m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets)
					m_iNextHitObjectTime = m_hitobjects[i]->getTime();
				else
				{
					m_currentHitObject = m_hitobjects[i];
					const long actualPrevHitObjectTime = m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration();
					m_iPreviousHitObjectTime = actualPrevHitObjectTime;

					if (m_iCurMusicPosWithOffsets > actualPrevHitObjectTime + (long)cv::osu::followpoints_prevfadetime.getFloat())
						m_iPreviousFollowPointObjectIndex = i;
				}
			}

			// PVS optimization
			if (usePVS)
			{
				if (m_hitobjects[i]->isFinished() && (m_iCurMusicPosWithOffsets - pvs > m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration())) // past objects
				{
					// ************ live pp block start ************ //
					if (isCircle)
						m_iCurrentNumCircles++;
					if (isSlider)
						m_iCurrentNumSliders++;
					if (isSpinner)
						m_iCurrentNumSpinners++;

					m_iCurrentHitObjectIndex = i;
					// ************ live pp block end ************** //

					continue;
				}
				if (m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets + pvs) // future objects
					break;
			}

			// ************ live pp block start ************ //
			if (m_iCurMusicPosWithOffsets >= m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration())
				m_iCurrentHitObjectIndex = i;
			// ************ live pp block end ************** //

			// main hitobject update
			m_hitobjects[i]->update(m_iCurMusicPosWithOffsets);

			// note blocking / notelock (1)
			auto *currentSliderPointer = m_hitobjects[i]->asSlider();
			if (notelockType > 0)
			{
				m_hitobjects[i]->setBlocked(blockNextNotes);

				if (notelockType == 1) // McOsu
				{
					// (nothing, handled in (2) block)
				}
				else if (notelockType == 2) // osu!stable
				{
					if (!m_hitobjects[i]->isFinished())
					{
						blockNextNotes = true;

						// old implementation
						// sliders are "finished" after their startcircle
						/*
						{
							auto *sliderPointer = m_hitobjects[i]->asSlider();

							// sliders with finished startcircles do not block
							if (sliderPointer != NULL && sliderPointer->isStartCircleFinished())
								blockNextNotes = false;
						}
						*/

						// new implementation
						// sliders are "finished" after they end
						// extra handling for simultaneous/2b hitobjects, as these would now otherwise get blocked completely
						// NOTE: this will (same as the old implementation) still unlock some simultaneous/2b patterns too early (slider slider circle [circle]), but nobody from that niche has complained so far
						{
							const bool isSlider = (currentSliderPointer != NULL);
							const bool isSpinner = (!isSlider && !(type == OsuHitObject::CIRCLE));

							if (isSlider || isSpinner)
							{
								if ((i + 1) < m_hitobjects.size())
								{
									if ((isSpinner || currentSliderPointer->isStartCircleFinished()) && (m_hitobjects[i + 1]->getTime() <= (m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration() + tolerance2B)))
										blockNextNotes = false;
								}
							}
						}
					}
				}
				else if (notelockType == 3) // osu!lazer 2020
				{
					if (!m_hitobjects[i]->isFinished())
					{
						const bool isSlider = (currentSliderPointer != NULL);
						const bool isSpinner = (!isSlider && !isCircle);

						if (!isSpinner) // spinners are completely ignored (transparent)
						{
							blockNextNotes = (m_iCurMusicPosWithOffsets <= m_hitobjects[i]->getTime());

							// sliders are "finished" after their startcircle
							{
								// sliders with finished startcircles do not block
								if (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished())
									blockNextNotes = false;
							}
						}
					}
				}
			}
			else
				m_hitobjects[i]->setBlocked(false);

			// click events (this also handles hitsounds!)
			const bool isCurrentHitObjectASliderAndHasItsStartCircleFinishedBeforeClickEvents = (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished());
			const bool isCurrentHitObjectFinishedBeforeClickEvents = m_hitobjects[i]->isFinished();
			{
				if (m_clicks.size() > 0)
					m_hitobjects[i]->onClickEvent(m_clicks);

				if (m_keyUps.size() > 0)
					m_hitobjects[i]->onKeyUpEvent(m_keyUps);
			}
			const bool isCurrentHitObjectFinishedAfterClickEvents = m_hitobjects[i]->isFinished();
			const bool isCurrentHitObjectASliderAndHasItsStartCircleFinishedAfterClickEvents = (currentSliderPointer != NULL && currentSliderPointer->isStartCircleFinished());

			// note blocking / notelock (2.1)
			if (!isCurrentHitObjectASliderAndHasItsStartCircleFinishedBeforeClickEvents && isCurrentHitObjectASliderAndHasItsStartCircleFinishedAfterClickEvents)
			{
				// in here if a slider had its startcircle clicked successfully in this update iteration

				if (notelockType == 2) // osu!stable
				{
					// edge case: frame perfect double tapping on overlapping sliders would incorrectly eat the second input, because the isStartCircleFinished() 2b edge case check handling happens before m_hitobjects[i]->onClickEvent(m_clicks);
					// so, we check if the currentSliderPointer got its isStartCircleFinished() within this m_hitobjects[i]->onClickEvent(m_clicks); and unlock blockNextNotes if that is the case
					// note that we still only unlock within duration + tolerance2B (same as in (1))
					if ((i + 1) < m_hitobjects.size())
					{
						if ((m_hitobjects[i + 1]->getTime() <= (m_hitobjects[i]->getTime() + m_hitobjects[i]->getDuration() + tolerance2B)))
							blockNextNotes = false;
					}
				}
			}

			// note blocking / notelock (2.2)
			if (!isCurrentHitObjectFinishedBeforeClickEvents && isCurrentHitObjectFinishedAfterClickEvents)
			{
				// in here if a hitobject has been clicked (and finished completely) successfully in this update iteration

				blockNextNotes = false;

				if (notelockType == 1) // McOsu
				{
					// auto miss all previous unfinished hitobjects, always
					// (can stop reverse iteration once we get to the first finished hitobject)

					for (int m=i-1; m>=0; m--)
					{
						if (!m_hitobjects[m]->isFinished())
						{
							const auto *sliderPointer = m_hitobjects[m]->asSlider();

							const bool isSlider = (sliderPointer != NULL);
							const bool isSpinner = (!isSlider && !isCircle);

							if (!isSpinner) // spinners are completely ignored (transparent)
							{
								if (m_hitobjects[i]->getTime() > (m_hitobjects[m]->getTime() + m_hitobjects[m]->getDuration())) // NOTE: 2b exception. only force miss if objects are not overlapping.
									m_hitobjects[m]->miss(m_iCurMusicPosWithOffsets);
							}
						}
						else
							break;
					}
				}
				else if (notelockType == 2) // osu!stable
				{
					// (nothing, handled in (1) and (2.1) blocks)
				}
				else if (notelockType == 3) // osu!lazer 2020
				{
					// auto miss all previous unfinished hitobjects if the current music time is > their time (center)
					// (can stop reverse iteration once we get to the first finished hitobject)

					for (int m=i-1; m>=0; m--)
					{
						if (!m_hitobjects[m]->isFinished())
						{
							const auto *sliderPointer = m_hitobjects[m]->asSlider();

							const bool isSlider = (sliderPointer != NULL);
							const bool isSpinner = (!isSlider && !isCircle);

							if (!isSpinner) // spinners are completely ignored (transparent)
							{
								if (m_iCurMusicPosWithOffsets > m_hitobjects[m]->getTime())
								{
									if (m_hitobjects[i]->getTime() > (m_hitobjects[m]->getTime() + m_hitobjects[m]->getDuration())) // NOTE: 2b exception. only force miss if objects are not overlapping.
										m_hitobjects[m]->miss(m_iCurMusicPosWithOffsets);
								}
							}
						}
						else
							break;
					}
				}
			}

			// ************ live pp block start ************ //
			if (isCircle && m_hitobjects[i]->isFinished())
				m_iCurrentNumCircles++;
			if (isSlider && m_hitobjects[i]->isFinished())
				m_iCurrentNumSliders++;
			if (isSpinner && m_hitobjects[i]->isFinished())
				m_iCurrentNumSpinners++;

			if (m_hitobjects[i]->isFinished())
				m_iCurrentHitObjectIndex = i;
			// ************ live pp block end ************** //

			// notes per second
			const long npsHalfGateSizeMS = (long)(500.0f * getSpeedMultiplier());
			if (m_hitobjects[i]->getTime() > m_iCurMusicPosWithOffsets-npsHalfGateSizeMS && m_hitobjects[i]->getTime() < m_iCurMusicPosWithOffsets+npsHalfGateSizeMS)
				m_iNPS++;

			// note density
			if (m_hitobjects[i]->isVisible())
				m_iND++;
		}

		// miss hiterrorbar slots
		// this gets the closest previous unfinished hitobject, as well as all following hitobjects which are in 50 range and could be clicked
		if (cv::osu::hiterrorbar_misaims.getBool())
		{
			m_misaimObjects.clear();
			OsuHitObject *lastUnfinishedHitObject = NULL;
			const long hitWindow50 = (long)OsuGameRules::getHitWindow50(this);
			for (int i=0; i<m_hitobjects.size(); i++) // this shouldn't hurt performance too much, since no expensive operations are happening within the loop
			{
				if (!m_hitobjects[i]->isFinished())
				{
					if (m_iCurMusicPosWithOffsets >= m_hitobjects[i]->getTime())
						lastUnfinishedHitObject = m_hitobjects[i];
					else if (std::abs(m_hitobjects[i]->getTime() - m_iCurMusicPosWithOffsets) < hitWindow50)
						m_misaimObjects.push_back(m_hitobjects[i]);
					else
						break;
				}
			}
			if (lastUnfinishedHitObject != NULL && std::abs(lastUnfinishedHitObject->getTime() - m_iCurMusicPosWithOffsets) < hitWindow50)
				m_misaimObjects.insert(m_misaimObjects.begin(), lastUnfinishedHitObject);

			// now, go through the remaining clicks, and go through the unfinished hitobjects.
			// handle misaim clicks sequentially (setting the misaim flag on the hitobjects to only allow 1 entry in the hiterrorbar for misses per object)
			// clicks don't have to be consumed here, as they are deleted below anyway
			for (int c=0; c<m_clicks.size(); c++)
			{
				for (int i=0; i<m_misaimObjects.size(); i++)
				{
					if (m_misaimObjects[i]->hasMisAimed()) // only 1 slot per object!
						continue;

					m_misaimObjects[i]->misAimed();
					const long delta = (long)m_clicks[c].musicPos - (long)m_misaimObjects[i]->getTime();
					osu->getHUD()->addHitError(delta, false, true);

					break; // the current click has been dealt with (and the hitobject has been misaimed)
				}
			}
		}

		// all remaining clicks which have not been consumed by any hitobjects can safely be deleted
		if (m_clicks.size() > 0)
		{
			if (cv::osu::play_hitsound_on_click_while_playing.getBool())
				osu->getSkin()->playHitCircleSound(0);

			// nightmare mod: extra clicks = sliderbreak
			if ((osu->getModNM() || cv::osu::mod_jigsaw1.getBool()) && !m_bIsInSkippableSection && !m_bInBreak && m_iCurrentHitObjectIndex > 0)
			{
				addSliderBreak();
				addHitResult(NULL, OsuScore::HIT::HIT_MISS_SLIDERBREAK, 0, false, true, true, true, true, false); // only decrease health
			}

			m_clicks.clear();
		}
		m_keyUps.clear();
	}

	// empty section detection & skipping
	if (m_hitobjects.size() > 0)
	{
		const long legacyOffset = (m_iPreviousHitObjectTime < m_hitobjects[0]->getTime() ? 0 : 1000); // Mc
		const long nextHitObjectDelta = m_iNextHitObjectTime - (long)m_iCurMusicPosWithOffsets;
		if (nextHitObjectDelta > 0 && nextHitObjectDelta > (long)cv::osu::skip_time.getInt() && m_iCurMusicPosWithOffsets > (m_iPreviousHitObjectTime + legacyOffset))
			m_bIsInSkippableSection = true;
		else if (!cv::osu::end_skip.getBool() && nextHitObjectDelta < 0)
			m_bIsInSkippableSection = true;
		else
			m_bIsInSkippableSection = false;
	}

	// warning arrow logic
	if (m_hitobjects.size() > 0)
	{
		const long legacyOffset = (m_iPreviousHitObjectTime < m_hitobjects[0]->getTime() ? 0 : 1000); // Mc
		const long minGapSize = 1000;
		const long lastVisibleMin = 400;
		const long blinkDelta = 100;

		const long gapSize = m_iNextHitObjectTime - (m_iPreviousHitObjectTime + legacyOffset);
		const long nextDelta = (m_iNextHitObjectTime - m_iCurMusicPosWithOffsets);
		const bool drawWarningArrows = gapSize > minGapSize && nextDelta > 0;
		if (drawWarningArrows && ((nextDelta <= lastVisibleMin+blinkDelta*13 && nextDelta > lastVisibleMin+blinkDelta*12)
								|| (nextDelta <= lastVisibleMin+blinkDelta*11 && nextDelta > lastVisibleMin+blinkDelta*10)
								|| (nextDelta <= lastVisibleMin+blinkDelta*9 && nextDelta > lastVisibleMin+blinkDelta*8)
								|| (nextDelta <= lastVisibleMin+blinkDelta*7 && nextDelta > lastVisibleMin+blinkDelta*6)
								|| (nextDelta <= lastVisibleMin+blinkDelta*5 && nextDelta > lastVisibleMin+blinkDelta*4)
								|| (nextDelta <= lastVisibleMin+blinkDelta*3 && nextDelta > lastVisibleMin+blinkDelta*2)
								|| (nextDelta <= lastVisibleMin+blinkDelta*1 && nextDelta > lastVisibleMin)))
			m_bShouldFlashWarningArrows = true;
		else
			m_bShouldFlashWarningArrows = false;
	}

	// break time detection, and background fade during breaks
	const OsuDatabaseBeatmap::BREAK breakEvent = getBreakForTimeRange(m_iPreviousHitObjectTime, m_iCurMusicPosWithOffsets, m_iNextHitObjectTime);
	const bool isInBreak = ((int)m_iCurMusicPosWithOffsets >= breakEvent.startTime && (int)m_iCurMusicPosWithOffsets <= breakEvent.endTime);
	if (isInBreak != m_bInBreak)
	{
		m_bInBreak = !m_bInBreak;

		if (!cv::osu::background_dont_fade_during_breaks.getBool() || m_fBreakBackgroundFade != 0.0f)
		{
			if (m_bInBreak && !cv::osu::background_dont_fade_during_breaks.getBool())
			{
				const int breakDuration = breakEvent.endTime - breakEvent.startTime;
				if (breakDuration > (int)(cv::osu::background_fade_min_duration.getFloat()*1000.0f))
					anim->moveLinear(&m_fBreakBackgroundFade, 1.0f, cv::osu::background_fade_in_duration.getFloat(), true);
			}
			else
				anim->moveLinear(&m_fBreakBackgroundFade, 0.0f, cv::osu::background_fade_out_duration.getFloat(), true);
		}
	}

	// section pass/fail logic
	if (m_hitobjects.size() > 0)
	{
		const long minGapSize = 2880;
		const long fadeStart = 1280;
		const long fadeEnd = 1480;

		const long gapSize = m_iNextHitObjectTime - m_iPreviousHitObjectTime;
		const long start = (gapSize / 2 > minGapSize ? m_iPreviousHitObjectTime + (gapSize / 2) : m_iNextHitObjectTime - minGapSize);
		const long nextDelta = m_iCurMusicPosWithOffsets - start;
		const bool inSectionPassFail = (gapSize > minGapSize && nextDelta > 0)
				&& m_iCurMusicPosWithOffsets > m_hitobjects[0]->getTime()
				&& m_iCurMusicPosWithOffsets < (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getTime() + m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getDuration())
				&& !m_bFailed
				&& m_bInBreak && (breakEvent.endTime - breakEvent.startTime) > minGapSize;

		const bool passing = (m_fHealth >= 0.5);

		// draw logic
		if (passing)
		{
			if (inSectionPassFail && ((nextDelta <= fadeEnd && nextDelta >= 280)
										|| (nextDelta <= 230 && nextDelta >= 160)
										|| (nextDelta <= 100 && nextDelta >= 20)))
			{
				const float fadeAlpha = 1.0f - (float)(nextDelta - fadeStart) / (float)(fadeEnd - fadeStart);
				m_fShouldFlashSectionPass = (nextDelta > fadeStart ? fadeAlpha : 1.0f);
			}
			else
				m_fShouldFlashSectionPass = 0.0f;
		}
		else
		{
			if (inSectionPassFail && ((nextDelta <= fadeEnd && nextDelta >= 280)
										|| (nextDelta <= 230 && nextDelta >= 130)))
			{
				const float fadeAlpha = 1.0f - (float)(nextDelta - fadeStart) / (float)(fadeEnd - fadeStart);
				m_fShouldFlashSectionFail = (nextDelta > fadeStart ? fadeAlpha : 1.0f);
			}
			else
				m_fShouldFlashSectionFail = 0.0f;
		}

		// sound logic
		if (inSectionPassFail)
		{
			if (m_iPreviousSectionPassFailTime != start
				&& ((passing && nextDelta >= 20)
					|| (!passing && nextDelta >= 130)))
			{
				m_iPreviousSectionPassFailTime = start;

				if (!wasSeekFrame)
				{
					if (passing)
						soundEngine->play(osu->getSkin()->getSectionPassSound());
					else
						soundEngine->play(osu->getSkin()->getSectionFailSound());
				}
			}
		}
	}

	// hp drain & failing
	if (cv::osu::drain_type.getInt() > 0)
	{
		const int drainType = cv::osu::drain_type.getInt();

		// handle constant drain
		if (drainType == 1 || drainType == 2) // osu!stable + osu!lazer 2020
		{
			if (m_fDrainRate > 0.0)
			{
				if (m_bIsPlaying 					// not paused
					&& !m_bInBreak					// not in a break
					&& !m_bIsInSkippableSection)	// not in a skippable section
				{
					// special case: break drain edge cases
					bool drainAfterLastHitobjectBeforeBreakStart = false;
					bool drainBeforeFirstHitobjectAfterBreakEnd = false;

					if (drainType == 1) // osu!stable
					{
						drainAfterLastHitobjectBeforeBreakStart = (m_selectedDifficulty2->getVersion() < 8 ? cv::osu::drain_stable_break_before_old.getBool() : cv::osu::drain_stable_break_before.getBool());
						drainBeforeFirstHitobjectAfterBreakEnd = cv::osu::drain_stable_break_after.getBool();
					}
					else if (drainType == 2) // osu!lazer 2020
					{
						drainAfterLastHitobjectBeforeBreakStart = cv::osu::drain_lazer_break_before.getBool();
						drainBeforeFirstHitobjectAfterBreakEnd = cv::osu::drain_lazer_break_after.getBool();
					}

					const bool isBetweenHitobjectsAndBreak = (int)m_iPreviousHitObjectTime <= breakEvent.startTime && (int)m_iNextHitObjectTime >= breakEvent.endTime && m_iCurMusicPosWithOffsets > m_iPreviousHitObjectTime;
					const bool isLastHitobjectBeforeBreakStart = isBetweenHitobjectsAndBreak && (int)m_iCurMusicPosWithOffsets <= breakEvent.startTime;
					const bool isFirstHitobjectAfterBreakEnd = isBetweenHitobjectsAndBreak && (int)m_iCurMusicPosWithOffsets >= breakEvent.endTime;

					if (!isBetweenHitobjectsAndBreak
						|| (drainAfterLastHitobjectBeforeBreakStart && isLastHitobjectBeforeBreakStart)
						|| (drainBeforeFirstHitobjectAfterBreakEnd && isFirstHitobjectAfterBreakEnd))
					{
						// special case: spinner nerf
						double spinnerDrainNerf = 1.0;

						if (drainType == 1) // osu!stable
						{
							if (this->getType() == OsuBeatmap::Type::STANDARD && this->asStd()->isSpinnerActive())
								spinnerDrainNerf = (double)cv::osu::drain_stable_spinner_nerf.getFloat();
						}

						addHealth(-m_fDrainRate * engine->getFrameTime() * (double)getSpeedMultiplier() * spinnerDrainNerf, false);
					}
				}
			}
		}

		// handle generic fail state (1) (see addHealth())
		{
			bool hasFailed = false;

			switch (drainType)
			{
			case 1: // osu!stable
				hasFailed = (m_fHealth < 0.001) && cv::osu::drain_stable_passive_fail.getBool();
				break;

			case 2: // osu!lazer 2020
				hasFailed = (m_fHealth < 0.001) && cv::osu::drain_lazer_passive_fail.getBool();
				break;

			default:
				hasFailed = (m_fHealth < 0.001);
				break;
			}

			if (hasFailed && !osu->getModNF())
				fail();
		}

		// revive in mp
		if (m_fHealth > 0.999 && osu->getScore()->isDead())
			osu->getScore()->setDead(false);

		// handle fail animation
		if (m_bFailed)
		{
			if (m_fFailAnim <= 0.0f)
			{
				if (m_music->isPlaying() || !osu->getPauseMenu()->isVisible())
				{
					soundEngine->pause(m_music);
					m_bIsPaused = true;

					osu->getPauseMenu()->setVisible(true);
					osu->updateConfineCursor();
				}
			}
			else
				m_music->setFrequency(m_fMusicFrequencyBackup*m_fFailAnim > 100 ? m_fMusicFrequencyBackup*m_fFailAnim : 100);
		}
	}
}

void OsuBeatmap::onKeyDown(KeyboardEvent &e)
{
	if (e == KEY_O && keyboard->isControlDown())
	{
		osu->toggleOptionsMenu();
		e.consume();
	}
}

void OsuBeatmap::onKeyUp(KeyboardEvent &/*e*/)
{
	// nothing
}

void OsuBeatmap::skipEmptySection()
{
	if (!m_bIsInSkippableSection) return;
	m_bIsInSkippableSection = false;

	const float offset = 2500.0f;
	float offsetMultiplier = osu->getSpeedMultiplier();
	{
		// only compensate if not within "normal" osu mod range (would make the game feel too different regarding time from skip until first hitobject)
		if (offsetMultiplier >= 0.74f && offsetMultiplier <= 1.51f)
			offsetMultiplier = 1.0f;

		// don't compensate speed increases at all actually
		if (offsetMultiplier > 1.0f)
			offsetMultiplier = 1.0f;

		// and cap slowdowns at sane value (~ spinner fadein start)
		if (offsetMultiplier <= 0.2f)
			offsetMultiplier = 0.2f;
	}

	const long nextHitObjectDelta = m_iNextHitObjectTime - (long)m_iCurMusicPosWithOffsets;

	if (!cv::osu::end_skip.getBool() && nextHitObjectDelta < 0)
		m_music->setPositionMS(std::max(m_music->getLengthMS(), (unsigned long)1) - 1);
	else
		m_music->setPositionMS(std::max(m_iNextHitObjectTime - (long)(offset * offsetMultiplier), (long)0));

	soundEngine->play(osu->getSkin()->getMenuHit());
}

void OsuBeatmap::keyPressed1(bool mouseButton)
{
	if (m_bContinueScheduled)
	{
		if (engine->getTime() < m_fPrevUnpauseTime + cv::osu::unpause_continue_delay.getFloat())
			return;

		m_bClickedContinue = !osu->getModSelector()->isMouseInside();
	}

	if (cv::osu::mod_fullalternate.getBool() && m_bPrevKeyWasKey1)
	{
		if (m_iCurrentHitObjectIndex > m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex)
		{
			soundEngine->play(getSkin()->getCombobreak());
			return;
		}
	}

	// key overlay & counter
	osu->getHUD()->animateInputoverlay(mouseButton ? 3 : 1, true);
	if (!m_bInBreak && !m_bIsInSkippableSection && m_bIsPlaying && !m_bFailed)
		osu->getScore()->addKeyCount(mouseButton ? 3 : 1);

	m_bPrevKeyWasKey1 = true;
	m_bClick1Held = true;

	//debugLog("async music pos = {}, curMusicPos = {}, curMusicPosWithOffsets = {}\n", m_music->getPositionMS(), m_iCurMusicPos, m_iCurMusicPosWithOffsets);
	//long curMusicPos = getMusicPositionMSInterpolated(); // this would only be useful if we also played hitsounds async! combined with checking which musicPos is bigger

	CLICK click;
	click.musicPos = m_iCurMusicPosWithOffsets;
	click.maniaColumn = -1;

	if ((!osu->getModAuto() && !osu->getModRelax()) || !cv::osu::auto_and_relax_block_user_input.getBool())
		m_clicks.push_back(click);
}

void OsuBeatmap::keyPressed2(bool mouseButton)
{
	if (m_bContinueScheduled)
	{
		if (engine->getTime() < m_fPrevUnpauseTime + cv::osu::unpause_continue_delay.getFloat())
			return;

		m_bClickedContinue = !osu->getModSelector()->isMouseInside();
	}

	if (cv::osu::mod_fullalternate.getBool() && !m_bPrevKeyWasKey1)
	{
		if (m_iCurrentHitObjectIndex > m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex)
		{
			soundEngine->play(getSkin()->getCombobreak());
			return;
		}
	}

	// key overlay & counter
	osu->getHUD()->animateInputoverlay(mouseButton ? 4 : 2, true);
	if (!m_bInBreak && !m_bIsInSkippableSection && m_bIsPlaying && !m_bFailed)
		osu->getScore()->addKeyCount(mouseButton ? 4 : 2);

	m_bPrevKeyWasKey1 = false;
	m_bClick2Held = true;

	//debugLog("async music pos = {}, curMusicPos = {}, curMusicPosWithOffsets = {}\n", m_music->getPositionMS(), m_iCurMusicPos, m_iCurMusicPosWithOffsets);
	//long curMusicPos = getMusicPositionMSInterpolated(); // this would only be useful if we also played hitsounds async! combined with checking which musicPos is bigger

	CLICK click;
	click.musicPos = m_iCurMusicPosWithOffsets;
	click.maniaColumn = -1;

	if ((!osu->getModAuto() && !osu->getModRelax()) || !cv::osu::auto_and_relax_block_user_input.getBool())
		m_clicks.push_back(click);
}

void OsuBeatmap::keyReleased1(bool  /*mouseButton*/)
{
	// key overlay
	osu->getHUD()->animateInputoverlay(1, false);
	osu->getHUD()->animateInputoverlay(3, false);

	m_bClick1Held = false;
}

void OsuBeatmap::keyReleased2(bool  /*mouseButton*/)
{
	// key overlay
	osu->getHUD()->animateInputoverlay(2, false);
	osu->getHUD()->animateInputoverlay(4, false);

	m_bClick2Held = false;
}

void OsuBeatmap::select()
{
	// if possible, continue playing where we left off
	if (m_music != NULL && (m_music->isPlaying()))
		m_iContinueMusicPos = m_music->getPositionMS();

	selectDifficulty2(m_selectedDifficulty2);

	loadMusic();
	handlePreviewPlay();
}

void OsuBeatmap::selectDifficulty2(OsuDatabaseBeatmap *difficulty2)
{
	if (difficulty2 != NULL)
	{
		m_selectedDifficulty2 = difficulty2;

		// need to recheck/reload the music here since every difficulty might be using a different sound file
		loadMusic();
		handlePreviewPlay();
	}

	if (cv::osu::beatmap_preview_mods_live.getBool())
		onModUpdate();
}

void OsuBeatmap::deselect()
{
	m_iContinueMusicPos = 0;

	unloadObjects();
}

bool OsuBeatmap::play()
{
	if (m_selectedDifficulty2 == NULL) return false;

	// reset everything, including deleting any previously loaded hitobjects from another diff which we might just have played
	unloadObjects();
	resetScoreInt();

	onBeforeLoad();

	// actually load the difficulty (and the hitobjects)
	{
		OsuDatabaseBeatmap::LOAD_GAMEPLAY_RESULT result = OsuDatabaseBeatmap::loadGameplay(m_selectedDifficulty2, this);
		if (result.errorCode != 0)
		{
			switch (result.errorCode)
			{
			case 1:
				{
					UString errorMessage = "Error: Couldn't load beatmap metadata :(";
					debugLog("Osu Error: Couldn't load beatmap metadata {:s}\n", m_selectedDifficulty2->getFilePath().toUtf8());

					osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;

			case 2:
				{
					UString errorMessage = "Error: Couldn't load beatmap file :(";
					debugLog("Osu Error: Couldn't load beatmap file {:s}\n", m_selectedDifficulty2->getFilePath().toUtf8());

					osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;

			case 3:
				{
					UString errorMessage = "Error: No timingpoints in beatmap :(";
					debugLog("Osu Error: No timingpoints in beatmap {:s}\n", m_selectedDifficulty2->getFilePath().toUtf8());

					osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;

			case 4:
				{
					UString errorMessage = "Error: No hitobjects in beatmap :(";
					debugLog("Osu Error: No hitobjects in beatmap {:s}\n", m_selectedDifficulty2->getFilePath().toUtf8());

					osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;

			case 5:
				{
					UString errorMessage = "Error: Too many hitobjects in beatmap :(";
					debugLog("Osu Error: Too many hitobjects in beatmap {:s}\n", m_selectedDifficulty2->getFilePath().toUtf8());

					osu->getNotificationOverlay()->addNotification(errorMessage, 0xffff0000);
				}
				break;
			}

			return false;
		}

		// move temp result data into beatmap
		m_iRandomSeed = result.randomSeed;
		m_hitobjects = std::move(result.hitobjects);
		m_breaks = std::move(result.breaks);
		osu->getSkin()->setBeatmapComboColors(std::move(result.combocolors)); // update combo colors in skin

		// load beatmap skin
		osu->getSkin()->loadBeatmapOverride(m_selectedDifficulty2->getFolder());
	}

	// the drawing order is different from the playing/input order.
	// for drawing, if multiple hitobjects occupy the exact same time (duration) then they get drawn on top of the active hitobject
	m_hitobjectsSortedByEndTime = m_hitobjects;

	// sort hitobjects by endtime
	constexpr auto hitObjectSortComparator = [](OsuHitObject const *a, OsuHitObject const *b) -> bool
	{
		// strict weak ordering!
		if ((a->getTime() + a->getDuration()) == (b->getTime() + b->getDuration()))
			return a->getSortHack() < b->getSortHack();
		else
			return (a->getTime() + a->getDuration()) < (b->getTime() + b->getDuration());
	};
	std::ranges::sort(m_hitobjectsSortedByEndTime, hitObjectSortComparator);

	onLoad();

	// load music
	unloadMusicInt(); // need to reload in case of speed/pitch changes (just to be sure)
	loadMusic(false, m_bForceStreamPlayback);

	m_music->setLoop(false);
	m_bIsPaused = false;
	m_bContinueScheduled = false;

	m_bInBreak = cv::osu::background_fade_after_load.getBool();
	anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
	m_fBreakBackgroundFade = cv::osu::background_fade_after_load.getBool() ? 1.0f : 0.0f;
	m_iPreviousSectionPassFailTime = -1;
	m_fShouldFlashSectionPass = 0.0f;
	m_fShouldFlashSectionFail = 0.0f;

	m_music->setPosition(0.0);
	m_iCurMusicPos = 0;

	// we are waiting for an asynchronous start of the beatmap in the next update()
	m_bIsWaiting = true;
	m_fWaitTime = Timing::getTimeReal();

	// NOTE: loading failures are handled dynamically in update(), so temporarily assume everything has worked in here
	m_bIsPlaying = true;
	return m_bIsPlaying;
}

void OsuBeatmap::restart(bool quick)
{
	soundEngine->stop(getSkin()->getFailsound());

	if (!m_bIsWaiting)
	{
		m_bIsRestartScheduled = true;
		m_bIsRestartScheduledQuick = quick;
	}
	else if (m_bIsPaused)
		pause(false);

	onRestart(quick);
}

void OsuBeatmap::actualRestart()
{
	// reset everything
	resetScoreInt();
	resetHitObjects(-1000);

	// we are waiting for an asynchronous start of the beatmap in the next update()
	m_bIsWaiting = true;
	m_fWaitTime = Timing::getTimeReal();

	// if the first hitobject starts immediately, add artificial wait time before starting the music
	if (m_hitobjects.size() > 0)
	{
		if (m_hitobjects[0]->getTime() < (long)cv::osu::early_note_time.getInt())
		{
			m_bIsWaiting = true;
			m_fWaitTime = Timing::getTimeReal() + cv::osu::early_note_time.getFloat()/1000.0f;
		}
	}

	// pause temporarily if playing
	if (m_music->isPlaying())
		soundEngine->pause(m_music);

	// reset/restore frequency (from potential fail before)
	m_music->setFrequency(0);

	m_music->setLoop(false);
	m_bIsPaused = false;
	m_bContinueScheduled = false;

	m_bInBreak = false;
	anim->deleteExistingAnimation(&m_fBreakBackgroundFade);
	m_fBreakBackgroundFade = 0.0f;
	m_iPreviousSectionPassFailTime = -1;
	m_fShouldFlashSectionPass = 0.0f;
	m_fShouldFlashSectionFail = 0.0f;

	onModUpdate(); // sanity

	// reset position
	m_music->setPosition(0.0);
	m_iCurMusicPos = 0;

	m_bIsPlaying = true;
}

void OsuBeatmap::pause(bool quitIfWaiting)
{
	if (m_selectedDifficulty2 == NULL) return;

	const bool isFirstPause = !m_bContinueScheduled;
	const bool forceContinueWithoutSchedule = osu->isInMultiplayer();

	// NOTE: this assumes that no beatmap ever goes far beyond the end of the music
	// NOTE: if pure virtual audio time is ever supported (playing without SoundEngine) then this needs to be adapted
	// fix pausing after music ends breaking beatmap state (by just not allowing it to be paused)
	if (m_fAfterMusicIsFinishedVirtualAudioTimeStart >= 0.0)
	{
		const auto delta = Timing::getTimeReal() - m_fAfterMusicIsFinishedVirtualAudioTimeStart;
		if (delta < 5.0) // WARNING: sanity limit, always allow escaping after 5 seconds of overflow time
			return;
	}

	if (m_bIsPlaying) // if we are playing, aka if this is the first time pausing
	{
		if (m_bIsWaiting && quitIfWaiting) // if we are still m_bIsWaiting, pausing the game via the escape key is the same as stopping playing
			stop();
		else
		{
			// first time pause pauses the music
			// case 1: the beatmap is already "finished", jump to the ranking screen if some small amount of time past the last objects endTime
			// case 2: in the middle somewhere, pause as usual
			OsuHitObject *lastHitObject = m_hitobjectsSortedByEndTime.size() > 0 ? m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size()-1] : NULL;
			if (lastHitObject != NULL && lastHitObject->isFinished() && (m_iCurMusicPos > lastHitObject->getTime() + lastHitObject->getDuration() + (long)cv::osu::end_skip_time.getInt()) && cv::osu::end_skip.getBool())
				stop(false);
			else
			{
				soundEngine->pause(m_music);
				m_bIsPlaying = false;
				m_bIsPaused = true;
			}
		}
	}
	else if (m_bIsPaused && !m_bContinueScheduled) // if this is the first time unpausing
	{
		if (osu->getModAuto() || osu->getModAutopilot() || m_bIsInSkippableSection || forceContinueWithoutSchedule) // under certain conditions, immediately continue the beatmap without waiting for the user to click
		{
			if (!m_bIsWaiting) // only force play() if we were not early waiting
				soundEngine->play(m_music);

			m_bIsPlaying = true;
			m_bIsPaused = false;
		}
		else // otherwise, schedule a continue (wait for user to click, handled in update())
		{
			// first time unpause schedules a continue
			m_bIsPaused = false;
			m_bContinueScheduled = true;
		}
	}
	else // if this is not the first time pausing/unpausing, then just toggle the pause state (the visibility of the pause menu is handled in the Osu class, a bit shit)
		m_bIsPaused = !m_bIsPaused;

	if (m_bIsPaused)
		onPaused(isFirstPause);
	else
	{
		m_fPrevUnpauseTime = engine->getTime();
		onUnpaused();
	}

	// if we have failed, and the user early exits to the pause menu, stop the failing animation
	if (m_bFailed)
		anim->deleteExistingAnimation(&m_fFailAnim);

	// make sure to grab/ungrab keyboard
	osu->updateWindowsKeyDisable();
}

void OsuBeatmap::pausePreviewMusic(bool toggle)
{
	if (m_music != NULL)
	{
		if (m_music->isPlaying())
			soundEngine->pause(m_music);
		else if (toggle)
			soundEngine->play(m_music);
	}
}

bool OsuBeatmap::isPreviewMusicPlaying()
{
	if (m_music != NULL)
		return m_music->isPlaying();

	return false;
}

void OsuBeatmap::stop(bool quit)
{
	if (m_selectedDifficulty2 == NULL) return;

	if (getSkin()->getFailsound()->isPlaying())
		soundEngine->stop(getSkin()->getFailsound());

	m_currentHitObject = NULL;

	m_bIsPlaying = false;
	m_bIsPaused = false;
	m_bContinueScheduled = false;

	onBeforeStop(quit);

	unloadObjects();

	onStop(quit);

	osu->onPlayEnd(quit);
}

void OsuBeatmap::fail()
{
	if (m_bFailed) return;

	if (!osu->isInMultiplayer() && cv::osu::drain_kill.getBool())
	{
		soundEngine->play(getSkin()->getFailsound());

		m_bFailed = true;
		m_fFailAnim = 1.0f;
		anim->moveLinear(&m_fFailAnim, 0.0f, cv::osu::fail_time.getFloat(), true); // trigger music slowdown and delayed menu, see update()
	}
	else if (!osu->getScore()->isDead())
	{
		anim->deleteExistingAnimation(&m_fHealth2);
		m_fHealth = 0.0;
		m_fHealth2 = 0.0f;

		if (cv::osu::drain_kill_notification_duration.getFloat() > 0.0f)
		{
			if (!osu->getScore()->hasDied())
				osu->getNotificationOverlay()->addNotification("You have failed, but you can keep playing!", 0xffffffff, false, cv::osu::drain_kill_notification_duration.getFloat());
		}
	}

	if (!osu->getScore()->isDead())
		osu->getScore()->setDead(true);
}

void OsuBeatmap::cancelFailing()
{
	if (!m_bFailed || m_fFailAnim <= 0.0f) return;

	m_bFailed = false;

	anim->deleteExistingAnimation(&m_fFailAnim);
	m_fFailAnim = 1.0f;

	if (m_music != NULL)
		m_music->setFrequency(0.0f);

	if (getSkin()->getFailsound()->isPlaying())
		soundEngine->stop(getSkin()->getFailsound());
}

void OsuBeatmap::setVolume(float volume)
{
	if (m_music != NULL)
		m_music->setVolume(volume);
}

void OsuBeatmap::setSpeed(float speed)
{
	if (m_music != NULL)
		m_music->setSpeed(speed);
}

void OsuBeatmap::setPitch(float pitch)
{
	if (m_music != NULL)
		m_music->setPitch(pitch);
}

void OsuBeatmap::seekPercent(double percent)
{
	if (m_selectedDifficulty2 == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed) return;

	osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::SEEK, (unsigned long)(m_music->getLengthMS() * percent));

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	m_music->setPosition(percent);
	m_music->setVolume(cv::osu::volume_music.getFloat());

	resetHitObjects(m_music->getPositionMS());
	resetScoreInt();

	m_iPreviousSectionPassFailTime = -1;

	if (m_bIsWaiting)
	{
		m_bIsWaiting = false;
		m_bIsPlaying = true;
		m_bIsRestartScheduledQuick = false;

		soundEngine->play(m_music);

		onPlayStart();
	}
}

void OsuBeatmap::seekPercentPlayable(double percent)
{
	if (m_selectedDifficulty2 == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed) return;

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	double actualPlayPercent = percent;
	if (m_hitobjects.size() > 0)
		actualPlayPercent = (((double)m_hitobjects[m_hitobjects.size()-1]->getTime() + (double)m_hitobjects[m_hitobjects.size()-1]->getDuration()) * percent) / (double)m_music->getLengthMS();

	seekPercent(actualPlayPercent);
}

void OsuBeatmap::seekMS(unsigned long ms)
{
	if (m_selectedDifficulty2 == NULL || (!m_bIsPlaying && !m_bIsPaused) || m_music == NULL || m_bFailed) return;

	osu->getMultiplayer()->onServerPlayStateChange(OsuMultiplayer::SEEK, ms);

	m_bWasSeekFrame = true;
	m_fWaitTime = 0.0f;

	m_music->setPositionMS(ms);
	m_music->setVolume(cv::osu::volume_music.getFloat());

	resetHitObjects(m_music->getPositionMS());
	resetScoreInt();

	m_iPreviousSectionPassFailTime = -1;

	if (m_bIsWaiting)
	{
		m_bIsWaiting = false;
		soundEngine->play(m_music);
	}
}

unsigned long OsuBeatmap::getTime() const
{
	if (m_music != NULL && m_music->isAsyncReady())
		return m_music->getPositionMS();
	else
		return 0;
}

unsigned long OsuBeatmap::getStartTimePlayable() const
{
	if (m_hitobjects.size() > 0)
		return (unsigned long)m_hitobjects[0]->getTime();
	else
		return 0;
}

unsigned long OsuBeatmap::getLength() const
{
	if (m_music != NULL && m_music->isAsyncReady())
		return m_music->getLengthMS();
	else if (m_selectedDifficulty2 != NULL)
		return m_selectedDifficulty2->getLengthMS();
	else
		return 0;
}

unsigned long OsuBeatmap::getLengthPlayable() const
{
	if (m_hitobjects.size() > 0)
		return (unsigned long)((m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration()) - m_hitobjects[0]->getTime());
	else
		return getLength();
}

float OsuBeatmap::getPercentFinished() const
{
	if (m_music != NULL)
		return (float)m_iCurMusicPos / (float)m_music->getLengthMS();
	else
		return 0.0f;
}

float OsuBeatmap::getPercentFinishedPlayable() const
{
	if (m_bIsWaiting)
		return 1.0f - (m_fWaitTime - Timing::getTimeReal())/(cv::osu::early_note_time.getFloat()/1000.0f);

	if (m_hitobjects.size() > 0)
		return (float)m_iCurMusicPos / ((float)m_hitobjects[m_hitobjects.size()-1]->getTime() + (float)m_hitobjects[m_hitobjects.size()-1]->getDuration());
	else
		return (float)m_iCurMusicPos / (float)m_music->getLengthMS();
}

int OsuBeatmap::getMostCommonBPM() const
{
	if (m_selectedDifficulty2 != NULL)
	{
		if (m_music != NULL)
			return (int)(m_selectedDifficulty2->getMostCommonBPM() * m_music->getSpeed());
		else
			return (int)(m_selectedDifficulty2->getMostCommonBPM() * osu->getSpeedMultiplier());
	}
	else
		return 0;
}

float OsuBeatmap::getSpeedMultiplier() const
{
	if (m_music != NULL)
	{
		const float speedRet = std::max(m_music->getSpeed(), 0.05f);
		//debugLog("music speed: {:.2f}\n", speedRet);
		return speedRet;
	}
	else
		return 1.0f;
}

OsuSkin *OsuBeatmap::getSkin() const
{
	return osu->getSkin();
}

float OsuBeatmap::getRawAR() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	return std::clamp<float>(m_selectedDifficulty2->getAR() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

float OsuBeatmap::getAR() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	float AR = getRawAR();
	{
		if (cv::osu::ar_override.getFloat() >= 0.0f)
			AR = cv::osu::ar_override.getFloat();

		if (cv::osu::ar_overridenegative.getFloat() < 0.0f)
			AR = cv::osu::ar_overridenegative.getFloat();

		if (cv::osu::ar_override_lock.getBool())
			AR = OsuGameRules::getRawConstantApproachRateForSpeedMultiplier(OsuGameRules::getRawApproachTime(AR), (m_music != NULL && m_bIsPlaying ? getSpeedMultiplier() : osu->getSpeedMultiplier()));

		if (cv::osu::mod_artimewarp.getBool() && m_hitobjects.size() > 0)
		{
			const float percent = 1.0f - ((double)(m_iCurMusicPos - m_hitobjects[0]->getTime()) / (double)(m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() - m_hitobjects[0]->getTime()))*(1.0f - cv::osu::mod_artimewarp_multiplier.getFloat());
			AR *= percent;
		}

		if (cv::osu::mod_arwobble.getBool())
			AR += std::sin((m_iCurMusicPos/1000.0f)*cv::osu::mod_arwobble_interval.getFloat())*cv::osu::mod_arwobble_strength.getFloat();
	}
	return AR;
}

float OsuBeatmap::getCS() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	float CS = std::clamp<float>(m_selectedDifficulty2->getCS() * osu->getCSDifficultyMultiplier(), 0.0f, 10.0f);
	{
		if (cv::osu::cs_override.getFloat() >= 0.0f)
			CS = cv::osu::cs_override.getFloat();

		if (cv::osu::cs_overridenegative.getFloat() < 0.0f)
			CS = cv::osu::cs_overridenegative.getFloat();

		if (cv::osu::mod_minimize.getBool() && m_hitobjects.size() > 0)
		{
			if (m_hitobjects.size() > 0)
			{
				const float percent = 1.0f + ((double)(m_iCurMusicPos - m_hitobjects[0]->getTime()) / (double)(m_hitobjects[m_hitobjects.size()-1]->getTime() + m_hitobjects[m_hitobjects.size()-1]->getDuration() - m_hitobjects[0]->getTime()))*cv::osu::mod_minimize_multiplier.getFloat();
				CS *= percent;
			}
		}

		if (cv::osu::cs_cap_sanity.getBool())
			CS = std::min(CS, 12.1429f);
	}
	return CS;
}

float OsuBeatmap::getHP() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	float HP = std::clamp<float>(m_selectedDifficulty2->getHP() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
	if (cv::osu::hp_override.getFloat() >= 0.0f)
		HP = cv::osu::hp_override.getFloat();

	return HP;
}

float OsuBeatmap::getRawOD() const
{
	if (m_selectedDifficulty2 == NULL) return 5.0f;

	return std::clamp<float>(m_selectedDifficulty2->getOD() * osu->getDifficultyMultiplier(), 0.0f, 10.0f);
}

float OsuBeatmap::getOD() const
{
	float OD = getRawOD();
	if (cv::osu::od_override.getFloat() >= 0.0f)
		OD = cv::osu::od_override.getFloat();

	if (cv::osu::od_override_lock.getBool())
		OD = OsuGameRules::getRawConstantOverallDifficultyForSpeedMultiplier(OsuGameRules::getRawHitWindow300(OD), (m_music != NULL && m_bIsPlaying ? getSpeedMultiplier() : osu->getSpeedMultiplier()));

	return OD;
}

bool OsuBeatmap::isClickHeld() const
{
	 return m_bClick1Held || m_bClick2Held;
}

UString OsuBeatmap::getTitle() const
{
	if (m_selectedDifficulty2 != NULL)
		return m_selectedDifficulty2->getTitle();
	else
		return "NULL";
}

UString OsuBeatmap::getArtist() const
{
	if (m_selectedDifficulty2 != NULL)
		return m_selectedDifficulty2->getArtist();
	else
		return "NULL";
}

unsigned long OsuBeatmap::getBreakDurationTotal() const
{
	unsigned long breakDurationTotal = 0;
	for (int i=0; i<m_breaks.size(); i++)
	{
		breakDurationTotal += (unsigned long)(m_breaks[i].endTime - m_breaks[i].startTime);
	}

	return breakDurationTotal;
}

OsuDatabaseBeatmap::BREAK OsuBeatmap::getBreakForTimeRange(long startMS, long positionMS, long endMS) const
{
	OsuDatabaseBeatmap::BREAK curBreak;

	curBreak.startTime = -1;
	curBreak.endTime = -1;

	for (int i=0; i<m_breaks.size(); i++)
	{
		if (m_breaks[i].startTime >= (int)startMS && m_breaks[i].endTime <= (int)endMS)
		{
			if ((int)positionMS >= curBreak.startTime)
				curBreak = m_breaks[i];
		}
	}

	return curBreak;
}

OsuScore::HIT OsuBeatmap::addHitResult(OsuHitObject *hitObject, OsuScore::HIT hit, long delta, bool isEndOfCombo, bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo, bool ignoreScore, bool ignoreHealth)
{
	// handle perfect & sudden death
	if (osu->getModSS())
	{
		if (!hitErrorBarOnly
			&& hit != OsuScore::HIT::HIT_300
			&& hit != OsuScore::HIT::HIT_300G
			&& hit != OsuScore::HIT::HIT_SLIDER10
			&& hit != OsuScore::HIT::HIT_SLIDER30
			&& hit != OsuScore::HIT::HIT_SPINNERSPIN
			&& hit != OsuScore::HIT::HIT_SPINNERBONUS)
		{
			restart();
			return OsuScore::HIT::HIT_MISS;
		}
	}
	else if (osu->getModSD())
	{
		if (hit == OsuScore::HIT::HIT_MISS)
		{
			if (cv::osu::mod_suddendeath_restart.getBool())
				restart();
			else
				fail();

			return OsuScore::HIT::HIT_MISS;
		}
	}

	// miss sound
	if (hit == OsuScore::HIT::HIT_MISS)
		playMissSound();

	// score
	osu->getScore()->addHitResult(this, hitObject, hit, delta, ignoreOnHitErrorBar, hitErrorBarOnly, ignoreCombo, ignoreScore);

	// health
	OsuScore::HIT returnedHit = OsuScore::HIT::HIT_MISS;
	if (!ignoreHealth)
	{
		addHealth(osu->getScore()->getHealthIncrease(this, hit), true);

		// geki/katu handling
		if (isEndOfCombo)
		{
			const int comboEndBitmask = osu->getScore()->getComboEndBitmask();

			if (comboEndBitmask == 0)
			{
				returnedHit = OsuScore::HIT::HIT_300G;
				addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
				osu->getScore()->addHitResultComboEnd(returnedHit);
			}
			else if ((comboEndBitmask & 2) == 0)
			{
				switch (hit)
				{
				case OsuScore::HIT::HIT_100:
					returnedHit = OsuScore::HIT::HIT_100K;
					addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
					osu->getScore()->addHitResultComboEnd(returnedHit);
					break;

				case OsuScore::HIT::HIT_300:
					returnedHit = OsuScore::HIT::HIT_300K;
					addHealth(osu->getScore()->getHealthIncrease(this, returnedHit), true);
					osu->getScore()->addHitResultComboEnd(returnedHit);
					break;
				default:
					break;
				}
			}
			else if (hit != OsuScore::HIT::HIT_MISS)
				addHealth(osu->getScore()->getHealthIncrease(this, OsuScore::HIT::HIT_MU), true);

			osu->getScore()->setComboEndBitmask(0);
		}
	}

	return returnedHit;
}

void OsuBeatmap::addSliderBreak()
{
	// handle perfect & sudden death
	if (osu->getModSS())
	{
		restart();
		return;
	}
	else if (osu->getModSD())
	{
		if (cv::osu::mod_suddendeath_restart.getBool())
			restart();
		else
			fail();

		return;
	}

	// miss sound
	playMissSound();

	// score
	osu->getScore()->addSliderBreak();
}

void OsuBeatmap::addScorePoints(int points, bool isSpinner)
{
	osu->getScore()->addPoints(points, isSpinner);
}

void OsuBeatmap::addHealth(double percent, bool isFromHitResult)
{
	if (cv::osu::drain_type.getInt() < 1) return;

	// never drain before first hitobject
	if (m_hitobjects.size() > 0 && m_iCurMusicPosWithOffsets < m_hitobjects[0]->getTime()) return;

	// never drain after last hitobject
	if (m_hitobjectsSortedByEndTime.size() > 0 && m_iCurMusicPosWithOffsets > (m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getTime() + m_hitobjectsSortedByEndTime[m_hitobjectsSortedByEndTime.size() - 1]->getDuration())) return;

	if (m_bFailed)
	{
		anim->deleteExistingAnimation(&m_fHealth2);

		m_fHealth = 0.0;
		m_fHealth2 = 0.0f;

		return;
	}

	if (isFromHitResult && percent > 0.0)
	{
		osu->getHUD()->animateKiBulge();

		if (m_fHealth > 0.9)
			osu->getHUD()->animateKiExplode();
	}

	const int drainType = cv::osu::drain_type.getInt();

	m_fHealth = std::clamp<double>(m_fHealth + percent, 0.0, 1.0);

	// handle generic fail state (2)
	const bool isDead = m_fHealth < 0.001;
	if (isDead && !osu->getModNF())
	{
		if (osu->getModEZ() && osu->getScore()->getNumEZRetries() > 0) // retries with ez
		{
			osu->getScore()->setNumEZRetries(osu->getScore()->getNumEZRetries() - 1);

			// special case: set health to 160/200 (osu!stable behavior, seems fine for all drains)
			m_fHealth = cv::osu::drain_stable_hpbar_recovery.getFloat() / cv::osu::drain_stable_hpbar_maximum.getFloat();
			m_fHealth2 = (float)m_fHealth;

			anim->deleteExistingAnimation(&m_fHealth2);
		}
		else if (isFromHitResult && percent < 0.0) // judgement fail
		{
			switch (drainType)
			{
			case 1: // osu!stable
				if (!cv::osu::drain_stable_passive_fail.getBool())
					fail();
				break;

			case 2: // osu!lazer 2020
				if (!cv::osu::drain_lazer_passive_fail.getBool())
					fail();
				break;

			case 3: // osu!lazer 2018
				fail();
				break;
			}
		}
	}
}

void OsuBeatmap::updateTimingPoints(long curPos)
{
	if (curPos < 0) return; // aspire pls >:(

	///debugLog("updateTimingPoints( {} )\n", curPos);

	const OsuDatabaseBeatmap::TIMING_INFO t = m_selectedDifficulty2->getTimingInfoForTime(curPos + (long)cv::osu::timingpoints_offset.getInt());
	osu->getSkin()->setSampleSet(t.sampleType); // normal/soft/drum is stored in the sample type! the sample set number is for custom sets
	osu->getSkin()->setSampleVolume(std::clamp<float>(t.volume / 100.0f, 0.0f, 1.0f));
}

bool OsuBeatmap::isLoading() const
{
	return (!m_music->isAsyncReady());
}

long OsuBeatmap::getPVS()
{
	// this is an approximation with generous boundaries, it doesn't need to be exact (just good enough to filter 10000 hitobjects down to a few hundred or so)
	// it will be used in both positive and negative directions (previous and future hitobjects) to speed up loops which iterate over all hitobjects
	return OsuGameRules::getApproachTime(this)
			+ OsuGameRules::getFadeInTime()
			+ (long)OsuGameRules::getHitWindowMiss(this)
			+ 1500; // sanity
}

bool OsuBeatmap::canDraw()
{
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled && !m_bIsWaiting)
		return false;
	if (m_selectedDifficulty2 == NULL || m_music == NULL) // sanity check
		return false;

	return true;
}

bool OsuBeatmap::canUpdate()
{
	if (!m_bIsPlaying && !m_bIsPaused && !m_bContinueScheduled)
		return false;

	return true;
}

void OsuBeatmap::handlePreviewPlay()
{
	if (m_music != NULL && (!m_music->isPlaying() || m_music->getPosition() > 0.95f) && m_selectedDifficulty2 != NULL)
	{
		// this is an assumption, but should be good enough for most songs
		// reset playback position when the song has nearly reached the end (when the user switches back to the results screen or the songbrowser after playing)
		if (m_music->getPosition() > 0.95f)
			m_iContinueMusicPos = 0;

		soundEngine->stop(m_music);

		if (soundEngine->play(m_music))
		{
			if (m_music->getFrequency() < m_fMusicFrequencyBackup) // player has died, reset frequency
				m_music->setFrequency(m_fMusicFrequencyBackup);

			if (cv::osu::main_menu_shuffle.getBool() && osu->getMainMenu()->isVisible())
				m_music->setPositionMS(0);
			else if (m_iContinueMusicPos != 0)
				m_music->setPositionMS(m_iContinueMusicPos);
			else
				m_music->setPositionMS(m_selectedDifficulty2->getPreviewTime() < 0 ? (unsigned long)(m_music->getLengthMS() * 0.40f) : m_selectedDifficulty2->getPreviewTime());

			m_music->setVolume(cv::osu::volume_music.getFloat());
		}
	}

	// always loop during preview
	if (m_music != NULL)
		m_music->setLoop(cv::osu::beatmap_preview_music_loop.getBool());
}

void OsuBeatmap::loadMusic(bool stream, bool prescan)
{
	stream = stream || m_bForceStreamPlayback;
	m_iResourceLoadUpdateDelayHack = 0;

	// load the song (again)
	if (m_selectedDifficulty2 != NULL && (m_music == NULL || m_selectedDifficulty2->getFullSoundFilePath() != m_music->getFilePath() || !m_music->isReady()))
	{
		unloadMusicInt();

		// if it's not a stream then we are loading the entire song into memory for playing
		if (!stream)
			resourceManager->requestNextLoadAsync();

		m_music = resourceManager->loadSoundAbs(m_selectedDifficulty2->getFullSoundFilePath(), "OSU_BEATMAP_MUSIC", stream, false, false, m_bForceStreamPlayback && prescan); // m_bForceStreamPlayback = prescan necessary! otherwise big mp3s will go out of sync
		m_music->setVolume(cv::osu::volume_music.getFloat());
		m_fMusicFrequencyBackup = m_music->getFrequency();
	}
}

void OsuBeatmap::unloadMusicInt()
{
	soundEngine->stop(m_music);
	resourceManager->destroyResource(m_music);

	m_music = NULL;
}

void OsuBeatmap::unloadObjects()
{
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		delete m_hitobjects[i];
	}
	m_hitobjects = std::vector<OsuHitObject*>();
	m_hitobjectsSortedByEndTime = std::vector<OsuHitObject*>();
	m_misaimObjects = std::vector<OsuHitObject*>();

	m_breaks = std::vector<OsuDatabaseBeatmap::BREAK>();

	m_clicks = std::vector<CLICK>();
	m_keyUps = std::vector<CLICK>();
}

void OsuBeatmap::resetHitObjects(long curPos)
{
	for (int i=0; i<m_hitobjects.size(); i++)
	{
		m_hitobjects[i]->onReset(curPos);
		m_hitobjects[i]->update(curPos); // fgt
		m_hitobjects[i]->onReset(curPos);
	}
	osu->getHUD()->resetHitErrorBar();
}

void OsuBeatmap::resetScoreInt()
{
	m_fHealth = 1.0;
	m_fHealth2 = 1.0f;
	m_bFailed = false;
	m_fFailAnim = 1.0f;
	anim->deleteExistingAnimation(&m_fFailAnim);

	osu->getScore()->reset();

	m_bIsFirstMissSound = true;
}

void OsuBeatmap::playMissSound()
{
	if ((m_bIsFirstMissSound && osu->getScore()->getCombo() > 0) || osu->getScore()->getCombo() > cv::osu::combobreak_sound_combo.getInt())
	{
		m_bIsFirstMissSound = false;
		soundEngine->play(getSkin()->getCombobreak());
	}
}

unsigned long OsuBeatmap::getMusicPositionMSInterpolated()
{
	if (!cv::osu::interpolate_music_pos.getBool() || isLoading())
		return m_music->getPositionMS();
	// TODO: fix snapping at beginning for maps with instant start

	unsigned long returnPos = 0;
	const auto curPos = static_cast<double>(m_music->getPositionMS());
	const float speed = m_music->getSpeed();

	// not reinventing the wheel, the interpolation magic numbers here are (c) peppy

	const double realTime = Timing::getTimeReal();
	const double interpolationDelta = (realTime - m_fLastRealTimeForInterpolationDelta) * 1000.0 * speed;
	const double interpolationDeltaLimit = ((realTime - m_fLastAudioTimeAccurateSet)*1000.0 < 1500 || speed < 1.0f ? 11 : 33);

	if (m_music->isPlaying() && !m_bWasSeekFrame)
	{
		double newInterpolatedPos = m_fInterpolatedMusicPos + interpolationDelta;
		double delta = newInterpolatedPos - curPos;

		// debugLog("speed = {:.2f} positionMS = {:.2f} delta = {:.2f}, interpolationDeltaLimit = {:.2f}\n", speed, curPos, delta, interpolationDeltaLimit);

		// approach and recalculate delta
		newInterpolatedPos -= delta / 8.0;
		delta = newInterpolatedPos - curPos;

		if (std::abs(delta) > interpolationDeltaLimit*2) // we're fucked, snap back to curPos
		{
			m_fInterpolatedMusicPos = (double)curPos;
		}
		else if (delta < -interpolationDeltaLimit) // undershot
		{
			m_fInterpolatedMusicPos += interpolationDelta * 2;
			m_fLastAudioTimeAccurateSet = realTime;
		}
		else if (delta < interpolationDeltaLimit) // normal
		{
			m_fInterpolatedMusicPos = newInterpolatedPos;
		}
		else // overshot
		{
			m_fInterpolatedMusicPos += interpolationDelta / 2;
			m_fLastAudioTimeAccurateSet = realTime;
		}

		// calculate final return value
		returnPos = (unsigned long)std::round(m_fInterpolatedMusicPos);
		if (speed < 1.0f && cv::osu::compensate_music_speed.getBool() && cv::snd_speed_compensate_pitch.getBool())
			returnPos += (unsigned long)(((1.0f - speed) / 0.75f) * 5); // osu (new)
			///returnPos += (unsigned long)((1.0f / speed) * 9); // Mc (old)
	}
	else // no interpolation
	{
		m_fInterpolatedMusicPos = curPos;
		m_fLastAudioTimeAccurateSet = realTime;
		returnPos = static_cast<unsigned long>(curPos);
	}

	m_fLastRealTimeForInterpolationDelta = realTime; // this is more accurate than engine->getFrameTime() for the delta calculation, since it correctly handles all possible delays inbetween

	// debugLog("returning {} \n", returnPos);
	// debugLog("delta = {}\n", (long)returnPos - m_iCurMusicPos);
	// debugLog("raw delta = {}\n", (long)returnPos - (long)curPos);

	return returnPos;
}
