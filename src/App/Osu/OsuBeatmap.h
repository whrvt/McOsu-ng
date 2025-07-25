//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		beatmap loader
//
// $NoKeywords: $osubm
//===============================================================================//

#pragma once
#ifndef OSUBEATMAP_H
#define OSUBEATMAP_H

#include "cbase.h"

#include "OsuScore.h"
#include "OsuDatabaseBeatmap.h"

class Sound;
class ConVar;

class Osu;
class OsuSkin;
class OsuHitObject;

class OsuDatabaseBeatmap;

class OsuBackgroundStarCacheLoader;
class OsuBackgroundStarCalcHandler;

class OsuBeatmapStandard;
class OsuBeatmapMania;
class OsuBeatmapExample;

class OsuBeatmap
{
public:
	struct CLICK
	{
		long musicPos;
		int maniaColumn;
	};
	enum Type : uint8_t { STANDARD, MANIA, EXAMPLE };

public:
	OsuBeatmap();
	virtual ~OsuBeatmap();

	virtual void draw();
	virtual void drawInt();
	virtual void draw3D();
	virtual void draw3D2();
	void drawDebug();
	void drawBackground();
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);

	virtual void onModUpdate() {;}	// this should make all the necessary internal updates to hitobjects when legacy osu mods or static mods change live (but also on start)
	[[nodiscard]] virtual bool isLoading() const;		// allows subclasses to delay the playing start, e.g. to load something

	virtual long getPVS();			// Potentially Visible Set gate time size, for optimizing draw() and update() when iterating over all hitobjects

	// callbacks called by the Osu class (osu!standard)
	void skipEmptySection();
	void keyPressed1(bool mouseButton);
	void keyPressed2(bool mouseButton);
	void keyReleased1(bool mouseButton);
	void keyReleased2(bool mouseButton);

	// songbrowser & player logic
	void select(); // loads the music of the currently selected diff and starts playing from the previewTime (e.g. clicking on a beatmap)
	void selectDifficulty2(OsuDatabaseBeatmap *difficulty2);
	void deselect(); // stops + unloads the currently loaded music and deletes all hitobjects
	bool play();
	void restart(bool quick = false);
	void pause(bool quitIfWaiting = true);
	void pausePreviewMusic(bool toggle = true);
	bool isPreviewMusicPlaying();
	void stop(bool quit = true);
	void fail();
	void cancelFailing();
	void resetScore() {resetScoreInt();}

	// loader
	void setMaxPossibleCombo(int maxPossibleCombo) {m_iMaxPossibleCombo = maxPossibleCombo;}
	void setScoreV2ComboPortionMaximum(unsigned long long scoreV2ComboPortionMaximum) {m_iScoreV2ComboPortionMaximum = scoreV2ComboPortionMaximum;}

	// music/sound
	void unloadMusic() {unloadMusicInt();}
	void setVolume(float volume);
	void setSpeed(float speed);
	void setPitch(float pitch);
	void seekPercent(double percent);
	void seekPercentPlayable(double percent);
	void seekMS(unsigned long ms);

	[[nodiscard]] inline Sound *getMusic() const {return m_music;}
	[[nodiscard]] unsigned long getTime() const;
	[[nodiscard]] unsigned long getStartTimePlayable() const;
	[[nodiscard]] unsigned long getLength() const;
	[[nodiscard]] unsigned long getLengthPlayable() const;
	[[nodiscard]] float getPercentFinished() const;
	[[nodiscard]] float getPercentFinishedPlayable() const;

	// live statistics
	[[nodiscard]] int getMostCommonBPM() const;
	[[nodiscard]] float getSpeedMultiplier() const;
	[[nodiscard]] inline int getNPS() const {return m_iNPS;}
	[[nodiscard]] inline int getND() const {return m_iND;}
	[[nodiscard]] inline int getHitObjectIndexForCurrentTime() const {return m_iCurrentHitObjectIndex;}
	[[nodiscard]] inline int getNumCirclesForCurrentTime() const {return m_iCurrentNumCircles;}
	[[nodiscard]] inline int getNumSlidersForCurrentTime() const {return m_iCurrentNumSliders;}
	[[nodiscard]] inline int getNumSpinnersForCurrentTime() const {return m_iCurrentNumSpinners;}
	[[nodiscard]] inline int getMaxPossibleCombo() const {return m_iMaxPossibleCombo;}
	[[nodiscard]] inline unsigned long long getScoreV2ComboPortionMaximum() const {return m_iScoreV2ComboPortionMaximum;}
	[[nodiscard]] inline double getAimStarsForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_aimStarsForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_aimStarsForNumHitObjects[std::clamp<int>(upToHitObjectIndex, 0, m_aimStarsForNumHitObjects.size()-1)] : 0);}
	[[nodiscard]] inline double getAimSliderFactorForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_aimSliderFactorForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_aimSliderFactorForNumHitObjects[std::clamp<int>(upToHitObjectIndex, 0, m_aimSliderFactorForNumHitObjects.size()-1)] : 0);}
	[[nodiscard]] inline double getAimDifficultSlidersForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_aimDifficultSlidersForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_aimDifficultSlidersForNumHitObjects[std::clamp<int>(upToHitObjectIndex, 0, m_aimDifficultSlidersForNumHitObjects.size()-1)] : 0);}
	[[nodiscard]] inline double getAimDifficultStrainsForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_aimDifficultStrainsForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_aimDifficultStrainsForNumHitObjects[std::clamp<int>(upToHitObjectIndex, 0, m_aimDifficultStrainsForNumHitObjects.size()-1)] : 0);}
	[[nodiscard]] inline double getSpeedStarsForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_speedStarsForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_speedStarsForNumHitObjects[std::clamp<int>(upToHitObjectIndex, 0, m_speedStarsForNumHitObjects.size()-1)] : 0);}
	[[nodiscard]] inline double getSpeedNotesForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_speedNotesForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_speedNotesForNumHitObjects[std::clamp<int>(upToHitObjectIndex, 0, m_speedNotesForNumHitObjects.size()-1)] : 0);}
	[[nodiscard]] inline double getSpeedDifficultStrainsForUpToHitObjectIndex(int upToHitObjectIndex) const {return (m_speedDifficultStrainsForNumHitObjects.size() > 0 && upToHitObjectIndex > -1 ? m_speedDifficultStrainsForNumHitObjects[std::clamp<int>(upToHitObjectIndex, 0, m_speedDifficultStrainsForNumHitObjects.size()-1)] : 0);}
	[[nodiscard]] inline const std::vector<double> &getAimStrains() const {return m_aimStrains;}
	[[nodiscard]] inline const std::vector<double> &getSpeedStrains() const {return m_speedStrains;}

	// used by OsuHitObject children and OsuModSelector
	[[nodiscard]] OsuSkin *getSkin() const; // maybe use this for beatmap skins, maybe
	[[nodiscard]] inline int getRandomSeed() const {return m_iRandomSeed;}

	[[nodiscard]] inline long getCurMusicPos() const {return m_iCurMusicPos;}
	[[nodiscard]] inline long getCurMusicPosWithOffsets() const {return m_iCurMusicPosWithOffsets;}

	[[nodiscard]] float getRawAR() const;
	[[nodiscard]] float getAR() const;
	[[nodiscard]] float getCS() const;
	[[nodiscard]] float getHP() const;
	[[nodiscard]] float getRawOD() const;
	[[nodiscard]] float getOD() const;

	// health
	[[nodiscard]] inline double getHealth() const {return m_fHealth;}
	[[nodiscard]] inline bool hasFailed() const {return m_bFailed;}
	[[nodiscard]] inline double getHPMultiplierNormal() const {return m_fHpMultiplierNormal;}
	[[nodiscard]] inline double getHPMultiplierComboEnd() const {return m_fHpMultiplierComboEnd;}

	// database (legacy)
	[[nodiscard]] inline OsuDatabaseBeatmap *getSelectedDifficulty2() const {return m_selectedDifficulty2;}

	// generic state
	[[nodiscard]] inline bool isPlaying() const {return m_bIsPlaying;}
	[[nodiscard]] inline bool isPaused() const {return m_bIsPaused;}
	[[nodiscard]] inline bool isRestartScheduled() const {return m_bIsRestartScheduled;}
	[[nodiscard]] inline bool isContinueScheduled() const {return m_bContinueScheduled;}
	[[nodiscard]] inline bool isWaiting() const {return m_bIsWaiting;}
	[[nodiscard]] inline bool isInSkippableSection() const {return m_bIsInSkippableSection;}
	[[nodiscard]] inline bool isInBreak() const {return m_bInBreak;}
	[[nodiscard]] inline bool shouldFlashWarningArrows() const {return m_bShouldFlashWarningArrows;}
	[[nodiscard]] inline float shouldFlashSectionPass() const {return m_fShouldFlashSectionPass;}
	[[nodiscard]] inline float shouldFlashSectionFail() const {return m_fShouldFlashSectionFail;}
	[[nodiscard]] bool isClickHeld() const; // is any key currently being held down
	[[nodiscard]] inline bool isKey1Down() const {return m_bClick1Held;}
	[[nodiscard]] inline bool isKey2Down() const {return m_bClick2Held;}
	[[nodiscard]] inline bool isLastKeyDownKey1() const {return m_bPrevKeyWasKey1;}

	[[nodiscard]] virtual Type getType() const = 0;

	virtual OsuBeatmapStandard* asStd() { return nullptr; }
	virtual OsuBeatmapMania* asMania() { return nullptr; }
	virtual OsuBeatmapExample* asExample() { return nullptr; }
	[[nodiscard]] const virtual OsuBeatmapStandard* asStd() const { return nullptr; }
	[[nodiscard]] const virtual OsuBeatmapMania* asMania() const { return nullptr; }
	[[nodiscard]] const virtual OsuBeatmapExample* asExample() const { return nullptr; }

	[[nodiscard]] UString getTitle() const;
	[[nodiscard]] UString getArtist() const;

	[[nodiscard]] inline const std::vector<OsuDatabaseBeatmap::BREAK> &getBreaks() const {return m_breaks;}
	[[nodiscard]] unsigned long getBreakDurationTotal() const;
	[[nodiscard]] OsuDatabaseBeatmap::BREAK getBreakForTimeRange(long startMS, long positionMS, long endMS) const;

	// OsuHitObject and other helper functions
	OsuScore::HIT addHitResult(OsuHitObject *hitObject, OsuScore::HIT hit, long delta, bool isEndOfCombo = false, bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false, bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false);
	void addSliderBreak();
	void addScorePoints(int points, bool isSpinner = false);
	void addHealth(double percent, bool isFromHitResult);
	void updateTimingPoints(long curPos);

	// ILLEGAL:
	[[nodiscard]] inline const std::vector<OsuHitObject*> &getHitObjectsPointer() const {return m_hitobjects;}
	[[nodiscard]] inline float getBreakBackgroundFadeAnim() const {return m_fBreakBackgroundFade;}

