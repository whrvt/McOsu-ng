//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap browser and selector
//
// $NoKeywords: $osusb
//===============================================================================//

#pragma once
#ifndef OSUSONGBROWSER2_H
#define OSUSONGBROWSER2_H

#include "OsuScreenBackable.h"
#include "MouseListener.h"

class Osu;
class OsuBeatmap;
class OsuDatabase;
class OsuDatabaseBeatmap;
class OsuDatabaseBeatmapStarCalculator;
class OsuSkinImage;

class OsuUIContextMenu;
class OsuUISearchOverlay;
class OsuUISelectionButton;
class OsuUISongBrowserInfoLabel;
class OsuUISongBrowserUserButton;
class OsuUISongBrowserScoreButton;
class OsuUISongBrowserButton;
class OsuUISongBrowserSongButton;
class OsuUISongBrowserSongDifficultyButton;
class OsuUISongBrowserCollectionButton;
class OsuUIUserStatsScreenLabel;

class CBaseUIContainer;
class CBaseUIImageButton;
class CBaseUIScrollView;
class CBaseUIButton;
class CBaseUILabel;

class McFont;
class ConVar;

class OsuSongBrowserBackgroundSearchMatcher;

class OsuSongBrowser2 final: public OsuScreenBackable
{
public:
	static void drawSelectedBeatmapBackgroundImage(Osu *osu, float alpha = 1.0f);

	struct SORTING_COMPARATOR
	{
		virtual ~SORTING_COMPARATOR() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const = 0;
	};

