//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		hud element drawing functions (accuracy, combo, score, etc.)
//
// $NoKeywords: $osuhud
//===============================================================================//

#include "OsuHUD.h"

#include "Engine.h"
#include "Environment.h"
#include "ConVar.h"
#include "Mouse.h"
#include "NetworkHandler.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "VertexArrayObject.h"
#include "Shader.h"

#include "CBaseUIContainer.h"

#include "Osu.h"
#include "OsuMultiplayer.h"
#include "OsuModFPoSu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"

#include "OsuBeatmap.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"

#include "OsuGameRules.h"
#include "OsuGameRulesMania.h"

#include "OsuScore.h"
#include "OsuSongBrowser2.h"
#include "OsuDatabase.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"

#include "OsuUIVolumeSlider.h"

#include "SoundEngine.h"

#include "DirectX11Interface.h"
#include "OpenGLES32Interface.h"

#include <utility>
namespace cv::osu {
ConVar automatic_cursor_size("osu_automatic_cursor_size", false, FCVAR_NONE);

ConVar cursor_alpha("osu_cursor_alpha", 1.0f, FCVAR_NONE);
ConVar cursor_scale("osu_cursor_scale", 1.0f, FCVAR_NONE);
ConVar cursor_expand_scale_multiplier("osu_cursor_expand_scale_multiplier", 1.3f, FCVAR_NONE);
ConVar cursor_expand_duration("osu_cursor_expand_duration", 0.1f, FCVAR_NONE);
ConVar cursor_trail_scale("osu_cursor_trail_scale", 1.0f, FCVAR_NONE);
ConVar cursor_trail_length("osu_cursor_trail_length", 0.17f, FCVAR_NONE, "how long unsmooth cursortrails should be, in seconds");
ConVar cursor_trail_spacing("osu_cursor_trail_spacing", 0.015f, FCVAR_NONE, "how big the gap between consecutive unsmooth cursortrail images should be, in seconds");
ConVar cursor_trail_alpha("osu_cursor_trail_alpha", 1.0f, FCVAR_NONE);
ConVar cursor_trail_smooth_force("osu_cursor_trail_smooth_force", false, FCVAR_NONE);
ConVar cursor_trail_smooth_length("osu_cursor_trail_smooth_length", 0.5f, FCVAR_NONE, "how long smooth cursortrails should be, in seconds");
ConVar cursor_trail_smooth_div("osu_cursor_trail_smooth_div", 4.0f, FCVAR_NONE, "divide the cursortrail.png image size by this much, for determining the distance to the next trail image");
ConVar cursor_trail_max_size("osu_cursor_trail_max_size", 2048, FCVAR_NONE, "maximum number of rendered trail images, array size limit");
ConVar cursor_trail_expand("osu_cursor_trail_expand", true, FCVAR_NONE, "if \"CursorExpand: 1\" in your skin.ini, whether the trail should then also expand or not");
ConVar cursor_ripple_duration("osu_cursor_ripple_duration", 0.7f, FCVAR_NONE, "time in seconds each cursor ripple is visible");
ConVar cursor_ripple_alpha("osu_cursor_ripple_alpha", 1.0f, FCVAR_NONE);
ConVar cursor_ripple_additive("osu_cursor_ripple_additive", true, FCVAR_NONE, "use additive blending");
ConVar cursor_ripple_anim_start_scale("osu_cursor_ripple_anim_start_scale", 0.05f, FCVAR_NONE, "start size multiplier");
ConVar cursor_ripple_anim_end_scale("osu_cursor_ripple_anim_end_scale", 0.5f, FCVAR_NONE, "end size multiplier");
ConVar cursor_ripple_anim_start_fadeout_delay("osu_cursor_ripple_anim_start_fadeout_delay", 0.0f, FCVAR_NONE, "delay in seconds after which to start fading out (limited by osu_cursor_ripple_duration of course)");
ConVar cursor_ripple_tint_r("osu_cursor_ripple_tint_r", 255, FCVAR_NONE, "from 0 to 255");
ConVar cursor_ripple_tint_g("osu_cursor_ripple_tint_g", 255, FCVAR_NONE, "from 0 to 255");
ConVar cursor_ripple_tint_b("osu_cursor_ripple_tint_b", 255, FCVAR_NONE, "from 0 to 255");

ConVar hud_shift_tab_toggles_everything("osu_hud_shift_tab_toggles_everything", true, FCVAR_NONE);
ConVar hud_scale("osu_hud_scale", 1.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_alpha("osu_hud_hiterrorbar_alpha", 1.0f, FCVAR_NONE, "opacity multiplier for entire hiterrorbar");
ConVar hud_hiterrorbar_bar_alpha("osu_hud_hiterrorbar_bar_alpha", 1.0f, FCVAR_NONE, "opacity multiplier for background color bar");
ConVar hud_hiterrorbar_centerline_alpha("osu_hud_hiterrorbar_centerline_alpha", 1.0f, FCVAR_NONE, "opacity multiplier for center line");
ConVar hud_hiterrorbar_entry_additive("osu_hud_hiterrorbar_entry_additive", true, FCVAR_NONE, "whether to use additive blending for all hit error entries/lines");
ConVar hud_hiterrorbar_entry_alpha("osu_hud_hiterrorbar_entry_alpha", 0.75f, FCVAR_NONE, "opacity multiplier for all hit error entries/lines");
ConVar hud_hiterrorbar_entry_300_r("osu_hud_hiterrorbar_entry_300_r", 50, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_300_g("osu_hud_hiterrorbar_entry_300_g", 188, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_300_b("osu_hud_hiterrorbar_entry_300_b", 231, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_100_r("osu_hud_hiterrorbar_entry_100_r", 87, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_100_g("osu_hud_hiterrorbar_entry_100_g", 227, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_100_b("osu_hud_hiterrorbar_entry_100_b", 19, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_50_r("osu_hud_hiterrorbar_entry_50_r", 218, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_50_g("osu_hud_hiterrorbar_entry_50_g", 174, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_50_b("osu_hud_hiterrorbar_entry_50_b", 70, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_miss_r("osu_hud_hiterrorbar_entry_miss_r", 205, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_miss_g("osu_hud_hiterrorbar_entry_miss_g", 0, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_miss_b("osu_hud_hiterrorbar_entry_miss_b", 0, FCVAR_NONE);
ConVar hud_hiterrorbar_centerline_r("osu_hud_hiterrorbar_centerline_r", 255, FCVAR_NONE);
ConVar hud_hiterrorbar_centerline_g("osu_hud_hiterrorbar_centerline_g", 255, FCVAR_NONE);
ConVar hud_hiterrorbar_centerline_b("osu_hud_hiterrorbar_centerline_b", 255, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_hit_fade_time("osu_hud_hiterrorbar_entry_hit_fade_time", 6.0f, FCVAR_NONE, "fade duration of 50/100/300 hit entries/lines in seconds");
ConVar hud_hiterrorbar_entry_miss_fade_time("osu_hud_hiterrorbar_entry_miss_fade_time", 4.0f, FCVAR_NONE, "fade duration of miss entries/lines in seconds");
ConVar hud_hiterrorbar_entry_miss_height_multiplier("osu_hud_hiterrorbar_entry_miss_height_multiplier", 1.5f, FCVAR_NONE);
ConVar hud_hiterrorbar_entry_misaim_height_multiplier("osu_hud_hiterrorbar_entry_misaim_height_multiplier", 4.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_scale("osu_hud_hiterrorbar_scale", 1.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_showmisswindow("osu_hud_hiterrorbar_showmisswindow", false, FCVAR_NONE);
ConVar hud_hiterrorbar_width_percent_with_misswindow("osu_hud_hiterrorbar_width_percent_with_misswindow", 0.4f, FCVAR_NONE);
ConVar hud_hiterrorbar_width_percent("osu_hud_hiterrorbar_width_percent", 0.15f, FCVAR_NONE);
ConVar hud_hiterrorbar_height_percent("osu_hud_hiterrorbar_height_percent", 0.007f, FCVAR_NONE);
ConVar hud_hiterrorbar_offset_percent("osu_hud_hiterrorbar_offset_percent", 0.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_offset_bottom_percent("osu_hud_hiterrorbar_offset_bottom_percent", 0.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_offset_top_percent("osu_hud_hiterrorbar_offset_top_percent", 0.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_offset_left_percent("osu_hud_hiterrorbar_offset_left_percent", 0.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_offset_right_percent("osu_hud_hiterrorbar_offset_right_percent", 0.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_bar_width_scale("osu_hud_hiterrorbar_bar_width_scale", 0.6f, FCVAR_NONE);
ConVar hud_hiterrorbar_bar_height_scale("osu_hud_hiterrorbar_bar_height_scale", 3.4f, FCVAR_NONE);
ConVar hud_hiterrorbar_max_entries("osu_hud_hiterrorbar_max_entries", 32, FCVAR_NONE, "maximum number of entries/lines");
ConVar hud_hiterrorbar_hide_during_spinner("osu_hud_hiterrorbar_hide_during_spinner", true, FCVAR_NONE);
ConVar hud_hiterrorbar_ur_scale("osu_hud_hiterrorbar_ur_scale", 1.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_ur_alpha("osu_hud_hiterrorbar_ur_alpha", 0.5f, FCVAR_NONE, "opacity multiplier for unstable rate text above hiterrorbar");
ConVar hud_hiterrorbar_ur_offset_x_percent("osu_hud_hiterrorbar_ur_offset_x_percent", 0.0f, FCVAR_NONE);
ConVar hud_hiterrorbar_ur_offset_y_percent("osu_hud_hiterrorbar_ur_offset_y_percent", 0.0f, FCVAR_NONE);
ConVar hud_scorebar_scale("osu_hud_scorebar_scale", 1.0f, FCVAR_NONE);
ConVar hud_scorebar_hide_during_breaks("osu_hud_scorebar_hide_during_breaks", true, FCVAR_NONE);
ConVar hud_scorebar_hide_anim_duration("osu_hud_scorebar_hide_anim_duration", 0.5f, FCVAR_NONE);
ConVar hud_combo_scale("osu_hud_combo_scale", 1.0f, FCVAR_NONE);
ConVar hud_score_scale("osu_hud_score_scale", 1.0f, FCVAR_NONE);
ConVar hud_accuracy_scale("osu_hud_accuracy_scale", 1.0f, FCVAR_NONE);
ConVar hud_progressbar_scale("osu_hud_progressbar_scale", 1.0f, FCVAR_NONE);
ConVar hud_playfield_border_size("osu_hud_playfield_border_size", 5.0f, FCVAR_NONE);
ConVar hud_statistics_scale("osu_hud_statistics_scale", 1.0f, FCVAR_NONE);
ConVar hud_statistics_spacing_scale("osu_hud_statistics_spacing_scale", 1.1f, FCVAR_NONE);
ConVar hud_statistics_offset_x("osu_hud_statistics_offset_x", 5.0f, FCVAR_NONE);
ConVar hud_statistics_offset_y("osu_hud_statistics_offset_y", 50.0f, FCVAR_NONE);
ConVar hud_statistics_pp_decimal_places("osu_hud_statistics_pp_decimal_places", 0, FCVAR_NONE, "number of decimal places for the live pp counter (min = 0, max = 2)");
ConVar hud_statistics_pp_offset_x("osu_hud_statistics_pp_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_pp_offset_y("osu_hud_statistics_pp_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_perfectpp_offset_x("osu_hud_statistics_perfectpp_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_perfectpp_offset_y("osu_hud_statistics_perfectpp_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_misses_offset_x("osu_hud_statistics_misses_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_misses_offset_y("osu_hud_statistics_misses_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_sliderbreaks_offset_x("osu_hud_statistics_sliderbreaks_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_sliderbreaks_offset_y("osu_hud_statistics_sliderbreaks_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_maxpossiblecombo_offset_x("osu_hud_statistics_maxpossiblecombo_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_maxpossiblecombo_offset_y("osu_hud_statistics_maxpossiblecombo_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_livestars_offset_x("osu_hud_statistics_livestars_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_livestars_offset_y("osu_hud_statistics_livestars_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_totalstars_offset_x("osu_hud_statistics_totalstars_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_totalstars_offset_y("osu_hud_statistics_totalstars_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_bpm_offset_x("osu_hud_statistics_bpm_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_bpm_offset_y("osu_hud_statistics_bpm_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_detected_bpm_offset_x("osu_hud_statistics_detected_bpm_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_detected_bpm_offset_y("osu_hud_statistics_detected_bpm_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_ar_offset_x("osu_hud_statistics_ar_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_ar_offset_y("osu_hud_statistics_ar_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_cs_offset_x("osu_hud_statistics_cs_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_cs_offset_y("osu_hud_statistics_cs_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_od_offset_x("osu_hud_statistics_od_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_od_offset_y("osu_hud_statistics_od_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_hp_offset_x("osu_hud_statistics_hp_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_hp_offset_y("osu_hud_statistics_hp_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_hitwindow300_offset_x("osu_hud_statistics_hitwindow300_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_hitwindow300_offset_y("osu_hud_statistics_hitwindow300_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_nps_offset_x("osu_hud_statistics_nps_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_nps_offset_y("osu_hud_statistics_nps_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_nd_offset_x("osu_hud_statistics_nd_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_nd_offset_y("osu_hud_statistics_nd_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_ur_offset_x("osu_hud_statistics_ur_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_ur_offset_y("osu_hud_statistics_ur_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_statistics_hitdelta_offset_x("osu_hud_statistics_hitdelta_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_statistics_hitdelta_offset_y("osu_hud_statistics_hitdelta_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_volume_duration("osu_hud_volume_duration", 1.0f, FCVAR_NONE);
ConVar hud_volume_size_multiplier("osu_hud_volume_size_multiplier", 1.5f, FCVAR_NONE);
ConVar hud_scoreboard_scale("osu_hud_scoreboard_scale", 1.0f, FCVAR_NONE);
ConVar hud_scoreboard_offset_y_percent("osu_hud_scoreboard_offset_y_percent", 0.11f, FCVAR_NONE);
ConVar hud_scoreboard_use_menubuttonbackground("osu_hud_scoreboard_use_menubuttonbackground", true, FCVAR_NONE);
ConVar hud_inputoverlay_scale("osu_hud_inputoverlay_scale", 1.0f, FCVAR_NONE);
ConVar hud_inputoverlay_offset_x("osu_hud_inputoverlay_offset_x", 0.0f, FCVAR_NONE);
ConVar hud_inputoverlay_offset_y("osu_hud_inputoverlay_offset_y", 0.0f, FCVAR_NONE);
ConVar hud_inputoverlay_anim_scale_duration("osu_hud_inputoverlay_anim_scale_duration", 0.16f, FCVAR_NONE);
ConVar hud_inputoverlay_anim_scale_multiplier("osu_hud_inputoverlay_anim_scale_multiplier", 0.8f, FCVAR_NONE);
ConVar hud_inputoverlay_anim_color_duration("osu_hud_inputoverlay_anim_color_duration", 0.1f, FCVAR_NONE);
ConVar hud_fps_smoothing("osu_hud_fps_smoothing", true, FCVAR_NONE);
ConVar hud_scrubbing_timeline_hover_tooltip_offset_multiplier("osu_hud_scrubbing_timeline_hover_tooltip_offset_multiplier", 1.0f, FCVAR_NONE);
ConVar hud_scrubbing_timeline_strains_height("osu_hud_scrubbing_timeline_strains_height", 200.0f, FCVAR_NONE);
ConVar hud_scrubbing_timeline_strains_alpha("osu_hud_scrubbing_timeline_strains_alpha", 0.4f, FCVAR_NONE);
ConVar hud_scrubbing_timeline_strains_aim_color_r("osu_hud_scrubbing_timeline_strains_aim_color_r", 0, FCVAR_NONE);
ConVar hud_scrubbing_timeline_strains_aim_color_g("osu_hud_scrubbing_timeline_strains_aim_color_g", 255, FCVAR_NONE);
ConVar hud_scrubbing_timeline_strains_aim_color_b("osu_hud_scrubbing_timeline_strains_aim_color_b", 0, FCVAR_NONE);
ConVar hud_scrubbing_timeline_strains_speed_color_r("osu_hud_scrubbing_timeline_strains_speed_color_r", 255, FCVAR_NONE);
ConVar hud_scrubbing_timeline_strains_speed_color_g("osu_hud_scrubbing_timeline_strains_speed_color_g", 0, FCVAR_NONE);
ConVar hud_scrubbing_timeline_strains_speed_color_b("osu_hud_scrubbing_timeline_strains_speed_color_b", 0, FCVAR_NONE);

ConVar draw_cursor_trail("osu_draw_cursor_trail", true, FCVAR_NONE);
ConVar draw_cursor_ripples("osu_draw_cursor_ripples", false, FCVAR_NONE);
ConVar draw_hud("osu_draw_hud", true, FCVAR_NONE);
ConVar draw_scorebar("osu_draw_scorebar", true, FCVAR_NONE);
ConVar draw_scorebarbg("osu_draw_scorebarbg", true, FCVAR_NONE);
ConVar draw_hiterrorbar("osu_draw_hiterrorbar", true, FCVAR_NONE);
ConVar draw_hiterrorbar_ur("osu_draw_hiterrorbar_ur", true, FCVAR_NONE);
ConVar draw_hiterrorbar_bottom("osu_draw_hiterrorbar_bottom", true, FCVAR_NONE);
ConVar draw_hiterrorbar_top("osu_draw_hiterrorbar_top", false, FCVAR_NONE);
ConVar draw_hiterrorbar_left("osu_draw_hiterrorbar_left", false, FCVAR_NONE);
ConVar draw_hiterrorbar_right("osu_draw_hiterrorbar_right", false, FCVAR_NONE);
ConVar draw_progressbar("osu_draw_progressbar", true, FCVAR_NONE);
ConVar draw_combo("osu_draw_combo", true, FCVAR_NONE);
ConVar draw_score("osu_draw_score", true, FCVAR_NONE);
ConVar draw_accuracy("osu_draw_accuracy", true, FCVAR_NONE);
ConVar draw_target_heatmap("osu_draw_target_heatmap", true, FCVAR_NONE);
ConVar draw_scrubbing_timeline("osu_draw_scrubbing_timeline", true, FCVAR_NONE);
ConVar draw_scrubbing_timeline_breaks("osu_draw_scrubbing_timeline_breaks", true, FCVAR_NONE);
ConVar draw_scrubbing_timeline_strain_graph("osu_draw_scrubbing_timeline_strain_graph", false, FCVAR_NONE);
ConVar draw_continue("osu_draw_continue", true, FCVAR_NONE);
ConVar draw_scoreboard("osu_draw_scoreboard", true, FCVAR_NONE);
ConVar draw_inputoverlay("osu_draw_inputoverlay", true, FCVAR_NONE);

ConVar draw_statistics_misses("osu_draw_statistics_misses", false, FCVAR_NONE);
ConVar draw_statistics_sliderbreaks("osu_draw_statistics_sliderbreaks", false, FCVAR_NONE);
ConVar draw_statistics_perfectpp("osu_draw_statistics_perfectpp", false, FCVAR_NONE);
ConVar draw_statistics_maxpossiblecombo("osu_draw_statistics_maxpossiblecombo", false, FCVAR_NONE);
ConVar draw_statistics_livestars("osu_draw_statistics_livestars", false, FCVAR_NONE);
ConVar draw_statistics_totalstars("osu_draw_statistics_totalstars", false, FCVAR_NONE);
ConVar draw_statistics_bpm("osu_draw_statistics_bpm", false, FCVAR_NONE);
ConVar draw_statistics_detected_bpm("osu_draw_statistics_detected_bpm", false, FCVAR_NONE, [](float on) -> void {
	soundEngine ? soundEngine->setBPMDetection(!!static_cast<int>(on)) : (void)0;
});
ConVar draw_statistics_ar("osu_draw_statistics_ar", false, FCVAR_NONE);
ConVar draw_statistics_cs("osu_draw_statistics_cs", false, FCVAR_NONE);
ConVar draw_statistics_od("osu_draw_statistics_od", false, FCVAR_NONE);
ConVar draw_statistics_hp("osu_draw_statistics_hp", false, FCVAR_NONE);
ConVar draw_statistics_nps("osu_draw_statistics_nps", false, FCVAR_NONE);
ConVar draw_statistics_nd("osu_draw_statistics_nd", false, FCVAR_NONE);
ConVar draw_statistics_ur("osu_draw_statistics_ur", false, FCVAR_NONE);
ConVar draw_statistics_pp("osu_draw_statistics_pp", false, FCVAR_NONE);
ConVar draw_statistics_hitwindow300("osu_draw_statistics_hitwindow300", false, FCVAR_NONE);
ConVar draw_statistics_hitdelta("osu_draw_statistics_hitdelta", false, FCVAR_NONE);

ConVar combo_anim1_duration("osu_combo_anim1_duration", 0.15f, FCVAR_NONE);
ConVar combo_anim1_size("osu_combo_anim1_size", 0.15f, FCVAR_NONE);
ConVar combo_anim2_duration("osu_combo_anim2_duration", 0.4f, FCVAR_NONE);
ConVar combo_anim2_size("osu_combo_anim2_size", 0.5f, FCVAR_NONE);
}

OsuHUD::OsuHUD() : OsuScreen()
{
	// convar callbacks
	cv::osu::hud_volume_size_multiplier.setCallback( fastdelegate::MakeDelegate(this, &OsuHUD::onVolumeOverlaySizeChange) );

	// resources
	m_tempFont = resourceManager->getFont("FONT_DEFAULT");
	m_cursorTrailShader = resourceManager->loadShader("cursortrail.mcshader", "cursortrail");
	m_cursorTrail.reserve(cv::osu::cursor_trail_max_size.getInt()*2);

	m_cursorTrailVAO = resourceManager->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_QUADS, Graphics::USAGE_TYPE::USAGE_DYNAMIC);

	m_fCurFps = 60.0f;
	m_fCurFpsSmooth = 60.0f;
	m_fFpsUpdate = 0.0f;

	m_fInputoverlayK1AnimScale = 1.0f;
	m_fInputoverlayK2AnimScale = 1.0f;
	m_fInputoverlayM1AnimScale = 1.0f;
	m_fInputoverlayM2AnimScale = 1.0f;

	m_fInputoverlayK1AnimColor = 0.0f;
	m_fInputoverlayK2AnimColor = 0.0f;
	m_fInputoverlayM1AnimColor = 0.0f;
	m_fInputoverlayM2AnimColor = 0.0f;

	m_fAccuracyXOffset = 0.0f;
	m_fAccuracyYOffset = 0.0f;
	m_fScoreHeight = 0.0f;

	m_fComboAnim1 = 0.0f;
	m_fComboAnim2 = 0.0f;

	m_fVolumeChangeTime = 0.0f;
	m_fVolumeChangeFade = 1.0f;
	m_fLastVolume = cv::osu::volume_master.getFloat();
	m_volumeSliderOverlayContainer = new CBaseUIContainer();

	m_volumeMaster = new OsuUIVolumeSlider(0, 0, 450, 75, "");
	m_volumeMaster->setType(OsuUIVolumeSlider::TYPE::MASTER);
	m_volumeMaster->setBlockSize(m_volumeMaster->getSize().y + 7, m_volumeMaster->getSize().y);
	m_volumeMaster->setAllowMouseWheel(false);
	m_volumeMaster->setAnimated(false);
	m_volumeMaster->setSelected(true);
	m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeMaster);
	m_volumeEffects = new OsuUIVolumeSlider(0, 0, m_volumeMaster->getSize().x, m_volumeMaster->getSize().y/1.5f, "");
	m_volumeEffects->setType(OsuUIVolumeSlider::TYPE::EFFECTS);
	m_volumeEffects->setBlockSize((m_volumeMaster->getSize().y + 7)/1.5f, m_volumeMaster->getSize().y/1.5f);
	m_volumeEffects->setAllowMouseWheel(false);
	m_volumeEffects->setKeyDelta(cv::osu::volume_change_interval.getFloat());
	m_volumeEffects->setAnimated(false);
	m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeEffects);
	m_volumeMusic = new OsuUIVolumeSlider(0, 0, m_volumeMaster->getSize().x, m_volumeMaster->getSize().y/1.5f, "");
	m_volumeMusic->setType(OsuUIVolumeSlider::TYPE::MUSIC);
	m_volumeMusic->setBlockSize((m_volumeMaster->getSize().y + 7)/1.5f, m_volumeMaster->getSize().y/1.5f);
	m_volumeMusic->setAllowMouseWheel(false);
	m_volumeMusic->setKeyDelta(cv::osu::volume_change_interval.getFloat());
	m_volumeMusic->setAnimated(false);
	m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeMusic);

	onVolumeOverlaySizeChange(UString::format("%f", cv::osu::hud_volume_size_multiplier.getFloat()), UString::format("%f", cv::osu::hud_volume_size_multiplier.getFloat()));

	m_fCursorExpandAnim = 1.0f;

	m_fHealth = 1.0f;
	m_fScoreBarBreakAnim = 0.0f;
	m_fKiScaleAnim = 0.8f;
}

OsuHUD::~OsuHUD()
{
	SAFE_DELETE(m_volumeSliderOverlayContainer);
}

void OsuHUD::draw()
{
	OsuBeatmap *beatmap = osu->getSelectedBeatmap();
	if (beatmap == NULL) return; // sanity check

	const auto *beatmapStd = beatmap->asStd();
	const auto *beatmapMania = beatmap->asMania();

	if (cv::osu::draw_hud.getBool())
	{
		if (cv::osu::draw_inputoverlay.getBool() && beatmapStd != NULL)
		{
			const bool isAutoClicking = (osu->getModAuto() || osu->getModRelax());
			if (!isAutoClicking)
				drawInputOverlay(osu->getScore()->getKeyCount(1), osu->getScore()->getKeyCount(2), osu->getScore()->getKeyCount(3), osu->getScore()->getKeyCount(4));
		}

		if (cv::osu::draw_scoreboard.getBool())
		{
			if (osu->isInMultiplayer())
				drawScoreBoardMP();
			else if (beatmap->getSelectedDifficulty2() != NULL)
				drawScoreBoard((std::string&)beatmap->getSelectedDifficulty2()->getMD5Hash(), osu->getScore());
		}

		if (!osu->isSkipScheduled() && beatmap->isInSkippableSection() && ((cv::osu::skip_intro_enabled.getBool() && beatmap->getHitObjectIndexForCurrentTime() < 1) || (cv::osu::skip_breaks_enabled.getBool() && beatmap->getHitObjectIndexForCurrentTime() > 0)))
			drawSkip();

		g->pushTransform();
		{
			if (osu->getModTarget() && cv::osu::draw_target_heatmap.getBool() && beatmapStd != NULL)
				g->translate(0, beatmapStd->getHitcircleDiameter()*(1.0f / (cv::osu::hud_scale.getFloat()*cv::osu::hud_statistics_scale.getFloat())));

			const int hitObjectIndexForCurrentTime = (beatmap->getHitObjectIndexForCurrentTime() < 1 ? -1 : beatmap->getHitObjectIndexForCurrentTime());

			drawStatistics(
					osu->getScore()->getNumMisses(),
					osu->getScore()->getNumSliderBreaks(),
					beatmap->getMaxPossibleCombo(),
					OsuDifficultyCalculator::calculateTotalStarsFromSkills(beatmap->getAimStarsForUpToHitObjectIndex(hitObjectIndexForCurrentTime), beatmap->getSpeedStarsForUpToHitObjectIndex(hitObjectIndexForCurrentTime)),
					osu->getSongBrowser()->getDynamicStarCalculator()->getTotalStars(),
					beatmap->getMostCommonBPM(),
					(soundEngine->shouldDetectBPM() && beatmap->getMusic()) ? beatmap->getMusic()->getBPM() : -1.0f,
					OsuGameRules::getApproachRateForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()),
					beatmap->getCS(),
					OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()),
					beatmap->getHP(),
					beatmap->getNPS(),
					beatmap->getND(),
					osu->getScore()->getUnstableRate(),
					osu->getScore()->getPPv2(),
					osu->getSongBrowser()->getDynamicStarCalculator()->getPPv2(),
					((int)OsuGameRules::getHitWindow300(beatmap) - 0.5f) * (1.0f / osu->getSpeedMultiplier()), // see OsuUISongBrowserInfoLabel::update()
					osu->getScore()->getHitErrorAvgCustomMin(),
					osu->getScore()->getHitErrorAvgCustomMax());
		}
		g->popTransform();

		// NOTE: special case for FPoSu, if players manually set fposu_draw_scorebarbg_on_top to 1
		if (cv::osu::draw_scorebarbg.getBool() && cv::osu::fposu::mod_fposu.getBool() && cv::osu::fposu::draw_scorebarbg_on_top.getBool())
			drawScorebarBg(cv::osu::hud_scorebar_hide_during_breaks.getBool() ? (1.0f - beatmap->getBreakBackgroundFadeAnim()) : 1.0f, m_fScoreBarBreakAnim);

		if (cv::osu::draw_scorebar.getBool())
			drawHPBar(m_fHealth, cv::osu::hud_scorebar_hide_during_breaks.getBool() ? (1.0f - beatmap->getBreakBackgroundFadeAnim()) : 1.0f, m_fScoreBarBreakAnim);

		// NOTE: moved to draw behind hitobjects in OsuBeatmapStandard::draw()
		if (cv::osu::fposu::mod_fposu.getBool())
		{
			if (cv::osu::draw_hiterrorbar.getBool() && (beatmapStd == NULL || (!beatmapStd->isSpinnerActive() || !cv::osu::hud_hiterrorbar_hide_during_spinner.getBool())) && !beatmap->isLoading())
			{
				if (beatmapStd != NULL)
					drawHitErrorBar(OsuGameRules::getHitWindow300(beatmap), OsuGameRules::getHitWindow100(beatmap), OsuGameRules::getHitWindow50(beatmap), OsuGameRules::getHitWindowMiss(beatmap), osu->getScore()->getUnstableRate());
				else if (beatmapMania != NULL)
					drawHitErrorBar(OsuGameRulesMania::getHitWindow300(beatmap), OsuGameRulesMania::getHitWindow100(beatmap), OsuGameRulesMania::getHitWindow50(beatmap), OsuGameRulesMania::getHitWindowMiss(beatmap), osu->getScore()->getUnstableRate());
			}
		}

		if (cv::osu::draw_score.getBool())
			drawScore(osu->getScore()->getScore());

		if (cv::osu::draw_combo.getBool())
			drawCombo(osu->getScore()->getCombo());

		if (cv::osu::draw_progressbar.getBool())
			drawProgressBar(beatmap->getPercentFinishedPlayable(), beatmap->isWaiting());

		if (cv::osu::draw_accuracy.getBool())
			drawAccuracy(osu->getScore()->getAccuracy()*100.0f);

		if (osu->getModTarget() && cv::osu::draw_target_heatmap.getBool() && beatmapStd != NULL)
			drawTargetHeatmap(beatmapStd->getHitcircleDiameter());
	}
	else if (!cv::osu::hud_shift_tab_toggles_everything.getBool())
	{
		if (cv::osu::draw_inputoverlay.getBool() && beatmapStd != NULL)
		{
			const bool isAutoClicking = (osu->getModAuto() || osu->getModRelax());
			if (!isAutoClicking)
				drawInputOverlay(osu->getScore()->getKeyCount(1), osu->getScore()->getKeyCount(2), osu->getScore()->getKeyCount(3), osu->getScore()->getKeyCount(4));
		}

		// NOTE: moved to draw behind hitobjects in OsuBeatmapStandard::draw()
		if (cv::osu::fposu::mod_fposu.getBool())
		{
			if (cv::osu::draw_hiterrorbar.getBool() && (beatmapStd == NULL || (!beatmapStd->isSpinnerActive() || !cv::osu::hud_hiterrorbar_hide_during_spinner.getBool())) && !beatmap->isLoading())
			{
				if (beatmapStd != NULL)
					drawHitErrorBar(OsuGameRules::getHitWindow300(beatmap), OsuGameRules::getHitWindow100(beatmap), OsuGameRules::getHitWindow50(beatmap), OsuGameRules::getHitWindowMiss(beatmap), osu->getScore()->getUnstableRate());
				else if (beatmapMania != NULL)
					drawHitErrorBar(OsuGameRulesMania::getHitWindow300(beatmap), OsuGameRulesMania::getHitWindow100(beatmap), OsuGameRulesMania::getHitWindow50(beatmap), OsuGameRulesMania::getHitWindowMiss(beatmap), osu->getScore()->getUnstableRate());
			}
		}
	}

	if (beatmap->shouldFlashSectionPass())
		drawSectionPass(beatmap->shouldFlashSectionPass());
	if (beatmap->shouldFlashSectionFail())
		drawSectionFail(beatmap->shouldFlashSectionFail());

	if (beatmap->shouldFlashWarningArrows())
		drawWarningArrows(beatmapStd != NULL ? beatmapStd->getHitcircleDiameter() : 0);

	if (beatmap->isContinueScheduled() && beatmapStd != NULL && cv::osu::draw_continue.getBool())
		drawContinue(beatmapStd->getContinueCursorPoint(), beatmapStd->getHitcircleDiameter());

	if (cv::osu::draw_scrubbing_timeline.getBool() && osu->isSeeking())
	{
		static std::vector<BREAK> breaks;
		breaks.clear();

		if (cv::osu::draw_scrubbing_timeline_breaks.getBool())
		{
			const unsigned long lengthPlayableMS = beatmap->getLengthPlayable();
			const unsigned long startTimePlayableMS = beatmap->getStartTimePlayable();
			const unsigned long endTimePlayableMS = startTimePlayableMS + lengthPlayableMS;

			const std::vector<OsuDatabaseBeatmap::BREAK> &beatmapBreaks = beatmap->getBreaks();

			breaks.reserve(beatmapBreaks.size());

			for (int i=0; i<beatmapBreaks.size(); i++)
			{
				const OsuDatabaseBeatmap::BREAK &bk = beatmapBreaks[i];

				// ignore breaks after last hitobject
				if (/*bk.endTime <= (int)startTimePlayableMS ||*/ std::cmp_greater_equal(bk.startTime, (startTimePlayableMS + lengthPlayableMS)))
					continue;

				BREAK bk2;

				bk2.startPercent = (float)(bk.startTime) / (float)(endTimePlayableMS);
				bk2.endPercent = (float)(bk.endTime) / (float)(endTimePlayableMS);

				//debugLog("{}: s = {:f}, e = {:f}\n", i, bk2.startPercent, bk2.endPercent);

				breaks.push_back(bk2);
			}
		}

		drawScrubbingTimeline(beatmap->getTime(), beatmap->getLength(), beatmap->getLengthPlayable(), beatmap->getStartTimePlayable(), beatmap->getPercentFinishedPlayable(), breaks);
	}
}

void OsuHUD::update()
{
	OsuBeatmap *beatmap = osu->getSelectedBeatmap();

	if (beatmap != NULL)
	{
		// health anim
		const double currentHealth = beatmap->getHealth();
		const double elapsedMS = engine->getFrameTime() * 1000.0;
		const double frameAimTime = 1000.0 / 60.0;
		const double frameRatio = elapsedMS / frameAimTime;
		if (m_fHealth < currentHealth)
			m_fHealth = std::min(1.0, m_fHealth + std::abs(currentHealth - m_fHealth) / 4.0 * frameRatio);
		else if (m_fHealth > currentHealth)
			m_fHealth = std::max(0.0, m_fHealth - std::abs(m_fHealth - currentHealth) / 6.0 * frameRatio);

		if (cv::osu::hud_scorebar_hide_during_breaks.getBool())
		{
			if (!anim->isAnimating(&m_fScoreBarBreakAnim) && !beatmap->isWaiting())
			{
				if (m_fScoreBarBreakAnim == 0.0f && beatmap->isInBreak())
					anim->moveLinear(&m_fScoreBarBreakAnim, 1.0f, cv::osu::hud_scorebar_hide_anim_duration.getFloat(), true);
				else if (m_fScoreBarBreakAnim == 1.0f && !beatmap->isInBreak())
					anim->moveLinear(&m_fScoreBarBreakAnim, 0.0f, cv::osu::hud_scorebar_hide_anim_duration.getFloat(), true);
			}
		}
		else
			m_fScoreBarBreakAnim = 0.0f;
	}

	// dynamic hud scaling updates
	m_fScoreHeight = osu->getSkin()->getScore0()->getHeight() * getScoreScale();

	// fps string update
	if (cv::osu::hud_fps_smoothing.getBool())
	{
		const float smooth = std::pow(0.05, engine->getFrameTime());
		m_fCurFpsSmooth = smooth*m_fCurFpsSmooth + (1.0f - smooth)*(1.0f / engine->getFrameTime());
		if (engine->getTime() > m_fFpsUpdate || std::abs(m_fCurFpsSmooth-m_fCurFps) > 2.0f)
		{
			m_fFpsUpdate = engine->getTime() + 0.25f;
			m_fCurFps = m_fCurFpsSmooth;
		}
	}
	else
		m_fCurFps = (1.0f / engine->getFrameTime());

	// target heatmap cleanup
	if (osu->getModTarget())
	{
		if (m_targets.size() > 0 && engine->getTime() > m_targets[0].time)
			m_targets.erase(m_targets.begin());
	}

	// cursor ripples cleanup
	if (cv::osu::draw_cursor_ripples.getBool())
	{
		if (m_cursorRipples.size() > 0 && engine->getTime() > m_cursorRipples[0].time)
			m_cursorRipples.erase(m_cursorRipples.begin());
	}

	// volume overlay
	m_volumeMaster->setEnabled(m_fVolumeChangeTime > engine->getTime());
	m_volumeEffects->setEnabled(m_volumeMaster->isEnabled());
	m_volumeMusic->setEnabled(m_volumeMaster->isEnabled());
	m_volumeSliderOverlayContainer->setSize(osu->getVirtScreenSize());
	m_volumeSliderOverlayContainer->update();

	if (!m_volumeMaster->isBusy())
		m_volumeMaster->setValue(cv::osu::volume_master.getFloat(), false);
	else
	{
		cv::osu::volume_master.setValue(m_volumeMaster->getFloat());
		m_fLastVolume = m_volumeMaster->getFloat();
	}

	if (!m_volumeMusic->isBusy())
		m_volumeMusic->setValue(cv::osu::volume_music.getFloat(), false);
	else
		cv::osu::volume_music.setValue(m_volumeMusic->getFloat());

	if (!m_volumeEffects->isBusy())
		m_volumeEffects->setValue(cv::osu::volume_effects.getFloat(), false);
	else
		cv::osu::volume_effects.setValue(m_volumeEffects->getFloat());

	// force focus back to master if no longer active
	if (engine->getTime() > m_fVolumeChangeTime && !m_volumeMaster->isSelected())
	{
		m_volumeMusic->setSelected(false);
		m_volumeEffects->setSelected(false);
		m_volumeMaster->setSelected(true);
	}

	// keep overlay alive as long as the user is doing something
	// switch selection if cursor moves inside one of the sliders
	if (m_volumeSliderOverlayContainer->isBusy() || (m_volumeMaster->isEnabled() && (m_volumeMaster->isMouseInside() || m_volumeEffects->isMouseInside() || m_volumeMusic->isMouseInside())))
	{
		animateVolumeChange();

		const std::vector<CBaseUIElement*> &elements = m_volumeSliderOverlayContainer->getElements();
		for (int i=0; i<elements.size(); i++)
		{
			if (((OsuUIVolumeSlider*)elements[i])->checkWentMouseInside())
			{
				for (int c=0; c<elements.size(); c++)
				{
					if (c != i)
						((OsuUIVolumeSlider*)elements[c])->setSelected(false);
				}
				((OsuUIVolumeSlider*)elements[i])->setSelected(true);
			}
		}
	}
}

void OsuHUD::onResolutionChange(Vector2  /*newResolution*/)
{
	updateLayout();
}

void OsuHUD::updateLayout()
{
	// volume overlay
	{
		const float dpiScale = Osu::getUIScale();
		const float sizeMultiplier = cv::osu::hud_volume_size_multiplier.getFloat() * dpiScale;

		m_volumeMaster->setSize(300*sizeMultiplier, 50*sizeMultiplier);
		m_volumeMaster->setBlockSize(m_volumeMaster->getSize().y + 7 * dpiScale, m_volumeMaster->getSize().y);

		m_volumeEffects->setSize(m_volumeMaster->getSize().x, m_volumeMaster->getSize().y/1.5f);
		m_volumeEffects->setBlockSize((m_volumeMaster->getSize().y + 7 * dpiScale)/1.5f, m_volumeMaster->getSize().y/1.5f);

		m_volumeMusic->setSize(m_volumeMaster->getSize().x, m_volumeMaster->getSize().y/1.5f);
		m_volumeMusic->setBlockSize((m_volumeMaster->getSize().y + 7 * dpiScale)/1.5f, m_volumeMaster->getSize().y/1.5f);
	}
}

void OsuHUD::drawDummy()
{
	drawPlayfieldBorder(OsuGameRules::getPlayfieldCenter(), OsuGameRules::getPlayfieldSize(), 0);

	if (cv::osu::draw_scorebarbg.getBool())
		drawScorebarBg(1.0f, 0.0f);

	if (cv::osu::draw_scorebar.getBool())
		drawHPBar(1.0, 1.0f, 0.0);

	if (cv::osu::draw_inputoverlay.getBool())
		drawInputOverlay(0, 0, 0, 0);

	SCORE_ENTRY scoreEntry;
	scoreEntry.name = cv::name.getString();
	scoreEntry.index = 0;
	scoreEntry.combo = 420;
	scoreEntry.score = 12345678;
	scoreEntry.accuracy = 1.0f;
	scoreEntry.missingBeatmap = false;
	scoreEntry.downloadingBeatmap = false;
	scoreEntry.dead = false;
	scoreEntry.highlight = true;
	if (cv::osu::draw_scoreboard.getBool())
	{
		static std::vector<SCORE_ENTRY> scoreEntries;
		scoreEntries.clear();
		{
			scoreEntries.push_back(scoreEntry);
		}
		drawScoreBoardInt(scoreEntries);
	}

	drawSkip();

	drawStatistics(0, 0, 727, 2.3f, 5.5f, 180, 180.0f, 9.0f, 4.0f, 8.0f, 6.0f, 4, 6, 90, 123.0f, 1234.0f, 25.0f, -5, 15);

	drawWarningArrows();

	if (cv::osu::draw_combo.getBool())
		drawCombo(scoreEntry.combo);

	if (cv::osu::draw_score.getBool())
		drawScore(scoreEntry.score);

	if (cv::osu::draw_progressbar.getBool())
		drawProgressBar(0.25f, false);

	if (cv::osu::draw_accuracy.getBool())
		drawAccuracy(scoreEntry.accuracy*100.0f);

	if (cv::osu::draw_hiterrorbar.getBool())
		drawHitErrorBar(50, 100, 150, 400, 70);
}

void OsuHUD::drawCursor(Vector2 pos, float alphaMultiplier, bool secondTrail, bool updateAndDrawTrail)
{
	if (cv::osu::draw_cursor_ripples.getBool() && (!cv::osu::fposu::mod_fposu.getBool() || !osu->isInPlayMode()))
		drawCursorRipples();

	Matrix4 mvp;
	drawCursorInt(m_cursorTrailShader, secondTrail ? m_cursorTrail2 : m_cursorTrail, mvp, pos, alphaMultiplier, false, updateAndDrawTrail);
}

void OsuHUD::drawCursorTrail(Vector2 pos, float alphaMultiplier, bool secondTrail)
{
	const bool fposuTrailJumpFix = (cv::osu::fposu::mod_fposu.getBool() && osu->isInPlayMode() && !osu->getFPoSu()->isCrosshairIntersectingScreen());

	const bool trailJumpFix = fposuTrailJumpFix;

	Matrix4 mvp;
	drawCursorTrailInt(m_cursorTrailShader, secondTrail ? m_cursorTrail2 : m_cursorTrail, mvp, pos, alphaMultiplier, trailJumpFix);
}

void OsuHUD::drawCursorSpectator1(Vector2 pos, float alphaMultiplier)
{
	Matrix4 mvp;
	drawCursorInt(m_cursorTrailShader, m_cursorTrailSpectator1, mvp, pos, alphaMultiplier);
}

void OsuHUD::drawCursorSpectator2(Vector2 pos, float alphaMultiplier)
{
	Matrix4 mvp;
	drawCursorInt(m_cursorTrailShader, m_cursorTrailSpectator2, mvp, pos, alphaMultiplier);
}

void OsuHUD::drawCursorInt(Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp, Vector2 pos, float alphaMultiplier, bool emptyTrailFrame, bool updateAndDrawTrail)
{
	if (updateAndDrawTrail)
		drawCursorTrailInt(trailShader, trail, mvp, pos, alphaMultiplier, emptyTrailFrame);

	drawCursorRaw(pos, alphaMultiplier);
}

void OsuHUD::drawCursorTrailInt(Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 & /*mvp*/, Vector2 pos, float alphaMultiplier, bool emptyTrailFrame)
{
	Image *trailImage = osu->getSkin()->getCursorTrail();

	if (cv::osu::draw_cursor_trail.getBool() && trailImage->isReady())
	{
		const bool smoothCursorTrail = osu->getSkin()->useSmoothCursorTrail() || cv::osu::cursor_trail_smooth_force.getBool();

		const float trailWidth = trailImage->getWidth() * getCursorTrailScaleFactor() * cv::osu::cursor_scale.getFloat();
		const float trailHeight = trailImage->getHeight() * getCursorTrailScaleFactor() * cv::osu::cursor_scale.getFloat();

		if (smoothCursorTrail)
			m_cursorTrailVAO->empty();

		// add the sample for the current frame
		addCursorTrailPosition(trail, pos, emptyTrailFrame);

		// this loop draws the old style trail, and updates the alpha values for each segment, and fills the vao for the new style trail
		const float trailLength = smoothCursorTrail ? cv::osu::cursor_trail_smooth_length.getFloat() : cv::osu::cursor_trail_length.getFloat();
		int i = trail.size() - 1;
		while (i >= 0)
		{
			trail[i].alpha = std::clamp<float>(((trail[i].time - engine->getTime()) / trailLength) * alphaMultiplier, 0.0f, 1.0f) * cv::osu::cursor_trail_alpha.getFloat();

			if (smoothCursorTrail)
			{
				const float scaleAnimTrailWidthHalf = (trailWidth/2) * trail[i].scale;
				const float scaleAnimTrailHeightHalf = (trailHeight/2) * trail[i].scale;

				const Vector3 topLeft = Vector3(trail[i].pos.x - scaleAnimTrailWidthHalf, trail[i].pos.y - scaleAnimTrailHeightHalf, trail[i].alpha);
				m_cursorTrailVAO->addVertex(topLeft);
				m_cursorTrailVAO->addTexcoord(0, 0);

				const Vector3 topRight = Vector3(trail[i].pos.x + scaleAnimTrailWidthHalf, trail[i].pos.y - scaleAnimTrailHeightHalf, trail[i].alpha);
				m_cursorTrailVAO->addVertex(topRight);
				m_cursorTrailVAO->addTexcoord(1, 0);

				const Vector3 bottomRight = Vector3(trail[i].pos.x + scaleAnimTrailWidthHalf, trail[i].pos.y + scaleAnimTrailHeightHalf, trail[i].alpha);
				m_cursorTrailVAO->addVertex(bottomRight);
				m_cursorTrailVAO->addTexcoord(1, 1);

				const Vector3 bottomLeft = Vector3(trail[i].pos.x - scaleAnimTrailWidthHalf, trail[i].pos.y + scaleAnimTrailHeightHalf, trail[i].alpha);
				m_cursorTrailVAO->addVertex(bottomLeft);
				m_cursorTrailVAO->addTexcoord(0, 1);
			}
			else // old style trail
			{
				if (trail[i].alpha > 0.0f)
					drawCursorTrailRaw(trail[i].alpha, trail[i].pos);
			}

			i--;
		}

		// draw new style continuous smooth trail
		if (smoothCursorTrail)
		{
			trailShader->enable();
			{
				trailShader->setUniform1f("time", (float)engine->getTime());

				if constexpr (Env::cfg(REND::DX11))
				{
					g->forceUpdateTransform();
					Matrix4 mvp = g->getMVP();
					trailShader->setUniformMatrix4fv("mvp", mvp);
				}

				trailImage->bind();
				{
					g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ADDITIVE);
					{
						g->setColor(0xffffffff);
						g->drawVAO(m_cursorTrailVAO);
					}
					g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
				}
				trailImage->unbind();
			}
			trailShader->disable();
		}
	}

	// trail cleanup
	while ((trail.size() > 1 && engine->getTime() > trail[0].time) || trail.size() > cv::osu::cursor_trail_max_size.getInt()) // always leave at least 1 previous entry in there
	{
		trail.erase(trail.begin());
	}
}

void OsuHUD::drawCursorRaw(Vector2 pos, float alphaMultiplier)
{
	Image *cursor = osu->getSkin()->getCursor();
	const float scale = getCursorScaleFactor() * (osu->getSkin()->isCursor2x() ? 0.5f : 1.0f);
	const float animatedScale = scale * (osu->getSkin()->getCursorExpand() ? m_fCursorExpandAnim : 1.0f);

	// draw cursor
	g->setColor(0xffffffff);
	g->setAlpha(cv::osu::cursor_alpha.getFloat()*alphaMultiplier);
	g->pushTransform();
	{
		g->scale(animatedScale*cv::osu::cursor_scale.getFloat(), animatedScale*cv::osu::cursor_scale.getFloat());

		if (!osu->getSkin()->getCursorCenter())
			g->translate((cursor->getWidth()/2.0f)*animatedScale*cv::osu::cursor_scale.getFloat(), (cursor->getHeight()/2.0f)*animatedScale*cv::osu::cursor_scale.getFloat());

		if (osu->getSkin()->getCursorRotate())
			g->rotate(fmod(engine->getTime()*37.0f, 360.0f));

		g->translate(pos.x, pos.y);
		g->drawImage(cursor);
	}
	g->popTransform();

	// draw cursor middle
	if (osu->getSkin()->getCursorMiddle() != osu->getSkin()->getMissingTexture())
	{
		g->setColor(0xffffffff);
		g->setAlpha(cv::osu::cursor_alpha.getFloat()*alphaMultiplier);
		g->pushTransform();
		{
			g->scale(scale*cv::osu::cursor_scale.getFloat(), scale*cv::osu::cursor_scale.getFloat());
			g->translate(pos.x, pos.y, 0.05f);

			if (!osu->getSkin()->getCursorCenter())
				g->translate((osu->getSkin()->getCursorMiddle()->getWidth()/2.0f)*scale*cv::osu::cursor_scale.getFloat(), (osu->getSkin()->getCursorMiddle()->getHeight()/2.0f)*scale*cv::osu::cursor_scale.getFloat());

			g->drawImage(osu->getSkin()->getCursorMiddle());
		}
		g->popTransform();
	}
}

void OsuHUD::drawCursorTrailRaw(float alpha, Vector2 pos)
{
	Image *trailImage = osu->getSkin()->getCursorTrail();
	const float scale = getCursorTrailScaleFactor();
	const float animatedScale = scale * (osu->getSkin()->getCursorExpand() && cv::osu::cursor_trail_expand.getBool() ? m_fCursorExpandAnim : 1.0f) * cv::osu::cursor_trail_scale.getFloat();

	g->setColor(0xffffffff);
	g->setAlpha(alpha);
	g->pushTransform();
	{
		g->scale(animatedScale*cv::osu::cursor_scale.getFloat(), animatedScale*cv::osu::cursor_scale.getFloat());
		g->translate(pos.x, pos.y);
		g->drawImage(trailImage);
	}
	g->popTransform();
}

void OsuHUD::drawCursorRipples()
{
	if (osu->getSkin()->getCursorRipple() == osu->getSkin()->getMissingTexture()) return;

	// allow overscale/underscale as usual
	// this does additionally scale with the resolution (which osu doesn't do for some reason for cursor ripples)
	const float normalized2xScale = (osu->getSkin()->isCursorRipple2x() ? 0.5f : 1.0f);
	const float imageScale = Osu::getImageScale(Vector2(520.0f, 520.0f), 233.0f);

	const float normalizedWidth = osu->getSkin()->getCursorRipple()->getWidth() * normalized2xScale * imageScale;
	const float normalizedHeight = osu->getSkin()->getCursorRipple()->getHeight() * normalized2xScale * imageScale;

	const float duration = std::max(cv::osu::cursor_ripple_duration.getFloat(), 0.0001f);
	const float fadeDuration = std::max(cv::osu::cursor_ripple_duration.getFloat() - cv::osu::cursor_ripple_anim_start_fadeout_delay.getFloat(), 0.0001f);

	if (cv::osu::cursor_ripple_additive.getBool())
		g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ADDITIVE);

	g->setColor(rgb(std::clamp<int>(cv::osu::cursor_ripple_tint_r.getInt(), 0, 255), std::clamp<int>(cv::osu::cursor_ripple_tint_g.getInt(), 0, 255), std::clamp<int>(cv::osu::cursor_ripple_tint_b.getInt(), 0, 255)));
	osu->getSkin()->getCursorRipple()->bind();
	{
		for (int i=0; i<m_cursorRipples.size(); i++)
		{
			const Vector2 &pos = m_cursorRipples[i].pos;
			const float &time = m_cursorRipples[i].time;

			const float animPercent = 1.0f - std::clamp<float>((time - engine->getTime()) / duration, 0.0f, 1.0f);
			const float fadePercent = 1.0f - std::clamp<float>((time - engine->getTime()) / fadeDuration, 0.0f, 1.0f);

			const float scale = std::lerp(cv::osu::cursor_ripple_anim_start_scale.getFloat(), cv::osu::cursor_ripple_anim_end_scale.getFloat(), 1.0f - (1.0f - animPercent)*(1.0f - animPercent)); // quad out

			g->setAlpha(cv::osu::cursor_ripple_alpha.getFloat() * (1.0f - fadePercent));
			g->drawQuad(pos.x - normalizedWidth*scale/2, pos.y - normalizedHeight*scale/2, normalizedWidth*scale, normalizedHeight*scale);
		}
	}
	osu->getSkin()->getCursorRipple()->unbind();

	if (cv::osu::cursor_ripple_additive.getBool())
		g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
}

void OsuHUD::drawFps(McFont *font, float fps)
{
	fps = std::round(fps);
	const UString fpsString = UString::format("%i fps", (int)(fps));
	const UString msString = UString::format("%.1f ms", (1.0f/fps)*1000.0f);

	const float dpiScale = Osu::getUIScale();

	const int margin = std::round(3.0f * dpiScale);
	const int shadowOffset = std::round(1.0f * dpiScale);

	// shadow
	g->setColor(0xff000000);
	g->pushTransform();
	{
		g->translate(osu->getVirtScreenWidth() - font->getStringWidth(fpsString) - margin + shadowOffset, osu->getVirtScreenHeight() - margin - font->getHeight() - margin + shadowOffset);
		g->drawString(font, fpsString);
	}
	g->popTransform();
	g->pushTransform();
	{
		g->translate(osu->getVirtScreenWidth() - font->getStringWidth(msString) - margin + shadowOffset, osu->getVirtScreenHeight() - margin + shadowOffset);
		g->drawString(font, msString);
	}
	g->popTransform();

	// top
	if (fps >= 200)
		g->setColor(0xffffffff);
	else if (fps >= 120)
		g->setColor(0xffdddd00);
	else
	{
		const float pulse = std::abs(std::sin(engine->getTime()*4));
		g->setColor(argb(1.0f, 1.0f, 0.26f*pulse, 0.26f*pulse));
	}

	g->pushTransform();
	{
		g->translate(osu->getVirtScreenWidth() - font->getStringWidth(fpsString) - margin, osu->getVirtScreenHeight() - margin - font->getHeight() - margin);
		g->drawString(font, fpsString);
	}
	g->popTransform();
	g->pushTransform();
	{
		g->translate(osu->getVirtScreenWidth() - font->getStringWidth(msString) - margin, osu->getVirtScreenHeight() - margin);
		g->drawString(font, msString);
	}
	g->popTransform();
}

void OsuHUD::drawPlayfieldBorder(Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter)
{
	drawPlayfieldBorder(playfieldCenter, playfieldSize, hitcircleDiameter, cv::osu::hud_playfield_border_size.getInt());
}

void OsuHUD::drawPlayfieldBorder(Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter, float borderSize)
{
	if (borderSize <= 0.0f) return;

	// respect playfield stretching
	playfieldSize.x += playfieldSize.x*cv::osu::playfield_stretch_x.getFloat();
	playfieldSize.y += playfieldSize.y*cv::osu::playfield_stretch_y.getFloat();

	Vector2 playfieldBorderTopLeft = Vector2((int)(playfieldCenter.x - playfieldSize.x/2 - hitcircleDiameter/2 - borderSize), (int)(playfieldCenter.y - playfieldSize.y/2 - hitcircleDiameter/2 - borderSize));
	Vector2 playfieldBorderSize = Vector2((int)(playfieldSize.x + hitcircleDiameter), (int)(playfieldSize.y + hitcircleDiameter));

	const Color innerColor = 0x44ffffff;
	const Color outerColor = 0x00000000;

	g->pushTransform();
	{
		g->translate(0, 0, 0.2f);

		// top
		{
			static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			vao.empty();

			vao.addVertex(playfieldBorderTopLeft);
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize*2, 0));
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, borderSize));
			vao.addColor(innerColor);

			g->drawVAO(&vao);
		}

		// left
		{
			static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			vao.empty();

			vao.addVertex(playfieldBorderTopLeft);
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, playfieldBorderSize.y + borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(0, playfieldBorderSize.y + 2*borderSize));
			vao.addColor(outerColor);

			g->drawVAO(&vao);
		}

		// right
		{
			static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			vao.empty();

			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, 0));
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, playfieldBorderSize.y + 2*borderSize));
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, playfieldBorderSize.y + borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, borderSize));
			vao.addColor(innerColor);

			g->drawVAO(&vao);
		}

		// bottom
		{
			static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			vao.empty();

			vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, playfieldBorderSize.y + borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, playfieldBorderSize.y + borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, playfieldBorderSize.y + 2*borderSize));
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(0, playfieldBorderSize.y + 2*borderSize));
			vao.addColor(outerColor);

			g->drawVAO(&vao);
		}
	}
	g->popTransform();

	/*
	//g->setColor(0x44ffffff);
	// top
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y, playfieldBorderSize.x + borderSize*2, borderSize);

	// left
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y + borderSize, borderSize, playfieldBorderSize.y);

	// right
	g->fillRect(playfieldBorderTopLeft.x + playfieldBorderSize.x + borderSize, playfieldBorderTopLeft.y + borderSize, borderSize, playfieldBorderSize.y);

	// bottom
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y+playfieldBorderSize.y + borderSize, playfieldBorderSize.x + borderSize*2, borderSize);
	*/
}

void OsuHUD::drawLoadingSmall()
{
	/*
	McFont *font = resourceManager->getFont("FONT_DEFAULT");
	UString loadingText = "Loading ...";
	float stringWidth = font->getStringWidth(loadingText);

	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(-stringWidth/2, font->getHeight()/2);
	g->rotate(engine->getTime()*180, 0, 0, 1);
	g->translate(0, 0);
	g->translate(osu->getVirtScreenWidth()/2, osu->getVirtScreenHeight()/2);
	g->drawString(font, loadingText);
	g->popTransform();
	*/
	const float scale = Osu::getImageScale(osu->getSkin()->getLoadingSpinner(), 29);

	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->rotate(engine->getTime()*180, 0, 0, 1);
		g->scale(scale, scale);
		g->translate(osu->getVirtScreenWidth()/2, osu->getVirtScreenHeight()/2);
		g->drawImage(osu->getSkin()->getLoadingSpinner());
	}
	g->popTransform();
}

void OsuHUD::drawBeatmapImportSpinner()
{
	const float scale = Osu::getImageScale(osu->getSkin()->getBeatmapImportSpinner(), 100);

	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->rotate(engine->getTime()*180, 0, 0, 1);
		g->scale(scale, scale);
		g->translate(osu->getVirtScreenWidth()/2, osu->getVirtScreenHeight()/2);
		g->drawImage(osu->getSkin()->getBeatmapImportSpinner());
	}
	g->popTransform();
}

void OsuHUD::drawVolumeChange()
{
	if (engine->getTime() > m_fVolumeChangeTime) return;

	const float dpiScale = Osu::getUIScale();
	const float sizeMultiplier = cv::osu::hud_volume_size_multiplier.getFloat() * dpiScale;
	const float height = osu->getVirtScreenHeight() - m_volumeMusic->getPos().y;

	// legacy
	/*
	g->setColor(argb((char)(68*m_fVolumeChangeFade), 255, 255, 255));
	g->fillRect(0, osu->getVirtScreenHeight()*(1.0f - m_fLastVolume), osu->getVirtScreenWidth(), osu->getVirtScreenHeight()*m_fLastVolume);
	*/

	if (m_fVolumeChangeFade != 1.0f)
	{
		g->push3DScene(McRect(m_volumeMaster->getPos().x, m_volumeMaster->getPos().y, m_volumeMaster->getSize().x, (osu->getVirtScreenHeight() - m_volumeMaster->getPos().y)*2.05f));
		//g->rotate3DScene(-(1.0f - m_fVolumeChangeFade)*90, 0, 0);
		//g->translate3DScene(0, (m_fVolumeChangeFade*60 - 60) * sizeMultiplier / 1.5f, ((m_fVolumeChangeFade)*500 - 500) * sizeMultiplier / 1.5f);
		g->translate3DScene(0, (m_fVolumeChangeFade*height - height)*1.05f, 0.0f);

	}

	m_volumeMaster->setPos(osu->getVirtScreenSize() - m_volumeMaster->getSize() - Vector2(m_volumeMaster->getMinimumExtraTextWidth(), m_volumeMaster->getSize().y));
	m_volumeEffects->setPos(m_volumeMaster->getPos() - Vector2(0, m_volumeEffects->getSize().y + 20 * sizeMultiplier));
	m_volumeMusic->setPos(m_volumeEffects->getPos() - Vector2(0, m_volumeMusic->getSize().y + 20 * sizeMultiplier));

	m_volumeSliderOverlayContainer->draw();

	if (m_fVolumeChangeFade != 1.0f)
		g->pop3DScene();
}

void OsuHUD::drawScoreNumber(unsigned long long number, float scale, bool drawLeadingZeroes)
{
	// get digits
	static std::vector<int> digits;
	digits.clear();
	while (number >= 10)
	{
		int curDigit = number % 10;
		number /= 10;

		digits.insert(digits.begin(), curDigit);
	}
	digits.insert(digits.begin(), number);
	if (digits.size() == 1)
	{
		if (drawLeadingZeroes)
			digits.insert(digits.begin(), 0);
	}

	// draw them
	// NOTE: just using the width here is incorrect, but it is the quickest solution instead of painstakingly reverse-engineering how osu does it
	float lastWidth = osu->getSkin()->getScore0()->getWidth();
	for (int i=0; i<digits.size(); i++)
	{
		switch (digits[i])
		{
		case 0:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore0());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 1:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore1());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 2:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore2());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 3:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore3());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 4:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore4());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 5:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore5());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 6:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore6());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 7:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore7());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 8:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore8());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 9:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScore9());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		}

		g->translate(-osu->getSkin()->getScoreOverlap()*(osu->getSkin()->isScore02x() ? 2 : 1)*scale, 0);
	}
}

void OsuHUD::drawComboNumber(unsigned long long number, float scale, bool drawLeadingZeroes)
{
	// get digits
	static std::vector<int> digits;
	digits.clear();
	while (number >= 10)
	{
		int curDigit = number % 10;
		number /= 10;

		digits.insert(digits.begin(), curDigit);
	}
	digits.insert(digits.begin(), number);
	if (digits.size() == 1)
	{
		if (drawLeadingZeroes)
			digits.insert(digits.begin(), 0);
	}

	// draw them
	// NOTE: just using the width here is incorrect, but it is the quickest solution instead of painstakingly reverse-engineering how osu does it
	float lastWidth = osu->getSkin()->getCombo0()->getWidth();
	for (int i=0; i<digits.size(); i++)
	{
		switch (digits[i])
		{
		case 0:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo0());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 1:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo1());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 2:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo2());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 3:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo3());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 4:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo4());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 5:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo5());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 6:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo6());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 7:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo7());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 8:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo8());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 9:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getCombo9());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		}

		g->translate(-osu->getSkin()->getComboOverlap()*(osu->getSkin()->isCombo02x() ? 2 : 1)*scale, 0);
	}
}

