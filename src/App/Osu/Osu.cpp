//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		yet another ouendan clone, because why not
//
// $NoKeywords: $osu
//===============================================================================//

#include "OsuDatabase.h"
#include "Osu.h"

#include "Engine.h"
#include "Environment.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "ConsoleBox.h"
#include "ResourceManager.h"
#include "Image.h"
#include "Font.h"
#include "Sound.h"
#include "AnimationHandler.h"
#include "SoundEngine.h"
#include "Console.h"
#include "ConVar.h"
#include "SteamworksInterface.h"
#include "RenderTarget.h"
#include "Shader.h"

#include "CWindowManager.h"
//#include "DebugMonitor.h"

#include "OsuMultiplayer.h"
#include "OsuMainMenu.h"
#include "OsuOptionsMenu.h"
#include "OsuSongBrowser2.h"
#include "OsuBackgroundImageHandler.h"
#include "OsuModSelector.h"
#include "OsuRankingScreen.h"
#include "OsuUserStatsScreen.h"
#include "OsuKeyBindings.h"
#include "OsuUpdateHandler.h"
#include "OsuNotificationOverlay.h"
#include "OsuTooltipOverlay.h"
#include "OsuGameRules.h"
#include "OsuPauseMenu.h"
#include "OsuScore.h"
#include "OsuSkin.h"
#include "OsuIcons.h"
#include "OsuHUD.h"
#include "OsuChangelog.h"
#include "OsuEditor.h"
#include "OsuRichPresence.h"
#include "OsuSteamWorkshop.h"
#include "OsuModFPoSu.h"

#include "OsuBeatmap.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuBeatmapStandard.h"
//#include "OsuBeatmapMania.h"

#include "OsuHitObject.h"

#include "OsuUIVolumeSlider.h"

// release configuration
bool Osu::autoUpdater = false;

namespace cv::osu {
ConVar version("osu_version", PACKAGE_VERSION, FCVAR_NONE);
ConVar release_stream("osu_release_stream", "desktop", FCVAR_NONE);
ConVar debug("osu_debug", false, FCVAR_NONE);

ConVar disable_mousebuttons("osu_disable_mousebuttons", false, FCVAR_NONE);
ConVar disable_mousewheel("osu_disable_mousewheel", false, FCVAR_NONE);
ConVar confine_cursor_windowed("osu_confine_cursor_windowed", false, FCVAR_NONE);
ConVar confine_cursor_fullscreen("osu_confine_cursor_fullscreen", true, FCVAR_NONE);
ConVar confine_cursor_never("osu_confine_cursor_never", false, FCVAR_NONE, "workaround for relative tablet motion quirks with Xwayland");

ConVar skin("osu_skin", "default", FCVAR_NONE); // set dynamically below in the constructor
ConVar skin_is_from_workshop("osu_skin_is_from_workshop", false, FCVAR_NONE, "determines whether osu_skin contains a relative folder name, or a full absolute path (for workshop skins)");
ConVar skin_workshop_title("osu_skin_workshop_title", "", FCVAR_NONE, "holds the title/name of the currently selected workshop skin, because osu_skin is already used up for the absolute path then");
ConVar skin_workshop_id("osu_skin_workshop_id", "0", FCVAR_NONE, "holds the id of the currently selected workshop skin");
ConVar skin_reload("osu_skin_reload");

ConVar volume_master("osu_volume_master", 1.0f, FCVAR_NONE);
ConVar volume_master_inactive("osu_volume_master_inactive", 0.25f, FCVAR_NONE);
ConVar volume_music("osu_volume_music", 0.4f, FCVAR_NONE);
ConVar volume_change_interval("osu_volume_change_interval", 0.05f, FCVAR_NONE);

ConVar speed_override("osu_speed_override", -1.0f, FCVAR_NONE);
ConVar pitch_override("osu_pitch_override", -1.0f, FCVAR_NONE);

ConVar pause_on_focus_loss("osu_pause_on_focus_loss", true, FCVAR_NONE);
ConVar quick_retry_delay("osu_quick_retry_delay", 0.27f, FCVAR_NONE);
ConVar scrubbing_smooth("osu_scrubbing_smooth", true, FCVAR_NONE);
ConVar skip_intro_enabled("osu_skip_intro_enabled", true, FCVAR_NONE, "enables/disables skip button for intro until first hitobject");
ConVar skip_breaks_enabled("osu_skip_breaks_enabled", true, FCVAR_NONE, "enables/disables skip button for breaks in the middle of beatmaps");
ConVar seek_delta("osu_seek_delta", 5, FCVAR_NONE, "how many seconds to skip backward/forward when quick seeking");

ConVar mods("osu_mods", "", FCVAR_NONE);
ConVar mod_touchdevice("osu_mod_touchdevice", false, FCVAR_NONE, "used for force applying touch pp nerf always");
ConVar mod_fadingcursor("osu_mod_fadingcursor", false, FCVAR_NONE);
ConVar mod_fadingcursor_combo("osu_mod_fadingcursor_combo", 50.0f, FCVAR_NONE);
ConVar mod_endless("osu_mod_endless", false, FCVAR_NONE);

ConVar notification("osu_notification");
ConVar notification_color_r("osu_notification_color_r", 255, FCVAR_NONE);
ConVar notification_color_g("osu_notification_color_g", 255, FCVAR_NONE);
ConVar notification_color_b("osu_notification_color_b", 255, FCVAR_NONE);

ConVar ui_scale("osu_ui_scale", 1.0f, FCVAR_NONE, "multiplier");
ConVar ui_scale_to_dpi("osu_ui_scale_to_dpi", true, FCVAR_NONE, "whether the game should scale its UI based on the DPI reported by your operating system");
ConVar ui_scale_to_dpi_minimum_width("osu_ui_scale_to_dpi_minimum_width", 2200, FCVAR_NONE, "any in-game resolutions below this will have osu_ui_scale_to_dpi force disabled");
ConVar ui_scale_to_dpi_minimum_height("osu_ui_scale_to_dpi_minimum_height", 1300, FCVAR_NONE, "any in-game resolutions below this will have osu_ui_scale_to_dpi force disabled");
ConVar letterboxing("osu_letterboxing", true, FCVAR_NONE);
ConVar letterboxing_offset_x("osu_letterboxing_offset_x", 0.0f, FCVAR_NONE);
ConVar letterboxing_offset_y("osu_letterboxing_offset_y", 0.0f, FCVAR_NONE);
ConVar resolution("osu_resolution", "1280x720", FCVAR_NONE);
ConVar resolution_enabled("osu_resolution_enabled", false, FCVAR_NONE);
ConVar resolution_keep_aspect_ratio("osu_resolution_keep_aspect_ratio", false, FCVAR_NONE);
ConVar force_legacy_slider_renderer("osu_force_legacy_slider_renderer", false, FCVAR_NONE, "on some older machines, this may be faster than vertexbuffers");

ConVar draw_fps("osu_draw_fps", true, FCVAR_NONE);
ConVar hide_cursor_during_gameplay("osu_hide_cursor_during_gameplay", false, FCVAR_NONE);

ConVar alt_f4_quits_even_while_playing("osu_alt_f4_quits_even_while_playing", true, FCVAR_NONE);
ConVar disable_windows_key_while_playing("osu_disable_windows_key_while_playing", true, FCVAR_NONE);
}

Vector2 Osu::g_vInternalResolution;
Vector2 Osu::osuBaseResolution = Vector2(640.0f, 480.0f);

Osu *osu = nullptr;