	struct SortByArtist final : public SORTING_COMPARATOR
	{
		virtual ~SortByArtist() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByBPM final : public SORTING_COMPARATOR
	{
		virtual ~SortByBPM() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByCreator final : public SORTING_COMPARATOR
	{
		virtual ~SortByCreator() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByDateAdded final : public SORTING_COMPARATOR
	{
		virtual ~SortByDateAdded() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByDifficulty final : public SORTING_COMPARATOR
	{
		virtual ~SortByDifficulty() {;}
		virtual bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByLength final : public SORTING_COMPARATOR
	{
		virtual ~SortByLength() {;}
		bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	struct SortByTitle final : public SORTING_COMPARATOR
	{
		virtual ~SortByTitle() {;}
		bool operator () (OsuUISongBrowserButton const *a, OsuUISongBrowserButton const *b) const;
	};

	enum class GROUP : uint8_t
	{
		GROUP_NO_GROUPING,
		GROUP_ARTIST,
		GROUP_BPM,
		GROUP_CREATOR,
		GROUP_DATEADDED,
		GROUP_DIFFICULTY,
		GROUP_LENGTH,
		GROUP_TITLE,
		GROUP_COLLECTIONS
	};

public:
	friend class OsuSongBrowserBackgroundSearchMatcher;

	OsuSongBrowser2();
	~OsuSongBrowser2() override;

	void draw() override;
	void update() override;

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	void onResolutionChange(Vector2 newResolution) override;

	void setVisible(bool visible) override;

	void onPlayEnd(bool quit = true); // called when a beatmap is finished playing (or the player quit)

	void onSelectionChange(OsuUISongBrowserButton *button, bool rebuild);
	void onDifficultySelected(OsuDatabaseBeatmap *diff2, bool play = false, bool mp = false);
	void onDifficultySelectedMP(OsuDatabaseBeatmap *diff2, bool play = false);
	void selectBeatmapMP(OsuDatabaseBeatmap *diff2);

	void onScoreContextMenu(OsuUISongBrowserScoreButton *scoreButton, int id);
	void onSongButtonContextMenu(OsuUISongBrowserSongButton *songButton, UString text, int id);
	void onCollectionButtonContextMenu(OsuUISongBrowserCollectionButton *collectionButton, UString text, int id);

	void highlightScore(uint64_t unixTimestamp);
	void selectRandomBeatmap(bool playMusicFromPreviewPoint = true);
	void playNextRandomBeatmap() {selectRandomBeatmap();playSelectedDifficulty();}
	void recalculateStarsForSelectedBeatmap(bool force = false);

	void refreshBeatmaps();
	void addBeatmap(OsuDatabaseBeatmap *beatmap);
	void readdBeatmap(OsuDatabaseBeatmap *diff2);

	void requestNextScrollToSongButtonJumpFix(const OsuUISongBrowserSongDifficultyButton *diffButton);
	void scrollToSongButton(const OsuUISongBrowserButton *songButton, bool alignOnTop = false);
	void scrollToSelectedSongButton();
	void rebuildSongButtons();
	void recreateCollectionsButtons();
	void rebuildScoreButtons();
	void updateSongButtonLayout();
	void updateSongButtonSorting();

	[[nodiscard]] OsuUISongBrowserButton *findCurrentlySelectedSongButton() const;
	[[nodiscard]] inline const std::vector<OsuUISongBrowserCollectionButton*> &getCollectionButtons() const {return m_collectionButtons;}

	[[nodiscard]] inline bool hasSelectedAndIsPlaying() const {return m_bHasSelectedAndIsPlaying;}
	[[nodiscard]] inline bool isInSearch() const {return m_bInSearch;}
	[[nodiscard]] inline bool isRightClickScrolling() const {return m_bSongBrowserRightClickScrolling;}

	[[nodiscard]] inline OsuDatabase *getDatabase() const {return m_db;}
	[[nodiscard]] inline OsuBeatmap *getSelectedBeatmap() const {return m_selectedBeatmap;}
	[[nodiscard]] inline const OsuDatabaseBeatmapStarCalculator *getDynamicStarCalculator() const {return m_dynamicStarCalculator;}

	[[nodiscard]] inline OsuUISongBrowserInfoLabel *getInfoLabel() {return m_songInfo;}

	[[nodiscard]] inline GROUP getGroupingMode() const {return m_group;}

private:
	enum class SORT : uint8_t
	{
		SORT_ARTIST,
		SORT_BPM,
		SORT_CREATOR,
		SORT_DATEADDED,
		SORT_DIFFICULTY,
		SORT_LENGTH,
		SORT_RANKACHIEVED,
		SORT_TITLE
	};

	struct SORTING_METHOD
	{
		SORT type;
		UString name;
		SORTING_COMPARATOR *comparator;
	};

	struct GROUPING
	{
		GROUP type;
		UString name;
		int id;
	};

private:
	static bool searchMatcher(const OsuDatabaseBeatmap *databaseBeatmap, const std::vector<UString> &searchStringTokens);
	static bool findSubstringInDifficulty(const OsuDatabaseBeatmap *diff, const UString &searchString);

	void updateLayout() override;
	void onBack() override;

	void updateScoreBrowserLayout();

	void scheduleSearchUpdate(bool immediately = false);

	void checkHandleKillBackgroundStarCalculator();
	bool checkHandleKillDynamicStarCalculator(bool timeout);
	void checkHandleKillBackgroundSearchMatcher();

	OsuUISelectionButton *addBottombarNavButton(std::function<OsuSkinImage*()> getImageFunc, std::function<OsuSkinImage*()> getImageOverFunc);
	CBaseUIButton *addTopBarRightTabButton(UString text);
	CBaseUIButton *addTopBarRightGroupButton(UString text);
	CBaseUIButton *addTopBarRightSortButton(UString text);
	CBaseUIButton *addTopBarLeftTabButton(UString text);
	CBaseUIButton *addTopBarLeftButton(UString text);

	void onDatabaseLoadingFinished();

	void onSearchUpdate();
	void rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(bool scrollToTop, bool doRebuildSongButtons = true);

	void onSortScoresClicked(CBaseUIButton *button);
	void onSortScoresChange(UString text, int id = -1);
	void onWebClicked(CBaseUIButton *button);

	void onGroupClicked(CBaseUIButton *button);
	void onGroupChange(UString text, int id = -1);

	void onSortClicked(CBaseUIButton *button);
	void onSortChange(UString text, int id = -1);
	void onSortChangeInt(UString text, bool autoScroll);

	void onGroupTabButtonClicked(CBaseUIButton *groupTabButton);
	void onGroupNoGrouping();
	void onGroupCollections(bool autoScroll = true);
	void onGroupArtist();
	void onGroupDifficulty();
	void onGroupBPM();
	void onGroupCreator();
	void onGroupDateadded();
	void onGroupLength();
	void onGroupTitle();

	void onAfterSortingOrGroupChange(bool autoScroll = true);
	void onAfterSortingOrGroupChangeUpdateInt(bool autoScroll);

	void onSelectionMode();
	void onSelectionMods();
	void onSelectionRandom();
	void onSelectionOptions();

	void onModeChange(UString text);
	void onModeChange2(UString text, int id = -1);

	void onUserButtonClicked();
	void onUserButtonChange(UString text, int id);

	void onScoreClicked(CBaseUIButton *button);

	void selectSongButton(OsuUISongBrowserButton *songButton);
	void selectPreviousRandomBeatmap();
	void playSelectedDifficulty();

	std::mt19937 m_rngalg;
	GROUP m_group;
	std::vector<GROUPING> m_groupings;
	SORT m_sortingMethod;
	std::vector<SORTING_METHOD> m_sortingMethods;

	// top bar
	float m_fSongSelectTopScale;

	// top bar left
	CBaseUIContainer *m_topbarLeft;
	OsuUISongBrowserInfoLabel *m_songInfo;
	std::vector<CBaseUIButton*> m_topbarLeftTabButtons;
	std::vector<CBaseUIButton*> m_topbarLeftButtons;
	CBaseUIButton *m_scoreSortButton;
	CBaseUIButton *m_webButton;

	// top bar right
	CBaseUIContainer *m_topbarRight;
	std::vector<CBaseUIButton*> m_topbarRightTabButtons;
	std::vector<CBaseUIButton*> m_topbarRightGroupButtons;
	std::vector<CBaseUIButton*> m_topbarRightSortButtons;
	CBaseUILabel *m_groupLabel;
	CBaseUIButton *m_groupButton;
	CBaseUIButton *m_noGroupingButton;
	CBaseUIButton *m_collectionsButton;
	CBaseUIButton *m_artistButton;
	CBaseUIButton *m_difficultiesButton;
	CBaseUILabel *m_sortLabel;
	CBaseUIButton *m_sortButton;
	OsuUIContextMenu *m_contextMenu;

	// bottom bar
	CBaseUIContainer *m_bottombar;
	std::vector<OsuUISelectionButton*> m_bottombarNavButtons;
	OsuUISongBrowserUserButton *m_userButton;
	OsuUIUserStatsScreenLabel *m_ppVersionInfoLabel;

	// score browser
	std::vector<OsuUISongBrowserScoreButton*> m_scoreButtonCache;
	CBaseUIScrollView *m_scoreBrowser;
	CBaseUIElement *m_scoreBrowserNoRecordsYetElement;

	// song browser
	CBaseUIScrollView *m_songBrowser;
	bool m_bSongBrowserRightClickScrollCheck;
	bool m_bSongBrowserRightClickScrolling;
	bool m_bNextScrollToSongButtonJumpFixScheduled;
	bool m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta;
	float m_fNextScrollToSongButtonJumpFixOldRelPosY;
	float m_fNextScrollToSongButtonJumpFixOldScrollSizeY;

	// song browser selection state logic
	OsuUISongBrowserSongButton *m_selectionPreviousSongButton;
	OsuUISongBrowserSongDifficultyButton *m_selectionPreviousSongDiffButton;
	OsuUISongBrowserCollectionButton *m_selectionPreviousCollectionButton;

	// beatmap database
	OsuDatabase *m_db;
	std::vector<OsuDatabaseBeatmap*> m_beatmaps;
	std::vector<OsuUISongBrowserSongButton*> m_songButtons;
	std::vector<OsuUISongBrowserButton*> m_visibleSongButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_collectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_artistCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_difficultyCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_bpmCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_creatorCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_dateaddedCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_lengthCollectionButtons;
	std::vector<OsuUISongBrowserCollectionButton*> m_titleCollectionButtons;
	bool m_bBeatmapRefreshScheduled;
	UString m_sLastOsuFolder;

	// keys
	bool m_bF1Pressed;
	bool m_bF2Pressed;
	bool m_bF3Pressed;
	bool m_bShiftPressed;
	bool m_bLeft;
	bool m_bRight;
	bool m_bRandomBeatmapScheduled;
	bool m_bPreviousRandomBeatmapScheduled;

	// behaviour
	OsuBeatmap *m_selectedBeatmap;
	bool m_bHasSelectedAndIsPlaying;
	float m_fPulseAnimation;
	float m_fBackgroundFadeInTime;
	std::vector<OsuDatabaseBeatmap*> m_previousRandomBeatmaps;

	// search
	OsuUISearchOverlay *m_search;
	UString m_sSearchString;
	UString m_sPrevSearchString;
	UString m_sPrevHardcodedSearchString;
	float m_fSearchWaitTime;
	bool m_bInSearch;
	GROUP m_searchPrevGroup;
	OsuSongBrowserBackgroundSearchMatcher *m_backgroundSearchMatcher;
	bool m_bOnAfterSortingOrGroupChangeUpdateScheduled;
	bool m_bOnAfterSortingOrGroupChangeUpdateScheduledAutoScroll;

	// background star calculation (entire database)
	float m_fBackgroundStarCalculationWorkNotificationTime;
	int m_iBackgroundStarCalculationIndex;
	OsuDatabaseBeatmapStarCalculator *m_backgroundStarCalculator;
	OsuDatabaseBeatmap *m_backgroundStarCalcTempParent;

	// background star calculation (currently selected beatmap)
	bool m_bBackgroundStarCalcScheduled;
	bool m_bBackgroundStarCalcScheduledForce;
	OsuDatabaseBeatmapStarCalculator *m_dynamicStarCalculator;
};

#endif
