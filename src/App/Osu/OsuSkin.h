//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		skin loader and container
//
// $NoKeywords: $osusk
//===============================================================================//

#pragma once
#ifndef OSUSKIN_H
#define OSUSKIN_H

#include "cbase.h"

class Image;
class Sound;
class Resource;
class ConVar;

class Osu;
class OsuSkinImage;

class OsuSkin
{
public:
	static constexpr const char *OSUSKIN_DEFAULT_SKIN_PATH = "default/";

	static ConVar *m_osu_skin_async;
	static ConVar *m_osu_skin_hd;

public:
	OsuSkin(UString name, UString filepath, bool isDefaultSkin = false, bool isWorkshopSkin = false);
	virtual ~OsuSkin();

	void update();

	bool isReady();
	[[nodiscard]] inline bool isWorkshopSkin() const {return m_bIsWorkshopSkin;}

	void load();
	void loadBeatmapOverride(UString filepath);
	void reloadSounds();

	// samples
	void setSampleSet(int sampleSet);
	void setSampleVolume(float volume, bool force = false);

	void playHitCircleSound(int sampleType, float pan = 0.0f);
	void playSliderTickSound(float pan = 0.0f);
	void playSliderSlideSound(float pan = 0.0f);
	void playSpinnerSpinSound();
	void playSpinnerBonusSound();

	void stopSliderSlideSound(int sampleSet = -2);
	void stopSpinnerSpinSound();

	// custom
	void randomizeFilePath();

	// drawable helpers
	[[nodiscard]] inline UString getName() const {return m_sName;}
	[[nodiscard]] inline UString getFilePath() const {return m_sFilePath;}

	// raw
	[[nodiscard]] inline Image *getMissingTexture() const {return m_missingTexture;}

	[[nodiscard]] inline Image *getHitCircle() const {return m_hitCircle;}
	[[nodiscard]] inline OsuSkinImage *getHitCircleOverlay2() const {return m_hitCircleOverlay2;}
	[[nodiscard]] inline Image *getApproachCircle() const {return m_approachCircle;}
	[[nodiscard]] inline Image *getReverseArrow() const {return m_reverseArrow;}
	[[nodiscard]] inline OsuSkinImage *getFollowPoint2() const {return m_followPoint2;}

	[[nodiscard]] inline Image *getDefault0() const {return m_default0;}
	[[nodiscard]] inline Image *getDefault1() const {return m_default1;}
	[[nodiscard]] inline Image *getDefault2() const {return m_default2;}
	[[nodiscard]] inline Image *getDefault3() const {return m_default3;}
	[[nodiscard]] inline Image *getDefault4() const {return m_default4;}
	[[nodiscard]] inline Image *getDefault5() const {return m_default5;}
	[[nodiscard]] inline Image *getDefault6() const {return m_default6;}
	[[nodiscard]] inline Image *getDefault7() const {return m_default7;}
	[[nodiscard]] inline Image *getDefault8() const {return m_default8;}
	[[nodiscard]] inline Image *getDefault9() const {return m_default9;}

	[[nodiscard]] inline Image *getScore0() const {return m_score0;}
	[[nodiscard]] inline Image *getScore1() const {return m_score1;}
	[[nodiscard]] inline Image *getScore2() const {return m_score2;}
	[[nodiscard]] inline Image *getScore3() const {return m_score3;}
	[[nodiscard]] inline Image *getScore4() const {return m_score4;}
	[[nodiscard]] inline Image *getScore5() const {return m_score5;}
	[[nodiscard]] inline Image *getScore6() const {return m_score6;}
	[[nodiscard]] inline Image *getScore7() const {return m_score7;}
	[[nodiscard]] inline Image *getScore8() const {return m_score8;}
	[[nodiscard]] inline Image *getScore9() const {return m_score9;}
	[[nodiscard]] inline Image *getScoreX() const {return m_scoreX;}
	[[nodiscard]] inline Image *getScorePercent() const {return m_scorePercent;}
	[[nodiscard]] inline Image *getScoreDot() const {return m_scoreDot;}

	[[nodiscard]] inline Image *getCombo0() const {return m_combo0;}
	[[nodiscard]] inline Image *getCombo1() const {return m_combo1;}
	[[nodiscard]] inline Image *getCombo2() const {return m_combo2;}
	[[nodiscard]] inline Image *getCombo3() const {return m_combo3;}
	[[nodiscard]] inline Image *getCombo4() const {return m_combo4;}
	[[nodiscard]] inline Image *getCombo5() const {return m_combo5;}
	[[nodiscard]] inline Image *getCombo6() const {return m_combo6;}
	[[nodiscard]] inline Image *getCombo7() const {return m_combo7;}
	[[nodiscard]] inline Image *getCombo8() const {return m_combo8;}
	[[nodiscard]] inline Image *getCombo9() const {return m_combo9;}
	[[nodiscard]] inline Image *getComboX() const {return m_comboX;}