Osu::Osu()
{
	osu = this;
	srand(time(NULL));

	// experimental mods list
	m_experimentalMods.push_back(&cv::osu::fposu::mod_strafing);
	m_experimentalMods.push_back(&cv::osu::fposu::mod_3d_depthwobble);
	m_experimentalMods.push_back(&cv::osu::mod_wobble);
	m_experimentalMods.push_back(&cv::osu::mod_arwobble);
	m_experimentalMods.push_back(&cv::osu::mod_timewarp);
	m_experimentalMods.push_back(&cv::osu::mod_artimewarp);
	m_experimentalMods.push_back(&cv::osu::mod_minimize);
	m_experimentalMods.push_back(&cv::osu::mod_fadingcursor);
	m_experimentalMods.push_back(&cv::osu::stdrules::mod_fps);
	m_experimentalMods.push_back(&cv::osu::mod_jigsaw1);
	m_experimentalMods.push_back(&cv::osu::mod_jigsaw2);
	m_experimentalMods.push_back(&cv::osu::mod_fullalternate);
	m_experimentalMods.push_back(&cv::osu::mod_random);
	m_experimentalMods.push_back(&cv::osu::mod_reverse_sliders);
	m_experimentalMods.push_back(&cv::osu::stdrules::mod_no50s);
	m_experimentalMods.push_back(&cv::osu::stdrules::mod_no100s);
	m_experimentalMods.push_back(&cv::osu::stdrules::mod_ming3012);
	m_experimentalMods.push_back(&cv::osu::stdrules::mod_halfwindow);
	m_experimentalMods.push_back(&cv::osu::stdrules::mod_millhioref);
	m_experimentalMods.push_back(&cv::osu::stdrules::mod_mafham);
	m_experimentalMods.push_back(&cv::osu::mod_strict_tracking);
	m_experimentalMods.push_back(&cv::osu::playfield_mirror_horizontal);
	m_experimentalMods.push_back(&cv::osu::playfield_mirror_vertical);

	m_experimentalMods.push_back(&cv::osu::mod_wobble2);
	m_experimentalMods.push_back(&cv::osu::mod_shirone);
	m_experimentalMods.push_back(&cv::osu::mod_approach_different);

	// engine settings/overrides
	soundEngine->setOnOutputDeviceChange(fastdelegate::MakeDelegate(this, &Osu::onAudioOutputDeviceChange));

	env->setWindowTitle(PACKAGE_NAME);
	env->setCursorVisible(false);

	engine->getConsoleBox()->setRequireShiftToActivate(true);
	soundEngine->setVolume(cv::osu::volume_master.getFloat());
	mouse->addListener(this);

	cv::name.setValue("Guest");
	cv::console_overlay.setValue(0.0f);
	cv::vsync.setValue(0.0f);

	cv::snd_change_check_interval.setDefaultFloat(0.5f);
	cv::snd_change_check_interval.setValue(cv::snd_change_check_interval.getDefaultFloat());

	cv::osu::resolution.setValue(UString::format("%ix%i", engine->getScreenWidth(), engine->getScreenHeight()));

	// init steam rich presence localization
	if constexpr (Env::cfg(FEAT::STEAM))
	{
		steam->setRichPresence("steam_display", "#Status");
		steam->setRichPresence("status", "...");
	}

	// override BASS universal offset if soloud+bass are both available at once
	if constexpr (Env::cfg(AUD::SOLOUD))
		if (soundEngine->getTypeId() == SoundEngine::SOLOUD)
			cv::osu::universal_offset_hardcoded.setValue(15.0f);

	env->setWindowResizable(false);

	// generate default osu! appdata user path
	UString userDataPath = env->getUserDataPath();
	if (userDataPath.length() > 1)
	{
		UString defaultOsuFolder = userDataPath;
		defaultOsuFolder.append(Env::cfg(OS::WINDOWS) ? "osu!\\" : "osu!/");
		cv::osu::folder.setValue(defaultOsuFolder);
	}

	// convar callbacks
	cv::osu::skin.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSkinChange) );
	cv::osu::skin_reload.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSkinReload) );

	cv::osu::volume_master.setCallback( fastdelegate::MakeDelegate(this, &Osu::onMasterVolumeChange) );
	cv::osu::volume_music.setCallback( fastdelegate::MakeDelegate(this, &Osu::onMusicVolumeChange) );
	cv::osu::speed_override.setCallback( fastdelegate::MakeDelegate(this, &Osu::onSpeedChange) );
	cv::osu::pitch_override.setCallback( fastdelegate::MakeDelegate(this, &Osu::onPitchChange) );

	cv::osu::playfield_rotation.setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );
	cv::osu::playfield_stretch_x.setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );
	cv::osu::playfield_stretch_y.setCallback( fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange) );

	cv::osu::mods.setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) );

	cv::osu::resolution.setCallback( fastdelegate::MakeDelegate(this, &Osu::onInternalResolutionChanged) );
	cv::osu::ui_scale.setCallback( fastdelegate::MakeDelegate(this, &Osu::onUIScaleChange) );
	cv::osu::ui_scale_to_dpi.setCallback( fastdelegate::MakeDelegate(this, &Osu::onUIScaleToDPIChange) );
	cv::osu::letterboxing.setCallback( fastdelegate::MakeDelegate(this, &Osu::onLetterboxingChange) );
	cv::osu::letterboxing_offset_x.setCallback( fastdelegate::MakeDelegate(this, &Osu::onLetterboxingOffsetChange) );
	cv::osu::letterboxing_offset_y.setCallback( fastdelegate::MakeDelegate(this, &Osu::onLetterboxingOffsetChange) );

	cv::osu::confine_cursor_windowed.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorWindowedChange) );
	cv::osu::confine_cursor_fullscreen.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorFullscreenChange) );
	cv::osu::confine_cursor_never.setCallback( fastdelegate::MakeDelegate(this, &Osu::onConfineCursorNeverChange) );

	cv::osu::playfield_mirror_horizontal.setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) ); // force a mod update on OsuBeatmap if changed
	cv::osu::playfield_mirror_vertical.setCallback( fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate) ); // force a mod update on OsuBeatmap if changed

	cv::osu::notification.setCallback( fastdelegate::MakeDelegate(this, &Osu::onNotification) );

  	// vars
	m_skin = NULL;
	m_songBrowser2 = NULL;
	m_backgroundImageHandler = NULL;
	m_modSelector = NULL;
	m_updateHandler = NULL;
	m_multiplayer = NULL;
	m_bindings = new OsuKeyBindings();

	m_bF1 = false;
	m_bUIToggleCheck = false;
	m_bScoreboardToggleCheck = false;
	m_bEscape = false;
	m_bKeyboardKey1Down = false;
	m_bKeyboardKey12Down = false;
	m_bKeyboardKey2Down = false;
	m_bKeyboardKey22Down = false;
	m_bMouseKey1Down = false;
	m_bMouseKey2Down = false;
	m_bSkipDownCheck = false;
	m_bSkipScheduled = false;
	m_bQuickRetryDown = false;
	m_fQuickRetryTime = 0.0f;
	m_bSeeking = false;
	m_bSeekKey = false;
	m_fPrevSeekMousePosX = -1.0f;
	m_fQuickSaveTime = 0.0f;

	m_bToggleModSelectionScheduled = false;
	m_bToggleSongBrowserScheduled = false;
	m_bToggleOptionsMenuScheduled = false;
	m_bOptionsMenuFullscreen = true;
	m_bToggleRankingScreenScheduled = false;
	m_bToggleUserStatsScreenScheduled = false;
	m_bToggleChangelogScheduled = false;
	m_bToggleEditorScheduled = false;

	m_bModAuto = false;
	m_bModAutopilot = false;
	m_bModRelax = false;
	m_bModSpunout = false;
	m_bModTarget = false;
	m_bModScorev2 = false;
	m_bModDT = false;
	m_bModNC = false;
	m_bModNF = false;
	m_bModHT = false;
	m_bModDC = false;
	m_bModHD = false;
	m_bModHR = false;
	m_bModEZ = false;
	m_bModSD = false;
	m_bModSS = false;
	m_bModNM = false;
	m_bModTD = false;

	m_bShouldCursorBeVisible = false;

	m_gamemode = GAMEMODE::STD;
	m_bScheduleEndlessModNextBeatmap = false;
	m_iMultiplayerClientNumEscPresses = 0;
	m_bWasBossKeyPaused = false;
	m_bSkinLoadScheduled = false;
	m_bSkinLoadWasReload = false;
	m_skinScheduledToLoad = NULL;
	m_bFontReloadScheduled = false;
	m_bFireResolutionChangedScheduled = false;
	m_bVolumeInactiveToActiveScheduled = false;
	m_fVolumeInactiveToActiveAnim = 0.0f;
	m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled = false;

	// debug
	m_windowManager = new CWindowManager();
	/*
	DebugMonitor *dm = new DebugMonitor();
	m_windowManager->addWindow(dm);
	dm->open();
	*/

	// renderer
	g_vInternalResolution = engine->getScreenSize();

	m_backBuffer = resourceManager->createRenderTarget(0, 0, getVirtScreenWidth(), getVirtScreenHeight());
	m_playfieldBuffer = resourceManager->createRenderTarget(0, 0, 64, 64);
	m_sliderFrameBuffer = resourceManager->createRenderTarget(0, 0, getVirtScreenWidth(), getVirtScreenHeight());
	m_frameBuffer = resourceManager->createRenderTarget(0, 0, 64, 64);
	m_frameBuffer2 = resourceManager->createRenderTarget(0, 0, 64, 64);

	// load a few select subsystems very early
	m_notificationOverlay = new OsuNotificationOverlay();
	m_score = new OsuScore();
	m_updateHandler = new OsuUpdateHandler();

	// exec the main config file (this must be right here!)
	Console::execConfigFile("underride"); // same as override, but for defaults
	Console::execConfigFile("osu");
	Console::execConfigFile("override"); // used for quickfixing live builds without redeploying/recompiling

	// update mod settings
	updateMods();

	// load global resources
	const int baseDPI = 96;
	const int newDPI = Osu::getUIScale() * baseDPI;

	McFont *defaultFont = resourceManager->loadFont("weblysleekuisb.ttf", "FONT_DEFAULT", 15, true, newDPI);
	m_titleFont = resourceManager->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_TITLE", 60, true, newDPI);
	m_subTitleFont = resourceManager->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_SUBTITLE", 21, true, newDPI);
	m_songBrowserFont = resourceManager->loadFont("SourceSansPro-Regular.otf", "FONT_OSU_SONGBROWSER", 35, true, newDPI);
	m_songBrowserFontBold = resourceManager->loadFont("SourceSansPro-Bold.otf", "FONT_OSU_SONGBROWSER_BOLD", 30, true, newDPI);
	m_fontIcons = resourceManager->loadFont("fontawesome-webfont.ttf", "FONT_OSU_ICONS", OsuIcons::icons, 26, true, newDPI);

	m_fonts.push_back(defaultFont);
	m_fonts.push_back(m_titleFont);
	m_fonts.push_back(m_subTitleFont);
	m_fonts.push_back(m_songBrowserFont);
	m_fonts.push_back(m_songBrowserFontBold);
	m_fonts.push_back(m_fontIcons);

	float averageIconHeight = 0.0f;
	for (int i=0; i<OsuIcons::icons.size(); i++)
	{
		UString iconString; iconString.insert(0, OsuIcons::icons[i]);
		const float height = m_fontIcons->getStringHeight(iconString);
		if (height > averageIconHeight)
			averageIconHeight = height;
	}
	m_fontIcons->setHeight(averageIconHeight);

	if (defaultFont->getDPI() != newDPI)
	{
		debugLog("default dpi {} newDPI {}\n", defaultFont->getDPI(), newDPI);
		m_bFontReloadScheduled = true;
		m_bFireResolutionChangedScheduled = true;
	}

	// load skin
	{
		UString skinFolder = cv::osu::folder.getString();
		skinFolder.append(cv::osu::folder_sub_skins.getString());
		skinFolder.append(cv::osu::skin.getString());
		skinFolder.append("/");
		if (m_skin == NULL) // the skin may already be loaded by Console::execConfigFile() above
			onSkinChange("", cv::osu::skin.getString());

		// enable async skin loading for user-action skin changes (but not during startup)
		cv::osu::skin_async.setValue(1.0f);
	}

	// load subsystems, add them to the screens array
	m_tooltipOverlay = new OsuTooltipOverlay();
	m_multiplayer = new OsuMultiplayer();
	m_mainMenu = new OsuMainMenu();
	m_optionsMenu = new OsuOptionsMenu();
	m_songBrowser2 = new OsuSongBrowser2();
	m_backgroundImageHandler = new OsuBackgroundImageHandler();
	m_modSelector = new OsuModSelector();
	m_rankingScreen = new OsuRankingScreen();
	m_userStatsScreen = new OsuUserStatsScreen();
	m_pauseMenu = new OsuPauseMenu();
	m_hud = new OsuHUD();
	m_changelog = new OsuChangelog();
	m_editor = new OsuEditor();
	if constexpr (Env::cfg(FEAT::STEAM))
		m_steamWorkshop = new OsuSteamWorkshop();
	m_fposu = new OsuModFPoSu();

	// the order in this vector will define in which order events are handled/consumed
	m_screens.push_back(m_notificationOverlay);
	m_screens.push_back(m_optionsMenu);
	m_screens.push_back(m_userStatsScreen);
	m_screens.push_back(m_rankingScreen);
	m_screens.push_back(m_modSelector);
	m_screens.push_back(m_pauseMenu);
	m_screens.push_back(m_hud);
	m_screens.push_back(m_songBrowser2);
	m_screens.push_back(m_changelog);
	m_screens.push_back(m_editor);
	m_screens.push_back(m_mainMenu);
	m_screens.push_back(m_tooltipOverlay);

	// make primary screen visible
	//m_optionsMenu->setVisible(true);
	//m_modSelector->setVisible(true);
	//m_songBrowser2->setVisible(true);
	//m_pauseMenu->setVisible(true);
	//m_rankingScreen->setVisible(true);
	//m_changelog->setVisible(true);
	//m_editor->setVisible(true);
	//m_userStatsScreen->setVisible(true);

	m_mainMenu->setVisible(true);

	m_updateHandler->checkForUpdates();

	/*
	// DEBUG: immediately start diff of a beatmap
	{
		UString debugFolder = "S:/GAMES/osu!/Songs/41823 The Quick Brown Fox - The Big Black/";
		UString debugDiffFileName = "The Quick Brown Fox - The Big Black (Blue Dragon) [WHO'S AFRAID OF THE BIG BLACK].osu";

		UString beatmapPath = debugFolder;
		beatmapPath.append(debugDiffFileName);

		OsuDatabaseBeatmap *debugDiff = new OsuDatabaseBeatmap(this, beatmapPath, debugFolder);

		m_songBrowser2->onDifficultySelected(debugDiff, true);
		//cv::osu::volume_master.setValue(1.0f);
		// WARNING: this will leak memory (one OsuDatabaseBeatmap object), but who cares (since debug only)
	}
	*/

	// memory/performance optimization; if osu_mod_mafham is not enabled, reduce the two rendertarget sizes to 64x64, same for fposu (and fposu_3d, and fposu_3d_spheres, and fposu_3d_spheres_aa)
	cv::osu::stdrules::mod_mafham.setCallback( fastdelegate::MakeDelegate(this, &Osu::onModMafhamChange) );
	cv::osu::fposu::mod_fposu.setCallback( fastdelegate::MakeDelegate(this, &Osu::onModFPoSuChange) );
	cv::osu::fposu::threeD.setCallback( fastdelegate::MakeDelegate(this, &Osu::onModFPoSu3DChange) );
	cv::osu::fposu::threeD_spheres.setCallback( fastdelegate::MakeDelegate(this, &Osu::onModFPoSu3DSpheresChange) );
	cv::osu::fposu::threeD_spheres_aa.setCallback( fastdelegate::MakeDelegate(this, &Osu::onModFPoSu3DSpheresAAChange) );
}

Osu::~Osu()
{
	// "leak" OsuUpdateHandler object, but not relevant since shutdown:
	// this is the only way of handling instant user shutdown requests properly, there is no solution for active working threads besides letting the OS kill them when the main threads exits.
	// we must not delete the update handler object, because the thread is potentially still accessing members during shutdown
	m_updateHandler->stop(); // tell it to stop at the next cancellation point, depending on the OS/runtime and engine shutdown time it may get killed before that

	SAFE_DELETE(m_windowManager);

	for (int i=0; i<m_screens.size(); i++)
	{
		debugLog("{}\n", i);
		SAFE_DELETE(m_screens[i]);
	}

	if constexpr (Env::cfg(FEAT::STEAM))
		SAFE_DELETE(m_steamWorkshop);
	SAFE_DELETE(m_fposu);

	SAFE_DELETE(m_updateHandler);
	SAFE_DELETE(m_score);
	SAFE_DELETE(m_multiplayer);
	SAFE_DELETE(m_skin);
	SAFE_DELETE(m_backgroundImageHandler);
	SAFE_DELETE(m_bindings);
}

