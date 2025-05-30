//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		global console variable registry
//
// $NoKeywords: $convar
//===============================================================================//

class ConVar;

#pragma once
#ifndef CVDEFS_H
#define CVDEFS_H
namespace cv {

// from AnimationHandler.cpp
extern ConVar debug_anim;

// from BassSound2.cpp
extern ConVar snd_async_buffer;

// from BassSoundEngine.cpp
extern ConVar snd_buffer;
extern ConVar snd_dev_buffer;
extern ConVar snd_dev_period;
extern ConVar snd_updateperiod;
extern ConVar win_snd_wasapi_buffer_size;
extern ConVar win_snd_wasapi_exclusive;
extern ConVar win_snd_wasapi_period_size;
extern ConVar win_snd_wasapi_shared_volume_affects_device;

// from BassSoundEngine2.cpp
extern ConVar snd_asio_buffer_size;
extern ConVar snd_ready_delay;

// from CBaseUIBoxShadow.cpp
extern ConVar debug_box_shadows;

// from CBaseUIScrollView.cpp
extern ConVar ui_scrollview_kinetic_approach_time;
extern ConVar ui_scrollview_kinetic_energy_multiplier;
extern ConVar ui_scrollview_mousewheel_multiplier;
extern ConVar ui_scrollview_mousewheel_overscrollbounce;
extern ConVar ui_scrollview_resistance;
extern ConVar ui_scrollview_scrollbarwidth;

// from CBaseUITextbox.cpp
extern ConVar ui_textbox_caret_blink_time;
extern ConVar ui_textbox_text_offset_x;

// from CBaseUIWindow.cpp
extern ConVar ui_window_animspeed;
extern ConVar ui_window_shadow_radius;

// from Camera.cpp
extern ConVar cl_pitchdown;
extern ConVar cl_pitchup;

// from ConVar.cpp
namespace ConVars {
	extern ConVar sv_cheats;
}

extern ConVar dumpcommands;
extern ConVar emptyDummyConVar;
extern ConVar find;
extern ConVar help;
extern ConVar listcommands;

// from Console.cpp
extern ConVar clear;
extern ConVar console_logging;
extern ConVar echo;
extern ConVar exec;
extern ConVar fizzbuzz;

// from ConsoleBox.cpp
extern ConVar console_overlay;
extern ConVar console_overlay_lines;
extern ConVar console_overlay_scale;
extern ConVar consolebox_animspeed;
extern ConVar consolebox_draw_helptext;
extern ConVar consolebox_draw_preview;
extern ConVar showconsolebox;

// from Engine.cpp
extern ConVar borderless;
extern ConVar center;
extern ConVar crash;
extern ConVar debug_engine;
extern ConVar disable_windows_key;
extern ConVar dpiinfo;
extern ConVar engine_throttle;
extern ConVar epilepsy;
extern ConVar errortest;
extern ConVar exit;
extern ConVar focus;
extern ConVar fullscreen;
extern ConVar host_timescale;
extern ConVar maximize;
extern ConVar minimize;
extern ConVar minimize_on_focus_lost_if_borderless_windowed_fullscreen;
extern ConVar minimize_on_focus_lost_if_fullscreen;
extern ConVar printsize;
extern ConVar resizable_toggle;
extern ConVar restart;
extern ConVar shutdown;
extern ConVar version;
extern ConVar windowed;

// from Environment.cpp
extern ConVar debug_env;
extern ConVar fullscreen_windowed_borderless;
extern ConVar monitor;
extern ConVar processpriority;

// from File.cpp
extern ConVar debug_file;
extern ConVar file_size_max;

// from Font.cpp
extern ConVar r_debug_drawstring_unbind;
extern ConVar r_debug_font_atlas_padding;
extern ConVar r_drawstring_max_string_length;

// from Graphics.cpp
extern ConVar mat_wireframe;
extern ConVar r_3dscene_zf;
extern ConVar r_3dscene_zn;
extern ConVar r_debug_disable_3dscene;
extern ConVar r_debug_disable_cliprect;
extern ConVar r_debug_drawimage;
extern ConVar r_debug_flush_drawstring;
extern ConVar r_globaloffset_x;
extern ConVar r_globaloffset_y;
extern ConVar vsync;

// from Mouse.cpp
extern ConVar debug_mouse;
extern ConVar debug_mouse_clicks;
extern ConVar mouse_fakelag;
extern ConVar mouse_raw_input;
extern ConVar mouse_raw_input_absolute_to_window;
extern ConVar mouse_sensitivity;
extern ConVar tablet_sensitivity_ignore;

// from NetworkHandler.cpp
extern ConVar cl_cmdrate;
extern ConVar cl_updaterate;
extern ConVar connect;
extern ConVar connect_duration;
extern ConVar debug_network;
extern ConVar debug_network_time;
extern ConVar disconnect;
extern ConVar disconnect_duration;
extern ConVar host;
extern ConVar host_max_clients;
extern ConVar host_port;
extern ConVar httpget;
extern ConVar kick;
extern ConVar name;
extern ConVar name_admin;
extern ConVar say;
extern ConVar status;
extern ConVar stop;

// from OpenGLES32Interface.cpp
extern ConVar r_gles_orphan_buffers;

// from OpenGLLegacyInterface.cpp
extern ConVar r_image_unbind_after_drawimage;

// from OpenGLSync.cpp
extern ConVar r_sync_debug;
extern ConVar r_sync_enabled;
extern ConVar r_sync_max_frames;
extern ConVar r_sync_timeout;

// from OpenGLVertexArrayObject.cpp
extern ConVar r_opengl_legacy_vao_use_vertex_array;

// from Profiler.cpp
extern ConVar vprof;

// from RenderTarget.cpp
extern ConVar debug_rt;

// from ResourceManager.cpp
extern ConVar debug_rm;
extern ConVar rm_interrupt_on_destroy;

// from SDLGLInterface.cpp
extern ConVar debug_opengl;

// from SDLSoundEngine.cpp
extern ConVar snd_chunk_size;

// from Shader.cpp
extern ConVar debug_shaders;

// from SoLoudSoundEngine.cpp
extern ConVar snd_sanity_simultaneous_limit;
extern ConVar snd_soloud_backend;
extern ConVar snd_soloud_buffer;

// from Sound.cpp
extern ConVar debug_snd;
extern ConVar snd_file_min_size;
extern ConVar snd_force_load_unknown;
extern ConVar snd_play_interp_duration;
extern ConVar snd_play_interp_ratio;
extern ConVar snd_speed_compensate_pitch;

// from SoundEngine.cpp
extern ConVar snd_change_check_interval;
extern ConVar snd_freq;
extern ConVar snd_output_device;
extern ConVar snd_restart;
extern ConVar snd_restrict_play_frame;
extern ConVar volume;
extern ConVar win_snd_fallback_dsound;

// from SoundTouchFilter.cpp
extern ConVar snd_enable_auto_offset;
extern ConVar snd_st_debug;

// from SteamworksInterface.cpp
extern ConVar debug_steam;
extern ConVar steam_timeout;

// from Thread.cpp
extern ConVar debug_thread;

// from VSControlBar.cpp
extern ConVar vs_repeat;
extern ConVar vs_shuffle;
extern ConVar vs_volume;

// from VSMusicBrowser.cpp
extern ConVar vs_browser_animspeed;

// from VSTitleBar.cpp
extern ConVar vs_percent;

// from VisualProfiler.cpp
extern ConVar debug_vprof;
extern ConVar vprof_display_mode;
extern ConVar vprof_graph;
extern ConVar vprof_graph_alpha;
extern ConVar vprof_graph_draw_overhead;
extern ConVar vprof_graph_height;
extern ConVar vprof_graph_margin;
extern ConVar vprof_graph_range_max;
extern ConVar vprof_graph_width;
extern ConVar vprof_spike;

// from main.cpp
extern ConVar fps_max;
extern ConVar fps_max_background;
extern ConVar fps_unlimited;
extern ConVar fps_yield;

} // namespace cv

#endif
