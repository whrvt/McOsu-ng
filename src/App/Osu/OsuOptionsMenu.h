//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		settings
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUOPTIONSMENU_H
#define OSUOPTIONSMENU_H

#include "OsuScreenBackable.h"
#include "OsuNotificationOverlay.h"

class Osu;

class OsuUIButton;
class OsuUISlider;
class OsuUIContextMenu;
class OsuUISearchOverlay;

class OsuOptionsMenuSliderPreviewElement;
class OsuOptionsMenuCategoryButton;
class OsuOptionsMenuKeyBindButton;
class OsuOptionsMenuResetButton;

class CBaseUIContainer;
class CBaseUIImageButton;
class CBaseUICheckbox;
class CBaseUIButton;
class CBaseUISlider;
class CBaseUIScrollView;
class CBaseUILabel;
class CBaseUITextbox;

class ConVar;

class OsuOptionsMenu : public OsuScreenBackable, public OsuNotificationOverlayKeyListener
{
public:
	OsuOptionsMenu();
	~OsuOptionsMenu() override;

	void draw() override;
	void update() override;

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	void onResolutionChange(Vector2 newResolution) override;

	void onKey(KeyboardEvent &e) override;

	void setVisible(bool visible) override;

	void save();

	void openAndScrollToSkinSection();

	void setFullscreen(bool fullscreen) {m_bFullscreen = fullscreen;}

	void setUsername(UString username);

	[[nodiscard]] inline bool isFullscreen() const {return m_bFullscreen;}
	bool isMouseInside();
	bool isBusy();
	[[maybe_unused]] [[nodiscard]] inline bool isWorkshopLoading() const {return m_bWorkshopSkinSelectScheduled;}

private:
	static constexpr const char *OSU_CONFIG_FILE_NAME = "osu.cfg";

	struct OPTIONS_ELEMENT
	{
		OPTIONS_ELEMENT()
		{
			resetButton = NULL;
			cvar = NULL;
			type = -1;

			label1Width = 0.0f;
			relSizeDPI = 96.0f;

			allowOverscale = false;
			allowUnderscale = false;
		}

		OsuOptionsMenuResetButton *resetButton;
		std::vector<CBaseUIElement*> elements;
		ConVar *cvar;
		int type;

		float label1Width;
		float relSizeDPI;

		bool allowOverscale;
		bool allowUnderscale;

		UString searchTags;
	};

	void updateLayout() override;
	void onBack() override;

	void setVisibleInt(bool visible, bool fromOnBack = false);
	void scheduleSearchUpdate();

	void updateOsuFolder();
	void updateName();
	void updateFposuDPI();
	void updateFposuCMper360();
	void updateSkinNameLabel();
	void updateNotelockSelectLabel();
	void updateHPDrainSelectLabel();

	// options
	void onFullscreenChange(CBaseUICheckbox *checkbox);
	void onBorderlessWindowedChange(CBaseUICheckbox *checkbox);
	void onDPIScalingChange(CBaseUICheckbox *checkbox);
	void onRawInputToAbsoluteWindowChange(CBaseUICheckbox *checkbox);
	void onSkinSelect();
	void onSkinSelect2(const UString& skinName, int id = -1);
	[[maybe_unused]] void onSkinSelectWorkshop();
	[[maybe_unused]] void onSkinSelectWorkshop2();
	[[maybe_unused]] void onSkinSelectWorkshop3();
	[[maybe_unused]] void onSkinSelectWorkshop4(const UString& skinName, int id);
	void openCurrentSkinFolder();
	void onSkinReload();
	void onSkinRandom();
	void onResolutionSelect();
	void onResolutionSelect2(const UString& resolution, int id = -1);
	void onOutputDeviceSelect();
	void onOutputDeviceSelect2(const UString& outputDeviceName, int id = -1);
	void onOutputDeviceResetClicked();
	void onOutputDeviceResetUpdate();
	void onOutputDeviceRestart();
	void onAudioCompatibilityModeChange(CBaseUICheckbox *checkbox);
	void onDownloadOsuClicked();
	void onManuallyManageBeatmapsClicked();
	void onBrowseOsuFolderClicked();
	void onCM360CalculatorLinkClicked();
	void onNotelockSelect();
	void onNotelockSelect2(const UString& notelockType, int id = -1);
	void onNotelockSelectResetClicked();
	void onNotelockSelectResetUpdate();
	void onHPDrainSelect();
	void onHPDrainSelect2(const UString& hpDrainType, int id = -1);
	void onHPDrainSelectResetClicked();
	void onHPDrainSelectResetUpdate();