void Osu::draw()
{
	if (m_skin == NULL) // sanity check
	{
		g->setColor(0xffff0000);
		g->fillRect(0, 0, getVirtScreenWidth(), getVirtScreenHeight());
		return;
	}

	// if we are not using the native window resolution, draw into the buffer
	const bool isBufferedDraw = cv::osu::resolution_enabled.getBool();

	if (isBufferedDraw)
		m_backBuffer->enable();

	// draw everything in the correct order
	if (isInPlayMode()) // if we are playing a beatmap
	{
		const bool isFPoSu = (cv::osu::fposu::mod_fposu.getBool());
		const bool isFPoSu3d = (isFPoSu && cv::osu::fposu::threeD.getBool());

		const bool isBufferedPlayfieldDraw = (isFPoSu && !isFPoSu3d);

		if (isBufferedPlayfieldDraw)
			m_playfieldBuffer->enable();

		if (!isFPoSu3d)
			getSelectedBeatmap()->draw();
		else
		{
			m_playfieldBuffer->enable();
			{
				getSelectedBeatmap()->drawInt();
			}
			m_playfieldBuffer->disable();

			m_fposu->draw();
		}

		if (!isFPoSu || isFPoSu3d)
			m_hud->draw();

		// quick retry fadeout overlay
		if (m_fQuickRetryTime != 0.0f && m_bQuickRetryDown)
		{
			float alphaPercent = 1.0f - (m_fQuickRetryTime - engine->getTime())/cv::osu::quick_retry_delay.getFloat();
			if (engine->getTime() > m_fQuickRetryTime)
				alphaPercent = 1.0f;

			g->setColor(argb((Channel)(255*alphaPercent), 0, 0, 0));
			g->fillRect(0, 0, getVirtScreenWidth(), getVirtScreenHeight());
		}

		// special cursor handling (fading cursor + invisible cursor mods + draw order etc.)
		const bool isAuto = (m_bModAuto || m_bModAutopilot);
		const bool allowDoubleCursor = isFPoSu;
		const bool allowDrawCursor = (!cv::osu::hide_cursor_during_gameplay.getBool() || getSelectedBeatmap()->isPaused());
		float fadingCursorAlpha = 1.0f - std::clamp<float>((float)m_score->getCombo()/cv::osu::mod_fadingcursor_combo.getFloat(), 0.0f, 1.0f);
		if (m_pauseMenu->isVisible() || getSelectedBeatmap()->isContinueScheduled())
			fadingCursorAlpha = 1.0f;

		const auto *beatmap = getSelectedBeatmap();
		const OsuBeatmapStandard* beatmapStd = beatmap ? beatmap->asStd() : nullptr;

		// draw auto cursor
		if (isAuto && allowDrawCursor && !isFPoSu && beatmapStd != nullptr && !beatmapStd->isLoading())
			m_hud->drawCursor(cv::osu::stdrules::mod_fps.getBool() ? OsuGameRules::getPlayfieldCenter() : beatmapStd->getCursorPos(), cv::osu::mod_fadingcursor.getBool() ? fadingCursorAlpha : 1.0f);

		m_pauseMenu->draw();
		m_modSelector->draw();
		m_optionsMenu->draw();

		if (cv::osu::draw_fps.getBool() && (!isFPoSu || isFPoSu3d))
			m_hud->drawFps();

		m_hud->drawVolumeChange();

		m_windowManager->draw();

		if (isFPoSu && !isFPoSu3d && cv::osu::draw_cursor_ripples.getBool())
			m_hud->drawCursorRipples();

		// draw FPoSu cursor trail
		if (isFPoSu && !isFPoSu3d && cv::osu::fposu::draw_cursor_trail.getBool())
			m_hud->drawCursorTrail(beatmapStd->getCursorPos(), cv::osu::mod_fadingcursor.getBool() ? fadingCursorAlpha : 1.0f);

		if (isBufferedPlayfieldDraw)
			m_playfieldBuffer->disable();

		if (isFPoSu && !isFPoSu3d)
		{
			m_fposu->draw();
			m_hud->draw();

			if (cv::osu::draw_fps.getBool())
				m_hud->drawFps();
		}

		// draw player cursor
		if ((!isAuto || allowDoubleCursor) && allowDrawCursor)
		{
			Vector2 cursorPos = (beatmapStd != NULL && !isAuto) ? beatmapStd->getCursorPos() : mouse->getPos();

			if (isFPoSu && (!isFPoSu3d || ((isAuto && !getSelectedBeatmap()->isPaused()) || (!getSelectedBeatmap()->isPaused() && !m_optionsMenu->isVisible() && !m_modSelector->isVisible()))))
				cursorPos = getVirtScreenSize() / 2.0f;

			const bool updateAndDrawTrail = !isFPoSu;

			m_hud->drawCursor(cursorPos, (cv::osu::mod_fadingcursor.getBool() && !isAuto) ? fadingCursorAlpha : 1.0f, isAuto, updateAndDrawTrail);
		}
	}
	else // if we are not playing
	{
		if (m_songBrowser2 != NULL)
			m_songBrowser2->draw();

		m_modSelector->draw();
		m_mainMenu->draw();
		m_changelog->draw();
		m_editor->draw();
		m_userStatsScreen->draw();
		m_rankingScreen->draw();
		m_optionsMenu->draw();

		if (isInMultiplayer())
			m_hud->drawScoreBoardMP();

		if (cv::osu::draw_fps.getBool())
			m_hud->drawFps();

		m_hud->drawVolumeChange();

		m_windowManager->draw();
		m_hud->drawCursor(mouse->getPos());
	}

	m_tooltipOverlay->draw();
	m_notificationOverlay->draw();

	// loading spinner for some async tasks
	if ((m_bSkinLoadScheduled && m_skin != m_skinScheduledToLoad) || (Env::cfg(FEAT::STEAM) && (m_optionsMenu->isWorkshopLoading() || m_steamWorkshop->isUploading())))
	{
		m_hud->drawLoadingSmall();
	}

	// if we are not using the native window resolution;
	if (isBufferedDraw)
	{
		// draw a scaled version from the buffer to the screen
		m_backBuffer->disable();

		Vector2 offset = Vector2(g->getResolution().x/2 - g_vInternalResolution.x/2, g->getResolution().y/2 - g_vInternalResolution.y/2);

		g->setBlending(false);
		{
			if (cv::osu::letterboxing.getBool())
				m_backBuffer->draw(offset.x*(1.0f + cv::osu::letterboxing_offset_x.getFloat()), offset.y*(1.0f + cv::osu::letterboxing_offset_y.getFloat()), g_vInternalResolution.x, g_vInternalResolution.y);
			else
			{
				if (cv::osu::resolution_keep_aspect_ratio.getBool())
				{
					const float scale = getImageScaleToFitResolution(m_backBuffer->getSize(), g->getResolution());
					const float scaledWidth = m_backBuffer->getWidth()*scale;
					const float scaledHeight = m_backBuffer->getHeight()*scale;
					m_backBuffer->draw(std::max(0.0f, g->getResolution().x/2.0f - scaledWidth/2.0f)*(1.0f + cv::osu::letterboxing_offset_x.getFloat()), std::max(0.0f, g->getResolution().y/2.0f - scaledHeight/2.0f)*(1.0f + cv::osu::letterboxing_offset_y.getFloat()), scaledWidth, scaledHeight);
				}
				else
					m_backBuffer->draw(0, 0, g->getResolution().x, g->getResolution().y);
			}
		}
		g->setBlending(true);
	}
}