	[[nodiscard]] inline OsuSkinImage *getPlaySkip() const {return m_playSkip;}
	[[nodiscard]] inline Image *getPlayWarningArrow() const {return m_playWarningArrow;}
	[[nodiscard]] inline OsuSkinImage *getPlayWarningArrow2() const {return m_playWarningArrow2;}
	[[nodiscard]] inline Image *getCircularmetre() const {return m_circularmetre;}
	[[nodiscard]] inline OsuSkinImage *getScorebarBg() const {return m_scorebarBg;}
	[[nodiscard]] inline OsuSkinImage *getScorebarColour() const {return m_scorebarColour;}
	[[nodiscard]] inline OsuSkinImage *getScorebarMarker() const {return m_scorebarMarker;}
	[[nodiscard]] inline OsuSkinImage *getScorebarKi() const {return m_scorebarKi;}
	[[nodiscard]] inline OsuSkinImage *getScorebarKiDanger() const {return m_scorebarKiDanger;}
	[[nodiscard]] inline OsuSkinImage *getScorebarKiDanger2() const {return m_scorebarKiDanger2;}
	[[nodiscard]] inline OsuSkinImage *getSectionPassImage() const {return m_sectionPassImage;}
	[[nodiscard]] inline OsuSkinImage *getSectionFailImage() const {return m_sectionFailImage;}
	[[nodiscard]] inline OsuSkinImage *getInputoverlayBackground() const {return m_inputoverlayBackground;}
	[[nodiscard]] inline OsuSkinImage *getInputoverlayKey() const {return m_inputoverlayKey;}

	[[nodiscard]] inline OsuSkinImage *getHit0() const {return m_hit0;}
	[[nodiscard]] inline OsuSkinImage *getHit50() const {return m_hit50;}
	[[nodiscard]] inline OsuSkinImage *getHit50g() const {return m_hit50g;}
	[[nodiscard]] inline OsuSkinImage *getHit50k() const {return m_hit50k;}
	[[nodiscard]] inline OsuSkinImage *getHit100() const {return m_hit100;}
	[[nodiscard]] inline OsuSkinImage *getHit100g() const {return m_hit100g;}
	[[nodiscard]] inline OsuSkinImage *getHit100k() const {return m_hit100k;}
	[[nodiscard]] inline OsuSkinImage *getHit300() const {return m_hit300;}
	[[nodiscard]] inline OsuSkinImage *getHit300g() const {return m_hit300g;}
	[[nodiscard]] inline OsuSkinImage *getHit300k() const {return m_hit300k;}

	[[nodiscard]] inline Image *getParticle50() const {return m_particle50;}
	[[nodiscard]] inline Image *getParticle100() const {return m_particle100;}
	[[nodiscard]] inline Image *getParticle300() const {return m_particle300;}

	[[nodiscard]] inline Image *getSliderGradient() const {return m_sliderGradient;}
	[[nodiscard]] inline OsuSkinImage *getSliderb() const {return m_sliderb;}
	[[nodiscard]] inline OsuSkinImage *getSliderFollowCircle2() const {return m_sliderFollowCircle2;}
	[[nodiscard]] inline Image *getSliderScorePoint() const {return m_sliderScorePoint;}
	[[nodiscard]] inline Image *getSliderStartCircle() const {return m_sliderStartCircle;}
	[[nodiscard]] inline OsuSkinImage *getSliderStartCircle2() const {return m_sliderStartCircle2;}
	[[nodiscard]] inline Image *getSliderStartCircleOverlay() const {return m_sliderStartCircleOverlay;}
	[[nodiscard]] inline OsuSkinImage *getSliderStartCircleOverlay2() const {return m_sliderStartCircleOverlay2;}
	[[nodiscard]] inline Image *getSliderEndCircle() const {return m_sliderEndCircle;}
	[[nodiscard]] inline OsuSkinImage *getSliderEndCircle2() const {return m_sliderEndCircle2;}
	[[nodiscard]] inline Image *getSliderEndCircleOverlay() const {return m_sliderEndCircleOverlay;}
	[[nodiscard]] inline OsuSkinImage *getSliderEndCircleOverlay2() const {return m_sliderEndCircleOverlay2;}