void OsuHUD::drawComboSimple(int combo, float scale)
{
	g->pushTransform();
	{
		drawComboNumber(combo, scale);

		// draw 'x' at the end
		if (osu->getSkin()->getComboX() != osu->getSkin()->getMissingTexture())
		{
			g->translate(osu->getSkin()->getComboX()->getWidth()*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getComboX());
		}
	}
	g->popTransform();
}

void OsuHUD::drawCombo(int combo)
{
	g->setColor(0xffffffff);

	const int offset = 5;

	// draw back (anim)
	float animScaleMultiplier = 1.0f + m_fComboAnim2*cv::osu::combo_anim2_size.getFloat();
	float scale = osu->getImageScale(osu->getSkin()->getCombo0(), 32)*animScaleMultiplier * cv::osu::hud_scale.getFloat() * cv::osu::hud_combo_scale.getFloat();
	if (m_fComboAnim2 > 0.01f)
	{
		g->setAlpha(m_fComboAnim2*0.65f);
		g->pushTransform();
		{
			g->scale(scale, scale);
			g->translate(offset, osu->getVirtScreenHeight() - osu->getSkin()->getCombo0()->getHeight()*scale/2.0f, 0.0f);
			drawComboNumber(combo, scale);

			// draw 'x' at the end
			if (osu->getSkin()->getComboX() != osu->getSkin()->getMissingTexture())
			{
				g->translate(osu->getSkin()->getComboX()->getWidth()*0.5f*scale, 0);
				g->drawImage(osu->getSkin()->getComboX());
			}
		}
		g->popTransform();
	}

	// draw front
	g->setAlpha(1.0f);
	const float animPercent = (m_fComboAnim1 < 1.0f ? m_fComboAnim1 : 2.0f - m_fComboAnim1);
	animScaleMultiplier = 1.0f + (0.5f*animPercent*animPercent)*cv::osu::combo_anim1_size.getFloat();
	scale = osu->getImageScale(osu->getSkin()->getCombo0(), 32) * animScaleMultiplier * cv::osu::hud_scale.getFloat() * cv::osu::hud_combo_scale.getFloat();
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(offset, osu->getVirtScreenHeight() - osu->getSkin()->getCombo0()->getHeight()*scale/2.0f, 0.0f);
		drawComboNumber(combo, scale);

		// draw 'x' at the end
		if (osu->getSkin()->getComboX() != osu->getSkin()->getMissingTexture())
		{
			g->translate(osu->getSkin()->getComboX()->getWidth()*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getComboX());
		}
	}
	g->popTransform();
}