	void onCheckboxChange(CBaseUICheckbox *checkbox);
	void onSliderChange(CBaseUISlider *slider);
	void onSliderChangeOneDecimalPlace(CBaseUISlider *slider);
	void onSliderChangeTwoDecimalPlaces(CBaseUISlider *slider);
	void onSliderChangeOneDecimalPlaceMeters(CBaseUISlider *slider);
	void onSliderChangeInt(CBaseUISlider *slider);
	void onSliderChangeIntMS(CBaseUISlider *slider);
	void onSliderChangeFloatMS(CBaseUISlider *slider);
	void onSliderChangePercent(CBaseUISlider *slider);

	void onKeyBindingButtonPressed(CBaseUIButton *button);
	void onKeyUnbindButtonPressed(CBaseUIButton *button);
	void onKeyBindingsResetAllPressed(CBaseUIButton *button);
	void onKeyBindingManiaPressedInt();
	void onKeyBindingManiaPressed(CBaseUIButton *button);
	void onSliderChangeSliderQuality(CBaseUISlider *slider);
	void onSliderChangeLetterboxingOffset(CBaseUISlider *slider);
	void onSliderChangeUIScale(CBaseUISlider *slider);

	void onWASAPIBufferChange(CBaseUISlider *slider);
	void onWASAPIPeriodChange(CBaseUISlider *slider);

	void onUseSkinsSoundSamplesChange(const UString &oldValue, const UString &newValue);
	void onHighQualitySlidersCheckboxChange(CBaseUICheckbox *checkbox);
	void onHighQualitySlidersConVarChange(const UString &oldValue, const UString &newValue);

	// categories
	void onCategoryClicked(CBaseUIButton *button);

	// reset
	void onResetUpdate(CBaseUIButton *button);
	void onResetClicked(CBaseUIButton *button);
	void onResetEverythingClicked(CBaseUIButton *button);

	// options
	void addSpacer(unsigned num = 1);
	CBaseUILabel *addSection(const UString& text);
	CBaseUILabel *addSubSection(const UString& text, UString searchTags = "");
	CBaseUILabel *addLabel(const UString& text);
	OsuUIButton *addButton(const UString& text);
	OPTIONS_ELEMENT addButton(const UString& text, const UString& labelText, bool withResetButton = false);
	OPTIONS_ELEMENT addButtonButton(const UString& text1, const UString& text2);
	OPTIONS_ELEMENT addButtonButtonLabel(const UString& text1, const UString& text2, const UString& labelText, bool withResetButton = false);
	OsuOptionsMenuKeyBindButton *addKeyBindButton(const UString& text, ConVar *cvar);
	CBaseUICheckbox *addCheckbox(UString text, ConVar *cvar);
	CBaseUICheckbox *addCheckbox(const UString& text, const UString& tooltipText = "", ConVar *cvar = NULL);
	OsuUISlider *addSlider(const UString& text, float min = 0.0f, float max = 1.0f, ConVar *cvar = NULL, float label1Width = 0.0f, bool allowOverscale = false, bool allowUnderscale = false);
	CBaseUITextbox *addTextbox(UString text, ConVar *cvar = NULL);
	CBaseUITextbox *addTextbox(UString text, const UString& labelText, ConVar *cvar = NULL);
	CBaseUIElement *addSkinPreview();
	CBaseUIElement *addSliderPreview();

	// categories
	OsuOptionsMenuCategoryButton *addCategory(CBaseUIElement *section, wchar_t icon);

	// vars
	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_categories;
	CBaseUIScrollView *m_options;
	OsuUIContextMenu *m_contextMenu;
	OsuUISearchOverlay *m_search;
	CBaseUILabel *m_spacer;
	OsuOptionsMenuCategoryButton *m_fposuCategoryButton;

	std::vector<OsuOptionsMenuCategoryButton*> m_categoryButtons;
	std::vector<OPTIONS_ELEMENT> m_elements;