	[[nodiscard]] inline Image *getSpinnerBackground() const {return m_spinnerBackground;}
	[[nodiscard]] inline Image *getSpinnerCircle() const {return m_spinnerCircle;}
	[[nodiscard]] inline Image *getSpinnerApproachCircle() const {return m_spinnerApproachCircle;}
	[[nodiscard]] inline Image *getSpinnerBottom() const {return m_spinnerBottom;}
	[[nodiscard]] inline Image *getSpinnerMiddle() const {return m_spinnerMiddle;}
	[[nodiscard]] inline Image *getSpinnerMiddle2() const {return m_spinnerMiddle2;}
	[[nodiscard]] inline Image *getSpinnerTop() const {return m_spinnerTop;}
	[[nodiscard]] inline Image *getSpinnerSpin() const {return m_spinnerSpin;}
	[[nodiscard]] inline Image *getSpinnerClear() const {return m_spinnerClear;}

	[[nodiscard]] inline Image *getDefaultCursor() const {return m_defaultCursor;}
	[[nodiscard]] inline Image *getCursor() const {return m_cursor;}
	[[nodiscard]] inline Image *getCursorMiddle() const {return m_cursorMiddle;}
	[[nodiscard]] inline Image *getCursorTrail() const {return m_cursorTrail;}
	[[nodiscard]] inline Image *getCursorRipple() const {return m_cursorRipple;}