void OsuHUD::drawScore(unsigned long long score)
{
	g->setColor(0xffffffff);

	int numDigits = 1;
	unsigned long long scoreCopy = score;
	while (scoreCopy >= 10)
	{
		scoreCopy /= 10;
		numDigits++;
	}

	const float scale = getScoreScale();
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(osu->getVirtScreenWidth() - osu->getSkin()->getScore0()->getWidth()*scale*numDigits + osu->getSkin()->getScoreOverlap()*(osu->getSkin()->isScore02x() ? 2 : 1)*scale*(numDigits-1), osu->getSkin()->getScore0()->getHeight()*scale/2);
		drawScoreNumber(score, scale, false);
	}
	g->popTransform();
}

void OsuHUD::drawScorebarBg(float alpha, float breakAnim)
{
	if (osu->getSkin()->getScorebarBg()->isMissingTexture()) return;

	const float scale = cv::osu::hud_scale.getFloat() * cv::osu::hud_scorebar_scale.getFloat();
	const float ratio = Osu::getImageScale(Vector2(1, 1), 1.0f);

	const Vector2 breakAnimOffset = Vector2(0, -20.0f * breakAnim) * ratio;

	g->setColor(0xffffffff);
	g->setAlpha(alpha * (1.0f - breakAnim));
	osu->getSkin()->getScorebarBg()->draw((osu->getSkin()->getScorebarBg()->getSize() / 2.0f) * scale + (breakAnimOffset * scale), scale);
}

