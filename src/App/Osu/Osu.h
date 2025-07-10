//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		yet another ouendan clone, because why not
//
// $NoKeywords: $osu
//===============================================================================//

//#define MCOSU_FPOSU_4D_MODE_FINISHED

#pragma once
#ifndef OSU_H
#define OSU_H

#include "App.h"

#include "OsuConVarDefs.h"
#include "KeyboardListener.h"
#include "MouseListener.h"
#include "OsuKeyBindings.h"
#include "UString.h"
#include "Vectors.h"

#include <cstdint>
#include <string>

class CWindowManager;

class OsuMultiplayer;
class OsuMainMenu;
class OsuPauseMenu;
class OsuOptionsMenu;
class OsuModSelector;
class OsuSongBrowser2;
class OsuBackgroundImageHandler;
class OsuRankingScreen;
class OsuUserStatsScreen;
class OsuUpdateHandler;
class OsuNotificationOverlay;
class OsuTooltipOverlay;
class OsuSteamWorkshop;
class OsuBeatmap;
class OsuScreen;
class OsuScore;
class OsuSkin;
class OsuHUD;
class OsuChangelog;
class OsuEditor;
class OsuModFPoSu;

class Graphics;

class ConVar;
class Image;
class McFont;
class RenderTarget;
class McRect;

class Osu final : public AppBase, public MouseListener
{
public:
	static bool autoUpdater;

	static Vector2 osuBaseResolution;

	static float getImageScaleToFitResolution(Image *img, Vector2 resolution);
	static float getImageScaleToFitResolution(Vector2 size, Vector2 resolution);
	static float getImageScaleToFillResolution(Vector2 size, Vector2 resolution);
	static float getImageScaleToFillResolution(Image *img, Vector2 resolution);
	static float getImageScale(Vector2 size, float osuSize);
	static float getImageScale(Image *img, float osuSize);
	static float getUIScale(float osuResolutionRatio);
	static float getUIScale(); // NOTE: includes premultiplied dpi scale!

	static bool findIgnoreCase(const std::string &haystack, const std::string &needle);

	enum class GAMEMODE : uint8_t
	{
		STD,
		MANIA
	};

public:
	Osu();
	~Osu() override;

	void draw() override;
	void update() override;
	inline bool isInCriticalInteractiveSession() override { return !isNotInPlayModeOrPaused();} // i.e. is in play mode and not paused

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	void onButtonChange(MouseButton::Index button, bool down) override;

	void onWheelVertical(int /*delta*/) override {;}
	void onWheelHorizontal(int /*delta*/) override {;}

	void onResolutionChanged(Vector2 newResolution) override;
	void onDPIChanged() override;

	void onFocusGained() override;
	void onFocusLost() override;
	void onMinimized() override;
	void onRestored() override {;}
	bool onShutdown() override;

	void onBeforePlayStart();			// called just before OsuBeatmap->play()
	void onPlayStart();					// called when a beatmap has successfully started playing
	void onPlayEnd(bool quit = true);	// called when a beatmap is finished playing (or the player quit)

	void toggleModSelection(bool waitForF1KeyUp = false);
	void toggleSongBrowser();
	void toggleOptionsMenu();
	void toggleRankingScreen();
	void toggleUserStatsScreen();
	void toggleChangelog();
	void toggleEditor();

	void volumeUp(int multiplier = 1) {onVolumeChange(multiplier);}
	void volumeDown(int multiplier = 1) {onVolumeChange(-multiplier);}

	void saveScreenshot();

	void setSkin(UString skin) {onSkinChange("", skin);}
	void reloadSkin() {onSkinReload();}

	void setGamemode(GAMEMODE gamemode) {m_gamemode = gamemode;}

	[[nodiscard]] inline GAMEMODE getGamemode() const {return m_gamemode;}

	[[nodiscard]] constexpr const Vector2 &getVirtScreenSize() const override {return g_vInternalResolution;}
	[[nodiscard]] McRect getVirtScreenRectWithinEngineRect() const override;
	[[nodiscard]] inline int getVirtScreenWidth() const override {return (int)g_vInternalResolution.x;}
	[[nodiscard]] inline int getVirtScreenHeight() const override {return (int)g_vInternalResolution.y;}