void Osu::update()
{
	const int wheelDelta = mouse->getWheelDeltaVertical(); // HACKHACK: songbrowser focus

	if (m_skin != NULL)
		m_skin->update();

	if (isInPlayMode() && cv::osu::fposu::mod_fposu.getBool())
		m_fposu->update();

	m_windowManager->update();

	for (int i=0; i<m_screens.size(); i++)
	{
		m_screens[i]->update();
	}

	// main beatmap update
	m_bSeeking = false;
	if (isInPlayMode())
	{
		getSelectedBeatmap()->update();

		// NOTE: force keep loaded background images while playing
		m_backgroundImageHandler->scheduleFreezeCache();

		// scrubbing/seeking
		if (m_bSeekKey)
		{
			if (!isInMultiplayer() || m_multiplayer->isServer())
			{
				m_bSeeking = true;
				const float mousePosX = (int)mouse->getPos().x;
				const float percent = std::clamp<float>(mousePosX / (float)getVirtScreenWidth(), 0.0f, 1.0f);

				if (mouse->isLeftDown())
				{
					if (mousePosX != m_fPrevSeekMousePosX || !cv::osu::scrubbing_smooth.getBool())
					{
						m_fPrevSeekMousePosX = mousePosX;

						// special case: allow cancelling the failing animation here
						if (getSelectedBeatmap()->hasFailed())
							getSelectedBeatmap()->cancelFailing();

						getSelectedBeatmap()->seekPercentPlayable(percent);
					}
					else
					{
						// special case: keep player invulnerable even if scrubbing position does not change
						getSelectedBeatmap()->resetScore();
					}
				}
				else
					m_fPrevSeekMousePosX = -1.0f;

				if (mouse->isRightDown())
					m_fQuickSaveTime = std::clamp<float>((float)((getSelectedBeatmap()->getStartTimePlayable()+getSelectedBeatmap()->getLengthPlayable())*percent) / (float)getSelectedBeatmap()->getLength(), 0.0f, 1.0f);
			}
		}

		// skip button clicking
		if (getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused() && !m_bSeeking && !m_hud->isVolumeOverlayBusy())
		{
			const bool isAnyOsuKeyDown = (m_bKeyboardKey1Down || m_bKeyboardKey12Down || m_bKeyboardKey2Down || m_bKeyboardKey22Down || m_bMouseKey1Down || m_bMouseKey2Down);
			const bool isAnyKeyDown = (isAnyOsuKeyDown || mouse->isLeftDown());

			if (isAnyKeyDown)
			{
				if (!m_bSkipDownCheck)
				{
					m_bSkipDownCheck = true;

					const bool isCursorInsideSkipButton = m_hud->getSkipClickRect().contains(mouse->getPos());

					if (isCursorInsideSkipButton)
						m_bSkipScheduled = true;
				}
			}
			else
				m_bSkipDownCheck = false;
		}
		else
			m_bSkipDownCheck = false;

		// skipping
		if (m_bSkipScheduled)
		{
			const bool isLoading = getSelectedBeatmap()->isLoading();

			if (getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused() && !isLoading)
			{
				if (!isInMultiplayer() || m_multiplayer->isServer())
				{
					if ((cv::osu::skip_intro_enabled.getBool() && getSelectedBeatmap()->getHitObjectIndexForCurrentTime() < 1) || (cv::osu::skip_breaks_enabled.getBool() && getSelectedBeatmap()->getHitObjectIndexForCurrentTime() > 0))
					{
						m_multiplayer->onServerPlayStateChange(OsuMultiplayer::STATE::SKIP);

						getSelectedBeatmap()->skipEmptySection();
					}
				}
			}

			if (!isLoading)
				m_bSkipScheduled = false;
		}

		// quick retry timer
		if (m_bQuickRetryDown && m_fQuickRetryTime != 0.0f && engine->getTime() > m_fQuickRetryTime)
		{
			m_fQuickRetryTime = 0.0f;

			if (!isInMultiplayer() || m_multiplayer->isServer())
			{
				getSelectedBeatmap()->restart(true);
				getSelectedBeatmap()->update();
				m_pauseMenu->setVisible(false);
			}
		}
	}

	// background image cache tick
	m_backgroundImageHandler->update(m_songBrowser2->isVisible()); // NOTE: must be before the asynchronous ui toggles due to potential 1-frame unloads after invisible songbrowser

	// asynchronous ui toggles
	// TODO: this is cancer, why did I even write this section
	if (m_bToggleModSelectionScheduled)
	{
		m_bToggleModSelectionScheduled = false;

		if (!isInPlayMode())
		{
			if (m_songBrowser2 != NULL)
				m_songBrowser2->setVisible(m_modSelector->isVisible());
		}

		m_modSelector->setVisible(!m_modSelector->isVisible());
	}
	if (m_bToggleSongBrowserScheduled)
	{
		m_bToggleSongBrowserScheduled = false;

		if (m_userStatsScreen->isVisible())
			m_userStatsScreen->setVisible(false);

		if (m_mainMenu->isVisible() && m_optionsMenu->isVisible())
			m_optionsMenu->setVisible(false);

		if (m_songBrowser2 != NULL)
			m_songBrowser2->setVisible(!m_songBrowser2->isVisible());

		m_mainMenu->setVisible(!(m_songBrowser2 != NULL && m_songBrowser2->isVisible()));
		updateConfineCursor();
	}
	if (m_bToggleOptionsMenuScheduled)
	{
		m_bToggleOptionsMenuScheduled = false;

		const bool fullscreen = false;
		const bool wasFullscreen = m_optionsMenu->isFullscreen();

		m_optionsMenu->setFullscreen(fullscreen);
		m_optionsMenu->setVisible(!m_optionsMenu->isVisible());
		if (fullscreen || wasFullscreen)
			m_mainMenu->setVisible(!m_optionsMenu->isVisible());
	}
	if (m_bToggleRankingScreenScheduled)
	{
		m_bToggleRankingScreenScheduled = false;

		m_rankingScreen->setVisible(!m_rankingScreen->isVisible());
		if (m_songBrowser2 != NULL)
			m_songBrowser2->setVisible(!m_rankingScreen->isVisible());
	}
	if (m_bToggleUserStatsScreenScheduled)
	{
		m_bToggleUserStatsScreenScheduled = false;

		m_userStatsScreen->setVisible(true);

		if (m_songBrowser2 != NULL && m_songBrowser2->isVisible())
			m_songBrowser2->setVisible(false);
	}
	if (m_bToggleChangelogScheduled)
	{
		m_bToggleChangelogScheduled = false;

		m_mainMenu->setVisible(!m_mainMenu->isVisible());
		m_changelog->setVisible(!m_mainMenu->isVisible());
	}
	if (m_bToggleEditorScheduled)
	{
		m_bToggleEditorScheduled = false;

		m_mainMenu->setVisible(!m_mainMenu->isVisible());
		m_editor->setVisible(!m_mainMenu->isVisible());
	}
	// HACKHACK: workaround for the garbage code above, the 1-frame delayed screen visibility changes can cause players to extremely rarely get stuck on a black screen without anything visible anymore (just because of quick menu navigation skills)
	if (!isInPlayMode())
	{
		int numVisibleScreens = 0;
		for (const OsuScreen *screen : m_screens)
		{
			if (screen->isVisible())
				numVisibleScreens++;
		}
		if (numVisibleScreens < 1)
		{
			if (m_fPrevNoScreenVisibleWorkaroundTime == 0.0f)
				m_fPrevNoScreenVisibleWorkaroundTime = engine->getTime();

			if (engine->getTime() > m_fPrevNoScreenVisibleWorkaroundTime + 2.0f)
			{
				m_fPrevNoScreenVisibleWorkaroundTime = 0.0f;
				m_mainMenu->setVisible(true);
			}
		}
		else
			m_fPrevNoScreenVisibleWorkaroundTime = 0.0f;
	}
	else
		m_fPrevNoScreenVisibleWorkaroundTime = 0.0f;

	// handle cursor visibility if outside of internal resolution
	// TODO: not a critical bug, but the real cursor gets visible way too early if sensitivity is > 1.0f, due to this using scaled/offset getMouse()->getPos()
	if (cv::osu::resolution_enabled.getBool())
	{
		McRect internalWindow = McRect(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
		bool cursorVisible = env->isCursorVisible();
		if (!internalWindow.contains(mouse->getPos()) && !env->isCursorClipped())
		{
			if (!cursorVisible)
				env->setCursorVisible(true);
		}
		else
		{
			if (cursorVisible != m_bShouldCursorBeVisible)
				env->setCursorVisible(m_bShouldCursorBeVisible);
		}
	}

	// handle mousewheel volume change
	if ((m_songBrowser2 != NULL && (!m_songBrowser2->isVisible() || keyboard->isAltDown() || m_hud->isVolumeOverlayBusy()))
			&& (!m_optionsMenu->isVisible() || !m_optionsMenu->isMouseInside() || keyboard->isAltDown())
			&& (!m_userStatsScreen->isVisible() || keyboard->isAltDown() || m_hud->isVolumeOverlayBusy())
			&& (!m_changelog->isVisible() || keyboard->isAltDown())
			&& (!m_modSelector->isMouseInScrollView() || keyboard->isAltDown()))
	{
		if ((!(isInPlayMode() && !m_pauseMenu->isVisible()) && !m_rankingScreen->isVisible()) || (isInPlayMode() && !cv::osu::disable_mousewheel.getBool()) || keyboard->isAltDown())
		{
			if (wheelDelta != 0)
			{
				const int multiplier = std::max(1, std::abs(wheelDelta) / 120);

				if (wheelDelta > 0)
					volumeUp(multiplier);
				else
					volumeDown(multiplier);
			}
		}
	}
	// endless mod
	if (m_bScheduleEndlessModNextBeatmap)
	{
		m_bScheduleEndlessModNextBeatmap = false;
		m_songBrowser2->playNextRandomBeatmap();
	}

	// multiplayer update
	m_multiplayer->update();

	// skin async loading
	if (m_bSkinLoadScheduled)
	{
		if (m_skinScheduledToLoad != NULL && m_skinScheduledToLoad->isReady())
		{
			m_bSkinLoadScheduled = false;

			if (m_skin != m_skinScheduledToLoad)
				SAFE_DELETE(m_skin);

			m_skin = m_skinScheduledToLoad;

			m_skinScheduledToLoad = NULL;

			// force layout update after all skin elements have been loaded
			fireResolutionChanged();

			// notify if done after reload
			if (m_bSkinLoadWasReload)
			{
				m_bSkinLoadWasReload = false;

				m_notificationOverlay->addNotification(m_skin->getName().length() > 0 ? UString::format("Skin reloaded! (%s)", m_skin->getName().toUtf8()) : "Skin reloaded!", 0xffffffff, false, 0.75f);
			}
		}
	}

	// volume inactive to active animation
	if (m_bVolumeInactiveToActiveScheduled && m_fVolumeInactiveToActiveAnim > 0.0f)
	{
		soundEngine->setVolume(std::lerp(cv::osu::volume_master_inactive.getFloat() * cv::osu::volume_master.getFloat(), cv::osu::volume_master.getFloat(), m_fVolumeInactiveToActiveAnim));

		// check if we're done
		if (m_fVolumeInactiveToActiveAnim == 1.0f)
			m_bVolumeInactiveToActiveScheduled = false;
	}

	// (must be before m_bFontReloadScheduled and m_bFireResolutionChangedScheduled are handled!)
	if (m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled)
	{
		m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled = false;

		m_bFontReloadScheduled = true;
		m_bFireResolutionChangedScheduled = true;
	}

	// delayed font reloads (must be before layout updates!)
	if (m_bFontReloadScheduled)
	{
		m_bFontReloadScheduled = false;
		reloadFonts();
	}

	// delayed layout updates
	if (m_bFireResolutionChangedScheduled)
	{
		m_bFireResolutionChangedScheduled = false;
		fireResolutionChanged();
	}
}

void Osu::updateMods()
{
	debugLog("\n");

	m_bModAuto = cv::osu::mods.getString().find("auto") != -1;
	m_bModAutopilot = cv::osu::mods.getString().find("autopilot") != -1;
	m_bModRelax = cv::osu::mods.getString().find("relax") != -1;
	m_bModSpunout = cv::osu::mods.getString().find("spunout") != -1;
	m_bModTarget = cv::osu::mods.getString().find("practicetarget") != -1;
	m_bModScorev2 = cv::osu::mods.getString().find("v2") != -1;
	m_bModDT = cv::osu::mods.getString().find("dt") != -1;
	m_bModNC = cv::osu::mods.getString().find("nc") != -1;
	m_bModNF = cv::osu::mods.getString().find("nf") != -1;
	m_bModHT = cv::osu::mods.getString().find("ht") != -1;
	m_bModDC = cv::osu::mods.getString().find("dc") != -1;
	m_bModHD = cv::osu::mods.getString().find("hd") != -1;
	m_bModHR = cv::osu::mods.getString().find("hr") != -1;
	m_bModEZ = cv::osu::mods.getString().find("ez") != -1;
	m_bModSD = cv::osu::mods.getString().find("sd") != -1;
	m_bModSS = cv::osu::mods.getString().find("ss") != -1;
	m_bModNM = cv::osu::mods.getString().find("nm") != -1;
	m_bModTD = cv::osu::mods.getString().find("nerftd") != -1;

	// static overrides
	onSpeedChange("", cv::osu::speed_override.getString());
	onPitchChange("", cv::osu::pitch_override.getString());

	// autopilot overrides auto
	if (m_bModAutopilot)
		m_bModAuto = false;

	// handle auto/pilot cursor visibility
	if (!m_bModAuto && !m_bModAutopilot)
	{
		m_bShouldCursorBeVisible = false;
		env->setCursorVisible(m_bShouldCursorBeVisible);
	}
	else if (isInPlayMode())
	{
		m_bShouldCursorBeVisible = true;
		env->setCursorVisible(m_bShouldCursorBeVisible);
	}

	// handle windows key disable/enable
	updateWindowsKeyDisable();

	// notify the possibly running beatmap of mod changes, for e.g. recalculating stacks dynamically if HR is toggled
	{
		if (getSelectedBeatmap() != NULL)
			getSelectedBeatmap()->onModUpdate();

		if (m_songBrowser2 != NULL)
			m_songBrowser2->recalculateStarsForSelectedBeatmap(true);
	}
}

void Osu::onKeyDown(KeyboardEvent &key)
{
	// global hotkeys

	// special hotkeys
	// reload & recompile shaders
	if (keyboard->isAltDown())
	{
		if (keyboard->isControlDown())
		{
			switch (static_cast<KEYCODE>(key))
			{
			case KEY_R: {
				resourceManager->reloadResource(resourceManager->getShader("slider"));
				resourceManager->reloadResource(resourceManager->getShader("cursortrail"));
				resourceManager->reloadResource(resourceManager->getShader("hitcircle3D"));
				break;
			}
			case KEY_S: {
				onSkinReload();
				break;
			}
			}
			key.consume();
		}
		// arrow keys volume (alt)
		else if (key == cv::osu::keybinds::INCREASE_VOLUME.getVal<KEYCODE>())
		{
			volumeUp();
			key.consume();
		}
		else if (key == cv::osu::keybinds::DECREASE_VOLUME.getVal<KEYCODE>())
		{
			volumeDown();
			key.consume();
		}
	}

	// disable mouse buttons hotkey
	if (key == cv::osu::keybinds::DISABLE_MOUSE_BUTTONS.getVal<KEYCODE>())
	{
		if (cv::osu::disable_mousebuttons.getBool())
		{
			cv::osu::disable_mousebuttons.setValue(0.0f);
			m_notificationOverlay->addNotification("Mouse buttons are enabled.");
		}
		else
		{
			cv::osu::disable_mousebuttons.setValue(1.0f);
			m_notificationOverlay->addNotification("Mouse buttons are disabled.");
		}
	}
	// screenshots
	else if (key == cv::osu::keybinds::SAVE_SCREENSHOT.getVal<KEYCODE>())
		saveScreenshot();
	// boss key (minimize + mute)
	else if (key == cv::osu::keybinds::BOSS_KEY.getVal<KEYCODE>())
	{
		env->minimize();
		if (getSelectedBeatmap() != NULL)
		{
			m_bWasBossKeyPaused = getSelectedBeatmap()->isPreviewMusicPlaying();
			getSelectedBeatmap()->pausePreviewMusic(false);
		}
	}

	// local hotkeys (and gameplay keys)

	// while playing (and not in options)
	if (isInPlayMode() && !m_optionsMenu->isVisible())
	{
		// while playing and not paused
		if (!getSelectedBeatmap()->isPaused())
		{
			getSelectedBeatmap()->onKeyDown(key);

			// K1
			{
				const bool isKeyLeftClick = (key == cv::osu::keybinds::LEFT_CLICK.getVal<KEYCODE>());
				const bool isKeyLeftClick2 = (key == cv::osu::keybinds::LEFT_CLICK_2.getVal<KEYCODE>());
				if ((!m_bKeyboardKey1Down && isKeyLeftClick) || (!m_bKeyboardKey12Down && isKeyLeftClick2))
				{
					if (isKeyLeftClick2)
						m_bKeyboardKey12Down = true;
					else
						m_bKeyboardKey1Down = true;

					onKey1Change(true, false);

					if (!getSelectedBeatmap()->hasFailed())
						key.consume();
				}
				else if (isKeyLeftClick || isKeyLeftClick2)
				{
					if (!getSelectedBeatmap()->hasFailed())
						key.consume();
				}
			}

			// K2
			{
				const bool isKeyRightClick = (key == cv::osu::keybinds::RIGHT_CLICK.getVal<KEYCODE>());
				const bool isKeyRightClick2 = (key == cv::osu::keybinds::RIGHT_CLICK_2.getVal<KEYCODE>());
				if ((!m_bKeyboardKey2Down && isKeyRightClick) || (!m_bKeyboardKey22Down && isKeyRightClick2))
				{
					if (isKeyRightClick2)
						m_bKeyboardKey22Down = true;
					else
						m_bKeyboardKey2Down = true;

					onKey2Change(true, false);

					if (!getSelectedBeatmap()->hasFailed())
						key.consume();
				}
				else if (isKeyRightClick || isKeyRightClick2)
				{
					if (!getSelectedBeatmap()->hasFailed())
						key.consume();
				}
			}

			// handle skipping
			if (key == KEY_ENTER || key == KEY_NUMPAD_ENTER || key == cv::osu::keybinds::SKIP_CUTSCENE.getVal<KEYCODE>())
				m_bSkipScheduled = true;

			// toggle ui
			if (!key.isConsumed() && key == cv::osu::keybinds::TOGGLE_SCOREBOARD.getVal<KEYCODE>() && !m_bScoreboardToggleCheck)
			{
				m_bScoreboardToggleCheck = true;

				if (keyboard->isShiftDown())
				{
					if (!m_bUIToggleCheck)
					{
						m_bUIToggleCheck = true;
						cv::osu::draw_hud.setValue(!cv::osu::draw_hud.getBool());
						m_notificationOverlay->addNotification(cv::osu::draw_hud.getBool() ? "In-game interface has been enabled." : "In-game interface has been disabled.", 0xffffffff, false, 0.1f);

						key.consume();
					}
				}
				else
				{
					cv::osu::draw_scoreboard.setValue(!cv::osu::draw_scoreboard.getBool());
					m_notificationOverlay->addNotification(cv::osu::draw_scoreboard.getBool() ? "Scoreboard is shown." : "Scoreboard is hidden.", 0xffffffff, false, 0.1f);

					key.consume();
				}
			}

			// allow live mod changing while playing
			if (!key.isConsumed()
				&& (key == KEY_F1 || key == cv::osu::keybinds::TOGGLE_MODSELECT.getVal<KEYCODE>())
				&& ((KEY_F1 != cv::osu::keybinds::LEFT_CLICK.getVal<KEYCODE>() && KEY_F1 != cv::osu::keybinds::LEFT_CLICK_2.getVal<KEYCODE>()) || (!m_bKeyboardKey1Down && !m_bKeyboardKey12Down))
				&& ((KEY_F1 != cv::osu::keybinds::RIGHT_CLICK.getVal<KEYCODE>() && KEY_F1 != cv::osu::keybinds::RIGHT_CLICK_2.getVal<KEYCODE>() ) || (!m_bKeyboardKey2Down && !m_bKeyboardKey22Down))
				&& !m_bF1
				&& !getSelectedBeatmap()->hasFailed()) // only if not failed though
			{
				m_bF1 = true;
				toggleModSelection(true);
			}

			// quick save/load
			if (!isInMultiplayer() || m_multiplayer->isServer())
			{
				if (key == cv::osu::keybinds::QUICK_SAVE.getVal<KEYCODE>())
					m_fQuickSaveTime = getSelectedBeatmap()->getPercentFinished();

				if (key == cv::osu::keybinds::QUICK_LOAD.getVal<KEYCODE>())
				{
					// special case: allow cancelling the failing animation here
					if (getSelectedBeatmap()->hasFailed())
						getSelectedBeatmap()->cancelFailing();

					getSelectedBeatmap()->seekPercent(m_fQuickSaveTime);
				}
			}

			// quick seek
			if (!isInMultiplayer() || m_multiplayer->isServer())
			{
				const bool backward = (key == cv::osu::keybinds::SEEK_TIME_BACKWARD.getVal<KEYCODE>());
				const bool forward = (key == cv::osu::keybinds::SEEK_TIME_FORWARD.getVal<KEYCODE>());

				if (backward || forward)
				{
					const bool isVolumeOverlayVisibleOrBusy = (m_hud->isVolumeOverlayVisible() || m_hud->isVolumeOverlayBusy());

					if (!isVolumeOverlayVisibleOrBusy)
					{
						const unsigned long lengthMS = getSelectedBeatmap()->getLength();
						const float percentFinished = getSelectedBeatmap()->getPercentFinished();

						if (lengthMS > 0)
						{
							double seekedPercent = 0.0;
							if (backward)
								seekedPercent -= cv::osu::seek_delta.getVal<double>() * (1.0 / (double)lengthMS) * 1000.0;
							else if (forward)
								seekedPercent += cv::osu::seek_delta.getVal<double>() * (1.0 / (double)lengthMS) * 1000.0;

							if (seekedPercent != 0.0f)
							{
								// special case: allow cancelling the failing animation here
								if (getSelectedBeatmap()->hasFailed())
									getSelectedBeatmap()->cancelFailing();

								getSelectedBeatmap()->seekPercent(percentFinished + seekedPercent);
							}
						}
					}
				}
			}
		}

		// while paused or maybe not paused

		// handle quick restart
		if (((key == cv::osu::keybinds::QUICK_RETRY.getVal<KEYCODE>() || (keyboard->isControlDown() && !keyboard->isAltDown() && key == KEY_R)) && !m_bQuickRetryDown))
		{
			m_bQuickRetryDown = true;
			m_fQuickRetryTime = engine->getTime() + cv::osu::quick_retry_delay.getFloat();
		}

		// handle seeking
		if (key == cv::osu::keybinds::SEEK_TIME.getVal<KEYCODE>())
			m_bSeekKey = true;

		// handle fposu key handling
		m_fposu->onKeyDown(key);
	}

	// forward to all subsystem, if not already consumed
	for (auto & screen : m_screens)
	{
		if (key.isConsumed())
			break;

		screen->onKeyDown(key);
	}

	// special handling, after subsystems, if still not consumed
	if (!key.isConsumed())
	{
		// if playing
		if (isInPlayMode())
		{
			// toggle pause menu
			if ((key == cv::osu::keybinds::GAME_PAUSE.getVal<KEYCODE>()) && !m_bEscape)
			{
				if (!isInMultiplayer() || m_multiplayer->isServer() || m_iMultiplayerClientNumEscPresses > 1)
				{
					if (m_iMultiplayerClientNumEscPresses > 1)
					{
						getSelectedBeatmap()->stop(true);
					}
					else
					{
						if (!getSelectedBeatmap()->hasFailed() || !m_pauseMenu->isVisible()) // you can open the pause menu while the failing animation is happening, but never close it then
						{
							m_bEscape = true;

							getSelectedBeatmap()->pause();
							m_pauseMenu->setVisible(getSelectedBeatmap()->isPaused());

							key.consume();
						}
						else // pressing escape while in failed pause menu
						{
							getSelectedBeatmap()->stop(true);
						}
					}
				}
				else
				{
					m_iMultiplayerClientNumEscPresses++;
					if (m_iMultiplayerClientNumEscPresses == 2)
						m_notificationOverlay->addNotification("Hit 'Escape' once more to exit this multiplayer match.", 0xffffffff, false, 0.75f);
				}
			}

			// local offset
			if (key == cv::osu::keybinds::INCREASE_LOCAL_OFFSET.getVal<KEYCODE>())
			{
				long offsetAdd = keyboard->isAltDown() ? 1 : 5;
				getSelectedBeatmap()->getSelectedDifficulty2()->setLocalOffset(getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset() + offsetAdd);
				m_notificationOverlay->addNotification(UString::format("Local beatmap offset set to %ld ms", getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset()));
			}
			if (key == cv::osu::keybinds::DECREASE_LOCAL_OFFSET.getVal<KEYCODE>())
			{
				long offsetAdd = -(keyboard->isAltDown() ? 1 : 5);
				getSelectedBeatmap()->getSelectedDifficulty2()->setLocalOffset(getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset() + offsetAdd);
				m_notificationOverlay->addNotification(UString::format("Local beatmap offset set to %ld ms", getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset()));
			}

			// mania scroll speed
			/*
			if (key == cv::osu::keybinds::INCREASE_SPEED.getVal<KEYCODE>())
			{
				ConVar *maniaSpeed = &cv::osu::mania_speed;
				maniaSpeed->setValue(std::clamp<float>(std::round((maniaSpeed->getFloat() + 0.05f) * 100.0f) / 100.0f, 0.05f, 10.0f));
				m_notificationOverlay->addNotification(UString::format("osu!mania speed set to %gx (fixed)", maniaSpeed->getFloat()));
			}
			if (key == cv::osu::keybinds::DECREASE_SPEED.getVal<KEYCODE>())
			{
				ConVar *maniaSpeed = &cv::osu::mania_speed;
				maniaSpeed->setValue(std::clamp<float>(std::round((maniaSpeed->getFloat() - 0.05f) * 100.0f) / 100.0f, 0.05f, 10.0f));
				m_notificationOverlay->addNotification(UString::format("osu!mania speed set to %gx (fixed)", maniaSpeed->getFloat()));
			}
			*/
		}

		// if playing or not playing

		// volume
		if (key == cv::osu::keybinds::INCREASE_VOLUME.getVal<KEYCODE>())
			volumeUp();
		if (key == cv::osu::keybinds::DECREASE_VOLUME.getVal<KEYCODE>())
			volumeDown();

		// volume slider selection
		if (m_hud->isVolumeOverlayVisible())
		{
			if (key != cv::osu::keybinds::INCREASE_VOLUME.getVal<KEYCODE>() && key != cv::osu::keybinds::DECREASE_VOLUME.getVal<KEYCODE>())
			{
				if (key == KEY_LEFT)
					m_hud->selectVolumeNext();
				if (key == KEY_RIGHT)
					m_hud->selectVolumePrev();
			}
		}
	}
}

void Osu::onKeyUp(KeyboardEvent &key)
{
	if (isInPlayMode())
	{
		if (!getSelectedBeatmap()->isPaused())
			getSelectedBeatmap()->onKeyUp(key); // only used for mania atm
	}

	// clicks
	{
		// K1
		{
			const bool isKeyLeftClick = (key == cv::osu::keybinds::LEFT_CLICK.getVal<KEYCODE>());
			const bool isKeyLeftClick2 = (key == cv::osu::keybinds::LEFT_CLICK_2.getVal<KEYCODE>());
			if ((isKeyLeftClick && m_bKeyboardKey1Down) || (isKeyLeftClick2 && m_bKeyboardKey12Down))
			{
				if (isKeyLeftClick2)
					m_bKeyboardKey12Down = false;
				else
					m_bKeyboardKey1Down = false;

				if (isInPlayMode())
					onKey1Change(false, false);
			}
		}

		// K2
		{
			const bool isKeyRightClick = (key == cv::osu::keybinds::RIGHT_CLICK.getVal<KEYCODE>());
			const bool isKeyRightClick2 = (key == cv::osu::keybinds::RIGHT_CLICK_2.getVal<KEYCODE>());
			if ((isKeyRightClick && m_bKeyboardKey2Down) || (isKeyRightClick2 && m_bKeyboardKey22Down))
			{
				if (isKeyRightClick2)
					m_bKeyboardKey22Down = false;
				else
					m_bKeyboardKey2Down = false;

				if (isInPlayMode())
					onKey2Change(false, false);
			}
		}
	}

	// forward to all subsystems, if not consumed
	for (auto & screen : m_screens)
	{
		if (key.isConsumed())
			break;

		screen->onKeyUp(key);
	}

	// misc hotkeys release
	if (key == KEY_F1 || key == cv::osu::keybinds::TOGGLE_MODSELECT.getVal<KEYCODE>())
		m_bF1 = false;
	if (key == cv::osu::keybinds::GAME_PAUSE.getVal<KEYCODE>() || key == KEY_ESCAPE)
		m_bEscape = false;
	if (key == KEY_LSHIFT || key == KEY_RSHIFT)
		m_bUIToggleCheck = false;
	if (key == cv::osu::keybinds::TOGGLE_SCOREBOARD.getVal<KEYCODE>())
	{
		m_bScoreboardToggleCheck = false;
		m_bUIToggleCheck = false;
	}
	if (key == cv::osu::keybinds::QUICK_RETRY.getVal<KEYCODE>() || key == KEY_R)
		m_bQuickRetryDown = false;
	if (key == cv::osu::keybinds::SEEK_TIME.getVal<KEYCODE>())
		m_bSeekKey = false;

	// handle fposu key handling
	m_fposu->onKeyUp(key);
}

void Osu::onChar(KeyboardEvent &e)
{
	for (auto & screen : m_screens)
	{
		if (e.isConsumed())
			break;

		screen->onChar(e);
	}
}

void Osu::onButtonChange(MouseButton::Index button, bool down)
{
	if ((button != BUTTON_LEFT && button != BUTTON_RIGHT) || (isInPlayMode() && !getSelectedBeatmap()->isPaused() && cv::osu::disable_mousebuttons.getBool()))
		return;

	switch (button)
	{
	case BUTTON_LEFT:
	{
		if (!m_bMouseKey1Down && down)
		{
			m_bMouseKey1Down = true;
			onKey1Change(true, true);
		}
		else if (m_bMouseKey1Down)
		{
			m_bMouseKey1Down = false;
			onKey1Change(false, true);
		}
		break;
	}
	case BUTTON_RIGHT:
	{
		if (!m_bMouseKey2Down && down)
		{
			m_bMouseKey2Down = true;
			onKey2Change(true, true);
		}
		else if (m_bMouseKey2Down)
		{
			m_bMouseKey2Down = false;
			onKey2Change(false, true);
		}
		break;
	}
	default:
		break;
	}
}

void Osu::toggleModSelection(bool waitForF1KeyUp)
{
	m_bToggleModSelectionScheduled = true;
	m_modSelector->setWaitForF1KeyUp(waitForF1KeyUp);
}

void Osu::toggleSongBrowser()
{
	m_bToggleSongBrowserScheduled = true;
}

void Osu::toggleOptionsMenu()
{
	m_bToggleOptionsMenuScheduled = true;
	m_bOptionsMenuFullscreen = m_mainMenu->isVisible();
}

void Osu::toggleRankingScreen()
{
	m_bToggleRankingScreenScheduled = true;
}

void Osu::toggleUserStatsScreen()
{
	m_bToggleUserStatsScreenScheduled = true;
}

void Osu::toggleChangelog()
{
	m_bToggleChangelogScheduled = true;
}

void Osu::toggleEditor()
{
	m_bToggleEditorScheduled = true;
}

void Osu::onVolumeChange(int multiplier)
{
	// sanity reset
	m_bVolumeInactiveToActiveScheduled = false;
	anim->deleteExistingAnimation(&m_fVolumeInactiveToActiveAnim);
	m_fVolumeInactiveToActiveAnim = 0.0f;

	// chose which volume to change, depending on the volume overlay, default is master
	ConVar *volumeConVar = &cv::osu::volume_master;
	if (m_hud->getVolumeMusicSlider()->isSelected())
		volumeConVar = &cv::osu::volume_music;
	else if (m_hud->getVolumeEffectsSlider()->isSelected())
		volumeConVar = &cv::osu::volume_effects;

	// change the volume
	if (m_hud->isVolumeOverlayVisible())
	{
		float newVolume = std::clamp<float>(volumeConVar->getFloat() + cv::osu::volume_change_interval.getFloat()*multiplier, 0.0f, 1.0f);
		volumeConVar->setValue(newVolume);
	}

	m_hud->animateVolumeChange();
}

void Osu::onAudioOutputDeviceChange()
{
	if (getSelectedBeatmap() != NULL && getSelectedBeatmap()->getMusic() != NULL)
	{
		resourceManager->reloadResource(getSelectedBeatmap()->getMusic());
		getSelectedBeatmap()->select();
	}

	if (m_skin != NULL)
		m_skin->reloadSounds();
}

void Osu::saveScreenshot()
{
	static int screenshotNumber = 0;

	if (!env->directoryExists("screenshots") && !env->createDirectory("screenshots"))
	{
		m_notificationOverlay->addNotification("Error: Couldn't create screenshots folder.", 0xffff0000, false, 3.0f);
		return;
	}

	while (env->fileExists(UString::format("screenshots/screenshot%i.png", screenshotNumber)))
		screenshotNumber++;

	std::vector<unsigned char> pixels = g->getScreenshot();

	if (pixels.empty())
	{
		static uint8_t once = 0;
		if (!once++)
			m_notificationOverlay->addNotification("Error: Couldn't grab a screenshot :(", 0xffff0000, false, 3.0f);
		debugLog("failed to get pixel data for screenshot\n");
		return;
	}

	const float outerWidth = g->getResolution().x;
	const float outerHeight = g->getResolution().y;
	const float innerWidth = g_vInternalResolution.x;
	const float innerHeight = g_vInternalResolution.y;

	soundEngine->play(m_skin->getShutter());

	// don't need cropping
	if (static_cast<int>(innerWidth) == static_cast<int>(outerWidth) && static_cast<int>(innerHeight) == static_cast<int>(outerHeight))
	{
		Image::saveToImage(&pixels[0],
		                   static_cast<unsigned int>(innerWidth),
		                   static_cast<unsigned int>(innerHeight),
		                   UString::format("screenshots/screenshot%i.png", screenshotNumber));
		return;
	}

	// need cropping
	float offsetXpct = 0, offsetYpct = 0;
	if (cv::osu::resolution_enabled.getBool() && cv::osu::letterboxing.getBool())
	{
		offsetXpct = cv::osu::letterboxing_offset_x.getFloat();
		offsetYpct = cv::osu::letterboxing_offset_y.getFloat();
	}

	const int startX = std::clamp<int>(static_cast<int>((outerWidth - innerWidth) * (1 + offsetXpct) / 2), 0, static_cast<int>(outerWidth - innerWidth));
	const int startY = std::clamp<int>(static_cast<int>((outerHeight - innerHeight) * (1 + offsetYpct) / 2), 0, static_cast<int>(outerHeight - innerHeight));

	std::vector<unsigned char> croppedPixels(static_cast<size_t>(innerWidth * innerHeight * 3));

	for (ssize_t y = 0; y < static_cast<ssize_t>(innerHeight); ++y)
	{
		auto srcRowStart = pixels.begin() + ((startY + y) * static_cast<ssize_t>(outerWidth) + startX) * 3;
		auto destRowStart = croppedPixels.begin() + (y * static_cast<ssize_t>(innerWidth)) * 3;
		// copy the entire row
		std::ranges::copy_n(srcRowStart, static_cast<ssize_t>(innerWidth) * 3, destRowStart);
	}

	Image::saveToImage(&croppedPixels[0],
	                   static_cast<unsigned int>(innerWidth),
	                   static_cast<unsigned int>(innerHeight),
	                   UString::format("screenshots/screenshot%i.png", screenshotNumber));
}

void Osu::onBeforePlayStart()
{
	debugLog("\n");

	soundEngine->play(m_skin->getMenuHit());

	updateMods();

	// mp hack
	{
		m_mainMenu->setVisible(false);
		m_modSelector->setVisible(false);
		m_optionsMenu->setVisible(false);
		m_pauseMenu->setVisible(false);
		m_rankingScreen->setVisible(false);
	}

	// HACKHACK: stuck key quickfix
	{
		m_bKeyboardKey1Down = false;
		m_bKeyboardKey12Down = false;
		m_bKeyboardKey2Down = false;
		m_bKeyboardKey22Down = false;
		m_bMouseKey1Down = false;
		m_bMouseKey2Down = false;

		if (getSelectedBeatmap() != NULL)
		{
			getSelectedBeatmap()->keyReleased1(false);
			getSelectedBeatmap()->keyReleased1(true);
			getSelectedBeatmap()->keyReleased2(false);
			getSelectedBeatmap()->keyReleased2(true);
		}
	}
}

void Osu::onPlayStart()
{
	debugLog("\n");

	cv::snd_change_check_interval.setValue(0.0f);

	if (m_bModAuto || m_bModAutopilot)
	{
		m_bShouldCursorBeVisible = true;
		env->setCursorVisible(m_bShouldCursorBeVisible);
	}

	if (getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset() != 0)
		m_notificationOverlay->addNotification(UString::format("Using local beatmap offset (%ld ms)", getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset()), 0xffffffff, false, 0.75f);

	m_fQuickSaveTime = 0.0f; // reset

	updateConfineCursor();
	updateWindowsKeyDisable();

	OsuRichPresence::onPlayStart();
}

void Osu::onPlayEnd(bool quit)
{
	debugLog("\n");

	OsuRichPresence::onPlayEnd(quit);

	cv::snd_change_check_interval.setValue(cv::snd_change_check_interval.getDefaultFloat());

	if (!quit)
	{
		if (!cv::osu::mod_endless.getBool())
		{
			if (isInMultiplayer())
			{
				// score packets while playing are sent unreliably, but at the end we want to have reliably synced values across all players
				// also, at the end, we display the maximum achieved combo instead of the current one
				m_multiplayer->onClientScoreChange(m_score->getComboMax(), m_score->getAccuracy(), m_score->getScore(), m_score->isDead(), true);
			}

			m_rankingScreen->setScore(m_score);
			m_rankingScreen->setBeatmapInfo(getSelectedBeatmap(), getSelectedBeatmap()->getSelectedDifficulty2());

			soundEngine->play(m_skin->getApplause());
		}
		else
		{
			m_bScheduleEndlessModNextBeatmap = true;
			return; // nothing more to do here
		}
	}

	m_mainMenu->setVisible(false);
	m_modSelector->setVisible(false);
	m_pauseMenu->setVisible(false);

	env->setCursorVisible(false);
	m_bShouldCursorBeVisible = false;

	m_iMultiplayerClientNumEscPresses = 0;

	if (m_songBrowser2 != NULL)
		m_songBrowser2->onPlayEnd(quit);

	if (quit)
		toggleSongBrowser();
	else
		toggleRankingScreen();

	updateConfineCursor();
	updateWindowsKeyDisable();
}

McRect Osu::getVirtScreenRectWithinEngineRect() const
{
	float offsetX = 0;
	float offsetY = 0;
	if (cv::osu::letterboxing.getBool())
	{
		offsetX = ((static_cast<float>(engine->getScreenWidth()) - g_vInternalResolution.x)/2.0f)*(1.0f + cv::osu::letterboxing_offset_x.getFloat());
		offsetY = ((static_cast<float>(engine->getScreenHeight()) - g_vInternalResolution.y)/2.0f)*(1.0f + cv::osu::letterboxing_offset_y.getFloat());
	}
	return {offsetX, offsetY, offsetX + g_vInternalResolution.x, offsetY + g_vInternalResolution.y};
}

OsuBeatmap *Osu::getSelectedBeatmap()
{
	if (m_songBrowser2 != NULL)
		return m_songBrowser2->getSelectedBeatmap();

	return NULL;
}

float Osu::getDifficultyMultiplier()
{
	float difficultyMultiplier = 1.0f;

	if (m_bModHR)
		difficultyMultiplier = 1.4f;
	if (m_bModEZ)
		difficultyMultiplier = 0.5f;

	return difficultyMultiplier;
}

float Osu::getCSDifficultyMultiplier()
{
	float difficultyMultiplier = 1.0f;

	if (m_bModHR)
		difficultyMultiplier = 1.3f; // different!
	if (m_bModEZ)
		difficultyMultiplier = 0.5f;

	return difficultyMultiplier;
}

float Osu::getScoreMultiplier()
{
	float multiplier = 1.0f;

	if (m_bModEZ || (m_bModNF && !m_bModScorev2))
		multiplier *= 0.50f;
	if (m_bModHT || m_bModDC)
		multiplier *= 0.30f;
	if (m_bModHR)
	{
		if (m_bModScorev2)
			multiplier *= 1.10f;
		else
			multiplier *= 1.06f;
	}
	if (m_bModDT || m_bModNC)
	{
		if (m_bModScorev2)
			multiplier *= 1.20f;
		else
			multiplier *= 1.12f;
	}
	if (m_bModHD)
		multiplier *= 1.06f;
	if (m_bModSpunout)
		multiplier *= 0.90f;

	return multiplier;
}

float Osu::getRawSpeedMultiplier()
{
	float speedMultiplier = 1.0f;

	if (m_bModDT || m_bModNC || m_bModHT || m_bModDC)
	{
		if (m_bModDT || m_bModNC)
			speedMultiplier = 1.5f;
		else
			speedMultiplier = 0.75f;
	}

	return speedMultiplier;
}

float Osu::getSpeedMultiplier()
{
	float speedMultiplier = getRawSpeedMultiplier();

	if (cv::osu::speed_override.getFloat() >= 0.0f)
		return std::max(cv::osu::speed_override.getFloat(), 0.05f);

	return speedMultiplier;
}

float Osu::getPitchMultiplier()
{
	float pitchMultiplier = 1.0f;

	if (m_bModDC)
		pitchMultiplier = 0.92f;

	if (m_bModNC)
		pitchMultiplier = 1.1166f;

	if (cv::osu::pitch_override.getFloat() > 0.0f)
		return cv::osu::pitch_override.getFloat();

	return pitchMultiplier;
}

bool Osu::isInPlayMode()
{
	return (m_songBrowser2 != NULL && m_songBrowser2->hasSelectedAndIsPlaying());
}

bool Osu::isNotInPlayModeOrPaused()
{
	return !isInPlayMode() || (getSelectedBeatmap() != NULL && getSelectedBeatmap()->isPaused());
}

bool Osu::isInMultiplayer()
{
	return m_multiplayer->isInMultiplayer();
}

bool Osu::shouldFallBackToLegacySliderRenderer()
{
	return cv::osu::force_legacy_slider_renderer.getBool()
			|| cv::osu::mod_wobble.getBool()
			|| cv::osu::mod_wobble2.getBool()
			|| cv::osu::mod_minimize.getBool()
			|| m_modSelector->isCSOverrideSliderActive()
			/* || (m_osu_playfield_rotation.getFloat() < -0.01f || m_osu_playfield_rotation.getFloat() > 0.01f)*/;
}



void Osu::onResolutionChanged(Vector2 newResolution)
{
	debugLog("res ({}, {}), minimized = {}\n", (int)newResolution.x, (int)newResolution.y, (int)engine->isMinimized());

	if (engine->isMinimized()) return; // ignore if minimized

	const float prevUIScale = getUIScale();

	if (!cv::osu::resolution_enabled.getBool())
		g_vInternalResolution = newResolution;
	else if (!engine->isMinimized()) // if we just got minimized, ignore the resolution change (for the internal stuff)
	{
		// clamp upwards to internal resolution (osu_resolution)
		if (g_vInternalResolution.x < m_vInternalResolution.x)
			g_vInternalResolution.x = m_vInternalResolution.x;
		if (g_vInternalResolution.y < m_vInternalResolution.y)
			g_vInternalResolution.y = m_vInternalResolution.y;

		// clamp downwards to engine resolution
		if (newResolution.x < g_vInternalResolution.x)
			g_vInternalResolution.x = newResolution.x;
		if (newResolution.y < g_vInternalResolution.y)
			g_vInternalResolution.y = newResolution.y;

		// disable internal resolution on specific conditions
		bool windowsBorderlessHackCondition = (Env::cfg(OS::WINDOWS) && env->isFullscreen() && env->isFullscreenWindowedBorderless() && (int)g_vInternalResolution.y == (int)env->getNativeScreenSize().y); // HACKHACK
		if (((int)g_vInternalResolution.x == engine->getScreenWidth() && (int)g_vInternalResolution.y == engine->getScreenHeight()) || !env->isFullscreen() || windowsBorderlessHackCondition)
		{
			debugLog("Internal resolution == Engine resolution || !Fullscreen, disabling resampler ({}, {})\n", (int)(g_vInternalResolution == engine->getScreenSize()), (int)(!env->isFullscreen()));
			cv::osu::resolution_enabled.setValue(0.0f);
			g_vInternalResolution = engine->getScreenSize();
		}
	}

	// update dpi specific engine globals
	cv::ui_scrollview_scrollbarwidth.setValue(15.0f * Osu::getUIScale()); // not happy with this as a convar

	// interfaces
	for (int i=0; i<m_screens.size(); i++)
	{
		m_screens[i]->onResolutionChange(g_vInternalResolution);
	}

	// rendertargets
	rebuildRenderTargets();

	// mouse scale/offset
	updateMouseSettings();

	// cursor clipping
	updateConfineCursor();

	// see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=323
	struct LossyComparisonToFixExcessFPUPrecisionBugBecauseFuckYou
	{
		static bool equalEpsilon(float f1, float f2)
		{
			return std::abs(f1 - f2) < 0.00001f;
		}
	};

	// a bit hacky, but detect resolution-specific-dpi-scaling changes and force a font and layout reload after a 1 frame delay (1/2)
	if (!LossyComparisonToFixExcessFPUPrecisionBugBecauseFuckYou::equalEpsilon(getUIScale(), prevUIScale))
		m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled = true;
}

void Osu::onDPIChanged()
{
	// delay
	m_bFontReloadScheduled = true;
	m_bFireResolutionChangedScheduled = true;
}

void Osu::rebuildRenderTargets()
{
	debugLog("Internal resolution {}x{}\n", g_vInternalResolution.x, g_vInternalResolution.y);

	m_backBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);

	if (cv::osu::fposu::mod_fposu.getBool())
		m_playfieldBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
	else
		m_playfieldBuffer->rebuild(0, 0, 64, 64);

	if (cv::osu::fposu::mod_fposu.getBool() && cv::osu::fposu::threeD.getBool() && cv::osu::fposu::threeD_spheres_aa.getBool())
	{
		Graphics::MULTISAMPLE_TYPE multisampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X;
		{
			if (cv::osu::fposu::threeD_spheres_aa.getInt() > 8)
				multisampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_16X;
			else if (cv::osu::fposu::threeD_spheres_aa.getInt() > 4)
				multisampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_8X;
			else if (cv::osu::fposu::threeD_spheres_aa.getInt() > 2)
				multisampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_4X;
			else if (cv::osu::fposu::threeD_spheres_aa.getInt() > 0)
				multisampleType = Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_2X;
		}
		m_sliderFrameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y, multisampleType);
	}
	else
		m_sliderFrameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y, Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);

	if (cv::osu::stdrules::mod_mafham.getBool())
	{
		m_frameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
		m_frameBuffer2->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
	}
	else
	{
		m_frameBuffer->rebuild(0, 0, 64, 64);
		m_frameBuffer2->rebuild(0, 0, 64, 64);
	}
}

