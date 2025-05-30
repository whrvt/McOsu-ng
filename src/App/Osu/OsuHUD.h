//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		hud element drawing functions (accuracy, combo, score, etc.)
//
// $NoKeywords: $osuhud
//===============================================================================//

#pragma once
#ifndef OSUHUD_H
#define OSUHUD_H

#include "OsuScreen.h"

class Osu;
class OsuScore;
class OsuBeatmapStandard;

class McFont;
class ConVar;
class Image;
class Shader;
class VertexArrayObject;

class OsuUIVolumeSlider;

class CBaseUIContainer;

class OsuHUD final : public OsuScreen
{
public:
	OsuHUD();
	~OsuHUD() override;

	void draw() override;
	void update() override;

	void onResolutionChange(Vector2 newResolution) override;

	void drawDummy();

	void drawCursor(Vector2 pos, float alphaMultiplier = 1.0f, bool secondTrail = false, bool updateAndDrawTrail = true);
	void drawCursorTrail(Vector2 pos, float alphaMultiplier = 1.0f, bool secondTrail = false); // NOTE: only use if drawCursor() with updateAndDrawTrail = false (FPoSu)
	void drawCursorSpectator1(Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorSpectator2(Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorRipples();
	void drawFps() {drawFps(m_tempFont, m_fCurFps);}
	void drawHitErrorBar(OsuBeatmapStandard *beatmapStd);
	void drawPlayfieldBorder(Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter);
	void drawPlayfieldBorder(Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter, float borderSize);
	void drawLoadingSmall();
	void drawBeatmapImportSpinner();
	void drawVolumeChange();
	void drawScoreNumber(unsigned long long number, float scale = 1.0f, bool drawLeadingZeroes = false);
	void drawComboNumber(unsigned long long number, float scale = 1.0f, bool drawLeadingZeroes = false);
	void drawComboSimple(int combo, float scale = 1.0f); // used by OsuRankingScreen
	void drawAccuracySimple(float accuracy, float scale = 1.0f); // used by OsuRankingScreen
	void drawWarningArrow(Vector2 pos, bool flipVertically, bool originLeft = true);
	void drawScoreBoard(std::string &beatmapMD5Hash, OsuScore *currentScore);
	void drawScoreBoardMP();
	void drawScorebarBg(float alpha, float breakAnim);
	void drawSectionPass(float alpha);
	void drawSectionFail(float alpha);

	void animateCombo();
	void addHitError(long delta, bool miss = false, bool misaim = false);
	void addTarget(float delta, float angle);
	void animateInputoverlay(int key, bool down);

	void animateVolumeChange();
	void addCursorRipple(Vector2 pos);
	void animateCursorExpand();
	void animateCursorShrink();
	void animateKiBulge();
	void animateKiExplode();

	void selectVolumePrev();
	void selectVolumeNext();

	void resetHitErrorBar();

	McRect getSkipClickRect();

	bool isVolumeOverlayVisible();
	bool isVolumeOverlayBusy();

	OsuUIVolumeSlider *getVolumeMasterSlider() {return m_volumeMaster;}
	OsuUIVolumeSlider *getVolumeEffectsSlider() {return m_volumeEffects;}
	OsuUIVolumeSlider *getVolumeMusicSlider() {return m_volumeMusic;}

	void drawSkip();

	// ILLEGAL:
	[[nodiscard]] inline float getScoreBarBreakAnim() const {return m_fScoreBarBreakAnim;}

private:
	struct CURSORTRAIL
	{
		Vector2 pos;
		float time;
		float alpha;
		float scale;
	};

	struct CURSORRIPPLE
	{
		Vector2 pos;
		float time;
	};

	struct HITERROR
	{
		float time;
		long delta;
		bool miss;
		bool misaim;
	};

	struct TARGET
	{
		float time;
		float delta;
		float angle;
	};

	struct SCORE_ENTRY
	{
		UString name;

		int index;
		int combo;
		unsigned long long score;
		float accuracy;

		bool missingBeatmap;
		bool downloadingBeatmap;
		bool dead;
		bool highlight;
	};

	struct BREAK
	{
		float startPercent;
		float endPercent;
	};

	void updateLayout();

	void addCursorTrailPosition(std::vector<CURSORTRAIL> &trail, Vector2 pos, bool empty = false);

	void drawCursorInt(Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp, Vector2 pos, float alphaMultiplier = 1.0f, bool emptyTrailFrame = false, bool updateAndDrawTrail = true);
	void drawCursorRaw(Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorTrailInt(Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp, Vector2 pos, float alphaMultiplier = 1.0f, bool emptyTrailFrame = false);
	void drawCursorTrailRaw(float alpha, Vector2 pos);
	void drawFps(McFont *font, float fps);
	void drawAccuracy(float accuracy);
	void drawCombo(int combo);
	void drawScore(unsigned long long score);
	void drawHPBar(double health, float alpha, float breakAnim);
	void drawScoreBoardInt(const std::vector<SCORE_ENTRY> &scoreEntries);

	void drawWarningArrows(float hitcircleDiameter = 0.0f);
	void drawContinue(Vector2 cursor, float hitcircleDiameter = 0.0f);
	void drawHitErrorBar(float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss, int ur);
	void drawHitErrorBarInt(float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss);
	void drawHitErrorBarInt2(Vector2 center, int ur);
	void drawProgressBar(float percent, bool waiting);
	void drawStatistics(int misses, int sliderbreaks, int maxPossibleCombo, float liveStars, float totalStars, int bpm, float ar, float cs, float od, float hp, int nps, int nd, int ur, float pp, float ppfc, float hitWindow300, int hitdeltaMin, int hitdeltaMax);
	void drawTargetHeatmap(float hitcircleDiameter);
	void drawScrubbingTimeline(unsigned long beatmapTime, unsigned long beatmapLength, unsigned long beatmapLengthPlayable, unsigned long beatmapStartTimePlayable, float beatmapPercentFinishedPlayable, const std::vector<BREAK> &breaks);
	void drawInputOverlay(int numK1, int numK2, int numM1, int numM2);

	float getCursorScaleFactor();
	float getCursorTrailScaleFactor();

	float getScoreScale();

	void onVolumeOverlaySizeChange(UString oldValue, UString newValue);

	McFont *m_tempFont;

	ConVar *m_name_ref;
	ConVar *m_host_timescale_ref;
	ConVar *m_osu_volume_master_ref;
	ConVar *m_osu_volume_effects_ref;
	ConVar *m_osu_volume_music_ref;
	ConVar *m_osu_volume_change_interval_ref;
	ConVar *m_osu_mod_target_300_percent_ref;
	ConVar *m_osu_mod_target_100_percent_ref;
	ConVar *m_osu_mod_target_50_percent_ref;
	ConVar *m_osu_mod_fposu_ref;
	ConVar *m_fposu_draw_scorebarbg_on_top_ref;
	ConVar *m_osu_playfield_stretch_x_ref;
	ConVar *m_osu_playfield_stretch_y_ref;
	ConVar *m_osu_mp_win_condition_accuracy_ref;
	ConVar *m_osu_background_dim_ref;
	ConVar *m_osu_skip_intro_enabled_ref;
	ConVar *m_osu_skip_breaks_enabled_ref;

	// shit code
	float m_fAccuracyXOffset;
	float m_fAccuracyYOffset;
	float m_fScoreHeight;

	float m_fComboAnim1;
	float m_fComboAnim2;

	// fps counter
	float m_fCurFps;
	float m_fCurFpsSmooth;
	float m_fFpsUpdate;

	// hit error bar
	std::vector<HITERROR> m_hiterrors;

	// inputoverlay / key overlay
	float m_fInputoverlayK1AnimScale;
	float m_fInputoverlayK2AnimScale;
	float m_fInputoverlayM1AnimScale;
	float m_fInputoverlayM2AnimScale;

	float m_fInputoverlayK1AnimColor;
	float m_fInputoverlayK2AnimColor;
	float m_fInputoverlayM1AnimColor;
	float m_fInputoverlayM2AnimColor;

	// volume
	float m_fLastVolume;
	float m_fVolumeChangeTime;
	float m_fVolumeChangeFade;
	CBaseUIContainer *m_volumeSliderOverlayContainer;
	OsuUIVolumeSlider *m_volumeMaster;
	OsuUIVolumeSlider *m_volumeEffects;
	OsuUIVolumeSlider *m_volumeMusic;

	// cursor & trail & ripples
	float m_fCursorExpandAnim;
	std::vector<CURSORTRAIL> m_cursorTrail;
	std::vector<CURSORTRAIL> m_cursorTrail2;
	std::vector<CURSORTRAIL> m_cursorTrailSpectator1;
	std::vector<CURSORTRAIL> m_cursorTrailSpectator2;
	Shader *m_cursorTrailShader;
	VertexArrayObject *m_cursorTrailVAO;
	std::vector<CURSORRIPPLE> m_cursorRipples;

	// target heatmap
	std::vector<TARGET> m_targets;

	// health
	double m_fHealth;
	float m_fScoreBarBreakAnim;
	float m_fKiScaleAnim;
};

#endif