	OsuBeatmap *getSelectedBeatmap();

	[[nodiscard]] inline OsuMultiplayer* getMultiplayer() const {return m_multiplayer;}
	[[nodiscard]] inline OsuOptionsMenu* getOptionsMenu() const {return m_optionsMenu;}
	[[nodiscard]] inline OsuSongBrowser2* getSongBrowser() const {return m_songBrowser2;}
	[[nodiscard]] inline OsuBackgroundImageHandler* getBackgroundImageHandler() const {return m_backgroundImageHandler;}
	[[nodiscard]] inline OsuSkin* getSkin() const {return m_skin;}
	[[nodiscard]] inline OsuHUD* getHUD() const {return m_hud;}
	[[nodiscard]] inline OsuNotificationOverlay* getNotificationOverlay() const {return m_notificationOverlay;}
	[[nodiscard]] inline OsuTooltipOverlay* getTooltipOverlay() const {return m_tooltipOverlay;}
	[[nodiscard]] inline OsuModSelector* getModSelector() const {return m_modSelector;}
	[[nodiscard]] inline OsuModFPoSu* getFPoSu() const {return m_fposu;}
	[[nodiscard]] inline OsuPauseMenu* getPauseMenu() const {return m_pauseMenu;}
	[[nodiscard]] inline OsuMainMenu* getMainMenu() const {return m_mainMenu;}
	[[nodiscard]] inline OsuRankingScreen* getRankingScreen() const {return m_rankingScreen;}
	[[nodiscard]] inline OsuScore* getScore() const {return m_score;}
	[[nodiscard]] inline OsuUpdateHandler* getUpdateHandler() const {return m_updateHandler;}
	[[nodiscard]] inline OsuUserStatsScreen* getUserStatsScreen() const {return m_userStatsScreen;}
	[[nodiscard]] inline OsuKeyBindings* getBindings() const {return m_bindings;}
	[[maybe_unused]]
	[[nodiscard]] inline OsuSteamWorkshop* getSteamWorkshop() const {return m_steamWorkshop;}

	[[nodiscard]] inline RenderTarget* getPlayfieldBuffer() const {return m_playfieldBuffer;}
	[[nodiscard]] inline RenderTarget* getSliderFrameBuffer() const {return m_sliderFrameBuffer;}
	[[nodiscard]] inline RenderTarget* getFrameBuffer() const {return m_frameBuffer;}
	[[nodiscard]] inline RenderTarget* getFrameBuffer2() const {return m_frameBuffer2;}
	[[nodiscard]] inline McFont* getTitleFont() const {return m_titleFont;}
	[[nodiscard]] inline McFont* getSubTitleFont() const {return m_subTitleFont;}
	[[nodiscard]] inline McFont* getSongBrowserFont() const {return m_songBrowserFont;}
	[[nodiscard]] inline McFont* getSongBrowserFontBold() const {return m_songBrowserFontBold;}
	[[nodiscard]] inline McFont* getFontIcons() const {return m_fontIcons;}

	float getDifficultyMultiplier();
	float getCSDifficultyMultiplier();
	float getScoreMultiplier();
	float getRawSpeedMultiplier();	// without override
	float getSpeedMultiplier();		// with override
	float getPitchMultiplier();