void Osu::reloadFonts()
{
	const int baseDPI = 96;
	const int newDPI = Osu::getUIScale() * baseDPI;

	for (McFont *font : m_fonts)
	{
		if (font->getDPI() != newDPI)
		{
			font->setDPI(newDPI);
			resourceManager->reloadResource(font);
		}
	}
}

void Osu::updateMouseSettings()
{
	// mouse scaling & offset
	Vector2 offset = Vector2(0, 0);
	Vector2 scale = Vector2(1, 1);
	if (cv::osu::resolution_enabled.getBool())
	{
		if (cv::osu::letterboxing.getBool())
		{
			// special case for osu: since letterboxed raw input absolute to window should mean the 'game' window, and not the 'engine' window, no offset scaling is necessary
			if (cv::mouse_raw_input_absolute_to_window.getBool())
				offset = -Vector2((engine->getScreenWidth()/2.0f - g_vInternalResolution.x/2), (engine->getScreenHeight()/2.0f - g_vInternalResolution.y/2));
			else
				offset = -Vector2((engine->getScreenWidth()/2.0f - g_vInternalResolution.x/2)*(1.0f + cv::osu::letterboxing_offset_x.getFloat()), (engine->getScreenHeight()/2.0f - g_vInternalResolution.y/2)*(1.0f + cv::osu::letterboxing_offset_y.getFloat()));

			scale = Vector2(g_vInternalResolution.x / static_cast<float>(engine->getScreenWidth()), g_vInternalResolution.y / static_cast<float>(engine->getScreenHeight()));
		}
	}

	mouse->setOffset(offset);
	mouse->setScale(scale);

	if (cv::osu::debug.getBool())
		debugLog("offset {:.2f},{:.2f} scale {:.2f},{:.2f}\n", offset.x, offset.y, scale.x, scale.y);
}