	[[nodiscard]] inline OsuSkinImage *getSelectionModEasy() const {return m_selectionModEasy;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModNoFail() const {return m_selectionModNoFail;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModHalfTime() const {return m_selectionModHalfTime;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModDayCore() const {return m_selectionModDayCore;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModHardRock() const {return m_selectionModHardRock;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModSuddenDeath() const {return m_selectionModSuddenDeath;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModPerfect() const {return m_selectionModPerfect;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModDoubleTime() const {return m_selectionModDoubleTime;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModNightCore() const {return m_selectionModNightCore;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModHidden() const {return m_selectionModHidden;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModFlashlight() const {return m_selectionModFlashlight;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModRelax() const {return m_selectionModRelax;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModAutopilot() const {return m_selectionModAutopilot;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModSpunOut() const {return m_selectionModSpunOut;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModAutoplay() const {return m_selectionModAutoplay;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModNightmare() const {return m_selectionModNightmare;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModTarget() const {return m_selectionModTarget;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModScorev2() const {return m_selectionModScorev2;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModTD() const {return m_selectionModTD;}

	[[nodiscard]] inline Image *getPauseContinue() const {return m_pauseContinue;}
	[[nodiscard]] inline Image *getPauseRetry() const {return m_pauseRetry;}
	[[nodiscard]] inline Image *getPauseBack() const {return m_pauseBack;}
	[[nodiscard]] inline Image *getPauseOverlay() const {return m_pauseOverlay;}
	[[nodiscard]] inline Image *getFailBackground() const {return m_failBackground;}
	[[nodiscard]] inline Image *getUnpause() const {return m_unpause;}

	[[nodiscard]] inline Image *getButtonLeft() const {return m_buttonLeft;}
	[[nodiscard]] inline Image *getButtonMiddle() const {return m_buttonMiddle;}
	[[nodiscard]] inline Image *getButtonRight() const {return m_buttonRight;}
	[[nodiscard]] inline Image *getDefaultButtonLeft() const {return m_defaultButtonLeft;}
	[[nodiscard]] inline Image *getDefaultButtonMiddle() const {return m_defaultButtonMiddle;}
	[[nodiscard]] inline Image *getDefaultButtonRight() const {return m_defaultButtonRight;}
	[[nodiscard]] inline OsuSkinImage *getMenuBack2() const {return m_menuBack;}
	[[nodiscard]] inline OsuSkinImage *getSelectionMode() const {return m_selectionMode;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModeOver() const {return m_selectionModeOver;}
	[[nodiscard]] inline OsuSkinImage *getSelectionMods() const {return m_selectionMods;}
	[[nodiscard]] inline OsuSkinImage *getSelectionModsOver() const {return m_selectionModsOver;}
	[[nodiscard]] inline OsuSkinImage *getSelectionRandom() const {return m_selectionRandom;}
	[[nodiscard]] inline OsuSkinImage *getSelectionRandomOver() const {return m_selectionRandomOver;}
	[[nodiscard]] inline OsuSkinImage *getSelectionOptions() const {return m_selectionOptions;}
	[[nodiscard]] inline OsuSkinImage *getSelectionOptionsOver() const {return m_selectionOptionsOver;}

	[[nodiscard]] inline Image *getSongSelectTop() const {return m_songSelectTop;}
	[[nodiscard]] inline Image *getSongSelectBottom() const {return m_songSelectBottom;}
	[[nodiscard]] inline Image *getMenuButtonBackground() const {return m_menuButtonBackground;}
	[[nodiscard]] inline OsuSkinImage *getMenuButtonBackground2() const {return m_menuButtonBackground2;}
	[[nodiscard]] inline Image *getStar() const {return m_star;}
	[[nodiscard]] inline Image *getRankingPanel() const {return m_rankingPanel;}
	[[nodiscard]] inline Image *getRankingGraph() const {return m_rankingGraph;}
	[[nodiscard]] inline Image *getRankingTitle() const {return m_rankingTitle;}
	[[nodiscard]] inline Image *getRankingMaxCombo() const {return m_rankingMaxCombo;}
	[[nodiscard]] inline Image *getRankingAccuracy() const {return m_rankingAccuracy;}
	[[nodiscard]] inline Image *getRankingA() const {return m_rankingA;}
	[[nodiscard]] inline Image *getRankingB() const {return m_rankingB;}
	[[nodiscard]] inline Image *getRankingC() const {return m_rankingC;}
	[[nodiscard]] inline Image *getRankingD() const {return m_rankingD;}
	[[nodiscard]] inline Image *getRankingS() const {return m_rankingS;}
	[[nodiscard]] inline Image *getRankingSH() const {return m_rankingSH;}
	[[nodiscard]] inline Image *getRankingX() const {return m_rankingX;}
	[[nodiscard]] inline Image *getRankingXH() const {return m_rankingXH;}
	[[nodiscard]] inline OsuSkinImage *getRankingAsmall() const {return m_rankingAsmall;}
	[[nodiscard]] inline OsuSkinImage *getRankingBsmall() const {return m_rankingBsmall;}
	[[nodiscard]] inline OsuSkinImage *getRankingCsmall() const {return m_rankingCsmall;}
	[[nodiscard]] inline OsuSkinImage *getRankingDsmall() const {return m_rankingDsmall;}
	[[nodiscard]] inline OsuSkinImage *getRankingSsmall() const {return m_rankingSsmall;}
	[[nodiscard]] inline OsuSkinImage *getRankingSHsmall() const {return m_rankingSHsmall;}
	[[nodiscard]] inline OsuSkinImage *getRankingXsmall() const {return m_rankingXsmall;}
	[[nodiscard]] inline OsuSkinImage *getRankingXHsmall() const {return m_rankingXHsmall;}
	[[nodiscard]] inline OsuSkinImage *getRankingPerfect() const {return m_rankingPerfect;}

	[[nodiscard]] inline Image *getBeatmapImportSpinner() const {return m_beatmapImportSpinner;}
	[[nodiscard]] inline Image *getLoadingSpinner() const {return m_loadingSpinner;}
	[[nodiscard]] inline Image *getCircleEmpty() const {return m_circleEmpty;}
	[[nodiscard]] inline Image *getCircleFull() const {return m_circleFull;}
	[[nodiscard]] inline Image *getSeekTriangle() const {return m_seekTriangle;}
	[[nodiscard]] inline Image *getUserIcon() const {return m_userIcon;}
	[[nodiscard]] inline Image *getBackgroundCube() const {return m_backgroundCube;}
	[[nodiscard]] inline Image *getMenuBackground() const {return m_menuBackground;}
	[[nodiscard]] inline Image *getSkybox() const {return m_skybox;}

	[[nodiscard]] inline Sound *getSpinnerBonus() const {return m_spinnerBonus;}
	[[nodiscard]] inline Sound *getSpinnerSpinSound() const {return m_spinnerSpinSound;}

	[[nodiscard]] inline Sound *getCombobreak() const {return m_combobreak;}
	[[nodiscard]] inline Sound *getFailsound() const {return m_failsound;}
	[[nodiscard]] inline Sound *getApplause() const {return m_applause;}
	[[nodiscard]] inline Sound *getMenuHit() const {return m_menuHit;}
	[[nodiscard]] inline Sound *getMenuClick() const {return m_menuClick;}
	[[nodiscard]] inline Sound *getCheckOn() const {return m_checkOn;}
	[[nodiscard]] inline Sound *getCheckOff() const {return m_checkOff;}
	[[nodiscard]] inline Sound *getShutter() const {return m_shutter;}
	[[nodiscard]] inline Sound *getSectionPassSound() const {return m_sectionPassSound;}
	[[nodiscard]] inline Sound *getSectionFailSound() const {return m_sectionFailSound;}

	[[nodiscard]] inline bool isCursor2x() const {return m_bCursor2x;}
	[[nodiscard]] inline bool isCursorTrail2x() const {return m_bCursorTrail2x;}
	[[nodiscard]] inline bool isCursorRipple2x() const {return m_bCursorRipple2x;}
	[[nodiscard]] inline bool isApproachCircle2x() const {return m_bApproachCircle2x;}
	[[nodiscard]] inline bool isReverseArrow2x() const {return m_bReverseArrow2x;}
	[[nodiscard]] inline bool isHitCircle2x() const {return m_bHitCircle2x;}
	[[nodiscard]] inline bool isDefault02x() const {return m_bIsDefault02x;}
	[[nodiscard]] inline bool isDefault12x() const {return m_bIsDefault12x;}
	[[nodiscard]] inline bool isScore02x() const {return m_bIsScore02x;}
	[[nodiscard]] inline bool isCombo02x() const {return m_bIsCombo02x;}
	[[nodiscard]] inline bool isSpinnerApproachCircle2x() const {return m_bSpinnerApproachCircle2x;}
	[[nodiscard]] inline bool isSpinnerBottom2x() const {return m_bSpinnerBottom2x;}
	[[nodiscard]] inline bool isSpinnerCircle2x() const {return m_bSpinnerCircle2x;}
	[[nodiscard]] inline bool isSpinnerTop2x() const {return m_bSpinnerTop2x;}
	[[nodiscard]] inline bool isSpinnerMiddle2x() const {return m_bSpinnerMiddle2x;}
	[[nodiscard]] inline bool isSpinnerMiddle22x() const {return m_bSpinnerMiddle22x;}
	[[nodiscard]] inline bool isSliderScorePoint2x() const {return m_bSliderScorePoint2x;}
	[[nodiscard]] inline bool isSliderStartCircle2x() const {return m_bSliderStartCircle2x;}
	[[nodiscard]] inline bool isSliderEndCircle2x() const {return m_bSliderEndCircle2x;}

	[[nodiscard]] inline bool isCircularmetre2x() const {return m_bCircularmetre2x;}

	[[nodiscard]] inline bool isPauseContinue2x() const {return m_bPauseContinue2x;}

	[[nodiscard]] inline bool isMenuButtonBackground2x() const {return m_bMenuButtonBackground2x;}
	[[nodiscard]] inline bool isStar2x() const {return m_bStar2x;}
	[[nodiscard]] inline bool isRankingPanel2x() const {return m_bRankingPanel2x;}
	[[nodiscard]] inline bool isRankingMaxCombo2x() const {return m_bRankingMaxCombo2x;}
	[[nodiscard]] inline bool isRankingAccuracy2x() const {return m_bRankingAccuracy2x;}
	[[nodiscard]] inline bool isRankingA2x() const {return m_bRankingA2x;}
	[[nodiscard]] inline bool isRankingB2x() const {return m_bRankingB2x;}
	[[nodiscard]] inline bool isRankingC2x() const {return m_bRankingC2x;}
	[[nodiscard]] inline bool isRankingD2x() const {return m_bRankingD2x;}
	[[nodiscard]] inline bool isRankingS2x() const {return m_bRankingS2x;}
	[[nodiscard]] inline bool isRankingSH2x() const {return m_bRankingSH2x;}
	[[nodiscard]] inline bool isRankingX2x() const {return m_bRankingX2x;}
	[[nodiscard]] inline bool isRankingXH2x() const {return m_bRankingXH2x;}

	// skin.ini
	[[nodiscard]] inline float getVersion() const {return m_fVersion;}
	[[nodiscard]] inline float getAnimationFramerate() const {return m_fAnimationFramerate;}
	Color getComboColorForCounter(int i, int offset);
	void setBeatmapComboColors(std::vector<Color> colors);
	[[nodiscard]] inline Color getSpinnerApproachCircleColor() const {return m_spinnerApproachCircleColor;}
	[[nodiscard]] inline Color getSliderBorderColor() const {return m_sliderBorderColor;}
	[[nodiscard]] inline Color getSliderTrackOverride() const {return m_sliderTrackOverride;}
	[[nodiscard]] inline Color getSliderBallColor() const {return m_sliderBallColor;}

	[[nodiscard]] inline Color getSongSelectActiveText() const {return m_songSelectActiveText;}
	[[nodiscard]] inline Color getSongSelectInactiveText() const {return m_songSelectInactiveText;}

	[[nodiscard]] inline Color getInputOverlayText() const {return m_inputOverlayText;}

	[[nodiscard]] inline bool getCursorCenter() const {return m_bCursorCenter;}
	[[nodiscard]] inline bool getCursorRotate() const {return m_bCursorRotate;}
	[[nodiscard]] inline bool getCursorExpand() const {return m_bCursorExpand;}

	[[nodiscard]] inline bool getSliderBallFlip() const {return m_bSliderBallFlip;}
	[[nodiscard]] inline bool getAllowSliderBallTint() const {return m_bAllowSliderBallTint;}
	[[nodiscard]] inline int getSliderStyle() const {return m_iSliderStyle;}
	[[nodiscard]] inline bool getHitCircleOverlayAboveNumber() const {return m_bHitCircleOverlayAboveNumber;}
	[[nodiscard]] inline bool isSliderTrackOverridden() const {return m_bSliderTrackOverride;}

	[[nodiscard]] inline UString getComboPrefix() const {return m_sComboPrefix;}
	[[nodiscard]] inline int getComboOverlap() const {return m_iComboOverlap;}

	[[nodiscard]] inline UString getScorePrefix() const {return m_sScorePrefix;}
	[[nodiscard]] inline int getScoreOverlap() const {return m_iScoreOverlap;}

	[[nodiscard]] inline UString getHitCirclePrefix() const {return m_sHitCirclePrefix;}
	[[nodiscard]] inline int getHitCircleOverlap() const {return m_iHitCircleOverlap;}

	// custom
	[[nodiscard]] inline bool useSmoothCursorTrail() const {return m_cursorMiddle != m_missingTexture;}
	[[nodiscard]] inline bool isDefaultSkin() const {return m_bIsDefaultSkin;}
	[[nodiscard]] inline int getSampleSet() const {return m_iSampleSet;}

private:
friend class OsuSkinImage;
	static ConVar *m_osu_skin_ref;
	static ConVar *m_osu_mod_fposu_ref;

	static Image *m_missingTexture;

	struct SOUND_SAMPLE
	{
		Sound *sound;
		float hardcodedVolumeMultiplier; // some samples in osu have hardcoded multipliers which can not be modified (i.e. you can NEVER reach 100% volume with them)
	};

	void onJustBeforeReady();

	bool parseSkinINI(UString filepath);

	bool compareFilenameWithSkinElementName(UString filename, UString skinElementName);

	OsuSkinImage *createOsuSkinImage(UString skinElementName, Vector2 baseSizeForScaling2x, float osuSize, bool ignoreDefaultSkin = false, UString animationSeparator = "-");
	void checkLoadImage(Image **addressOfPointer, UString skinElementName, UString resourceName, bool ignoreDefaultSkin = false, UString fileExtension = "png", bool forceLoadMipmaps = false, bool forceUseDefaultSkin = false);
	void checkLoadSound(Sound **addressOfPointer, UString skinElementName, UString resourceName, bool isOverlayable = false, bool isSample = false, bool loop = false, float hardcodedVolumeMultiplier = -1.0f);

	void onEffectVolumeChange(UString oldValue, UString newValue);
	void onIgnoreBeatmapSampleVolumeChange(UString oldValue, UString newValue);
	void onExport(UString folderName);

	bool skinFileExists(const UString &path);
	UString buildUserPath(const UString &element, const char *ext, bool hd = false) const;
	UString buildDefaultPath(const UString &element, const char *ext, bool hd = false) const;

	bool m_bReady;
	bool m_bReadyOnce;
	bool m_bIsDefaultSkin;
	bool m_bIsWorkshopSkin;
	UString m_sName;
	UString m_sFilePath;
	UString m_sSkinIniFilePath;
	std::vector<Resource*> m_resources;
	std::vector<Sound*> m_sounds;
	std::vector<SOUND_SAMPLE> m_soundSamples;
	std::vector<OsuSkinImage*> m_images;

	std::unordered_map<UString, bool> m_fileExistsCache;

	// images
	Image *m_hitCircle;
	OsuSkinImage *m_hitCircleOverlay2;
	Image *m_approachCircle;
	Image *m_reverseArrow;
	OsuSkinImage *m_followPoint2;

	Image *m_default0;
	Image *m_default1;
	Image *m_default2;
	Image *m_default3;
	Image *m_default4;
	Image *m_default5;
	Image *m_default6;
	Image *m_default7;
	Image *m_default8;
	Image *m_default9;

	Image *m_score0;
	Image *m_score1;
	Image *m_score2;
	Image *m_score3;
	Image *m_score4;
	Image *m_score5;
	Image *m_score6;
	Image *m_score7;
	Image *m_score8;
	Image *m_score9;
	Image *m_scoreX;
	Image *m_scorePercent;
	Image *m_scoreDot;

	Image *m_combo0;
	Image *m_combo1;
	Image *m_combo2;
	Image *m_combo3;
	Image *m_combo4;
	Image *m_combo5;
	Image *m_combo6;
	Image *m_combo7;
	Image *m_combo8;
	Image *m_combo9;
	Image *m_comboX;

	OsuSkinImage *m_playSkip;
	Image *m_playWarningArrow;
	OsuSkinImage *m_playWarningArrow2;
	Image *m_circularmetre;
	OsuSkinImage *m_scorebarBg;
	OsuSkinImage *m_scorebarColour;
	OsuSkinImage *m_scorebarMarker;
	OsuSkinImage *m_scorebarKi;
	OsuSkinImage *m_scorebarKiDanger;
	OsuSkinImage *m_scorebarKiDanger2;
	OsuSkinImage *m_sectionPassImage;
	OsuSkinImage *m_sectionFailImage;
	OsuSkinImage *m_inputoverlayBackground;
	OsuSkinImage *m_inputoverlayKey;

	OsuSkinImage *m_hit0;
	OsuSkinImage *m_hit50;
	OsuSkinImage *m_hit50g;
	OsuSkinImage *m_hit50k;
	OsuSkinImage *m_hit100;
	OsuSkinImage *m_hit100g;
	OsuSkinImage *m_hit100k;
	OsuSkinImage *m_hit300;
	OsuSkinImage *m_hit300g;
	OsuSkinImage *m_hit300k;

	Image *m_particle50;
	Image *m_particle100;
	Image *m_particle300;

	Image *m_sliderGradient;
	OsuSkinImage *m_sliderb;
	OsuSkinImage *m_sliderFollowCircle2;
	Image *m_sliderScorePoint;
	Image *m_sliderStartCircle;
	OsuSkinImage *m_sliderStartCircle2;
	Image *m_sliderStartCircleOverlay;
	OsuSkinImage *m_sliderStartCircleOverlay2;
	Image *m_sliderEndCircle;
	OsuSkinImage *m_sliderEndCircle2;
	Image *m_sliderEndCircleOverlay;
	OsuSkinImage *m_sliderEndCircleOverlay2;

	Image *m_spinnerBackground;
	Image *m_spinnerCircle;
	Image *m_spinnerApproachCircle;
	Image *m_spinnerBottom;
	Image *m_spinnerMiddle;
	Image *m_spinnerMiddle2;
	Image *m_spinnerTop;
	Image *m_spinnerSpin;
	Image *m_spinnerClear;

	Image *m_defaultCursor;
	Image *m_cursor;
	Image *m_cursorMiddle;
	Image *m_cursorTrail;
	Image *m_cursorRipple;

	OsuSkinImage *m_selectionModEasy;
	OsuSkinImage *m_selectionModNoFail;
	OsuSkinImage *m_selectionModHalfTime;
	OsuSkinImage *m_selectionModDayCore;
	OsuSkinImage *m_selectionModHardRock;
	OsuSkinImage *m_selectionModSuddenDeath;
	OsuSkinImage *m_selectionModPerfect;
	OsuSkinImage *m_selectionModDoubleTime;
	OsuSkinImage *m_selectionModNightCore;
	OsuSkinImage *m_selectionModHidden;
	OsuSkinImage *m_selectionModFlashlight;
	OsuSkinImage *m_selectionModRelax;
	OsuSkinImage *m_selectionModAutopilot;
	OsuSkinImage *m_selectionModSpunOut;
	OsuSkinImage *m_selectionModAutoplay;
	OsuSkinImage *m_selectionModNightmare;
	OsuSkinImage *m_selectionModTarget;
	OsuSkinImage *m_selectionModScorev2;
	OsuSkinImage *m_selectionModTD;
	OsuSkinImage *m_selectionModCinema;

	Image *m_pauseContinue;
	Image *m_pauseReplay;
	Image *m_pauseRetry;
	Image *m_pauseBack;
	Image *m_pauseOverlay;
	Image *m_failBackground;
	Image *m_unpause;

	Image *m_buttonLeft;
	Image *m_buttonMiddle;
	Image *m_buttonRight;
	Image *m_defaultButtonLeft;
	Image *m_defaultButtonMiddle;
	Image *m_defaultButtonRight;
	OsuSkinImage *m_menuBack;
	OsuSkinImage *m_selectionMode;
	OsuSkinImage *m_selectionModeOver;
	OsuSkinImage *m_selectionMods;
	OsuSkinImage *m_selectionModsOver;
	OsuSkinImage *m_selectionRandom;
	OsuSkinImage *m_selectionRandomOver;
	OsuSkinImage *m_selectionOptions;
	OsuSkinImage *m_selectionOptionsOver;

	Image *m_songSelectTop;
	Image *m_songSelectBottom;
	Image *m_menuButtonBackground;
	OsuSkinImage *m_menuButtonBackground2;
	Image *m_star;
	Image *m_rankingPanel;
	Image *m_rankingGraph;
	Image *m_rankingTitle;
	Image *m_rankingMaxCombo;
	Image *m_rankingAccuracy;
	Image *m_rankingA;
	Image *m_rankingB;
	Image *m_rankingC;
	Image *m_rankingD;
	Image *m_rankingS;
	Image *m_rankingSH;
	Image *m_rankingX;
	Image *m_rankingXH;
	OsuSkinImage *m_rankingAsmall;
	OsuSkinImage *m_rankingBsmall;
	OsuSkinImage *m_rankingCsmall;
	OsuSkinImage *m_rankingDsmall;
	OsuSkinImage *m_rankingSsmall;
	OsuSkinImage *m_rankingSHsmall;
	OsuSkinImage *m_rankingXsmall;
	OsuSkinImage *m_rankingXHsmall;
	OsuSkinImage *m_rankingPerfect;

	Image *m_beatmapImportSpinner;
	Image *m_loadingSpinner;
	Image *m_circleEmpty;
	Image *m_circleFull;
	Image *m_seekTriangle;
	Image *m_userIcon;
	Image *m_backgroundCube;
	Image *m_menuBackground;
	Image *m_skybox;

	// sounds
	Sound *m_normalHitNormal;
	Sound *m_normalHitWhistle;
	Sound *m_normalHitFinish;
	Sound *m_normalHitClap;

	Sound *m_normalSliderTick;
	Sound *m_normalSliderSlide;
	Sound *m_normalSliderWhistle;

	Sound *m_softHitNormal;
	Sound *m_softHitWhistle;
	Sound *m_softHitFinish;
	Sound *m_softHitClap;

	Sound *m_softSliderTick;
	Sound *m_softSliderSlide;
	Sound *m_softSliderWhistle;

	Sound *m_drumHitNormal;
	Sound *m_drumHitWhistle;
	Sound *m_drumHitFinish;
	Sound *m_drumHitClap;

	Sound *m_drumSliderTick;
	Sound *m_drumSliderSlide;
	Sound *m_drumSliderWhistle;

	Sound *m_spinnerBonus;
	Sound *m_spinnerSpinSound;

	Sound *m_combobreak;
	Sound *m_failsound;
	Sound *m_applause;
	Sound *m_menuHit;
	Sound *m_menuClick;
	Sound *m_checkOn;
	Sound *m_checkOff;
	Sound *m_shutter;
	Sound *m_sectionPassSound;
	Sound *m_sectionFailSound;


	// colors
	std::vector<Color> m_comboColors;
	std::vector<Color> m_beatmapComboColors;
	Color m_spinnerApproachCircleColor;
	Color m_sliderBorderColor;
	Color m_sliderTrackOverride;
	Color m_sliderBallColor;

	Color m_songSelectInactiveText;
	Color m_songSelectActiveText;

	Color m_inputOverlayText;

	// scaling
	bool m_bCursor2x;
	bool m_bCursorTrail2x;
	bool m_bCursorRipple2x;
	bool m_bApproachCircle2x;
	bool m_bReverseArrow2x;
	bool m_bHitCircle2x;
	bool m_bIsDefault02x;
	bool m_bIsDefault12x;
	bool m_bIsScore02x;
	bool m_bIsCombo02x;
	bool m_bSpinnerApproachCircle2x;
	bool m_bSpinnerBottom2x;
	bool m_bSpinnerCircle2x;
	bool m_bSpinnerTop2x;
	bool m_bSpinnerMiddle2x;
	bool m_bSpinnerMiddle22x;
	bool m_bSliderScorePoint2x;
	bool m_bSliderStartCircle2x;
	bool m_bSliderEndCircle2x;

	bool m_bCircularmetre2x;

	bool m_bPauseContinue2x;

	bool m_bMenuButtonBackground2x;
	bool m_bStar2x;
	bool m_bRankingPanel2x;
	bool m_bRankingMaxCombo2x;
	bool m_bRankingAccuracy2x;
	bool m_bRankingA2x;
	bool m_bRankingB2x;
	bool m_bRankingC2x;
	bool m_bRankingD2x;
	bool m_bRankingS2x;
	bool m_bRankingSH2x;
	bool m_bRankingX2x;
	bool m_bRankingXH2x;

	// skin.ini
	float m_fVersion;
	float m_fAnimationFramerate;
	bool m_bCursorCenter;
	bool m_bCursorRotate;
	bool m_bCursorExpand;

	bool m_bSliderBallFlip;
	bool m_bAllowSliderBallTint;
	int m_iSliderStyle;
	bool m_bHitCircleOverlayAboveNumber;
	bool m_bSliderTrackOverride;

	UString m_sComboPrefix;
	int m_iComboOverlap;

	UString m_sScorePrefix;
	int m_iScoreOverlap;

	UString m_sHitCirclePrefix;
	int m_iHitCircleOverlap;

	// custom
	int m_iSampleSet;
	int m_iSampleVolume;

	std::vector<UString> filepathsForRandomSkin;
	bool m_bIsRandom;
	bool m_bIsRandomElements;

	std::vector<UString> m_filepathsForExport;
};

#endif