	[[nodiscard]] inline bool getModAuto() const {return m_bModAuto;}
	[[nodiscard]] inline bool getModAutopilot() const {return m_bModAutopilot;}
	[[nodiscard]] inline bool getModRelax() const {return m_bModRelax;}
	[[nodiscard]] inline bool getModSpunout() const {return m_bModSpunout;}
	[[nodiscard]] inline bool getModTarget() const {return m_bModTarget;}
	[[nodiscard]] inline bool getModScorev2() const {return m_bModScorev2;}
	[[nodiscard]] inline bool getModDT() const {return m_bModDT;}
	[[nodiscard]] inline bool getModNC() const {return m_bModNC;}
	[[nodiscard]] inline bool getModNF() const {return m_bModNF;}
	[[nodiscard]] inline bool getModHT() const {return m_bModHT;}
	[[nodiscard]] inline bool getModDC() const {return m_bModDC;}
	[[nodiscard]] inline bool getModHD() const {return m_bModHD;}
	[[nodiscard]] inline bool getModHR() const {return m_bModHR;}
	[[nodiscard]] inline bool getModEZ() const {return m_bModEZ;}
	[[nodiscard]] inline bool getModSD() const {return m_bModSD;}
	[[nodiscard]] inline bool getModSS() const {return m_bModSS;}
	[[nodiscard]] inline bool getModNM() const {return m_bModNM;}
	[[nodiscard]] inline bool getModTD() const {return m_bModTD;}

	[[nodiscard]] inline std::vector<ConVar*> getExperimentalMods() const {return m_experimentalMods;}

	bool isInPlayMode();
	bool isNotInPlayModeOrPaused();

	bool isInMultiplayer();
	[[nodiscard]] inline bool isSkinLoading() const {return m_bSkinLoadScheduled;}

	[[nodiscard]] inline bool isSkipScheduled() const {return m_bSkipScheduled;}
	[[nodiscard]] inline bool isSeeking() const {return m_bSeeking;}
	[[nodiscard]] inline float getQuickSaveTime() const {return m_fQuickSaveTime;}

	bool shouldFallBackToLegacySliderRenderer(); // certain mods or actions require OsuSliders to render dynamically (e.g. wobble or the CS override slider)

	void updateMods();
	void updateConfineCursor();
	void updateMouseSettings();
	void updateWindowsKeyDisable();

private:
	static Vector2 g_vInternalResolution;

	void updateModsForConVarTemplate(const UString&, const UString&) {updateMods();}
	void onVolumeChange(int multiplier);
	void onAudioOutputDeviceChange();

	void rebuildRenderTargets();
	void reloadFonts();
	void fireResolutionChanged();

	// callbacks
	void onInternalResolutionChanged(const UString& oldValue, const UString& args);

	void onSkinReload();
	void onSkinChange(const UString &oldValue, const UString &newValue);

	void onMasterVolumeChange(const UString &oldValue, const UString &newValue);
	void onMusicVolumeChange(const UString &oldValue, const UString &newValue);
	void onSpeedChange(const UString &oldValue, const UString &newValue);
	void onPitchChange(const UString &oldValue, const UString &newValue);

	void onPlayfieldChange(const UString &oldValue, const UString &newValue);

	void onUIScaleChange(const UString &oldValue, const UString &newValue);
	void onUIScaleToDPIChange(const UString &oldValue, const UString &newValue);
	void onLetterboxingChange(const UString &oldValue, const UString &newValue);

	void onConfineCursorWindowedChange(const UString &oldValue, const UString &newValue);
	void onConfineCursorFullscreenChange(const UString &oldValue, const UString &newValue);
	void onConfineCursorNeverChange(const UString &oldValue, const UString &newValue);

	void onKey1Change(bool pressed, bool mouseButton);
	void onKey2Change(bool pressed, bool mouseButton);

	void onModMafhamChange(const UString &oldValue, const UString &newValue);
	void onModFPoSuChange(const UString &oldValue, const UString &newValue);
	void onModFPoSu3DChange(const UString &oldValue, const UString &newValue);
	void onModFPoSu3DSpheresChange(const UString &oldValue, const UString &newValue);
	void onModFPoSu3DSpheresAAChange(const UString &oldValue, const UString &newValue);

	void onLetterboxingOffsetChange(const UString &oldValue, const UString &newValue);

	void onNotification(const UString& args);