void Osu::updateWindowsKeyDisable()
{
	const bool isPlayerPlaying = engine->hasFocus() && isInPlayMode() && getSelectedBeatmap() != NULL && (!getSelectedBeatmap()->isPaused() || getSelectedBeatmap()->isRestartScheduled()) && !m_bModAuto;
	if (cv::osu::disable_windows_key_while_playing.getBool())
	{
		cv::disable_windows_key.setValue(isPlayerPlaying ? 1.0f : 0.0f);
	}
	// currently only used to signal SDL
	env->listenToTextInput(!isPlayerPlaying);

	if (cv::osu::debug.getBool())
		debugLog("isPlayerPlaying {}\n", isPlayerPlaying);
}

void Osu::fireResolutionChanged()
{
	onResolutionChanged(g_vInternalResolution);
}

void Osu::onInternalResolutionChanged(UString oldValue, UString args)
{
	if (args.length() < 7) return;

	const float prevUIScale = getUIScale();

	std::vector<UString> resolution = args.split("x");
	if (resolution.size() != 2)
		debugLog("Error: Invalid parameter count for command 'osu_resolution'! (Usage: e.g. \"osu_resolution 1280x720\")");
	else
	{
		int width = resolution[0].toFloat();
		int height = resolution[1].toFloat();

		if (width < 300 || height < 240)
			debugLog("Error: Invalid values for resolution for command 'osu_resolution'!");
		else
		{
			Vector2 newInternalResolution = Vector2(width, height);

			// clamp requested internal resolution to current renderer resolution
			// however, this could happen while we are transitioning into fullscreen. therefore only clamp when not in fullscreen or not in fullscreen transition
			bool isTransitioningIntoFullscreenHack = g->getResolution().x < env->getNativeScreenSize().x || g->getResolution().y < env->getNativeScreenSize().y;
			if (!env->isFullscreen() || !isTransitioningIntoFullscreenHack)
			{
				if (newInternalResolution.x > g->getResolution().x)
					newInternalResolution.x = g->getResolution().x;
				if (newInternalResolution.y > g->getResolution().y)
					newInternalResolution.y = g->getResolution().y;
			}

			// enable and store, then force onResolutionChanged()
			cv::osu::resolution_enabled.setValue(1.0f);
			g_vInternalResolution = newInternalResolution;
			m_vInternalResolution = newInternalResolution;
			fireResolutionChanged();
		}
	}

	// a bit hacky, but detect resolution-specific-dpi-scaling changes and force a font and layout reload after a 1 frame delay (2/2)
	if (getUIScale() != prevUIScale)
		m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled = true;
}