void OsuHUD::drawSectionPass(float alpha)
{
	if (!osu->getSkin()->getSectionPassImage()->isMissingTexture())
	{
		g->setColor(0xffffffff);
		g->setAlpha(alpha);
		osu->getSkin()->getSectionPassImage()->draw(osu->getVirtScreenSize() / 2.0f);
	}
}

void OsuHUD::drawSectionFail(float alpha)
{
	if (!osu->getSkin()->getSectionFailImage()->isMissingTexture())
	{
		g->setColor(0xffffffff);
		g->setAlpha(alpha);
		osu->getSkin()->getSectionFailImage()->draw(osu->getVirtScreenSize() / 2.0f);
	}
}

void OsuHUD::drawHPBar(double health, float alpha, float breakAnim)
{
	const bool useNewDefault = !osu->getSkin()->getScorebarMarker()->isMissingTexture();

	const float scale = cv::osu::hud_scale.getFloat() * cv::osu::hud_scorebar_scale.getFloat();
	const float ratio = Osu::getImageScale(Vector2(1, 1), 1.0f);

	const Vector2 colourOffset = (useNewDefault ? Vector2(7.5f, 7.8f) : Vector2(3.0f, 10.0f)) * ratio;
	const float currentXPosition = (colourOffset.x + (health * osu->getSkin()->getScorebarColour()->getSize().x));
	const Vector2 markerOffset = (useNewDefault ? Vector2(currentXPosition, (8.125f + 2.5f) * ratio) : Vector2(currentXPosition, 10.0f * ratio));
	const Vector2 breakAnimOffset = Vector2(0, -20.0f * breakAnim) * ratio;

	// lerp color depending on health
	if (useNewDefault)
	{
		if (health < 0.2)
		{
			const float factor = std::max(0.0, (0.2 - health) / 0.2);
			const float value = std::lerp(0.0f, 1.0f, factor);
			g->setColor(argb(1.0f, value, 0.0f, 0.0f));
		}
		else if (health < 0.5)
		{
			const float factor = std::max(0.0, (0.5 - health) / 0.5);
			const float value = std::lerp(1.0f, 0.0f, factor);
			g->setColor(argb(1.0f, value, value, value));
		}
		else
			g->setColor(0xffffffff);
	}
	else
		g->setColor(0xffffffff);

	if (breakAnim != 0.0f || alpha != 1.0f)
		g->setAlpha(alpha * (1.0f - breakAnim));

	// draw health bar fill
	{
		// DEBUG:
		/*
		g->fillRect(0, 25, osu->getVirtScreenWidth()*health, 10);
		*/

		osu->getSkin()->getScorebarColour()->setDrawClipWidthPercent(health);
		osu->getSkin()->getScorebarColour()->draw((osu->getSkin()->getScorebarColour()->getSize() / 2.0f * scale) + (colourOffset * scale) + (breakAnimOffset * scale), scale);
	}

	// draw ki
	{
		OsuSkinImage *ki = NULL;

		if (useNewDefault)
			ki = osu->getSkin()->getScorebarMarker();
		else if (osu->getSkin()->getScorebarColour()->isFromDefaultSkin() || !osu->getSkin()->getScorebarKi()->isFromDefaultSkin())
		{
			if (health < 0.2)
				ki = osu->getSkin()->getScorebarKiDanger2();
			else if (health < 0.5)
				ki = osu->getSkin()->getScorebarKiDanger();
			else
				ki = osu->getSkin()->getScorebarKi();
		}

		if (ki != NULL && !ki->isMissingTexture())
		{
			if (!useNewDefault || health >= 0.2)
			{
				ki->draw((markerOffset * scale) + (breakAnimOffset * scale), scale * m_fKiScaleAnim);
			}
		}
	}
}