	// interfaces
	OsuMultiplayer *m_multiplayer;
	OsuMainMenu *m_mainMenu;
	OsuOptionsMenu *m_optionsMenu;
	OsuSongBrowser2 *m_songBrowser2;
	OsuBackgroundImageHandler *m_backgroundImageHandler;
	OsuModSelector *m_modSelector;
	OsuRankingScreen *m_rankingScreen;
	OsuUserStatsScreen *m_userStatsScreen;
	OsuPauseMenu *m_pauseMenu;
	OsuSkin *m_skin;
	OsuHUD *m_hud;
	OsuTooltipOverlay *m_tooltipOverlay;
	OsuNotificationOverlay *m_notificationOverlay;
	OsuScore *m_score;
	OsuChangelog *m_changelog;
	OsuEditor *m_editor;
	OsuUpdateHandler *m_updateHandler;
	[[maybe_unused]] OsuSteamWorkshop *m_steamWorkshop;
	OsuModFPoSu *m_fposu;
	OsuKeyBindings *m_bindings;

	std::vector<OsuScreen*> m_screens;
	float m_fPrevNoScreenVisibleWorkaroundTime;

	// rendering
	RenderTarget *m_backBuffer;
	RenderTarget *m_playfieldBuffer;
	RenderTarget *m_sliderFrameBuffer;
	RenderTarget *m_frameBuffer;
	RenderTarget *m_frameBuffer2;
	Vector2 m_vInternalResolution;

	// mods
	bool m_bModAuto;
	bool m_bModAutopilot;
	bool m_bModRelax;
	bool m_bModSpunout;
	bool m_bModTarget;
	bool m_bModScorev2;
	bool m_bModDT;
	bool m_bModNC;
	bool m_bModNF;
	bool m_bModHT;
	bool m_bModDC;
	bool m_bModHD;
	bool m_bModHR;
	bool m_bModEZ;
	bool m_bModSD;
	bool m_bModSS;
	bool m_bModNM;
	bool m_bModTD;

	std::vector<ConVar*> m_experimentalMods;

	// keys
	bool m_bF1;
	bool m_bUIToggleCheck;
	bool m_bScoreboardToggleCheck;
	bool m_bEscape;
	bool m_bKeyboardKey1Down;
	bool m_bKeyboardKey12Down;
	bool m_bKeyboardKey2Down;
	bool m_bKeyboardKey22Down;
	bool m_bMouseKey1Down;
	bool m_bMouseKey2Down;
	bool m_bSkipDownCheck;
	bool m_bSkipScheduled;
	bool m_bQuickRetryDown;
	float m_fQuickRetryTime;
	bool m_bSeekKey;
	bool m_bSeeking;
	float m_fPrevSeekMousePosX;
	float m_fQuickSaveTime;

	// async toggles
	// TODO: this way of doing things is bullshit
	bool m_bToggleModSelectionScheduled;
	bool m_bToggleSongBrowserScheduled;
	bool m_bToggleOptionsMenuScheduled;
	bool m_bOptionsMenuFullscreen;
	bool m_bToggleRankingScreenScheduled;
	bool m_bToggleUserStatsScreenScheduled;
	bool m_bToggleChangelogScheduled;
	bool m_bToggleEditorScheduled;

	// cursor
	bool m_bShouldCursorBeVisible;

	// global resources
	std::vector<McFont*> m_fonts;
	McFont *m_titleFont;
	McFont *m_subTitleFont;
	McFont *m_songBrowserFont;
	McFont *m_songBrowserFontBold;
	McFont *m_fontIcons;

	// debugging
	CWindowManager *m_windowManager;

	// custom
	GAMEMODE m_gamemode;
	bool m_bScheduleEndlessModNextBeatmap;
	int m_iMultiplayerClientNumEscPresses;
	bool m_bWasBossKeyPaused;
	bool m_bSkinLoadScheduled;
	bool m_bSkinLoadWasReload;
	OsuSkin *m_skinScheduledToLoad;
	bool m_bFontReloadScheduled;
	bool m_bFireResolutionChangedScheduled;
	bool m_bVolumeInactiveToActiveScheduled;
	float m_fVolumeInactiveToActiveAnim;
	bool m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled;
};

extern Osu *osu;

using App = Osu;

#endif