void Osu::onFocusGained()
{
	// cursor clipping
	updateConfineCursor();

	if (m_bWasBossKeyPaused)
	{
		m_bWasBossKeyPaused = false;
		if (getSelectedBeatmap() != NULL)
			getSelectedBeatmap()->pausePreviewMusic();
	}

	updateWindowsKeyDisable();

	if constexpr (Env::cfg(AUD::WASAPI)) // NOTE: wasapi exclusive mode controls the system volume, so don't bother
		return;

	m_fVolumeInactiveToActiveAnim = 0.0f;
	anim->moveLinear(&m_fVolumeInactiveToActiveAnim, 1.0f, 0.3f, 0.1f, true);
}

void Osu::onFocusLost()
{
	if (isInPlayMode() && !getSelectedBeatmap()->isPaused() && cv::osu::pause_on_focus_loss.getBool())
	{
		if (!isInMultiplayer())
		{
			getSelectedBeatmap()->pause(false);
			m_pauseMenu->setVisible(true);
			m_modSelector->setVisible(false);
		}
	}

	updateWindowsKeyDisable();

	// release cursor clip
	updateConfineCursor();

	if constexpr (Env::cfg(AUD::WASAPI)) // NOTE: wasapi exclusive mode controls the system volume, so don't bother
		return;

	m_bVolumeInactiveToActiveScheduled = true;

	anim->deleteExistingAnimation(&m_fVolumeInactiveToActiveAnim);
	m_fVolumeInactiveToActiveAnim = 0.0f;

	soundEngine->setVolume(cv::osu::volume_master_inactive.getFloat() * cv::osu::volume_master.getFloat());
}

void Osu::onMinimized()
{
	if constexpr (Env::cfg(AUD::WASAPI)) // NOTE: wasapi exclusive mode controls the system volume, so don't bother
		return;
	
	m_bVolumeInactiveToActiveScheduled = true;

	anim->deleteExistingAnimation(&m_fVolumeInactiveToActiveAnim);
	m_fVolumeInactiveToActiveAnim = 0.0f;

	soundEngine->setVolume(cv::osu::volume_master_inactive.getFloat() * cv::osu::volume_master.getFloat());
}

bool Osu::onShutdown()
{
	debugLog("\n");

	if (!cv::osu::alt_f4_quits_even_while_playing.getBool() && isInPlayMode())
	{
		if (getSelectedBeatmap() != NULL)
			getSelectedBeatmap()->stop();

		return false;
	}

	// save everything
	m_optionsMenu->save();
	m_songBrowser2->getDatabase()->save();

	// the only time where a shutdown could be problematic is while an update is being installed, so we block it here
	return m_updateHandler == NULL || m_updateHandler->getStatus() != OsuUpdateHandler::STATUS::STATUS_INSTALLING_UPDATE;
}

void Osu::onSkinReload()
{
	m_bSkinLoadWasReload = true;
	onSkinChange("", cv::osu::skin.getString());
}