void OsuHUD::drawAccuracySimple(float accuracy, float scale)
{
	// get integer & fractional parts of the number
	const int accuracyInt = (int)accuracy;
	const int accuracyFrac = std::clamp<int>(((int)(std::round((accuracy - accuracyInt)*100.0f))), 0, 99); // round up

	// draw it
	g->pushTransform();
	{
		drawScoreNumber(accuracyInt, scale, true);

		// draw dot '.' between the integer and fractional part
		if (osu->getSkin()->getScoreDot() != osu->getSkin()->getMissingTexture())
		{
			g->setColor(0xffffffff);
			g->translate(osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScoreDot());
			g->translate(osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
			g->translate(-osu->getSkin()->getScoreOverlap()*(osu->getSkin()->isScore02x() ? 2 : 1)*scale, 0);
		}

		drawScoreNumber(accuracyFrac, scale, true);

		// draw '%' at the end
		if (osu->getSkin()->getScorePercent() != osu->getSkin()->getMissingTexture())
		{
			g->setColor(0xffffffff);
			g->translate(osu->getSkin()->getScorePercent()->getWidth()*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScorePercent());
		}
	}
	g->popTransform();
}

void OsuHUD::drawAccuracy(float accuracy)
{
	g->setColor(0xffffffff);

	// get integer & fractional parts of the number
	const int accuracyInt = (int)accuracy;
	const int accuracyFrac = std::clamp<int>(((int)(std::round((accuracy - accuracyInt)*100.0f))), 0, 99); // round up

	// draw it
	const int offset = 5;
	const float scale = osu->getImageScale(osu->getSkin()->getScore0(), 13) * cv::osu::hud_scale.getFloat() * cv::osu::hud_accuracy_scale.getFloat();
	g->pushTransform();
	{
		const int numDigits = (accuracyInt > 99 ? 5 : 4);
		const float xOffset = osu->getSkin()->getScore0()->getWidth()*scale*numDigits + (osu->getSkin()->getScoreDot() != osu->getSkin()->getMissingTexture() ? osu->getSkin()->getScoreDot()->getWidth() : 0)*scale + (osu->getSkin()->getScorePercent() != osu->getSkin()->getMissingTexture() ? osu->getSkin()->getScorePercent()->getWidth() : 0)*scale - osu->getSkin()->getScoreOverlap()*(osu->getSkin()->isScore02x() ? 2 : 1)*scale*(numDigits+1);

		m_fAccuracyXOffset = osu->getVirtScreenWidth() - xOffset - offset;
		m_fAccuracyYOffset = (cv::osu::draw_score.getBool() ? m_fScoreHeight : 0.0f) + osu->getSkin()->getScore0()->getHeight()*scale/2 + offset*2;

		g->scale(scale, scale);
		g->translate(m_fAccuracyXOffset, m_fAccuracyYOffset);

		drawScoreNumber(accuracyInt, scale, true);

		// draw dot '.' between the integer and fractional part
		if (osu->getSkin()->getScoreDot() != osu->getSkin()->getMissingTexture())
		{
			g->setColor(0xffffffff);
			g->translate(osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScoreDot());
			g->translate(osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
			g->translate(-osu->getSkin()->getScoreOverlap()*(osu->getSkin()->isScore02x() ? 2 : 1)*scale, 0);
		}

		drawScoreNumber(accuracyFrac, scale, true);

		// draw '%' at the end
		if (osu->getSkin()->getScorePercent() != osu->getSkin()->getMissingTexture())
		{
			g->setColor(0xffffffff);
			g->translate(osu->getSkin()->getScorePercent()->getWidth()*0.5f*scale, 0);
			g->drawImage(osu->getSkin()->getScorePercent());
		}
	}
	g->popTransform();
}

void OsuHUD::drawSkip()
{
	const float scale = cv::osu::hud_scale.getFloat();

	g->setColor(0xffffffff);
	osu->getSkin()->getPlaySkip()->draw(osu->getVirtScreenSize() - (osu->getSkin()->getPlaySkip()->getSize()/2.0f)*scale, cv::osu::hud_scale.getFloat());
}

void OsuHUD::drawWarningArrow(Vector2 pos, bool flipVertically, bool originLeft)
{
	const float scale = cv::osu::hud_scale.getFloat() * osu->getImageScale(osu->getSkin()->getPlayWarningArrow(), 78);

	g->pushTransform();
	{
		g->scale(flipVertically ? -scale : scale, scale);
		g->translate(pos.x + (flipVertically ? (-osu->getSkin()->getPlayWarningArrow()->getWidth()*scale/2.0f) : (osu->getSkin()->getPlayWarningArrow()->getWidth()*scale/2.0f)) * (originLeft ? 1.0f : -1.0f), pos.y, 0.0f);
		g->drawImage(osu->getSkin()->getPlayWarningArrow());
	}
	g->popTransform();
}

void OsuHUD::drawWarningArrows(float  /*hitcircleDiameter*/)
{
	const float divider = 18.0f;
	const float part = OsuGameRules::getPlayfieldSize().y * (1.0f / divider);

	g->setColor(0xffffffff);
	drawWarningArrow(Vector2(osu->getUIScale(28), OsuGameRules::getPlayfieldCenter().y - OsuGameRules::getPlayfieldSize().y/2 + part*2), false);
	drawWarningArrow(Vector2(osu->getUIScale(28), OsuGameRules::getPlayfieldCenter().y - OsuGameRules::getPlayfieldSize().y/2 + part*2 + part*13), false);

	drawWarningArrow(Vector2(osu->getVirtScreenWidth() - osu->getUIScale(28), OsuGameRules::getPlayfieldCenter().y - OsuGameRules::getPlayfieldSize().y/2 + part*2), true);
	drawWarningArrow(Vector2(osu->getVirtScreenWidth() - osu->getUIScale(28), OsuGameRules::getPlayfieldCenter().y - OsuGameRules::getPlayfieldSize().y/2 + part*2 + part*13), true);
}

void OsuHUD::drawScoreBoard(std::string &beatmapMD5Hash, OsuScore *currentScore)
{
	const int maxVisibleDatabaseScores = 4;

	const std::vector<OsuDatabase::Score> *scores = &((*osu->getSongBrowser()->getDatabase()->getScores())[beatmapMD5Hash]);
	const int numScores = scores->size();

	if (numScores < 1) return;

	static std::vector<SCORE_ENTRY> scoreEntries;
	scoreEntries.clear();
	scoreEntries.reserve(numScores);

	const bool isUnranked = (osu->getModAuto() || (osu->getModAutopilot() && osu->getModRelax()));

	bool injectCurrentScore = true;
	for (int i=0; i<numScores && i<maxVisibleDatabaseScores; i++)
	{
		SCORE_ENTRY scoreEntry;

		scoreEntry.name = (*scores)[i].playerName;

		scoreEntry.index = -1;
		scoreEntry.combo = (*scores)[i].comboMax;
		scoreEntry.score = (*scores)[i].score;
		scoreEntry.accuracy = OsuScore::calculateAccuracy((*scores)[i].num300s, (*scores)[i].num100s, (*scores)[i].num50s, (*scores)[i].numMisses);

		scoreEntry.missingBeatmap = false;
		scoreEntry.downloadingBeatmap = false;
		scoreEntry.dead = false;
		scoreEntry.highlight = false;

		const bool isLastScore = (i == numScores-1) || (i == maxVisibleDatabaseScores-1);
		const bool isCurrentScoreMore = (currentScore->getScore() > scoreEntry.score);
		bool scoreAlreadyAdded = false;
		if (injectCurrentScore && (isCurrentScoreMore || isLastScore))
		{
			injectCurrentScore = false;

			SCORE_ENTRY currentScoreEntry;

			currentScoreEntry.name = (isUnranked ? "McOsu" : cv::name.getString());

			currentScoreEntry.combo = currentScore->getComboMax();
			currentScoreEntry.score = currentScore->getScore();
			currentScoreEntry.accuracy = currentScore->getAccuracy();

			currentScoreEntry.index = i;
			if (!isCurrentScoreMore)
			{
				for (int j=i; j<numScores; j++)
				{
					if (currentScoreEntry.score > (*scores)[j].score)
						break;
					else
						currentScoreEntry.index = j+1;
				}
			}

			currentScoreEntry.missingBeatmap = false;
			currentScoreEntry.downloadingBeatmap = false;
			currentScoreEntry.dead = currentScore->isDead();
			currentScoreEntry.highlight = true;

			if (isLastScore)
			{
				if (isCurrentScoreMore)
					scoreEntries.push_back(std::move(currentScoreEntry));

				scoreEntries.push_back(std::move(scoreEntry));
				scoreAlreadyAdded = true;

				if (!isCurrentScoreMore)
					scoreEntries.push_back(std::move(currentScoreEntry));
			}
			else
				scoreEntries.push_back(std::move(currentScoreEntry));
		}

		if (!scoreAlreadyAdded)
			scoreEntries.push_back(std::move(scoreEntry));
	}

	drawScoreBoardInt(scoreEntries);
}

void OsuHUD::drawScoreBoardMP()
{
	const int numPlayers = osu->getMultiplayer()->getPlayers()->size();

	static std::vector<SCORE_ENTRY> scoreEntries;
	scoreEntries.clear();
	scoreEntries.reserve(numPlayers);

	for (int i=0; i<numPlayers; i++)
	{
		SCORE_ENTRY scoreEntry;

		scoreEntry.name = (*osu->getMultiplayer()->getPlayers())[i].name;

		scoreEntry.index = -1;
		scoreEntry.combo = (*osu->getMultiplayer()->getPlayers())[i].combo;
		scoreEntry.score = (*osu->getMultiplayer()->getPlayers())[i].score;
		scoreEntry.accuracy = (*osu->getMultiplayer()->getPlayers())[i].accuracy;

		scoreEntry.missingBeatmap = (*osu->getMultiplayer()->getPlayers())[i].missingBeatmap;
		scoreEntry.downloadingBeatmap = (*osu->getMultiplayer()->getPlayers())[i].downloadingBeatmap;
		scoreEntry.dead = (*osu->getMultiplayer()->getPlayers())[i].dead;
		scoreEntry.highlight = ((*osu->getMultiplayer()->getPlayers())[i].id == networkHandler->getLocalClientID());

		scoreEntries.push_back(std::move(scoreEntry));
	}

	drawScoreBoardInt(scoreEntries);
}

void OsuHUD::drawScoreBoardInt(const std::vector<OsuHUD::SCORE_ENTRY> &scoreEntries)
{
	if (scoreEntries.size() < 1) return;

	const bool useMenuButtonBackground = cv::osu::hud_scoreboard_use_menubuttonbackground.getBool();
	OsuSkinImage *backgroundImage = osu->getSkin()->getMenuButtonBackground2();
	const float oScale = backgroundImage->getResolutionScale() * 0.99f; // for converting harcoded osu offset pixels to screen pixels

	McFont *indexFont = osu->getSongBrowserFontBold();
	McFont *nameFont = osu->getSongBrowserFont();
	McFont *scoreFont = osu->getSongBrowserFont();
	McFont *comboFont = scoreFont;
	McFont *accFont = comboFont;

	const Color backgroundColor = 0x55114459;
	const Color backgroundColorHighlight = 0x55777777;
	const Color backgroundColorTop = 0x551b6a8c;
	const Color backgroundColorDead = 0x55660000;
	const Color backgroundColorMissingBeatmap = 0x55aa0000;

	const Color indexColor = 0x11ffffff;
	const Color indexColorHighlight = 0x22ffffff;

	const Color nameScoreColor = 0xffaaaaaa;
	const Color nameScoreColorHighlight = 0xffffffff;
	const Color nameScoreColorTop = 0xffeeeeee;
	const Color nameScoreColorDownloading = 0xffeeee00;
	const Color nameScoreColorDead = 0xffee0000;

	const Color comboAccuracyColor = 0xff5d9ca1;
	const Color comboAccuracyColorHighlight = 0xff99fafe;
	const Color comboAccuracyColorTop = 0xff84dbe0;

	const Color textShadowColor = 0x66000000;

	const bool drawTextShadow = (cv::osu::background_dim.getFloat() < 0.7f);

	const float scale = cv::osu::hud_scoreboard_scale.getFloat() * cv::osu::hud_scale.getFloat();
	const float height = osu->getVirtScreenHeight() * 0.07f * scale;
	const float width = height*2.6f; // was 2.75f
	const float margin = height*0.1f;
	const float padding = height*0.05f;

	const float minStartPosY = osu->getVirtScreenHeight() - (scoreEntries.size()*height + (scoreEntries.size()-1)*margin);

	const float startPosX = 0;
	const float startPosY = std::clamp<float>(osu->getVirtScreenHeight()/2 - (scoreEntries.size()*height + (scoreEntries.size()-1)*margin)/2 + cv::osu::hud_scoreboard_offset_y_percent.getFloat()*osu->getVirtScreenHeight(), 0.0f, minStartPosY);
	for (int i=0; i<scoreEntries.size(); i++)
	{
		const float x = startPosX;
		const float y = startPosY + i*(height + margin);

		// draw background
		g->setColor((scoreEntries[i].missingBeatmap ? backgroundColorMissingBeatmap : (scoreEntries[i].dead ? backgroundColorDead : (scoreEntries[i].highlight ? backgroundColorHighlight : (i == 0 ? backgroundColorTop : backgroundColor)))));
		if (useMenuButtonBackground)
		{
			const float backgroundScale = 0.62f + 0.005f;
			backgroundImage->draw(Vector2(x + (backgroundImage->getSizeBase().x/2)*backgroundScale*scale - (470*oScale)*backgroundScale*scale, y + height/2), backgroundScale*scale);
		}
		else
			g->fillRect(x, y, width, height);

		// draw index
		const float indexScale = 0.5f;
		g->pushTransform();
		{
			const float scale = (height / indexFont->getHeight())*indexScale;

			UString indexString = UString::format("%i", (scoreEntries[i].index > -1 ? scoreEntries[i].index : i) + 1);
			const float stringWidth = indexFont->getStringWidth(indexString);

			g->scale(scale, scale);
			g->translate(x + width - stringWidth*scale - 2*padding, y + indexFont->getHeight()*scale);
			g->setColor((scoreEntries[i].highlight ? indexColorHighlight : indexColor));
			g->drawString(indexFont, indexString);
		}
		g->popTransform();

		// draw name
		const bool isDownloadingOrHasNoMap = (scoreEntries[i].downloadingBeatmap || scoreEntries[i].missingBeatmap);
		const float nameScale = 0.315f;
		if (!isDownloadingOrHasNoMap)
		{
			g->pushTransform();
			{
				const bool isInPlayMode = osu->isInPlayMode();

				if (isInPlayMode)
					g->pushClipRect(McRect(x, y, width - 2*padding, height));

				UString nameString = scoreEntries[i].name;
				if (scoreEntries[i].downloadingBeatmap)
					nameString.append(" [downloading]");
				else if (scoreEntries[i].missingBeatmap)
					nameString.append(" [no map]");

				const float scale = (height / nameFont->getHeight())*nameScale;

				g->scale(scale, scale);
				g->translate(x + padding, y + padding + nameFont->getHeight()*scale);
				if (drawTextShadow)
				{
					g->translate(1, 1);
					g->setColor(textShadowColor);
					g->drawString(nameFont, nameString);
					g->translate(-1, -1);
				}
				g->setColor((scoreEntries[i].dead || scoreEntries[i].missingBeatmap ? (scoreEntries[i].downloadingBeatmap ? nameScoreColorDownloading : nameScoreColorDead) : (scoreEntries[i].highlight ? nameScoreColorHighlight : (i == 0 ? nameScoreColorTop : nameScoreColor))));
				g->drawString(nameFont, nameString);

				if (isInPlayMode)
					g->popClipRect();
			}
			g->popTransform();
		}

		// draw score
		const float scoreScale = 0.26f;
		g->pushTransform();
		{
			const float scale = (height / scoreFont->getHeight())*scoreScale;

			UString scoreString = UString::format("%llu", scoreEntries[i].score);

			g->scale(scale, scale);
			g->translate(x + padding*1.35f, y + height - 2*padding);
			if (drawTextShadow)
			{
				g->translate(1, 1);
				g->setColor(textShadowColor);
				g->drawString(scoreFont, scoreString);
				g->translate(-1, -1);
			}
			g->setColor((scoreEntries[i].dead ? nameScoreColorDead : (scoreEntries[i].highlight ? nameScoreColorHighlight : (i == 0 ? nameScoreColorTop : nameScoreColor))));
			g->drawString(scoreFont, scoreString);
		}
		g->popTransform();

		// draw combo
		const float comboScale = scoreScale;
		g->pushTransform();
		{
			const float scale = (height / comboFont->getHeight())*comboScale;

			UString comboString = UString::format("%ix", scoreEntries[i].combo);
			const float stringWidth = comboFont->getStringWidth(comboString);

			g->scale(scale, scale);
			g->translate(x + width - stringWidth*scale - padding*1.35f, y + height - 2*padding);
			if (drawTextShadow)
			{
				g->translate(1, 1);
				g->setColor(textShadowColor);
				g->drawString(comboFont, comboString);
				g->translate(-1, -1);
			}
			g->setColor((scoreEntries[i].highlight ? comboAccuracyColorHighlight : (i == 0 ? comboAccuracyColorTop : comboAccuracyColor)));
			g->drawString(scoreFont, comboString);
		}
		g->popTransform();

		// draw accuracy
		if (osu->isInMultiplayer() && (!osu->isInPlayMode() || cv::osu::mp_win_condition_accuracy.getBool()))
		{
			const float accScale = comboScale;
			g->pushTransform();
			{
				const float scale = (height / accFont->getHeight())*accScale;

				UString accString = UString::format("%.2f%%", scoreEntries[i].accuracy*100.0f);
				const float stringWidth = accFont->getStringWidth(accString);

				g->scale(scale, scale);
				g->translate(x + width - stringWidth*scale - padding*1.35f, y + accFont->getHeight()*scale + 2*padding);
				///if (drawTextShadow)
				{
					g->translate(1, 1);
					g->setColor(textShadowColor);
					g->drawString(accFont, accString);
					g->translate(-1, -1);
				}
				g->setColor((scoreEntries[i].highlight ? comboAccuracyColorHighlight : (i == 0 ? comboAccuracyColorTop : comboAccuracyColor)));
				g->drawString(accFont, accString);
			}
			g->popTransform();
		}

		// HACKHACK: code duplication
		if (isDownloadingOrHasNoMap)
		{
			g->pushTransform();
			{
				const bool isInPlayMode = osu->isInPlayMode();

				if (isInPlayMode)
					g->pushClipRect(McRect(x, y, width - 2*padding, height));

				UString nameString = scoreEntries[i].name;
				if (scoreEntries[i].downloadingBeatmap)
					nameString.append(" [downloading]");
				else if (scoreEntries[i].missingBeatmap)
					nameString.append(" [no map]");

				const float scale = (height / nameFont->getHeight())*nameScale;

				g->scale(scale, scale);
				g->translate(x + padding, y + padding + nameFont->getHeight()*scale);
				if (drawTextShadow)
				{
					g->translate(1, 1);
					g->setColor(textShadowColor);
					g->drawString(nameFont, nameString);
					g->translate(-1, -1);
				}
				g->setColor((scoreEntries[i].dead || scoreEntries[i].missingBeatmap ? (scoreEntries[i].downloadingBeatmap ? nameScoreColorDownloading : nameScoreColorDead) : (scoreEntries[i].highlight ? nameScoreColorHighlight : (i == 0 ? nameScoreColorTop : nameScoreColor))));
				g->drawString(nameFont, nameString);

				if (isInPlayMode)
					g->popClipRect();
			}
			g->popTransform();
		}
	}
}

void OsuHUD::drawContinue(Vector2 cursor, float hitcircleDiameter)
{
	Image *unpause = osu->getSkin()->getUnpause();
	const float unpauseScale = Osu::getImageScale(unpause, 80);

	Image *cursorImage = osu->getSkin()->getDefaultCursor();
	const float cursorScale = Osu::getImageScaleToFitResolution(cursorImage, Vector2(hitcircleDiameter, hitcircleDiameter));

	// bleh
	if (cursor.x < cursorImage->getWidth() || cursor.y < cursorImage->getHeight() || cursor.x > osu->getVirtScreenWidth()-cursorImage->getWidth() || cursor.y > osu->getVirtScreenHeight()-cursorImage->getHeight())
		cursor = osu->getVirtScreenSize()/2.0f;

	// base
	g->setColor(rgb(255, 153, 51));
	g->pushTransform();
	{
		g->scale(cursorScale, cursorScale);
		g->translate(cursor.x, cursor.y);
		g->drawImage(cursorImage);
	}
	g->popTransform();

	// pulse animation
	const float cursorAnimPulsePercent = std::clamp<float>(fmod(engine->getTime(), 1.35f), 0.0f, 1.0f);
	g->setColor(argb((short)(255.0f*(1.0f-cursorAnimPulsePercent)), 255, 153, 51));
	g->pushTransform();
	{
		g->scale(cursorScale*(1.0f + cursorAnimPulsePercent), cursorScale*(1.0f + cursorAnimPulsePercent));
		g->translate(cursor.x, cursor.y);
		g->drawImage(cursorImage);
	}
	g->popTransform();

	// unpause click message
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->scale(unpauseScale, unpauseScale);
		g->translate(cursor.x + 20 + (unpause->getWidth()/2)*unpauseScale, cursor.y + 20 + (unpause->getHeight()/2)*unpauseScale);
		g->drawImage(unpause);
	}
	g->popTransform();
}

void OsuHUD::drawHitErrorBar(OsuBeatmapStandard *beatmapStd)
{
	if (cv::osu::draw_hud.getBool() || !cv::osu::hud_shift_tab_toggles_everything.getBool())
	{
		if (cv::osu::draw_hiterrorbar.getBool() && (!beatmapStd->isSpinnerActive() || !cv::osu::hud_hiterrorbar_hide_during_spinner.getBool()) && !beatmapStd->isLoading())
			drawHitErrorBar(OsuGameRules::getHitWindow300(beatmapStd), OsuGameRules::getHitWindow100(beatmapStd), OsuGameRules::getHitWindow50(beatmapStd), OsuGameRules::getHitWindowMiss(beatmapStd), osu->getScore()->getUnstableRate());
	}
}

void OsuHUD::drawHitErrorBar(float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss, int ur)
{
	const Vector2 center = Vector2(osu->getVirtScreenWidth()/2.0f, osu->getVirtScreenHeight() - osu->getVirtScreenHeight()*2.15f*cv::osu::hud_hiterrorbar_height_percent.getFloat()*cv::osu::hud_scale.getFloat()*cv::osu::hud_hiterrorbar_scale.getFloat() - osu->getVirtScreenHeight()*cv::osu::hud_hiterrorbar_offset_percent.getFloat());

	if (cv::osu::draw_hiterrorbar_bottom.getBool())
	{
		g->pushTransform();
		{
			const Vector2 localCenter = Vector2(center.x, center.y - (osu->getVirtScreenHeight() * cv::osu::hud_hiterrorbar_offset_bottom_percent.getFloat()));

			drawHitErrorBarInt2(localCenter, ur);
			g->translate(localCenter.x, localCenter.y);
			drawHitErrorBarInt(hitWindow300, hitWindow100, hitWindow50, hitWindowMiss);
		}
		g->popTransform();
	}

	if (cv::osu::draw_hiterrorbar_top.getBool())
	{
		g->pushTransform();
		{
			const Vector2 localCenter = Vector2(center.x, osu->getVirtScreenHeight() - center.y + (osu->getVirtScreenHeight() * cv::osu::hud_hiterrorbar_offset_top_percent.getFloat()));

			g->scale(1, -1);
			//drawHitErrorBarInt2(localCenter, ur);
			g->translate(localCenter.x, localCenter.y);
			drawHitErrorBarInt(hitWindow300, hitWindow100, hitWindow50, hitWindowMiss);
		}
		g->popTransform();
	}

	if (cv::osu::draw_hiterrorbar_left.getBool())
	{
		g->pushTransform();
		{
			const Vector2 localCenter = Vector2(osu->getVirtScreenHeight() - center.y + (osu->getVirtScreenWidth() * cv::osu::hud_hiterrorbar_offset_left_percent.getFloat()), osu->getVirtScreenHeight()/2.0f);

			g->rotate(90);
			//drawHitErrorBarInt2(localCenter, ur);
			g->translate(localCenter.x, localCenter.y);
			drawHitErrorBarInt(hitWindow300, hitWindow100, hitWindow50, hitWindowMiss);
		}
		g->popTransform();
	}

	if (cv::osu::draw_hiterrorbar_right.getBool())
	{
		g->pushTransform();
		{
			const Vector2 localCenter = Vector2(osu->getVirtScreenWidth() - (osu->getVirtScreenHeight() - center.y) - (osu->getVirtScreenWidth() * cv::osu::hud_hiterrorbar_offset_right_percent.getFloat()), osu->getVirtScreenHeight()/2.0f);

			g->scale(-1, 1);
			g->rotate(-90);
			//drawHitErrorBarInt2(localCenter, ur);
			g->translate(localCenter.x, localCenter.y);
			drawHitErrorBarInt(hitWindow300, hitWindow100, hitWindow50, hitWindowMiss);
		}
		g->popTransform();
	}
}

void OsuHUD::drawHitErrorBarInt(float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss)
{
	const float alpha = cv::osu::hud_hiterrorbar_alpha.getFloat();
	if (alpha <= 0.0f) return;

	const float alphaEntry = alpha * cv::osu::hud_hiterrorbar_entry_alpha.getFloat();
	const int alphaCenterlineInt = std::clamp<int>((int)(alpha * cv::osu::hud_hiterrorbar_centerline_alpha.getFloat() * 255.0f), 0, 255);
	const int alphaBarInt = std::clamp<int>((int)(alpha * cv::osu::hud_hiterrorbar_bar_alpha.getFloat() * 255.0f), 0, 255);

	const Color color300 = argb(alphaBarInt, std::clamp<int>(cv::osu::hud_hiterrorbar_entry_300_r.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_entry_300_g.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_entry_300_b.getInt(), 0, 255));
	const Color color100 = argb(alphaBarInt, std::clamp<int>(cv::osu::hud_hiterrorbar_entry_100_r.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_entry_100_g.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_entry_100_b.getInt(), 0, 255));
	const Color color50 = argb(alphaBarInt, std::clamp<int>(cv::osu::hud_hiterrorbar_entry_50_r.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_entry_50_g.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_entry_50_b.getInt(), 0, 255));
	const Color colorMiss = argb(alphaBarInt, std::clamp<int>(cv::osu::hud_hiterrorbar_entry_miss_r.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_entry_miss_g.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_entry_miss_b.getInt(), 0, 255));

	Vector2 size = Vector2(osu->getVirtScreenWidth()*cv::osu::hud_hiterrorbar_width_percent.getFloat(), osu->getVirtScreenHeight()*cv::osu::hud_hiterrorbar_height_percent.getFloat())*cv::osu::hud_scale.getFloat()*cv::osu::hud_hiterrorbar_scale.getFloat();
	if (cv::osu::hud_hiterrorbar_showmisswindow.getBool())
		size = Vector2(osu->getVirtScreenWidth()*cv::osu::hud_hiterrorbar_width_percent_with_misswindow.getFloat(), osu->getVirtScreenHeight()*cv::osu::hud_hiterrorbar_height_percent.getFloat())*cv::osu::hud_scale.getFloat()*cv::osu::hud_hiterrorbar_scale.getFloat();

	const Vector2 center = Vector2(0, 0); // NOTE: moved to drawHitErrorBar()

	const float entryHeight = size.y*cv::osu::hud_hiterrorbar_bar_height_scale.getFloat();
	const float entryWidth = size.y*cv::osu::hud_hiterrorbar_bar_width_scale.getFloat();

	float totalHitWindowLength = hitWindow50;
	if (cv::osu::hud_hiterrorbar_showmisswindow.getBool())
		totalHitWindowLength = hitWindowMiss;

	const float percent50 = hitWindow50 / totalHitWindowLength;
	const float percent100 = hitWindow100 / totalHitWindowLength;
	const float percent300 = hitWindow300 / totalHitWindowLength;

	// draw background bar with color indicators for 300s, 100s and 50s (and the miss window)
	if (alphaBarInt > 0)
	{
		const bool half = cv::osu::stdrules::mod_halfwindow.getBool();
		const bool halfAllow300s = cv::osu::stdrules::mod_halfwindow_allow_300s.getBool();

		if (cv::osu::hud_hiterrorbar_showmisswindow.getBool())
		{
			g->setColor(colorMiss);
			g->fillRect(center.x - size.x/2.0f, center.y - size.y/2.0f, size.x, size.y);
		}

		if (!cv::osu::stdrules::mod_no100s.getBool() && !cv::osu::stdrules::mod_no50s.getBool())
		{
			g->setColor(color50);
			g->fillRect(center.x - size.x*percent50/2.0f, center.y - size.y/2.0f, size.x*percent50 * (half ? 0.5f : 1.0f), size.y);
		}

		if (!cv::osu::stdrules::mod_ming3012.getBool() && !cv::osu::stdrules::mod_no100s.getBool())
		{
			g->setColor(color100);
			g->fillRect(center.x - size.x*percent100/2.0f, center.y - size.y/2.0f, size.x*percent100 * (half ? 0.5f : 1.0f), size.y);
		}

		g->setColor(color300);
		g->fillRect(center.x - size.x*percent300/2.0f, center.y - size.y/2.0f, size.x*percent300 * (half && !halfAllow300s ? 0.5f : 1.0f), size.y);
	}

	// draw hit errors
	{
		if (cv::osu::hud_hiterrorbar_entry_additive.getBool())
			g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ADDITIVE);

		const bool modMing3012 = cv::osu::stdrules::mod_ming3012.getBool();
		const float hitFadeDuration = cv::osu::hud_hiterrorbar_entry_hit_fade_time.getFloat();
		const float missFadeDuration = cv::osu::hud_hiterrorbar_entry_miss_fade_time.getFloat();
		for (int i=m_hiterrors.size()-1; i>=0; i--)
		{
			const float percent = std::clamp<float>((float)m_hiterrors[i].delta / (float)totalHitWindowLength, -5.0f, 5.0f);
			float fade = std::clamp<float>((m_hiterrors[i].time - engine->getTime()) / (m_hiterrors[i].miss || m_hiterrors[i].misaim ? missFadeDuration : hitFadeDuration), 0.0f, 1.0f);
			fade *= fade; // quad out

			Color barColor;
			{
				if (m_hiterrors[i].miss || m_hiterrors[i].misaim)
					barColor = colorMiss;
				else
					barColor = (std::abs(percent) <= percent300 ? color300 : (std::abs(percent) <= percent100 && !modMing3012 ? color100 : color50));
			}

			g->setColor(barColor);
			g->setAlpha(alphaEntry * fade);

			float missHeightMultiplier = 1.0f;
			if (m_hiterrors[i].miss)
				missHeightMultiplier = cv::osu::hud_hiterrorbar_entry_miss_height_multiplier.getFloat();
			if (m_hiterrors[i].misaim)
				missHeightMultiplier = cv::osu::hud_hiterrorbar_entry_misaim_height_multiplier.getFloat();

			//Color leftColor = argb((int)((255/2) * alphaEntry * fade), barColor.R(), barColor.G(), barColor.B());
			//Color centerColor = argb((int)(barColor.a * alphaEntry * fade), barColor.R(), barColor.G(), barColor.B());
			//Color rightColor = leftColor;

			g->fillRect(center.x - (entryWidth/2.0f) + percent*(size.x/2.0f), center.y - (entryHeight*missHeightMultiplier)/2.0f, entryWidth, (entryHeight*missHeightMultiplier));
			//g->fillGradient((int)(center.x - (entryWidth/2.0f) + percent*(size.x/2.0f)), center.y - (entryHeight*missHeightMultiplier)/2.0f, (int)(entryWidth/2.0f), (entryHeight*missHeightMultiplier), leftColor, centerColor, leftColor, centerColor);
			//g->fillGradient((int)(center.x - (entryWidth/2.0f/2.0f) + percent*(size.x/2.0f)), center.y - (entryHeight*missHeightMultiplier)/2.0f, (int)(entryWidth/2.0f), (entryHeight*missHeightMultiplier), centerColor, rightColor, centerColor, rightColor);
		}

		if (cv::osu::hud_hiterrorbar_entry_additive.getBool())
			g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
	}

	// white center line
	if (alphaCenterlineInt > 0)
	{
		g->setColor(argb(alphaCenterlineInt, std::clamp<int>(cv::osu::hud_hiterrorbar_centerline_r.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_centerline_g.getInt(), 0, 255), std::clamp<int>(cv::osu::hud_hiterrorbar_centerline_b.getInt(), 0, 255)));
		g->fillRect(center.x - entryWidth/2.0f/2.0f, center.y - entryHeight/2.0f, entryWidth/2.0f, entryHeight);
	}
}

