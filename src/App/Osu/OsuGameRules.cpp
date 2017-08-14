//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		difficulty & playfield behaviour
//
// $NoKeywords: $osugr
//===============================================================================//

#include "OsuGameRules.h"

ConVar OsuGameRules::osu_playfield_border_top_percent("osu_playfield_border_top_percent", 0.117f);
ConVar OsuGameRules::osu_playfield_border_bottom_percent("osu_playfield_border_bottom_percent", 0.0834f);

ConVar OsuGameRules::osu_hitobject_fade_out_time("osu_hitobject_fade_out_time", 0.293f);
ConVar OsuGameRules::osu_hitobject_fade_out_time_speed_multiplier_min("osu_hitobject_fade_out_time_speed_multiplier_min", 0.5f, "The minimum multiplication factor allowed for the speed multiplier influencing the fadeout duration");

ConVar OsuGameRules::osu_circle_fade_out_scale("osu_circle_fade_out_scale", 0.4f);

ConVar OsuGameRules::osu_slider_followcircle_fadein_fade_time("osu_slider_followcircle_fadein_fade_time", 0.06f);
ConVar OsuGameRules::osu_slider_followcircle_fadeout_fade_time("osu_slider_followcircle_fadeout_fade_time", 0.25f);
ConVar OsuGameRules::osu_slider_followcircle_fadein_scale("osu_slider_followcircle_fadein_scale", 0.5f);
ConVar OsuGameRules::osu_slider_followcircle_fadein_scale_time("osu_slider_followcircle_fadein_scale_time", 0.18f);
ConVar OsuGameRules::osu_slider_followcircle_fadeout_scale("osu_slider_followcircle_fadeout_scale", 0.8f);
ConVar OsuGameRules::osu_slider_followcircle_fadeout_scale_time("osu_slider_followcircle_fadeout_scale_time", 0.25f);
ConVar OsuGameRules::osu_slider_followcircle_tick_pulse_time("osu_slider_followcircle_tick_pulse_time", 0.2f);
ConVar OsuGameRules::osu_slider_followcircle_tick_pulse_scale("osu_slider_followcircle_tick_pulse_scale", 0.1f);

ConVar OsuGameRules::osu_mod_no50s("osu_mod_no50s", false);
ConVar OsuGameRules::osu_mod_no100s("osu_mod_no100s", false);
ConVar OsuGameRules::osu_mod_ming3012("osu_mod_ming3012", false);
ConVar OsuGameRules::osu_mod_millhioref("osu_mod_millhioref", false);
ConVar OsuGameRules::osu_mod_millhioref_multiplier("osu_mod_millhioref_multiplier", 2.0f);

// all hitwindow values here are in milliseconds
ConVar OsuGameRules::osu_hitwindow_300_min("osu_hitwindow_300_min", 80);
ConVar OsuGameRules::osu_hitwindow_300_mid("osu_hitwindow_300_mid", 50);
ConVar OsuGameRules::osu_hitwindow_300_max("osu_hitwindow_300_max", 20);

ConVar OsuGameRules::osu_hitwindow_100_min("osu_hitwindow_100_min", 140);
ConVar OsuGameRules::osu_hitwindow_100_mid("osu_hitwindow_100_mid", 100);
ConVar OsuGameRules::osu_hitwindow_100_max("osu_hitwindow_100_max", 60);

ConVar OsuGameRules::osu_hitwindow_50_min("osu_hitwindow_50_min", 200);
ConVar OsuGameRules::osu_hitwindow_50_mid("osu_hitwindow_50_mid", 150);
ConVar OsuGameRules::osu_hitwindow_50_max("osu_hitwindow_50_max", 100);

ConVar OsuGameRules::osu_hitwindow_miss("osu_hitwindow_miss", 400);