protected:
	
	

	
	
	
	
	
	
	

	
	
	
	
	
	
	
	

	

	// overridable child events
	virtual void onBeforeLoad() {;}			 // called before hitobjects are loaded
	virtual void onLoad() {;}				 // called after hitobjects have been loaded
	virtual void onPlayStart() {;}			 // called when the player starts playing (everything has been loaded, including the music)
	virtual void onBeforeStop(bool  /*quit*/) {;} // called before hitobjects are unloaded (quit = don't display ranking screen)
	virtual void onStop(bool  /*quit*/) {;}		 // called after hitobjects have been unloaded, but before Osu::onPlayEnd() (quit = don't display ranking screen)
	virtual void onPaused(bool  /*first*/) {;}
	virtual void onUnpaused() {;}
	virtual void onRestart(bool  /*quick*/) {;}

	// internal
	bool canDraw();
	bool canUpdate();

	void actualRestart();

	void handlePreviewPlay();
	void loadMusic(bool stream = true, bool prescan = false);
	void unloadMusicInt();
	void unloadObjects();

	void resetHitObjects(long curPos = 0);
	void resetScoreInt();

	void playMissSound();

	unsigned long getMusicPositionMSInterpolated();

	// beatmap state
	bool m_bIsPlaying;
	bool m_bIsPaused;
	bool m_bIsWaiting;
	bool m_bIsRestartScheduled;
	bool m_bIsRestartScheduledQuick;

	bool m_bIsInSkippableSection;
	bool m_bShouldFlashWarningArrows;
	float m_fShouldFlashSectionPass;
	float m_fShouldFlashSectionFail;
	bool m_bContinueScheduled;
	unsigned long m_iContinueMusicPos;
	double m_fWaitTime;
	double m_fPrevUnpauseTime;

	// database
	OsuDatabaseBeatmap *m_selectedDifficulty2;

	// sound
	Sound *m_music;
	float m_fMusicFrequencyBackup;
	long m_iCurMusicPos;
	long m_iCurMusicPosWithOffsets;
	bool m_bWasSeekFrame;
	double m_fInterpolatedMusicPos;
	double m_fLastAudioTimeAccurateSet;
	double m_fLastRealTimeForInterpolationDelta;
	int m_iResourceLoadUpdateDelayHack;
	bool m_bForceStreamPlayback;
	double m_fAfterMusicIsFinishedVirtualAudioTimeStart;
	bool m_bIsFirstMissSound;

	// health
	bool m_bFailed;
	float m_fFailAnim;
	double m_fHealth;
	float m_fHealth2;

	// drain
	double m_fDrainRate;
	double m_fHpMultiplierNormal;
	double m_fHpMultiplierComboEnd;

	// breaks
	std::vector<OsuDatabaseBeatmap::BREAK> m_breaks;
	float m_fBreakBackgroundFade;
	bool m_bInBreak;
	OsuHitObject *m_currentHitObject;
	long m_iNextHitObjectTime;
	long m_iPreviousHitObjectTime;
	long m_iPreviousSectionPassFailTime;

	// player input
	bool m_bClick1Held;
	bool m_bClick2Held;
	bool m_bClickedContinue;
	bool m_bPrevKeyWasKey1;
	int m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex;
	std::vector<CLICK> m_clicks;
	std::vector<CLICK> m_keyUps;

	// hitobjects
	std::vector<OsuHitObject*> m_hitobjects;
	std::vector<OsuHitObject*> m_hitobjectsSortedByEndTime;
	std::vector<OsuHitObject*> m_misaimObjects;
	int m_iRandomSeed;

	// statistics
	int m_iNPS;
	int m_iND;
	int m_iCurrentHitObjectIndex;
	int m_iCurrentNumCircles;
	int m_iCurrentNumSliders;
	int m_iCurrentNumSpinners;
	int m_iMaxPossibleCombo;
	unsigned long long m_iScoreV2ComboPortionMaximum;
	std::vector<double> m_aimStarsForNumHitObjects;
	std::vector<double> m_aimSliderFactorForNumHitObjects;
	std::vector<double> m_aimDifficultSlidersForNumHitObjects;
	std::vector<double> m_aimDifficultStrainsForNumHitObjects;
	std::vector<double> m_speedStarsForNumHitObjects;
	std::vector<double> m_speedNotesForNumHitObjects;
	std::vector<double> m_speedDifficultStrainsForNumHitObjects;
	std::vector<double> m_aimStrains;
	std::vector<double> m_speedStrains;

	// custom
	int m_iPreviousFollowPointObjectIndex; // TODO: this shouldn't be in this class

private:
	friend class OsuBackgroundStarCacheLoader;
	friend class OsuBackgroundStarCalcHandler;
};

#endif