void OsuHUD::drawHitErrorBarInt2(Vector2 center, int ur)
{
	const float alpha = cv::osu::hud_hiterrorbar_alpha.getFloat() * cv::osu::hud_hiterrorbar_ur_alpha.getFloat();
	if (alpha <= 0.0f) return;

	const float dpiScale = Osu::getUIScale();

	const float hitErrorBarSizeY = osu->getVirtScreenHeight()*cv::osu::hud_hiterrorbar_height_percent.getFloat()*cv::osu::hud_scale.getFloat()*cv::osu::hud_hiterrorbar_scale.getFloat();
	const float entryHeight = hitErrorBarSizeY*cv::osu::hud_hiterrorbar_bar_height_scale.getFloat();

	if (cv::osu::draw_hiterrorbar_ur.getBool())
	{
		g->pushTransform();
		{
			UString urText = UString::format("%i UR", ur);
			McFont *urTextFont = osu->getSongBrowserFont();

			const float hitErrorBarScale = cv::osu::hud_scale.getFloat() * cv::osu::hud_hiterrorbar_scale.getFloat();
			const float urTextScale = hitErrorBarScale * cv::osu::hud_hiterrorbar_ur_scale.getFloat() * 0.5f;
			const float urTextWidth = urTextFont->getStringWidth(urText) * urTextScale;
			const float urTextHeight = urTextFont->getHeight() * hitErrorBarScale;

			g->scale(urTextScale, urTextScale);
			g->translate((int)(center.x + (-urTextWidth/2.0f) + (urTextHeight)*(cv::osu::hud_hiterrorbar_ur_offset_x_percent.getFloat())*dpiScale) + 1, (int)(center.y + (urTextHeight)*(cv::osu::hud_hiterrorbar_ur_offset_y_percent.getFloat())*dpiScale - entryHeight/1.25f) + 1);

			// shadow
			g->setColor(0xff000000);
			g->setAlpha(alpha);
			g->drawString(urTextFont, urText);

			g->translate(-1, -1);

			// text
			g->setColor(0xffffffff);
			g->setAlpha(alpha);
			g->drawString(urTextFont, urText);
		}
		g->popTransform();
	}
}