void Osu::onSkinChange(UString oldValue, UString newValue)
{
	if (m_skin != NULL)
	{
		if (m_bSkinLoadScheduled || m_skinScheduledToLoad != NULL) return;
		if (newValue.length() < 1) return;
	}

	UString skinFolder = cv::osu::folder.getString();
	skinFolder.append(cv::osu::folder_sub_skins.getString());
	skinFolder.append(newValue);
	skinFolder.append("/");

	// workshop skins use absolute paths

	const bool isWorkshopSkin = Env::cfg(FEAT::STEAM) ? cv::osu::skin_is_from_workshop.getBool() : false;

	// reset playtime tracking
	if constexpr (Env::cfg(FEAT::STEAM))
	{
		steam->stopWorkshopPlaytimeTrackingForAllItems();

		if (isWorkshopSkin)
		{
			skinFolder = newValue;

			// ensure that the skinFolder ends with a slash
			if (skinFolder[skinFolder.length()-1] != L'/' && skinFolder[skinFolder.length()-1] != L'\\')
				skinFolder.append("/");

			steam->startWorkshopItemPlaytimeTracking((uint64_t)cv::osu::skin_workshop_id.getString().toLong());

			// set correct name of workshop skin
			newValue = cv::osu::skin_workshop_title.getString();
		}
	}
	m_skinScheduledToLoad = new OsuSkin(newValue, skinFolder, newValue == "default", isWorkshopSkin);

	// initial load
	if (m_skin == NULL)
		m_skin = m_skinScheduledToLoad;

	m_bSkinLoadScheduled = true;
}

void Osu::onMasterVolumeChange(UString oldValue, UString newValue)
{
	if (m_bVolumeInactiveToActiveScheduled) return; // not very clean, but w/e

	float newVolume = newValue.toFloat();
	soundEngine->setVolume(newVolume);
}

void Osu::onMusicVolumeChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->setVolume(newValue.toFloat());
}

void Osu::onSpeedChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
	{
		float speed = newValue.toFloat();
		getSelectedBeatmap()->setSpeed(speed >= 0.0f ? speed : getSpeedMultiplier());
	}
}

void Osu::onPitchChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
	{
		float pitch = newValue.toFloat();
		getSelectedBeatmap()->setPitch(pitch > 0.0f ? pitch : getPitchMultiplier());
	}
}

void Osu::onPlayfieldChange(UString oldValue, UString newValue)
{
	if (getSelectedBeatmap() != NULL)
		getSelectedBeatmap()->onModUpdate();
}

void Osu::onUIScaleChange(UString oldValue, UString newValue)
{
	const float oldVal = oldValue.toFloat();
	const float newVal = newValue.toFloat();

	if (oldVal != newVal)
	{
		// delay
		m_bFontReloadScheduled = true;
		m_bFireResolutionChangedScheduled = true;
	}
}

void Osu::onUIScaleToDPIChange(UString oldValue, UString newValue)
{
	const bool oldVal = oldValue.toFloat() > 0.0f;
	const bool newVal = newValue.toFloat() > 0.0f;

	if (oldVal != newVal)
	{
		// delay
		m_bFontReloadScheduled = true;
		m_bFireResolutionChangedScheduled = true;
	}
}

void Osu::onLetterboxingChange(UString oldValue, UString newValue)
{
	if (cv::osu::resolution_enabled.getBool())
	{
		bool oldVal = oldValue.toFloat() > 0.0f;
		bool newVal = newValue.toFloat() > 0.0f;

		if (oldVal != newVal)
			m_bFireResolutionChangedScheduled = true; // delay
	}
}

void Osu::updateConfineCursor()
{
	if (cv::osu::debug.getBool())
		debugLog("\n");

	if (engine->hasFocus()
			&& !cv::osu::confine_cursor_never.getBool()
			&& ((cv::osu::confine_cursor_fullscreen.getBool() && env->isFullscreen())
			||  (cv::osu::confine_cursor_windowed.getBool() && !env->isFullscreen())
			||  (isInPlayMode() && !m_pauseMenu->isVisible() && !getModAuto() && !getModAutopilot())))
	{
		float offsetX = 0;
		float offsetY = 0;
		if (cv::osu::letterboxing.getBool())
		{
			offsetX = ((engine->getScreenWidth() - g_vInternalResolution.x)/2)*(1.0f + cv::osu::letterboxing_offset_x.getFloat());
			offsetY = ((engine->getScreenHeight() - g_vInternalResolution.y)/2)*(1.0f + cv::osu::letterboxing_offset_y.getFloat());
		}
		env->setCursorClip(true, McRect(offsetX, offsetY, g_vInternalResolution.x, g_vInternalResolution.y));
	}
	else
		env->setCursorClip(false, McRect());
}

void Osu::onConfineCursorWindowedChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onConfineCursorFullscreenChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onConfineCursorNeverChange(UString oldValue, UString newValue)
{
	updateConfineCursor();
}

void Osu::onKey1Change(bool pressed, bool mouseButton)
{
	int numKeys1Down = 0;
	if (m_bKeyboardKey1Down)
		numKeys1Down++;
	if (m_bKeyboardKey12Down)
		numKeys1Down++;
	if (m_bMouseKey1Down)
		numKeys1Down++;

	const bool isKeyPressed1Allowed = (numKeys1Down == 1); // all key1 keys (incl. multiple bindings) act as one single key with state handover

	// WARNING: if paused, keyReleased*() will be called out of sequence every time due to the fix. do not put actions in it
	if (isInPlayMode()/* && !getSelectedBeatmap()->isPaused()*/) // NOTE: allow keyup even while beatmap is paused, to correctly not-continue immediately due to pressed keys
	{
		if (!(mouseButton && cv::osu::disable_mousebuttons.getBool()))
		{
			// quickfix
			if (cv::osu::disable_mousebuttons.getBool())
				m_bMouseKey1Down = false;

			if (pressed && isKeyPressed1Allowed && !getSelectedBeatmap()->isPaused()) // see above note
				getSelectedBeatmap()->keyPressed1(mouseButton);
			else if (!m_bKeyboardKey1Down && !m_bKeyboardKey12Down && !m_bMouseKey1Down)
				getSelectedBeatmap()->keyReleased1(mouseButton);
		}
	}

	// cursor anim + ripples
	const bool doAnimate = !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouseButton && cv::osu::disable_mousebuttons.getBool());
	if (doAnimate)
	{
		if (pressed && isKeyPressed1Allowed)
		{
			m_hud->animateCursorExpand();
			m_hud->addCursorRipple(mouse->getPos());
		}
		else if (!m_bKeyboardKey1Down && !m_bKeyboardKey12Down && !m_bMouseKey1Down && !m_bKeyboardKey2Down && !m_bKeyboardKey22Down && !m_bMouseKey2Down)
			m_hud->animateCursorShrink();
	}
}

void Osu::onKey2Change(bool pressed, bool mouseButton)
{
	int numKeys2Down = 0;
	if (m_bKeyboardKey2Down)
		numKeys2Down++;
	if (m_bKeyboardKey22Down)
		numKeys2Down++;
	if (m_bMouseKey2Down)
		numKeys2Down++;

	const bool isKeyPressed2Allowed = (numKeys2Down == 1); // all key2 keys (incl. multiple bindings) act as one single key with state handover

	// WARNING: if paused, keyReleased*() will be called out of sequence every time due to the fix. do not put actions in it
	if (isInPlayMode()/* && !getSelectedBeatmap()->isPaused()*/) // NOTE: allow keyup even while beatmap is paused, to correctly not-continue immediately due to pressed keys
	{
		if (!(mouseButton && cv::osu::disable_mousebuttons.getBool()))
		{
			// quickfix
			if (cv::osu::disable_mousebuttons.getBool())
				m_bMouseKey2Down = false;

			if (pressed && isKeyPressed2Allowed && !getSelectedBeatmap()->isPaused()) // see above note
				getSelectedBeatmap()->keyPressed2(mouseButton);
			else if (!m_bKeyboardKey2Down && !m_bKeyboardKey22Down && !m_bMouseKey2Down)
				getSelectedBeatmap()->keyReleased2(mouseButton);
		}
	}

	// cursor anim + ripples
	const bool doAnimate = !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouseButton && cv::osu::disable_mousebuttons.getBool());
	if (doAnimate)
	{
		if (pressed && isKeyPressed2Allowed)
		{
			m_hud->animateCursorExpand();
			m_hud->addCursorRipple(mouse->getPos());
		}
		else if (!m_bKeyboardKey2Down && !m_bKeyboardKey22Down && !m_bMouseKey2Down && !m_bKeyboardKey1Down && !m_bKeyboardKey12Down && !m_bMouseKey1Down)
			m_hud->animateCursorShrink();
	}
}

void Osu::onModMafhamChange(UString oldValue, UString newValue)
{
	rebuildRenderTargets();
}

void Osu::onModFPoSuChange(UString oldValue, UString newValue)
{
	rebuildRenderTargets();
}

void Osu::onModFPoSu3DChange(UString oldValue, UString newValue)
{
	rebuildRenderTargets();
}

void Osu::onModFPoSu3DSpheresChange(UString oldValue, UString newValue)
{
	rebuildRenderTargets();
}

void Osu::onModFPoSu3DSpheresAAChange(UString oldValue, UString newValue)
{
	rebuildRenderTargets();
}

void Osu::onLetterboxingOffsetChange(UString oldValue, UString newValue)
{
	updateMouseSettings();
	updateConfineCursor();
}

void Osu::onNotification(UString args)
{
	m_notificationOverlay->addNotification(args, rgb(cv::osu::notification_color_r.getInt(), cv::osu::notification_color_g.getInt(), cv::osu::notification_color_b.getInt()));
}



float Osu::getImageScaleToFitResolution(Vector2 size, Vector2 resolution)
{
	return resolution.x/size.x > resolution.y/size.y ? resolution.y/size.y : resolution.x/size.x;
}

float Osu::getImageScaleToFitResolution(Image *img, Vector2 resolution)
{
	return getImageScaleToFitResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScaleToFillResolution(Vector2 size, Vector2 resolution)
{
	return resolution.x/size.x < resolution.y/size.y ? resolution.y/size.y : resolution.x/size.x;
}

float Osu::getImageScaleToFillResolution(Image *img, Vector2 resolution)
{
	return getImageScaleToFillResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScale(Vector2 size, float osuSize)
{
	int swidth = osu->getVirtScreenWidth();
	int sheight = osu->getVirtScreenHeight();

	if (swidth * 3 > sheight * 4)
		swidth = sheight * 4 / 3;
	else
		sheight = swidth * 3 / 4;

	const float xMultiplier = swidth / osuBaseResolution.x;
	const float yMultiplier = sheight / osuBaseResolution.y;

	const float xDiameter = osuSize*xMultiplier;
	const float yDiameter = osuSize*yMultiplier;

	return xDiameter/size.x > yDiameter/size.y ? xDiameter/size.x : yDiameter/size.y;
}

float Osu::getImageScale(Image *img, float osuSize)
{
	return getImageScale(Vector2(img->getWidth(), img->getHeight()), osuSize);
}

float Osu::getUIScale(float osuResolutionRatio)
{
	int swidth = osu->getVirtScreenWidth();
	int sheight = osu->getVirtScreenHeight();

	if (swidth * 3 > sheight * 4)
		swidth = sheight * 4 / 3;
	else
		sheight = swidth * 3 / 4;

	const float xMultiplier = swidth / osuBaseResolution.x;
	const float yMultiplier = sheight / osuBaseResolution.y;

	const float xDiameter = osuResolutionRatio*xMultiplier;
	const float yDiameter = osuResolutionRatio*yMultiplier;

	return xDiameter > yDiameter ? xDiameter : yDiameter;
}

float Osu::getUIScale()
{
	if (osu != NULL)
	{
		if (osu->getVirtScreenWidth() < cv::osu::ui_scale_to_dpi_minimum_width.getInt() || osu->getVirtScreenHeight() < cv::osu::ui_scale_to_dpi_minimum_height.getInt())
			return cv::osu::ui_scale.getFloat();
	}
	else if (engine->getScreenWidth() < cv::osu::ui_scale_to_dpi_minimum_width.getInt() || engine->getScreenHeight() < cv::osu::ui_scale_to_dpi_minimum_height.getInt())
		return cv::osu::ui_scale.getFloat();

	return ((cv::osu::ui_scale_to_dpi.getBool() ? env->getDPIScale() : 1.0f) * cv::osu::ui_scale.getFloat());
}

bool Osu::findIgnoreCase(const std::string &haystack, const std::string &needle)
{
	auto result = std::ranges::search(
		haystack,
		needle,  
		[](char ch1, char ch2)
		{return std::tolower(ch1) == std::tolower(ch2);}
	);

	return !result.empty();
}