	// custom
	bool m_bFullscreen;
	float m_fAnimation;

	CBaseUICheckbox *m_fullscreenCheckbox;
	CBaseUISlider *m_backgroundDimSlider;
	CBaseUISlider *m_backgroundBrightnessSlider;
	CBaseUISlider *m_hudSizeSlider;
	CBaseUISlider *m_hudComboScaleSlider;
	CBaseUISlider *m_hudScoreScaleSlider;
	CBaseUISlider *m_hudAccuracyScaleSlider;
	CBaseUISlider *m_hudHiterrorbarScaleSlider;
	CBaseUISlider *m_hudHiterrorbarURScaleSlider;
	CBaseUISlider *m_hudProgressbarScaleSlider;
	CBaseUISlider *m_hudScoreBarScaleSlider;
	CBaseUISlider *m_hudScoreBoardScaleSlider;
	CBaseUISlider *m_hudInputoverlayScaleSlider;
	CBaseUISlider *m_playfieldBorderSizeSlider;
	CBaseUISlider *m_statisticsOverlayScaleSlider;
	CBaseUISlider *m_statisticsOverlayXOffsetSlider;
	CBaseUISlider *m_statisticsOverlayYOffsetSlider;
	CBaseUISlider *m_cursorSizeSlider;
	CBaseUILabel *m_skinLabel;
	CBaseUIElement *m_skinSelectLocalButton;
	CBaseUIElement *m_skinSelectWorkshopButton;
	CBaseUIElement *m_resolutionSelectButton;
	CBaseUILabel *m_resolutionLabel;
	CBaseUITextbox *m_osuFolderTextbox;
	CBaseUITextbox *m_nameTextbox;
	CBaseUIElement *m_outputDeviceSelectButton;
	CBaseUILabel *m_outputDeviceLabel;
	OsuOptionsMenuResetButton *m_outputDeviceResetButton;
	CBaseUISlider *m_wasapiBufferSizeSlider;
	CBaseUISlider *m_wasapiPeriodSizeSlider;
	OsuOptionsMenuResetButton *m_wasapiBufferSizeResetButton;
	OsuOptionsMenuResetButton *m_wasapiPeriodSizeResetButton;
	CBaseUISlider *m_sliderQualitySlider;
	CBaseUISlider *m_letterboxingOffsetXSlider;
	CBaseUISlider *m_letterboxingOffsetYSlider;
	CBaseUIButton *m_letterboxingOffsetResetButton;
	OsuOptionsMenuSliderPreviewElement *m_sliderPreviewElement;
	CBaseUITextbox *m_dpiTextbox;
	CBaseUITextbox *m_cm360Textbox;
	CBaseUIElement *m_skinSection;
	CBaseUISlider *m_uiScaleSlider;
	OsuOptionsMenuResetButton *m_uiScaleResetButton;
	CBaseUIElement *m_notelockSelectButton;
	CBaseUILabel *m_notelockSelectLabel;
	OsuOptionsMenuResetButton *m_notelockSelectResetButton;
	CBaseUIElement *m_hpDrainSelectButton;
	CBaseUILabel *m_hpDrainSelectLabel;
	OsuOptionsMenuResetButton *m_hpDrainSelectResetButton;

	ConVar *m_waitingKey;

	float m_fOsuFolderTextboxInvalidAnim;
	float m_fVibrationStrengthExampleTimer;
	bool m_bLetterboxingOffsetUpdateScheduled;
	bool m_bWorkshopSkinSelectScheduled;
	bool m_bUIScaleChangeScheduled;
	bool m_bUIScaleScrollToSliderScheduled;
	bool m_bDPIScalingScrollToSliderScheduled;
	bool m_bWASAPIBufferChangeScheduled;
	bool m_bWASAPIPeriodChangeScheduled;

	bool m_bIsOsuFolderDialogOpen;

	int m_iNumResetAllKeyBindingsPressed;
	int m_iNumResetEverythingPressed;

	// mania layout
	int m_iManiaK;
	int m_iManiaKey;

	// search
	UString m_sSearchString;
	float m_fSearchOnCharKeybindHackTime;

	// notelock
	std::vector<UString> m_notelockTypes;

	// drain
	std::vector<UString> m_drainTypes;
};

#endif
