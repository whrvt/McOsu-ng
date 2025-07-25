//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		osu-specific console variable registry
//
// $NoKeywords: $convar $osucv
//===============================================================================//

#pragma once
#ifndef OSUCVDEFS_H
#define OSUCVDEFS_H

class ConVar;

namespace cv::osu {

// from Osu.cpp
extern ConVar alt_f4_quits_even_while_playing;
extern ConVar confine_cursor_fullscreen;
extern ConVar confine_cursor_never;
extern ConVar confine_cursor_windowed;
extern ConVar debug;
extern ConVar disable_mousebuttons;
extern ConVar disable_mousewheel;
extern ConVar disable_windows_key_while_playing;
extern ConVar draw_fps;
extern ConVar force_legacy_slider_renderer;
extern ConVar hide_cursor_during_gameplay;
extern ConVar letterboxing;
extern ConVar letterboxing_offset_x;
extern ConVar letterboxing_offset_y;
extern ConVar mod_endless;
extern ConVar mod_fadingcursor;
extern ConVar mod_fadingcursor_combo;
extern ConVar mod_touchdevice;
extern ConVar mods;
extern ConVar notification;
extern ConVar notification_color_b;
extern ConVar notification_color_g;
extern ConVar notification_color_r;
extern ConVar pause_on_focus_loss;
extern ConVar pitch_override;
extern ConVar quick_retry_delay;
extern ConVar release_stream;
extern ConVar resolution;
extern ConVar resolution_enabled;
extern ConVar resolution_keep_aspect_ratio;
extern ConVar scrubbing_smooth;
extern ConVar seek_delta;
extern ConVar skin;
extern ConVar skin_is_from_workshop;
extern ConVar skin_reload;
extern ConVar skin_workshop_id;
extern ConVar skin_workshop_title;
extern ConVar skip_breaks_enabled;
extern ConVar skip_intro_enabled;
extern ConVar speed_override;
extern ConVar ui_scale;
extern ConVar ui_scale_to_dpi;
extern ConVar ui_scale_to_dpi_minimum_height;
extern ConVar ui_scale_to_dpi_minimum_width;
extern ConVar version;
extern ConVar volume_change_interval;
extern ConVar volume_master;
extern ConVar volume_master_inactive;
extern ConVar volume_music;

// from OsuBackgroundImageHandler.cpp
extern ConVar background_image_cache_size;
extern ConVar background_image_eviction_delay_frames;
extern ConVar background_image_eviction_delay_seconds;
extern ConVar background_image_loading_delay;
extern ConVar load_beatmap_background_images;

// from OsuBackgroundStarCacheLoader.cpp
extern ConVar debug_background_star_cache_loader;

// from OsuBeatmap.cpp
extern ConVar ar_override;
extern ConVar ar_override_lock;
extern ConVar ar_overridenegative;
extern ConVar auto_and_relax_block_user_input;
extern ConVar background_alpha;
extern ConVar background_brightness;
extern ConVar background_color_b;
extern ConVar background_color_g;
extern ConVar background_color_r;
extern ConVar background_dim;
extern ConVar background_dont_fade_during_breaks;
extern ConVar background_fade_after_load;
extern ConVar background_fade_in_duration;
extern ConVar background_fade_min_duration;
extern ConVar background_fade_out_duration;
extern ConVar beatmap_preview_mods_live;
extern ConVar beatmap_preview_music_loop;
extern ConVar combobreak_sound_combo;
extern ConVar compensate_music_speed;
extern ConVar cs_cap_sanity;
extern ConVar cs_override;
extern ConVar cs_overridenegative;
extern ConVar debug_draw_timingpoints;
extern ConVar drain_kill;
extern ConVar drain_kill_notification_duration;
extern ConVar drain_lazer_break_after;
extern ConVar drain_lazer_break_before;
extern ConVar drain_lazer_passive_fail;
extern ConVar drain_stable_break_after;
extern ConVar drain_stable_break_before;
extern ConVar drain_stable_break_before_old;
extern ConVar drain_stable_hpbar_recovery;
extern ConVar drain_stable_passive_fail;
extern ConVar drain_stable_spinner_nerf;
extern ConVar drain_type;
extern ConVar draw_beatmap_background_image;
extern ConVar draw_hitobjects;
extern ConVar early_note_time;
extern ConVar end_delay_time;
extern ConVar end_skip;
extern ConVar end_skip_time;
extern ConVar fail_time;
extern ConVar followpoints_prevfadetime;
extern ConVar hiterrorbar_misaims;
extern ConVar hp_override;
extern ConVar interpolate_music_pos;
extern ConVar mod_artimewarp;
extern ConVar mod_artimewarp_multiplier;
extern ConVar mod_arwobble;
extern ConVar mod_arwobble_interval;
extern ConVar mod_arwobble_strength;
extern ConVar mod_fullalternate;
extern ConVar mod_jigsaw1;
extern ConVar mod_minimize;
extern ConVar mod_minimize_multiplier;
extern ConVar mod_suddendeath_restart;
extern ConVar mod_timewarp;
extern ConVar mod_timewarp_multiplier;
extern ConVar notelock_stable_tolerance2b;
extern ConVar notelock_type;
extern ConVar od_override;
extern ConVar od_override_lock;
extern ConVar old_beatmap_offset;
extern ConVar play_hitsound_on_click_while_playing;
extern ConVar pvs;
extern ConVar quick_retry_time;
extern ConVar skip_time;
extern ConVar timingpoints_offset;
extern ConVar universal_offset;
extern ConVar universal_offset_hardcoded;
extern ConVar universal_offset_hardcoded_fallback_dsound;
extern ConVar unpause_continue_delay;

// from OsuBeatmapMania.cpp
extern ConVar mania_k_override;
extern ConVar mania_playfield_height_percent;
extern ConVar mania_playfield_offset_x_percent;
extern ConVar mania_playfield_width_percent;

// from OsuBeatmapStandard.cpp
extern ConVar auto_cursordance;
extern ConVar auto_snapping_strength;
extern ConVar autopilot_lenience;
extern ConVar autopilot_snapping_strength;
extern ConVar debug_hiterrorbar_misaims;
extern ConVar drain_lazer_health_max;
extern ConVar drain_lazer_health_mid;
extern ConVar drain_lazer_health_min;
extern ConVar draw_followpoints;
extern ConVar draw_playfield_border;
extern ConVar draw_reverse_order;
extern ConVar followpoints_anim;
extern ConVar followpoints_approachtime;
extern ConVar followpoints_clamp;
extern ConVar followpoints_connect_combos;
extern ConVar followpoints_connect_spinners;
extern ConVar followpoints_scale_multiplier;
extern ConVar followpoints_separation_multiplier;
extern ConVar mandala;
extern ConVar mandala_num;
extern ConVar mod_jigsaw2;
extern ConVar mod_jigsaw_followcircle_radius_factor;
extern ConVar mod_mafham_render_chunksize;
extern ConVar mod_shirone;
extern ConVar mod_shirone_combo;
extern ConVar mod_wobble;
extern ConVar mod_wobble2;
extern ConVar mod_wobble_frequency;
extern ConVar mod_wobble_rotation_speed;
extern ConVar mod_wobble_strength;
extern ConVar number_scale_multiplier;
extern ConVar playfield_circular;
extern ConVar playfield_mirror_horizontal;
extern ConVar playfield_mirror_vertical;
extern ConVar playfield_rotation;
extern ConVar playfield_stretch_x;
extern ConVar playfield_stretch_y;
extern ConVar pp_live_timeout;
extern ConVar stacking;
extern ConVar stacking_leniency_override;

// from OsuCircle.cpp
extern ConVar approach_circle_alpha_multiplier;
extern ConVar bug_flicker_log;
extern ConVar circle_color_saturation;
extern ConVar circle_number_rainbow;
extern ConVar circle_rainbow;
extern ConVar circle_shake_duration;
extern ConVar circle_shake_strength;
extern ConVar draw_approach_circles;
extern ConVar draw_circles;
extern ConVar draw_numbers;
extern ConVar slider_draw_endcircle;

// from OsuDatabase.cpp
extern ConVar collections_custom_enabled;
extern ConVar collections_custom_version;
extern ConVar collections_legacy_enabled;
extern ConVar collections_save_immediately;
extern ConVar database_enabled;
extern ConVar database_ignore_version;
extern ConVar database_ignore_version_warnings;
extern ConVar database_stars_cache_enabled;
extern ConVar database_version;
extern ConVar folder;
extern ConVar folder_sub_skins;
extern ConVar folder_sub_songs;
extern ConVar scores_bonus_pp;
extern ConVar scores_custom_enabled;
extern ConVar scores_custom_version;
extern ConVar scores_enabled;
extern ConVar scores_export;
extern ConVar scores_legacy_enabled;
extern ConVar scores_rename;
extern ConVar scores_save_immediately;
extern ConVar scores_sort_by_pp;
extern ConVar user_beatmap_pp_sanity_limit_for_stats;
extern ConVar user_include_relax_and_autopilot_for_stats;
extern ConVar user_switcher_include_legacy_scores_for_names;

// from OsuDatabaseBeatmap.cpp
extern ConVar beatmap_max_num_hitobjects;
extern ConVar beatmap_max_num_slider_scoringtimes;
extern ConVar beatmap_version;
extern ConVar ignore_beatmap_combo_numbers;
extern ConVar mod_random;
extern ConVar mod_random_circle_offset_x_percent;
extern ConVar mod_random_circle_offset_y_percent;
extern ConVar mod_random_seed;
extern ConVar mod_random_slider_offset_x_percent;
extern ConVar mod_random_slider_offset_y_percent;
extern ConVar mod_random_spinner_offset_x_percent;
extern ConVar mod_random_spinner_offset_y_percent;
extern ConVar mod_reverse_sliders;
extern ConVar mod_strict_tracking;
extern ConVar mod_strict_tracking_remove_slider_ticks;
extern ConVar number_max;
extern ConVar show_approach_circle_on_first_hidden_object;
extern ConVar slider_max_repeats;
extern ConVar slider_max_ticks;
extern ConVar stars_stacking;

// from OsuDifficultyCalculator.cpp
extern ConVar stars_always_recalc_live_strains;
extern ConVar stars_and_pp_lazer_relax_autopilot_nerf_disabled;
extern ConVar stars_ignore_clamped_sliders;
extern ConVar stars_slider_curve_points_separation;
extern ConVar stars_xexxar_angles_sliders;

// from OsuGameRules.cpp
namespace stdrules {
extern ConVar playfield_border_top_percent;
extern ConVar playfield_border_bottom_percent;

extern ConVar hitobject_hittable_dim;
extern ConVar hitobject_hittable_dim_start_percent;
extern ConVar hitobject_hittable_dim_duration;

extern ConVar hitobject_fade_in_time;

extern ConVar hitobject_fade_out_time;
extern ConVar hitobject_fade_out_time_speed_multiplier_min;

extern ConVar circle_fade_out_scale;

extern ConVar slider_followcircle_fadein_fade_time;
extern ConVar slider_followcircle_fadeout_fade_time;
extern ConVar slider_followcircle_fadein_scale;
extern ConVar slider_followcircle_fadein_scale_time;
extern ConVar slider_followcircle_fadeout_scale;
extern ConVar slider_followcircle_fadeout_scale_time;
extern ConVar slider_followcircle_tick_pulse_time;
extern ConVar slider_followcircle_tick_pulse_scale;

extern ConVar spinner_fade_out_time_multiplier;

extern ConVar slider_followcircle_size_multiplier;

extern ConVar mod_fps;
extern ConVar mod_no50s;
extern ConVar mod_no100s;
extern ConVar mod_ming3012;
extern ConVar mod_millhioref;
extern ConVar mod_millhioref_multiplier;
extern ConVar mod_mafham;
extern ConVar mod_mafham_render_livesize;
extern ConVar stacking_ar_override;
extern ConVar mod_halfwindow;
extern ConVar mod_halfwindow_allow_300s;

extern ConVar approachtime_min;
extern ConVar approachtime_mid;
extern ConVar approachtime_max;

extern ConVar hitwindow_300_min;
extern ConVar hitwindow_300_mid;
extern ConVar hitwindow_300_max;

extern ConVar hitwindow_100_min;
extern ConVar hitwindow_100_mid;
extern ConVar hitwindow_100_max;

extern ConVar hitwindow_50_min;
extern ConVar hitwindow_50_mid;
extern ConVar hitwindow_50_max;

extern ConVar hitwindow_miss;
}

// from OsuGameRulesMania.cpp
namespace maniarules {
extern ConVar mania_hitwindow_od_add_multiplier;
extern ConVar mania_hitwindow_300r;
extern ConVar mania_hitwindow_300;
extern ConVar mania_hitwindow_200;
extern ConVar mania_hitwindow_100;
extern ConVar mania_hitwindow_50;
extern ConVar mania_hitwindow_miss;
}

// from OsuHUD.cpp
extern ConVar automatic_cursor_size;
extern ConVar combo_anim1_duration;
extern ConVar combo_anim1_size;
extern ConVar combo_anim2_duration;
extern ConVar combo_anim2_size;
extern ConVar cursor_alpha;
extern ConVar cursor_expand_duration;
extern ConVar cursor_expand_scale_multiplier;
extern ConVar cursor_ripple_additive;
extern ConVar cursor_ripple_alpha;
extern ConVar cursor_ripple_anim_end_scale;
extern ConVar cursor_ripple_anim_start_fadeout_delay;
extern ConVar cursor_ripple_anim_start_scale;
extern ConVar cursor_ripple_duration;
extern ConVar cursor_ripple_tint_b;
extern ConVar cursor_ripple_tint_g;
extern ConVar cursor_ripple_tint_r;
extern ConVar cursor_scale;
extern ConVar cursor_trail_alpha;
extern ConVar cursor_trail_expand;
extern ConVar cursor_trail_length;
extern ConVar cursor_trail_max_size;
extern ConVar cursor_trail_scale;
extern ConVar cursor_trail_smooth_div;
extern ConVar cursor_trail_smooth_force;
extern ConVar cursor_trail_smooth_length;
extern ConVar cursor_trail_spacing;
extern ConVar draw_accuracy;
extern ConVar draw_combo;
extern ConVar draw_continue;
extern ConVar draw_cursor_ripples;
extern ConVar draw_cursor_trail;
extern ConVar draw_hiterrorbar;
extern ConVar draw_hiterrorbar_bottom;
extern ConVar draw_hiterrorbar_left;
extern ConVar draw_hiterrorbar_right;
extern ConVar draw_hiterrorbar_top;
extern ConVar draw_hiterrorbar_ur;
extern ConVar draw_hud;
extern ConVar draw_inputoverlay;
extern ConVar draw_progressbar;
extern ConVar draw_score;
extern ConVar draw_scorebar;
extern ConVar draw_scorebarbg;
extern ConVar draw_scoreboard;
extern ConVar draw_scrubbing_timeline;
extern ConVar draw_scrubbing_timeline_breaks;
extern ConVar draw_scrubbing_timeline_strain_graph;
extern ConVar draw_statistics_ar;
extern ConVar draw_statistics_bpm;
extern ConVar draw_statistics_detected_bpm;
extern ConVar draw_statistics_cs;
extern ConVar draw_statistics_hitdelta;
extern ConVar draw_statistics_hitwindow300;
extern ConVar draw_statistics_hp;
extern ConVar draw_statistics_livestars;
extern ConVar draw_statistics_maxpossiblecombo;
extern ConVar draw_statistics_misses;
extern ConVar draw_statistics_nd;
extern ConVar draw_statistics_nps;
extern ConVar draw_statistics_od;
extern ConVar draw_statistics_perfectpp;
extern ConVar draw_statistics_pp;
extern ConVar draw_statistics_sliderbreaks;
extern ConVar draw_statistics_totalstars;
extern ConVar draw_statistics_ur;
extern ConVar draw_target_heatmap;
extern ConVar hud_accuracy_scale;
extern ConVar hud_combo_scale;
extern ConVar hud_fps_smoothing;
extern ConVar hud_hiterrorbar_alpha;
extern ConVar hud_hiterrorbar_bar_alpha;
extern ConVar hud_hiterrorbar_bar_height_scale;
extern ConVar hud_hiterrorbar_bar_width_scale;
extern ConVar hud_hiterrorbar_centerline_alpha;
extern ConVar hud_hiterrorbar_centerline_b;
extern ConVar hud_hiterrorbar_centerline_g;
extern ConVar hud_hiterrorbar_centerline_r;
extern ConVar hud_hiterrorbar_entry_100_b;
extern ConVar hud_hiterrorbar_entry_100_g;
extern ConVar hud_hiterrorbar_entry_100_r;
extern ConVar hud_hiterrorbar_entry_300_b;
extern ConVar hud_hiterrorbar_entry_300_g;
extern ConVar hud_hiterrorbar_entry_300_r;
extern ConVar hud_hiterrorbar_entry_50_b;
extern ConVar hud_hiterrorbar_entry_50_g;
extern ConVar hud_hiterrorbar_entry_50_r;
extern ConVar hud_hiterrorbar_entry_additive;
extern ConVar hud_hiterrorbar_entry_alpha;
extern ConVar hud_hiterrorbar_entry_hit_fade_time;
extern ConVar hud_hiterrorbar_entry_misaim_height_multiplier;
extern ConVar hud_hiterrorbar_entry_miss_b;
extern ConVar hud_hiterrorbar_entry_miss_fade_time;
extern ConVar hud_hiterrorbar_entry_miss_g;
extern ConVar hud_hiterrorbar_entry_miss_height_multiplier;
extern ConVar hud_hiterrorbar_entry_miss_r;
extern ConVar hud_hiterrorbar_height_percent;
extern ConVar hud_hiterrorbar_hide_during_spinner;
extern ConVar hud_hiterrorbar_max_entries;
extern ConVar hud_hiterrorbar_offset_bottom_percent;
extern ConVar hud_hiterrorbar_offset_left_percent;
extern ConVar hud_hiterrorbar_offset_percent;
extern ConVar hud_hiterrorbar_offset_right_percent;
extern ConVar hud_hiterrorbar_offset_top_percent;
extern ConVar hud_hiterrorbar_scale;
extern ConVar hud_hiterrorbar_showmisswindow;
extern ConVar hud_hiterrorbar_ur_alpha;
extern ConVar hud_hiterrorbar_ur_offset_x_percent;
extern ConVar hud_hiterrorbar_ur_offset_y_percent;
extern ConVar hud_hiterrorbar_ur_scale;
extern ConVar hud_hiterrorbar_width_percent;
extern ConVar hud_hiterrorbar_width_percent_with_misswindow;
extern ConVar hud_inputoverlay_anim_color_duration;
extern ConVar hud_inputoverlay_anim_scale_duration;
extern ConVar hud_inputoverlay_anim_scale_multiplier;
extern ConVar hud_inputoverlay_offset_x;
extern ConVar hud_inputoverlay_offset_y;
extern ConVar hud_inputoverlay_scale;
extern ConVar hud_playfield_border_size;
extern ConVar hud_progressbar_scale;
extern ConVar hud_scale;
extern ConVar hud_score_scale;
extern ConVar hud_scorebar_hide_anim_duration;
extern ConVar hud_scorebar_hide_during_breaks;
extern ConVar hud_scorebar_scale;
extern ConVar hud_scoreboard_offset_y_percent;
extern ConVar hud_scoreboard_scale;
extern ConVar hud_scoreboard_use_menubuttonbackground;
extern ConVar hud_scrubbing_timeline_hover_tooltip_offset_multiplier;
extern ConVar hud_scrubbing_timeline_strains_aim_color_b;
extern ConVar hud_scrubbing_timeline_strains_aim_color_g;
extern ConVar hud_scrubbing_timeline_strains_aim_color_r;
extern ConVar hud_scrubbing_timeline_strains_alpha;
extern ConVar hud_scrubbing_timeline_strains_height;
extern ConVar hud_scrubbing_timeline_strains_speed_color_b;
extern ConVar hud_scrubbing_timeline_strains_speed_color_g;
extern ConVar hud_scrubbing_timeline_strains_speed_color_r;
extern ConVar hud_shift_tab_toggles_everything;
extern ConVar hud_statistics_ar_offset_x;
extern ConVar hud_statistics_ar_offset_y;
extern ConVar hud_statistics_bpm_offset_x;
extern ConVar hud_statistics_bpm_offset_y;
extern ConVar hud_statistics_detected_bpm_offset_x;
extern ConVar hud_statistics_detected_bpm_offset_y;
extern ConVar hud_statistics_cs_offset_x;
extern ConVar hud_statistics_cs_offset_y;
extern ConVar hud_statistics_hitdelta_offset_x;
extern ConVar hud_statistics_hitdelta_offset_y;
extern ConVar hud_statistics_hitwindow300_offset_x;
extern ConVar hud_statistics_hitwindow300_offset_y;
extern ConVar hud_statistics_hp_offset_x;
extern ConVar hud_statistics_hp_offset_y;
extern ConVar hud_statistics_livestars_offset_x;
extern ConVar hud_statistics_livestars_offset_y;
extern ConVar hud_statistics_maxpossiblecombo_offset_x;
extern ConVar hud_statistics_maxpossiblecombo_offset_y;
extern ConVar hud_statistics_misses_offset_x;
extern ConVar hud_statistics_misses_offset_y;
extern ConVar hud_statistics_nd_offset_x;
extern ConVar hud_statistics_nd_offset_y;
extern ConVar hud_statistics_nps_offset_x;
extern ConVar hud_statistics_nps_offset_y;
extern ConVar hud_statistics_od_offset_x;
extern ConVar hud_statistics_od_offset_y;
extern ConVar hud_statistics_offset_x;
extern ConVar hud_statistics_offset_y;
extern ConVar hud_statistics_perfectpp_offset_x;
extern ConVar hud_statistics_perfectpp_offset_y;
extern ConVar hud_statistics_pp_decimal_places;
extern ConVar hud_statistics_pp_offset_x;
extern ConVar hud_statistics_pp_offset_y;
extern ConVar hud_statistics_scale;
extern ConVar hud_statistics_sliderbreaks_offset_x;
extern ConVar hud_statistics_sliderbreaks_offset_y;
extern ConVar hud_statistics_spacing_scale;
extern ConVar hud_statistics_totalstars_offset_x;
extern ConVar hud_statistics_totalstars_offset_y;
extern ConVar hud_statistics_ur_offset_x;
extern ConVar hud_statistics_ur_offset_y;
extern ConVar hud_volume_duration;
extern ConVar hud_volume_size_multiplier;

// from OsuHitObject.cpp
extern ConVar approach_scale_multiplier;
extern ConVar hitresult_animated;
extern ConVar hitresult_delta_colorize;
extern ConVar hitresult_delta_colorize_early_b;
extern ConVar hitresult_delta_colorize_early_g;
extern ConVar hitresult_delta_colorize_early_r;
extern ConVar hitresult_delta_colorize_interpolate;
extern ConVar hitresult_delta_colorize_late_b;
extern ConVar hitresult_delta_colorize_late_g;
extern ConVar hitresult_delta_colorize_late_r;
extern ConVar hitresult_delta_colorize_multiplier;
extern ConVar hitresult_draw_300s;
extern ConVar hitresult_duration;
extern ConVar hitresult_duration_max;
extern ConVar hitresult_fadein_duration;
extern ConVar hitresult_fadeout_duration;
extern ConVar hitresult_fadeout_start_time;
extern ConVar hitresult_miss_fadein_scale;
extern ConVar hitresult_scale;
extern ConVar mod_approach_different;
extern ConVar mod_approach_different_initial_size;
extern ConVar mod_approach_different_style;
extern ConVar mod_hd_circle_fadein_end_percent;
extern ConVar mod_hd_circle_fadein_start_percent;
extern ConVar mod_hd_circle_fadeout_end_percent;
extern ConVar mod_hd_circle_fadeout_start_percent;
extern ConVar mod_mafham_ignore_hittable_dim;
extern ConVar mod_target_100_percent;
extern ConVar mod_target_300_percent;
extern ConVar mod_target_50_percent;
extern ConVar relax_offset;
extern ConVar timingpoints_force;

// from OsuKeyBindings.cpp
namespace keybinds {
extern ConVar LEFT_CLICK;
extern ConVar RIGHT_CLICK;
extern ConVar LEFT_CLICK_2;
extern ConVar RIGHT_CLICK_2;
extern ConVar FPOSU_ZOOM;
extern ConVar INCREASE_SPEED;
extern ConVar DECREASE_SPEED;
extern ConVar INCREASE_VOLUME;
extern ConVar DECREASE_VOLUME;
extern ConVar INCREASE_LOCAL_OFFSET;
extern ConVar DECREASE_LOCAL_OFFSET;
extern ConVar GAME_PAUSE;
extern ConVar SKIP_CUTSCENE;
extern ConVar TOGGLE_SCOREBOARD;
extern ConVar SEEK_TIME;
extern ConVar SEEK_TIME_BACKWARD;
extern ConVar SEEK_TIME_FORWARD;
extern ConVar QUICK_RETRY;
extern ConVar QUICK_SAVE;
extern ConVar QUICK_LOAD;
extern ConVar SAVE_SCREENSHOT;
extern ConVar DISABLE_MOUSE_BUTTONS;
extern ConVar BOSS_KEY;
extern ConVar TOGGLE_MODSELECT;
extern ConVar RANDOM_BEATMAP;
extern ConVar MOD_EASY;
extern ConVar MOD_NOFAIL;
extern ConVar MOD_HALFTIME;
extern ConVar MOD_HARDROCK;
extern ConVar MOD_SUDDENDEATH;
extern ConVar MOD_DOUBLETIME;
extern ConVar MOD_HIDDEN;
extern ConVar MOD_FLASHLIGHT;
extern ConVar MOD_RELAX;
extern ConVar MOD_AUTOPILOT;
extern ConVar MOD_SPUNOUT;
extern ConVar MOD_AUTO;
extern ConVar MOD_SCOREV2;
}

// from OsuMainMenu.cpp
extern ConVar draw_main_menu_button;
extern ConVar draw_main_menu_button_subtext;
extern ConVar draw_main_menu_workshop_button;
extern ConVar draw_menu_background;
extern ConVar main_menu_alpha;
extern ConVar main_menu_banner_always_text;
extern ConVar main_menu_banner_ifupdatedfromoldversion_le3300_text;
extern ConVar main_menu_banner_ifupdatedfromoldversion_le3303_text;
extern ConVar main_menu_banner_ifupdatedfromoldversion_le3308_text;
extern ConVar main_menu_banner_ifupdatedfromoldversion_le3310_text;
extern ConVar main_menu_banner_ifupdatedfromoldversion_text;
extern ConVar main_menu_friend;
extern ConVar main_menu_shuffle;
extern ConVar main_menu_slider_text_alpha;
extern ConVar main_menu_slider_text_offset_x;
extern ConVar main_menu_slider_text_offset_y;
extern ConVar main_menu_slider_text_scale;
extern ConVar main_menu_slider_text_scissor;
extern ConVar main_menu_slider_text_feather;
extern ConVar main_menu_startup_anim_duration;
extern ConVar main_menu_use_slider_text;
extern ConVar toggle_preview_music;

// from OsuManiaNote.cpp
extern ConVar mania_note_height;
extern ConVar mania_speed;

// from OsuModFPoSu.cpp
namespace fposu {
extern ConVar threeD;
extern ConVar threeD_approachcircles_look_at_player;
extern ConVar threeD_beatmap_background_image_distance_multiplier;
extern ConVar threeD_curve_multiplier;
extern ConVar threeD_draw_beatmap_background_image;
extern ConVar threeD_hitobjects_look_at_player;
extern ConVar threeD_playfield_scale;
extern ConVar threeD_skybox;
extern ConVar threeD_skybox_size;
extern ConVar threeD_spheres;
extern ConVar threeD_spheres_aa;
extern ConVar threeD_spheres_light_ambient;
extern ConVar threeD_spheres_light_brightness;
extern ConVar threeD_spheres_light_diffuse;
extern ConVar threeD_spheres_light_phong;
extern ConVar threeD_spheres_light_phong_exponent;
extern ConVar threeD_spheres_light_position_x;
extern ConVar threeD_spheres_light_position_y;
extern ConVar threeD_spheres_light_position_z;
extern ConVar threeD_spheres_light_rim;
extern ConVar threeD_wireframe;
extern ConVar absolute_mode;
extern ConVar cube;
extern ConVar cube_size;
extern ConVar cube_tint_b;
extern ConVar cube_tint_g;
extern ConVar cube_tint_r;
extern ConVar curved;
extern ConVar distance;
extern ConVar draw_cursor_trail;
extern ConVar draw_scorebarbg_on_top;
extern ConVar fov;
extern ConVar invert_horizontal;
extern ConVar invert_vertical;
extern ConVar mod_3d_depthwobble;
extern ConVar mod_strafing;
extern ConVar mod_strafing_frequency_x;
extern ConVar mod_strafing_frequency_y;
extern ConVar mod_strafing_frequency_z;
extern ConVar mod_strafing_strength_x;
extern ConVar mod_strafing_strength_y;
extern ConVar mod_strafing_strength_z;
extern ConVar mouse_cm_360;
extern ConVar mouse_dpi;
extern ConVar noclip;
extern ConVar noclipaccelerate;
extern ConVar noclipfriction;
extern ConVar noclipspeed;
extern ConVar playfield_position_x;
extern ConVar playfield_position_y;
extern ConVar playfield_position_z;
extern ConVar playfield_rotation_x;
extern ConVar playfield_rotation_y;
extern ConVar playfield_rotation_z;
extern ConVar skybox;
extern ConVar vertical_fov;
extern ConVar zoom_anim_duration;
extern ConVar zoom_fov;
extern ConVar zoom_sensitivity_ratio;
extern ConVar zoom_toggle;
extern ConVar mod_fposu;
}

// from OsuMultiplayer.cpp
extern ConVar mp_broadcastcommand;
extern ConVar mp_broadcastforceclientbeatmapdownload;
extern ConVar mp_clientcastcommand;
extern ConVar mp_freemod;
extern ConVar mp_freemod_all;
extern ConVar mp_request_beatmap_download;
extern ConVar mp_select_beatmap;
extern ConVar mp_allow_client_beatmap_select;
extern ConVar mp_win_condition_accuracy;

// from OsuNotificationOverlay.cpp
extern ConVar notification_duration;

// from OsuOptionsMenu.cpp
extern ConVar mania_keylayout_wizard;
extern ConVar options_high_quality_sliders;
extern ConVar options_save_on_back;
extern ConVar options_slider_preview_use_legacy_renderer;
extern ConVar options_slider_quality;

// from OsuPauseMenu.cpp
extern ConVar pause_anim_duration;
extern ConVar pause_dim_alpha;
extern ConVar pause_dim_background;

// from OsuRankingScreen.cpp
extern ConVar draw_rankingscreen_background_image;
extern ConVar rankingscreen_pp;
extern ConVar rankingscreen_topbar_height_percent;

// from OsuRichPresence.cpp
extern ConVar rich_presence;
extern ConVar rich_presence_discord_show_totalpp;
extern ConVar rich_presence_dynamic_windowtitle;
extern ConVar rich_presence_show_recentplaystats;

// from OsuScore.cpp
extern ConVar debug_pp;
extern ConVar drain_lazer_100;
extern ConVar drain_lazer_2018_100;
extern ConVar drain_lazer_2018_300;
extern ConVar drain_lazer_2018_50;
extern ConVar drain_lazer_2018_miss;
extern ConVar drain_lazer_2018_multiplier;
extern ConVar drain_lazer_300;
extern ConVar drain_lazer_50;
extern ConVar drain_lazer_miss;
extern ConVar drain_lazer_multiplier;
extern ConVar drain_stable_hpbar_maximum;
extern ConVar hiterrorbar_misses;
extern ConVar hud_statistics_hitdelta_chunksize;

// from OsuSkin.cpp
extern ConVar export_skin;
extern ConVar ignore_beatmap_combo_colors;
extern ConVar ignore_beatmap_sample_volume;
extern ConVar mod_fposu_sound_panning;
extern ConVar mod_fps_sound_panning;
extern ConVar skin_animation_force;
extern ConVar skin_async;
extern ConVar skin_color_index_add;
extern ConVar skin_export;
extern ConVar skin_force_hitsound_sample_set;
extern ConVar skin_hd;
extern ConVar skin_mipmaps;
extern ConVar skin_random;
extern ConVar skin_random_elements;
extern ConVar skin_use_skin_hitsounds;
extern ConVar sound_panning;
extern ConVar sound_panning_multiplier;
extern ConVar volume_effects;

// from OsuSkinImage.cpp
extern ConVar skin_animation_fps_override;

// from OsuSlider.cpp
extern ConVar mod_hd_slider_fade_percent;
extern ConVar mod_hd_slider_fast_fade;
extern ConVar slider_ball_tint_combo_color;
extern ConVar slider_body_fade_out_time_multiplier;
extern ConVar slider_body_lazer_fadeout_style;
extern ConVar slider_body_smoothsnake;
extern ConVar slider_break_epilepsy;
extern ConVar slider_draw_body;
extern ConVar slider_end_inside_check_offset;
extern ConVar slider_end_miss_breaks_combo;
extern ConVar slider_reverse_arrow_alpha_multiplier;
extern ConVar slider_reverse_arrow_animated;
extern ConVar slider_reverse_arrow_black_threshold;
extern ConVar slider_reverse_arrow_fadein_duration;
extern ConVar slider_scorev2;
extern ConVar slider_shrink;
extern ConVar slider_snake_duration_multiplier;
extern ConVar snaking_sliders;

// from OsuSliderCurves.cpp
extern ConVar slider_curve_max_length;
extern ConVar slider_curve_max_points;
extern ConVar slider_curve_points_separation;

// from OsuSliderRenderer.cpp
extern ConVar slider_alpha_multiplier;
extern ConVar slider_body_alpha_multiplier;
extern ConVar slider_body_color_saturation;
extern ConVar slider_body_unit_circle_subdivisions;
extern ConVar slider_border_size_multiplier;
extern ConVar slider_border_tint_combo_color;
extern ConVar slider_debug_draw;
extern ConVar slider_debug_draw_square_vao;
extern ConVar slider_debug_wireframe;
extern ConVar slider_legacy_use_baked_vao;
extern ConVar slider_osu_next_style;
extern ConVar slider_rainbow;
extern ConVar slider_use_gradient_image;

// from OsuSongBrowser2.cpp
extern ConVar debug_background_star_calc;
extern ConVar draw_songbrowser_background_image;
extern ConVar draw_songbrowser_menu_background_image;
extern ConVar draw_songbrowser_strain_graph;
extern ConVar gamemode;
extern ConVar songbrowser_background_fade_in_duration;
extern ConVar songbrowser_background_star_calculation;
extern ConVar songbrowser_bottombar_percent;
extern ConVar songbrowser_debug;
extern ConVar songbrowser_dynamic_star_recalc;
extern ConVar songbrowser_scorebrowser_enabled;
extern ConVar songbrowser_scores_sortingtype;
extern ConVar songbrowser_search_delay;
extern ConVar songbrowser_search_hardcoded_filter;
extern ConVar songbrowser_sortingtype;
extern ConVar songbrowser_topbar_left_percent;
extern ConVar songbrowser_topbar_left_width_percent;
extern ConVar songbrowser_topbar_middle_width_percent;
extern ConVar songbrowser_topbar_right_height_percent;
extern ConVar songbrowser_topbar_right_percent;

// from OsuSpinner.cpp
extern ConVar spinner_use_ar_fadein;

// from OsuSteamWorkshop.cpp
extern ConVar workshop_upload_skin;

// from OsuTooltipOverlay.cpp
extern ConVar tooltip_anim_duration;

// from OsuUISongBrowserButton.cpp
extern ConVar songbrowser_button_active_color_a;
extern ConVar songbrowser_button_active_color_b;
extern ConVar songbrowser_button_active_color_g;
extern ConVar songbrowser_button_active_color_r;
extern ConVar songbrowser_button_inactive_color_a;
extern ConVar songbrowser_button_inactive_color_b;
extern ConVar songbrowser_button_inactive_color_g;
extern ConVar songbrowser_button_inactive_color_r;

// from OsuUISongBrowserCollectionButton.cpp
extern ConVar songbrowser_button_collection_active_color_a;
extern ConVar songbrowser_button_collection_active_color_b;
extern ConVar songbrowser_button_collection_active_color_g;
extern ConVar songbrowser_button_collection_active_color_r;
extern ConVar songbrowser_button_collection_inactive_color_a;
extern ConVar songbrowser_button_collection_inactive_color_b;
extern ConVar songbrowser_button_collection_inactive_color_g;
extern ConVar songbrowser_button_collection_inactive_color_r;

// from OsuUISongBrowserSongButton.cpp
extern ConVar draw_songbrowser_thumbnails;
extern ConVar songbrowser_thumbnail_delay;
extern ConVar songbrowser_thumbnail_fade_in_duration;

// from OsuUISongBrowserSongDifficultyButton.cpp
extern ConVar songbrowser_button_difficulty_inactive_color_a;
extern ConVar songbrowser_button_difficulty_inactive_color_b;
extern ConVar songbrowser_button_difficulty_inactive_color_g;
extern ConVar songbrowser_button_difficulty_inactive_color_r;

// from OsuUISongBrowserUserButton.cpp
extern ConVar user_draw_accuracy;
extern ConVar user_draw_level;
extern ConVar user_draw_level_bar;
extern ConVar user_draw_pp;

// from OsuUserStatsScreen.cpp
extern ConVar ui_top_ranks_max;

} // namespace cv

#endif