void OsuHUD::drawProgressBar(float percent, bool waiting)
{
	if (!cv::osu::draw_accuracy.getBool())
		m_fAccuracyXOffset = osu->getVirtScreenWidth();

	const float num_segments = 15*8;
	const int offset = 20;
	const float radius = osu->getUIScale(10.5f) * cv::osu::hud_scale.getFloat() * cv::osu::hud_progressbar_scale.getFloat();
	const float circularMetreScale = ((2*radius)/osu->getSkin()->getCircularmetre()->getWidth()) * 1.3f; // hardcoded 1.3 multiplier?!
	const float actualCircularMetreScale = ((2*radius)/osu->getSkin()->getCircularmetre()->getWidth());
	Vector2 center = Vector2(m_fAccuracyXOffset - radius - offset, m_fAccuracyYOffset);

	// clamp to top edge of screen
	if (center.y - (osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f < 0)
		center.y += std::abs(center.y - (osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f);

	// clamp to bottom edge of score numbers
	if (cv::osu::draw_score.getBool() && center.y-radius < m_fScoreHeight)
		center.y = m_fScoreHeight + radius;

	const float theta = 2 * PI / float(num_segments);
	const float s = sinf(theta); // precalculate the sine and cosine
	const float c = cosf(theta);
	float t;
	float x = 0;
	float y = -radius; // we start at the top

	if (waiting)
		g->setColor(0xaa00f200);
	else
		g->setColor(0xaaf2f2f2);

	{
		static VertexArrayObject vao;
		vao.empty();

		Vector2 prevVertex;
		for (int i=0; i<num_segments+1; i++)
		{
			float curPercent = (i*(360.0f / num_segments)) / 360.0f;
			if (curPercent > percent)
				break;

			// build current vertex
			Vector2 curVertex = Vector2(x + center.x, y + center.y);

			// add vertex, triangle strip style (counter clockwise)
			if (i > 0)
			{
				vao.addVertex(curVertex);
				vao.addVertex(prevVertex);
				vao.addVertex(center);
			}

			// apply the rotation
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;

			// save
			prevVertex = curVertex;
		}

		// draw it
		g->drawVAO(&vao);
	}

	// draw circularmetre
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->scale(circularMetreScale, circularMetreScale);
		g->translate(center.x, center.y, 0.65f);
		g->drawImage(osu->getSkin()->getCircularmetre());
	}
	g->popTransform();
}

void OsuHUD::drawStatistics(int misses, int sliderbreaks, int maxPossibleCombo, float liveStars, float totalStars, int bpm, float curBPM, float ar, float cs, float od, float hp, int nps, int nd, int ur, float pp, float ppfc, float hitWindow300, int hitdeltaMin, int hitdeltaMax)
{
    McFont *font = osu->getTitleFont();
    const float offsetScale = Osu::getImageScale(Vector2(1.0f, 1.0f), 1.0f);
    const float scale = cv::osu::hud_statistics_scale.getFloat() * cv::osu::hud_scale.getFloat();
    const float yDelta = (font->getHeight() + 10) * cv::osu::hud_statistics_spacing_scale.getFloat();

	static constexpr Color shadowColor = rgb(0, 0, 0);
	static constexpr Color textColor = rgb(255, 255, 255);

    font->beginBatch();

    g->pushTransform();
    {
        g->scale(scale, scale);
        g->translate(cv::osu::hud_statistics_offset_x.getInt(),
                    (int)(font->getHeight() * scale) + (cv::osu::hud_statistics_offset_y.getInt() * offsetScale));

        float currentY = 0;

        auto addStatistic = [&](bool shouldDraw, const UString &text, float xOffset, float yOffset)
        {
            if (!shouldDraw || text.length() < 1)
                return;

            const Vector3 shadowPos(xOffset, currentY + yOffset, 0.25f);
            font->addToBatch(text, shadowPos, shadowColor);

            const Vector3 textPos(xOffset - 2, currentY + yOffset - 2, 0.325f);
            font->addToBatch(text, textPos, textColor);

            currentY += yDelta;
        };

		addStatistic(cv::osu::draw_statistics_pp.getBool(),
					 (cv::osu::hud_statistics_pp_decimal_places.getInt() < 1 ? UString::format("%ipp", (int)std::round(pp)) : (cv::osu::hud_statistics_pp_decimal_places.getInt() > 1 ? UString::format("%.2fpp", pp) : UString::format("%.1fpp", pp))),
					 cv::osu::hud_statistics_pp_offset_x.getInt(),
					 cv::osu::hud_statistics_pp_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_perfectpp.getBool(),
					 (cv::osu::hud_statistics_pp_decimal_places.getInt() < 1 ? UString::format("SS: %ipp", (int)std::round(ppfc)) : (cv::osu::hud_statistics_pp_decimal_places.getInt() > 1 ? UString::format("SS: %.2fpp", ppfc) : UString::format("SS: %.1fpp", ppfc))),
					 cv::osu::hud_statistics_perfectpp_offset_x.getInt(),
					 cv::osu::hud_statistics_perfectpp_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_misses.getBool(),
					 UString::format("Miss: %i", misses),
					 cv::osu::hud_statistics_misses_offset_x.getInt(),
					 cv::osu::hud_statistics_misses_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_sliderbreaks.getBool(),
					 UString::format("SBrk: %i", sliderbreaks),
					 cv::osu::hud_statistics_sliderbreaks_offset_x.getInt(),
					 cv::osu::hud_statistics_sliderbreaks_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_maxpossiblecombo.getBool(),
					 UString::format("FC: %ix", maxPossibleCombo),
					 cv::osu::hud_statistics_maxpossiblecombo_offset_x.getInt(),
					 cv::osu::hud_statistics_maxpossiblecombo_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_livestars.getBool(),
					 UString::format("%.3g***", liveStars),
					 cv::osu::hud_statistics_livestars_offset_x.getInt(),
					 cv::osu::hud_statistics_livestars_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_totalstars.getBool(),
					 UString::format("%.3g*", totalStars),
					 cv::osu::hud_statistics_totalstars_offset_x.getInt(),
					 cv::osu::hud_statistics_totalstars_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_bpm.getBool(),
					 UString::format("BPM: %i", bpm),
					 cv::osu::hud_statistics_bpm_offset_x.getInt(),
					 cv::osu::hud_statistics_bpm_offset_y.getInt());

		if constexpr (Env::cfg(AUD::SOLOUD))
		if (soundEngine->getTypeId() == SoundEngine::SOLOUD)
		addStatistic(cv::osu::draw_statistics_detected_bpm.getBool(),
					 UString::format("Cur. BPM: %i", curBPM > 0.0f ? static_cast<int>(curBPM) : bpm),
					 cv::osu::hud_statistics_detected_bpm_offset_x.getInt(),
					 cv::osu::hud_statistics_detected_bpm_offset_y.getInt());

		ar = std::round(ar * 100.0f) / 100.0f;
		addStatistic(cv::osu::draw_statistics_ar.getBool(),
					 UString::format("AR: %g", ar),
					 cv::osu::hud_statistics_ar_offset_x.getInt(),
					 cv::osu::hud_statistics_ar_offset_y.getInt());

		cs = std::round(cs * 100.0f) / 100.0f;
		addStatistic(cv::osu::draw_statistics_cs.getBool(),
					 UString::format("CS: %g", cs),
					 cv::osu::hud_statistics_cs_offset_x.getInt(),
					 cv::osu::hud_statistics_cs_offset_y.getInt());

		od = std::round(od * 100.0f) / 100.0f;
		addStatistic(cv::osu::draw_statistics_od.getBool(),
					 UString::format("OD: %g", od),
					 cv::osu::hud_statistics_od_offset_x.getInt(),
					 cv::osu::hud_statistics_od_offset_y.getInt());

		hp = std::round(hp * 100.0f) / 100.0f;
		addStatistic(cv::osu::draw_statistics_hp.getBool(),
					 UString::format("HP: %g", hp),
					 cv::osu::hud_statistics_hp_offset_x.getInt(),
					 cv::osu::hud_statistics_hp_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_hitwindow300.getBool(),
					 UString::format("300: +-%ims", (int)hitWindow300),
					 cv::osu::hud_statistics_hitwindow300_offset_x.getInt(),
					 cv::osu::hud_statistics_hitwindow300_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_nps.getBool(),
					 UString::format("NPS: %i", nps),
					 cv::osu::hud_statistics_nps_offset_x.getInt(),
					 cv::osu::hud_statistics_nps_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_nd.getBool(),
					 UString::format("ND: %i", nd),
					 cv::osu::hud_statistics_nd_offset_x.getInt(),
					 cv::osu::hud_statistics_nd_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_ur.getBool(),
					 UString::format("UR: %i", ur),
					 cv::osu::hud_statistics_ur_offset_x.getInt(),
					 cv::osu::hud_statistics_ur_offset_y.getInt());

		addStatistic(cv::osu::draw_statistics_hitdelta.getBool(),
					 UString::format("-%ims +%ims", std::abs(hitdeltaMin), hitdeltaMax),
					 cv::osu::hud_statistics_hitdelta_offset_x.getInt(),
					 cv::osu::hud_statistics_hitdelta_offset_y.getInt());
	}
	font->flushBatch();
	g->popTransform();
}

void OsuHUD::drawTargetHeatmap(float hitcircleDiameter)
{
	const Vector2 center = Vector2((int)(hitcircleDiameter/2.0f + 5.0f), (int)(hitcircleDiameter/2.0f + 5.0f));

	// constexpr const COLORPART brightnessSub = 0;
	static constexpr Color color300 = rgb(0, 255, 255);
	static constexpr Color color100 = rgb(0, 255, 0);
	static constexpr Color color50 = rgb(255, 165, 0);
	static constexpr Color colorMiss = rgb(255, 0, 0);

	OsuCircle::drawCircle(osu->getSkin(), center, hitcircleDiameter, rgb(50, 50, 50));

	const int size = hitcircleDiameter*0.075f;
	for (int i=0; i<m_targets.size(); i++)
	{
		const float delta = m_targets[i].delta;

		const float overlap = 0.15f;
		Color color;
		if (delta < cv::osu::mod_target_300_percent.getFloat()-overlap)
			color = color300;
		else if (delta < cv::osu::mod_target_300_percent.getFloat()+overlap)
		{
			const float factor300 = (cv::osu::mod_target_300_percent.getFloat() + overlap - delta) / (2.0f*overlap);
			const float factor100 = 1.0f - factor300;
			color = argb(1.0f, color300.Rf()*factor300 + color100.Rf()*factor100, color300.Gf()*factor300 + color100.Gf()*factor100, color300.Bf()*factor300 + color100.Bf()*factor100);
		}
		else if (delta < cv::osu::mod_target_100_percent.getFloat()-overlap)
			color = color100;
		else if (delta < cv::osu::mod_target_100_percent.getFloat()+overlap)
		{
			const float factor100 = (cv::osu::mod_target_100_percent.getFloat() + overlap - delta) / (2.0f*overlap);
			const float factor50 = 1.0f - factor100;
			color = argb(1.0f, color100.Rf()*factor100 + color50.Rf()*factor50, color100.Gf()*factor100 + color50.Gf()*factor50, color100.Bf()*factor100 + color50.Bf()*factor50);
		}
		else if (delta < cv::osu::mod_target_50_percent.getFloat())
			color = color50;
		else
			color = colorMiss;

		g->setColor(color);
		g->setAlpha(std::clamp<float>((m_targets[i].time - engine->getTime())/3.5f, 0.0f, 1.0f));

		const float theta = glm::radians(m_targets[i].angle);
		const float cs = std::cos(theta);
		const float sn = std::sin(theta);

		Vector2 up = Vector2(-1, 0);
		Vector2 offset;
		offset.x = up.x * cs - up.y * sn;
		offset.y = up.x * sn + up.y * cs;
		offset.normalize();
		offset *= (delta*(hitcircleDiameter/2.0f));

		//g->fillRect(center.x-size/2 - offset.x, center.y-size/2 - offset.y, size, size);

		const float imageScale = osu->getImageScaleToFitResolution(osu->getSkin()->getCircleFull(), Vector2(size, size));
		g->pushTransform();
		{
			g->scale(imageScale, imageScale);
			g->translate(center.x - offset.x, center.y - offset.y);
			g->drawImage(osu->getSkin()->getCircleFull());
		}
		g->popTransform();
	}
}

void OsuHUD::drawScrubbingTimeline(unsigned long beatmapTime, unsigned long beatmapLength, unsigned long beatmapLengthPlayable, unsigned long beatmapStartTimePlayable, float beatmapPercentFinishedPlayable, const std::vector<BREAK> &breaks)
{
	const float dpiScale = Osu::getUIScale();

	const Vector2 cursorPos = mouse->getPos();

	const Color grey = 0xffbbbbbb;
	const Color greyTransparent = 0xbbbbbbbb;
	const Color greyDark = 0xff777777;
	const Color green = 0xff00ff00;

	McFont *timeFont = osu->getSubTitleFont();

	const float breakHeight = 15 * dpiScale;

	const float currentTimeTopTextOffset = 7 * dpiScale;
	const float currentTimeLeftRightTextOffset = 5 * dpiScale;
	const float startAndEndTimeTextOffset = 5 * dpiScale + breakHeight;

	const unsigned long lengthFullMS = beatmapLength;
	const unsigned long lengthMS = beatmapLengthPlayable;
	const unsigned long startTimeMS = beatmapStartTimePlayable;
	const unsigned long endTimeMS = startTimeMS + lengthMS;
	const unsigned long currentTimeMS = beatmapTime;

	// draw strain graph
	if (cv::osu::draw_scrubbing_timeline_strain_graph.getBool() && osu->getSongBrowser()->getDynamicStarCalculator()->isAsyncReady())
	{
		// this is still WIP

		// TODO: should use strains from beatmap, not songbrowser (because songbrowser doesn't update onModUpdate() while playing)
		const std::vector<double> &aimStrains = osu->getSongBrowser()->getDynamicStarCalculator()->getAimStrains();
		const std::vector<double> &speedStrains = osu->getSongBrowser()->getDynamicStarCalculator()->getSpeedStrains();
		const float speedMultiplier = osu->getSpeedMultiplier();

		if (aimStrains.size() > 0 && aimStrains.size() == speedStrains.size())
		{
			const float strainStepMS = 400.0f * speedMultiplier;

			// get highest strain values for normalization
			double highestAimStrain = 0.0;
			double highestSpeedStrain = 0.0;
			double highestStrain = 0.0;
			int highestStrainIndex = -1;
			for (int i=0; i<aimStrains.size(); i++)
			{
				const double aimStrain = aimStrains[i];
				const double speedStrain = speedStrains[i];
				const double strain = aimStrain + speedStrain;

				if (strain > highestStrain)
				{
					highestStrain = strain;
					highestStrainIndex = i;
				}
				if (aimStrain > highestAimStrain)
					highestAimStrain = aimStrain;
				if (speedStrain > highestSpeedStrain)
					highestSpeedStrain = speedStrain;
			}

			// draw strain bar graph
			if (highestAimStrain > 0.0 && highestSpeedStrain > 0.0 && highestStrain > 0.0)
			{
				const float msPerPixel = (float)lengthMS / (float)(osu->getVirtScreenWidth() - (((float)startTimeMS / (float)endTimeMS))*osu->getVirtScreenWidth());
				const float strainWidth = strainStepMS / msPerPixel;
				const float strainHeightMultiplier = cv::osu::hud_scrubbing_timeline_strains_height.getFloat() * dpiScale;

				const float offsetX = ((float)startTimeMS / (float)strainStepMS) * strainWidth; // compensate for startTimeMS

				const float alpha = cv::osu::hud_scrubbing_timeline_strains_alpha.getFloat();

				const Color aimStrainColor = argb(alpha, cv::osu::hud_scrubbing_timeline_strains_aim_color_r.getInt() / 255.0f, cv::osu::hud_scrubbing_timeline_strains_aim_color_g.getInt() / 255.0f, cv::osu::hud_scrubbing_timeline_strains_aim_color_b.getInt() / 255.0f);
				const Color speedStrainColor = argb(alpha, cv::osu::hud_scrubbing_timeline_strains_speed_color_r.getInt() / 255.0f, cv::osu::hud_scrubbing_timeline_strains_speed_color_g.getInt() / 255.0f, cv::osu::hud_scrubbing_timeline_strains_speed_color_b.getInt() / 255.0f);

				g->setDepthBuffer(true);
				for (int i=0; i<aimStrains.size(); i++)
				{
					const double aimStrain = (aimStrains[i]) / highestStrain;
					const double speedStrain = (speedStrains[i]) / highestStrain;
					//const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

					const double aimStrainHeight = aimStrain * strainHeightMultiplier;
					const double speedStrainHeight = speedStrain * strainHeightMultiplier;
					//const double strainHeight = strain * strainHeightMultiplier;

					g->setColor(aimStrainColor);
					g->fillRect(i*strainWidth + offsetX, cursorPos.y - aimStrainHeight, std::max(1.0f, std::round(strainWidth + 0.5f)), aimStrainHeight);

					g->setColor(speedStrainColor);
					g->fillRect(i*strainWidth + offsetX, cursorPos.y - aimStrainHeight - speedStrainHeight, std::max(1.0f, std::round(strainWidth + 0.5f)), speedStrainHeight + 1);
				}
				g->setDepthBuffer(false);

				// highlight highest total strain value (+- section block)
				if (highestStrainIndex > -1)
				{
					const double aimStrain = (aimStrains[highestStrainIndex]) / highestStrain;
					const double speedStrain = (speedStrains[highestStrainIndex]) / highestStrain;
					//const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

					const double aimStrainHeight = aimStrain * strainHeightMultiplier;
					const double speedStrainHeight = speedStrain * strainHeightMultiplier;
					//const double strainHeight = strain * strainHeightMultiplier;

					Vector2 topLeftCenter = Vector2(highestStrainIndex*strainWidth + offsetX + strainWidth/2.0f, cursorPos.y - aimStrainHeight - speedStrainHeight);

					const float margin = 5.0f * dpiScale;

					g->setColor(0xffffffff);
					g->setAlpha(alpha);
					g->drawRect(topLeftCenter.x - margin*strainWidth, topLeftCenter.y - margin*strainWidth, strainWidth*2*margin, aimStrainHeight + speedStrainHeight + 2*margin*strainWidth);
				}
			}
		}
	}

	// breaks
	g->setColor(greyTransparent);
	for (int i=0; i<breaks.size(); i++)
	{
		const int width = std::max((int)(osu->getVirtScreenWidth() * std::clamp<float>(breaks[i].endPercent - breaks[i].startPercent, 0.0f, 1.0f)), 2);
		g->fillRect(osu->getVirtScreenWidth() * breaks[i].startPercent, cursorPos.y + 1, width, breakHeight);
	}

	// line
	g->setColor(0xff000000);
	g->drawLine(0, cursorPos.y + 1, osu->getVirtScreenWidth(), cursorPos.y + 1);
	g->setColor(grey);
	g->drawLine(0, cursorPos.y, osu->getVirtScreenWidth(), cursorPos.y);

	// current time triangle
	Vector2 triangleTip = Vector2(osu->getVirtScreenWidth()*beatmapPercentFinishedPlayable, cursorPos.y);
	g->pushTransform();
	{
		g->translate(triangleTip.x + 1, triangleTip.y - osu->getSkin()->getSeekTriangle()->getHeight()/2.0f + 1);
		g->setColor(0xff000000);
		g->drawImage(osu->getSkin()->getSeekTriangle());
		g->translate(-1, -1);
		g->setColor(green);
		g->drawImage(osu->getSkin()->getSeekTriangle());
	}
	g->popTransform();

	// current time text
	UString currentTimeText = UString::format("%i:%02i", (currentTimeMS/1000) / 60, (currentTimeMS/1000) % 60);
	g->pushTransform();
	{
		g->translate(std::clamp<float>(triangleTip.x - timeFont->getStringWidth(currentTimeText)/2.0f, currentTimeLeftRightTextOffset, osu->getVirtScreenWidth() - timeFont->getStringWidth(currentTimeText) - currentTimeLeftRightTextOffset) + 1, triangleTip.y - osu->getSkin()->getSeekTriangle()->getHeight() - currentTimeTopTextOffset + 1);
		g->setColor(0xff000000);
		g->drawString(timeFont, currentTimeText);
		g->translate(-1, -1);
		g->setColor(green);
		g->drawString(timeFont, currentTimeText);
	}
	g->popTransform();

	// start time text
	UString startTimeText = UString::format("(%i:%02i)", (startTimeMS/1000) / 60, (startTimeMS/1000) % 60);
	g->pushTransform();
	{
		g->translate((int)(startAndEndTimeTextOffset + 1), (int)(triangleTip.y + startAndEndTimeTextOffset + timeFont->getHeight() + 1));
		g->setColor(0xff000000);
		g->drawString(timeFont, startTimeText);
		g->translate(-1, -1);
		g->setColor(greyDark);
		g->drawString(timeFont, startTimeText);
	}
	g->popTransform();

	// end time text
	UString endTimeText = UString::format("%i:%02i", (endTimeMS/1000) / 60, (endTimeMS/1000) % 60);
	g->pushTransform();
	{
		g->translate((int)(osu->getVirtScreenWidth() - timeFont->getStringWidth(endTimeText) - startAndEndTimeTextOffset + 1), (int)(triangleTip.y + startAndEndTimeTextOffset + timeFont->getHeight() + 1));
		g->setColor(0xff000000);
		g->drawString(timeFont, endTimeText);
		g->translate(-1, -1);
		g->setColor(greyDark);
		g->drawString(timeFont, endTimeText);
	}
	g->popTransform();

	// quicksave time triangle & text
	if (osu->getQuickSaveTime() != 0.0f)
	{
		const float quickSaveTimeToPlayablePercent = std::clamp<float>(((lengthFullMS*osu->getQuickSaveTime())) / (float)endTimeMS, 0.0f, 1.0f);
		triangleTip = Vector2(osu->getVirtScreenWidth()*quickSaveTimeToPlayablePercent, cursorPos.y);
		g->pushTransform();
		{
			g->rotate(180);
			g->translate(triangleTip.x + 1, triangleTip.y + osu->getSkin()->getSeekTriangle()->getHeight()/2.0f + 1);
			g->setColor(0xff000000);
			g->drawImage(osu->getSkin()->getSeekTriangle());
			g->translate(-1, -1);
			g->setColor(grey);
			g->drawImage(osu->getSkin()->getSeekTriangle());
		}
		g->popTransform();

		// end time text
		const unsigned long quickSaveTimeMS = lengthFullMS*osu->getQuickSaveTime();
		UString endTimeText = UString::format("%i:%02i", (quickSaveTimeMS/1000) / 60, (quickSaveTimeMS/1000) % 60);
		g->pushTransform();
		{
			g->translate((int)(std::clamp<float>(triangleTip.x - timeFont->getStringWidth(currentTimeText)/2.0f, currentTimeLeftRightTextOffset, osu->getVirtScreenWidth() - timeFont->getStringWidth(currentTimeText) - currentTimeLeftRightTextOffset) + 1), (int)(triangleTip.y + startAndEndTimeTextOffset + timeFont->getHeight()*2.2f + 1 + currentTimeTopTextOffset*std::max(1.0f, getCursorScaleFactor()*cv::osu::cursor_scale.getFloat())*cv::osu::hud_scrubbing_timeline_hover_tooltip_offset_multiplier.getFloat()));
			g->setColor(0xff000000);
			g->drawString(timeFont, endTimeText);
			g->translate(-1, -1);
			g->setColor(grey);
			g->drawString(timeFont, endTimeText);
		}
		g->popTransform();
	}

	// current time hover text
	const unsigned long hoverTimeMS = std::clamp<float>((cursorPos.x / (float)osu->getVirtScreenWidth()), 0.0f, 1.0f) * endTimeMS;
	UString hoverTimeText = UString::format("%i:%02i", (hoverTimeMS/1000) / 60, (hoverTimeMS/1000) % 60);
	triangleTip = Vector2(cursorPos.x, cursorPos.y);
	g->pushTransform();
	{
		g->translate((int)std::clamp<float>(triangleTip.x - timeFont->getStringWidth(currentTimeText)/2.0f, currentTimeLeftRightTextOffset, osu->getVirtScreenWidth() - timeFont->getStringWidth(currentTimeText) - currentTimeLeftRightTextOffset) + 1, (int)(triangleTip.y - osu->getSkin()->getSeekTriangle()->getHeight() - timeFont->getHeight()*1.2f - currentTimeTopTextOffset*std::max(1.0f, getCursorScaleFactor()*cv::osu::cursor_scale.getFloat())*cv::osu::hud_scrubbing_timeline_hover_tooltip_offset_multiplier.getFloat()*2.0f - 1));
		g->setColor(0xff000000);
		g->drawString(timeFont, hoverTimeText);
		g->translate(-1, -1);
		g->setColor(0xff666666);
		g->drawString(timeFont, hoverTimeText);
	}
	g->popTransform();
}

void OsuHUD::drawInputOverlay(int numK1, int numK2, int numM1, int numM2)
{
	OsuSkinImage *inputoverlayBackground = osu->getSkin()->getInputoverlayBackground();
	OsuSkinImage *inputoverlayKey = osu->getSkin()->getInputoverlayKey();

	const float scale = cv::osu::hud_scale.getFloat() * cv::osu::hud_inputoverlay_scale.getFloat();	// global scaler
	const float oScale = inputoverlayBackground->getResolutionScale() * 1.6f;				// for converting harcoded osu offset pixels to screen pixels
	const float offsetScale = Osu::getImageScale(Vector2(1.0f, 1.0f), 1.0f);			// for scaling the x/y offset convars relative to screen size

	const float xStartOffset = cv::osu::hud_inputoverlay_offset_x.getFloat()*offsetScale;
	const float yStartOffset = cv::osu::hud_inputoverlay_offset_y.getFloat()*offsetScale;

	const float xStart = osu->getVirtScreenWidth() - xStartOffset;
	const float yStart = osu->getVirtScreenHeight()/2 - (40.0f*oScale)*scale + yStartOffset;

	// background
	{
		const float xScale = 1.05f + 0.001f;
		const float rot = 90.0f;

		const float xOffset = (inputoverlayBackground->getSize().y / 2);
		const float yOffset = (inputoverlayBackground->getSize().x / 2 ) * xScale;

		g->setColor(0xffffffff);
		g->pushTransform();
		{
			g->scale(xScale, 1.0f);
			g->rotate(rot);
			inputoverlayBackground->draw(Vector2(xStart - xOffset*scale + 1, yStart + yOffset*scale), scale);
		}
		g->popTransform();
	}

	// keys
	{
		const float textFontHeightPercent = 0.3f;
		const Color colorIdle = rgb(255, 255, 255);
		const Color colorKeyboard = rgb(255, 222, 0);
		const Color colorMouse = rgb(248, 0, 158);

		McFont *textFont = osu->getSongBrowserFont();
		McFont *textFontBold = osu->getSongBrowserFontBold();

		for (int i=0; i<4; i++)
		{
			textFont = osu->getSongBrowserFont(); // reset

			UString text;
			Color color = colorIdle;
			float animScale = 1.0f;
			float animColor = 0.0f;
			switch (i)
			{
			case 0:
				text = numK1 > 0 ? UString::format("%i", numK1) : UString("K1");
				color = colorKeyboard;
				animScale = m_fInputoverlayK1AnimScale;
				animColor = m_fInputoverlayK1AnimColor;
				if (numK1 > 0)
					textFont = textFontBold;
				break;
			case 1:
				text = numK2 > 0 ? UString::format("%i", numK2) : UString("K2");
				color = colorKeyboard;
				animScale = m_fInputoverlayK2AnimScale;
				animColor = m_fInputoverlayK2AnimColor;
				if (numK2 > 0)
					textFont = textFontBold;
				break;
			case 2:
				text = numM1 > 0 ? UString::format("%i", numM1) : UString("M1");
				color = colorMouse;
				animScale = m_fInputoverlayM1AnimScale;
				animColor = m_fInputoverlayM1AnimColor;
				if (numM1 > 0)
					textFont = textFontBold;
				break;
			case 3:
				text = numM2 > 0 ? UString::format("%i", numM2) : UString("M2");
				color = colorMouse;
				animScale = m_fInputoverlayM2AnimScale;
				animColor = m_fInputoverlayM2AnimColor;
				if (numM2 > 0)
					textFont = textFontBold;
				break;
			}

			// key
			const Vector2 pos = Vector2(xStart - (15.0f*oScale)*scale + 1, yStart + (19.0f*oScale + i*29.5f*oScale)*scale);
			g->setColor(argb(1.0f,
					(1.0f - animColor)*colorIdle.Rf() + animColor*color.Rf(),
					(1.0f - animColor)*colorIdle.Gf() + animColor*color.Gf(),
					(1.0f - animColor)*colorIdle.Bf() + animColor*color.Bf()));
			inputoverlayKey->draw(pos, scale*animScale);

			// text
			const float keyFontScale = (inputoverlayKey->getSizeBase().y * textFontHeightPercent) / textFont->getHeight();
			const float stringWidth = textFont->getStringWidth(text) * keyFontScale;
			const float stringHeight = textFont->getHeight() * keyFontScale;
			g->setColor(osu->getSkin()->getInputOverlayText());
			g->pushTransform();
			{
				g->scale(keyFontScale*scale*animScale, keyFontScale*scale*animScale);
				g->translate(pos.x - (stringWidth/2.0f)*scale*animScale, pos.y + (stringHeight/2.0f)*scale*animScale);
				g->drawString(textFont, text);
			}
			g->popTransform();
		}
	}
}

float OsuHUD::getCursorScaleFactor()
{
	// FUCK OSU hardcoded piece of shit code
	const float spriteRes = 768.0f;

	float mapScale = 1.0f;
	if (cv::osu::automatic_cursor_size.getBool() && osu->isInPlayMode())
		mapScale = 1.0f - 0.7f * (float)(osu->getSelectedBeatmap()->getCS() - 4.0f) / 5.0f;

	return ((float)osu->getVirtScreenHeight() / spriteRes) * mapScale;
}

float OsuHUD::getCursorTrailScaleFactor()
{
	return getCursorScaleFactor() * (osu->getSkin()->isCursorTrail2x() ? 0.5f : 1.0f);
}

float OsuHUD::getScoreScale()
{
	return osu->getImageScale(osu->getSkin()->getScore0(), 13*1.5f) * cv::osu::hud_scale.getFloat() * cv::osu::hud_score_scale.getFloat();
}

void OsuHUD::onVolumeOverlaySizeChange(const UString & /*oldValue*/, const UString & /*newValue*/)
{
	updateLayout();
}

void OsuHUD::animateCombo()
{
	m_fComboAnim1 = 0.0f;
	m_fComboAnim2 = 1.0f;

	anim->moveLinear(&m_fComboAnim1, 2.0f, cv::osu::combo_anim1_duration.getFloat(), true);
	anim->moveQuadOut(&m_fComboAnim2, 0.0f, cv::osu::combo_anim2_duration.getFloat(), 0.0f, true);
}

void OsuHUD::addHitError(long delta, bool miss, bool misaim)
{
	// add entry
	{
		HITERROR h;

		h.delta = delta;
		h.time = engine->getTime() + (miss || misaim ? cv::osu::hud_hiterrorbar_entry_miss_fade_time.getFloat() : cv::osu::hud_hiterrorbar_entry_hit_fade_time.getFloat());
		h.miss = miss;
		h.misaim = misaim;

		m_hiterrors.push_back(h);
	}

	// remove old
	for (int i=0; i<m_hiterrors.size(); i++)
	{
		if (engine->getTime() > m_hiterrors[i].time)
		{
			m_hiterrors.erase(m_hiterrors.begin() + i);
			i--;
		}
	}

	if (m_hiterrors.size() > cv::osu::hud_hiterrorbar_max_entries.getInt())
		m_hiterrors.erase(m_hiterrors.begin());
}

void OsuHUD::addTarget(float delta, float angle)
{
	TARGET t;
	t.time = engine->getTime() + 3.5f;
	t.delta = delta;
	t.angle = angle;

	m_targets.push_back(t);
}

void OsuHUD::animateInputoverlay(int key, bool down)
{
	if (!cv::osu::draw_inputoverlay.getBool() || (!cv::osu::draw_hud.getBool() && cv::osu::hud_shift_tab_toggles_everything.getBool())) return;

	float *animScale = &m_fInputoverlayK1AnimScale;
	float *animColor = &m_fInputoverlayK1AnimColor;

	switch (key)
	{
	case 1:
		animScale = &m_fInputoverlayK1AnimScale;
		animColor = &m_fInputoverlayK1AnimColor;
		break;
	case 2:
		animScale = &m_fInputoverlayK2AnimScale;
		animColor = &m_fInputoverlayK2AnimColor;
		break;
	case 3:
		animScale = &m_fInputoverlayM1AnimScale;
		animColor = &m_fInputoverlayM1AnimColor;
		break;
	case 4:
		animScale = &m_fInputoverlayM2AnimScale;
		animColor = &m_fInputoverlayM2AnimColor;
		break;
	}

	if (down)
	{
		// scale
		*animScale = 1.0f;
		anim->moveQuadOut(animScale, cv::osu::hud_inputoverlay_anim_scale_multiplier.getFloat(), cv::osu::hud_inputoverlay_anim_scale_duration.getFloat(), true);

		// color
		*animColor = 1.0f;
		anim->deleteExistingAnimation(animColor);
	}
	else
	{
		// scale
		// NOTE: osu is running the keyup anim in parallel, but only allowing it to override once the keydown anim has finished, and with some weird speedup?
		const float remainingDuration = anim->getRemainingDuration(animScale);
		anim->moveQuadOut(animScale, 1.0f, cv::osu::hud_inputoverlay_anim_scale_duration.getFloat() - std::min(remainingDuration*1.4f, cv::osu::hud_inputoverlay_anim_scale_duration.getFloat()), remainingDuration);

		// color
		anim->moveLinear(animColor, 0.0f, cv::osu::hud_inputoverlay_anim_color_duration.getFloat(), true);
	}
}

void OsuHUD::animateVolumeChange()
{
	const bool active = m_fVolumeChangeTime > engine->getTime();

	m_fVolumeChangeTime = engine->getTime() + cv::osu::hud_volume_duration.getFloat() + 0.2f;

	if (!active)
	{
		m_fVolumeChangeFade = 0.0f;
		anim->moveQuadOut(&m_fVolumeChangeFade, 1.0f, 0.15f, true);
	}
	else
		anim->moveQuadOut(&m_fVolumeChangeFade, 1.0f, 0.1f * (1.0f - m_fVolumeChangeFade), true);

	anim->moveQuadOut(&m_fVolumeChangeFade, 0.0f, 0.25f, cv::osu::hud_volume_duration.getFloat(), false);
	anim->moveQuadOut(&m_fLastVolume, cv::osu::volume_master.getFloat(), 0.15f, 0.0f, true);
}

void OsuHUD::addCursorRipple(Vector2 pos)
{
	if (!cv::osu::draw_cursor_ripples.getBool()) return;

	CURSORRIPPLE ripple;
	ripple.pos = pos;
	ripple.time = engine->getTime() + cv::osu::cursor_ripple_duration.getFloat();

	m_cursorRipples.push_back(ripple);
}

void OsuHUD::animateCursorExpand()
{
	m_fCursorExpandAnim = 1.0f;
	anim->moveQuadOut(&m_fCursorExpandAnim, cv::osu::cursor_expand_scale_multiplier.getFloat(), cv::osu::cursor_expand_duration.getFloat(), 0.0f, true);
}

void OsuHUD::animateCursorShrink()
{
	anim->moveQuadOut(&m_fCursorExpandAnim, 1.0f, cv::osu::cursor_expand_duration.getFloat(), 0.0f, true);
}

void OsuHUD::animateKiBulge()
{
	m_fKiScaleAnim = 1.2f;
	anim->moveLinear(&m_fKiScaleAnim, 0.8f, 0.150f, true);
}

void OsuHUD::animateKiExplode()
{
	// TODO: scale + fadeout of extra ki image additive, duration = 0.120, quad out:
	// if additive: fade from 0.5 alpha to 0, scale from 1.0 to 2.0
	// if not additive: fade from 1.0 alpha to 0, scale from 1.0 to 1.6
}

void OsuHUD::addCursorTrailPosition(std::vector<CURSORTRAIL> &trail, Vector2 pos, bool empty)
{
	if (empty) return;
	if (pos.x < -osu->getVirtScreenWidth() || pos.x > osu->getVirtScreenWidth()*2 || pos.y < -osu->getVirtScreenHeight() || pos.y > osu->getVirtScreenHeight()*2) return; // fuck oob trails

	Image *trailImage = osu->getSkin()->getCursorTrail();

	const bool smoothCursorTrail = osu->getSkin()->useSmoothCursorTrail() || cv::osu::cursor_trail_smooth_force.getBool();

	const float scaleAnim = (osu->getSkin()->getCursorExpand() && cv::osu::cursor_trail_expand.getBool() ? m_fCursorExpandAnim : 1.0f) * cv::osu::cursor_trail_scale.getFloat();
	const float trailWidth = trailImage->getWidth() * getCursorTrailScaleFactor() * scaleAnim * cv::osu::cursor_scale.getFloat();

	CURSORTRAIL ct;
	ct.pos = pos;
	ct.time = engine->getTime() + (smoothCursorTrail ? cv::osu::cursor_trail_smooth_length.getFloat() : cv::osu::cursor_trail_length.getFloat());
	ct.alpha = 1.0f;
	ct.scale = scaleAnim;

	if (smoothCursorTrail)
	{
		// interpolate mid points between the last point and the current point
		if (trail.size() > 0)
		{
			const Vector2 prevPos = trail[trail.size()-1].pos;
			const float prevTime = trail[trail.size()-1].time;
			const float prevScale = trail[trail.size()-1].scale;

			Vector2 delta = pos - prevPos;
			const int numMidPoints = (int)(delta.length() / (trailWidth/cv::osu::cursor_trail_smooth_div.getFloat()));
			if (numMidPoints > 0)
			{
				const Vector2 step = delta.normalize() * (trailWidth/cv::osu::cursor_trail_smooth_div.getFloat());
				const float timeStep = (ct.time - prevTime) / (float)(numMidPoints);
				const float scaleStep = (ct.scale - prevScale) / (float)(numMidPoints);
				for (int i=std::clamp<int>(numMidPoints-cv::osu::cursor_trail_max_size.getInt()/2, 0, cv::osu::cursor_trail_max_size.getInt()); i<numMidPoints; i++) // limit to half the maximum new mid points per frame
				{
					CURSORTRAIL mid;
					mid.pos = prevPos + step*(i+1.0f);
					mid.time = prevTime + timeStep*(i+1.0f);
					mid.alpha = 1.0f;
					mid.scale = prevScale + scaleStep*(i+1.0f);
					trail.push_back(mid);
				}
			}
		}
		else
			trail.push_back(ct);
	}
	else if ((trail.size() > 0 && engine->getTime() > trail[trail.size()-1].time-cv::osu::cursor_trail_length.getFloat()+cv::osu::cursor_trail_spacing.getFloat()) || trail.size() == 0)
	{
		if (trail.size() > 0 && trail[trail.size()-1].pos == pos)
		{
			trail[trail.size()-1].time = ct.time;
			trail[trail.size()-1].alpha = 1.0f;
			trail[trail.size()-1].scale = ct.scale;
		}
		else
			trail.push_back(ct);
	}

	// early cleanup
	while (trail.size() > cv::osu::cursor_trail_max_size.getInt())
	{
		trail.erase(trail.begin());
	}
}

void OsuHUD::selectVolumePrev()
{
	const std::vector<CBaseUIElement*> &elements = m_volumeSliderOverlayContainer->getElements();
	for (int i=0; i<elements.size(); i++)
	{
		if (((OsuUIVolumeSlider*)elements[i])->isSelected())
		{
			const int prevIndex = (i == 0 ? elements.size()-1 : i-1);
			((OsuUIVolumeSlider*)elements[i])->setSelected(false);
			((OsuUIVolumeSlider*)elements[prevIndex])->setSelected(true);
			break;
		}
	}
	animateVolumeChange();
}

void OsuHUD::selectVolumeNext()
{
	const std::vector<CBaseUIElement*> &elements = m_volumeSliderOverlayContainer->getElements();
	for (int i=0; i<elements.size(); i++)
	{
		if (((OsuUIVolumeSlider*)elements[i])->isSelected())
		{
			const int nextIndex = (i == elements.size()-1 ? 0 : i+1);
			((OsuUIVolumeSlider*)elements[i])->setSelected(false);
			((OsuUIVolumeSlider*)elements[nextIndex])->setSelected(true);
			break;
		}
	}
	animateVolumeChange();
}

void OsuHUD::resetHitErrorBar()
{
	m_hiterrors.clear();
}

McRect OsuHUD::getSkipClickRect()
{
	const float skipScale = cv::osu::hud_scale.getFloat();
	return McRect(osu->getVirtScreenWidth() - osu->getSkin()->getPlaySkip()->getSize().x*skipScale, osu->getVirtScreenHeight() - osu->getSkin()->getPlaySkip()->getSize().y*skipScale, osu->getSkin()->getPlaySkip()->getSize().x*skipScale, osu->getSkin()->getPlaySkip()->getSize().y*skipScale);
}

bool OsuHUD::isVolumeOverlayVisible()
{
	return engine->getTime() < m_fVolumeChangeTime;
}

bool OsuHUD::isVolumeOverlayBusy()
{
	return (m_volumeMaster->isEnabled() && (m_volumeMaster->isBusy() || m_volumeMaster->isMouseInside()))
		|| (m_volumeEffects->isEnabled() && (m_volumeEffects->isBusy() || m_volumeEffects->isMouseInside()))
		|| (m_volumeMusic->isEnabled() && (m_volumeMusic->isBusy() || m_volumeMusic->isMouseInside()));
}
