//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		settings
//
// $NoKeywords: $
//===============================================================================//

#include "OsuOptionsMenu.h"

#include "SoundEngine.h"
#include "Engine.h"
#include "ConVar.h"
#include "Keyboard.h"
#include "Environment.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "SteamworksInterface.h"
#include "Mouse.h"
#include "File.h"

#include "OsuUIElement.h"

#include "Osu.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"
#include "OsuKeyBindings.h"
#include "OsuTooltipOverlay.h"
#include "OsuNotificationOverlay.h"
#include "OsuSteamWorkshop.h"
#include "OsuMainMenu.h"
#include "OsuSliderRenderer.h"
#include "OsuCircle.h"
#include "OsuIcons.h"

#include "OsuUIButton.h"
#include "OsuUISlider.h"
#include "OsuUICheckbox.h"
#include "OsuUIBackButton.h"
#include "OsuUIContextMenu.h"
#include "OsuUISearchOverlay.h"

#include <iostream>
#include <fstream>
#include <utility>
void _osuOptionsSliderQualityWrapper(float newValue)
{
	float value = std::lerp(1.0f, 2.5f, 1.0f - newValue);
	cv::osu::slider_curve_points_separation.setValue(value);
};
namespace cv::osu {
ConVar options_save_on_back("osu_options_save_on_back", true, FCVAR_NONE);
ConVar options_high_quality_sliders("osu_options_high_quality_sliders", false, FCVAR_NONE);
ConVar mania_keylayout_wizard("osu_mania_keylayout_wizard");
ConVar options_slider_preview_use_legacy_renderer("osu_options_slider_preview_use_legacy_renderer", false, FCVAR_NONE, "apparently newer AMD drivers with old gpus are crashing here with the legacy renderer? was just me being lazy anyway, so now there is a vao render path as it should be");
ConVar options_slider_quality("osu_options_slider_quality", 0.0f, FCVAR_NONE, _osuOptionsSliderQualityWrapper);
}

class OsuOptionsMenuSkinPreviewElement final : public OsuUIElement
{
public:
	OsuOptionsMenuSkinPreviewElement(float xPos, float yPos, float xSize, float ySize, UString name) : OsuUIElement(xPos, yPos, xSize, ySize, std::move(name)) {m_iMode = 0;}

	void draw() override
	{
		if (!m_bVisible) return;

		OsuSkin *skin = osu->getSkin();

		float hitcircleDiameter = m_vSize.y*0.5f;
		float numberScale = (hitcircleDiameter / (160.0f * (skin->isDefault12x() ? 2.0f : 1.0f))) * 1 * cv::osu::number_scale_multiplier.getFloat();
		float overlapScale = (hitcircleDiameter / (160.0f)) * 1 * cv::osu::number_scale_multiplier.getFloat();
		float scoreScale = 0.5f;

		if (m_iMode == 0)
		{
			float approachScale = std::clamp<float>(1.0f + 1.5f - fmod(engine->getTime()*3, 3.0f), 0.0f, 2.5f);
			float approachAlpha = std::clamp<float>(fmod(engine->getTime()*3, 3.0f)/1.5f, 0.0f, 1.0f);
			approachAlpha = -approachAlpha*(approachAlpha-2.0f);
			approachAlpha = -approachAlpha*(approachAlpha-2.0f);
			float approachCircleAlpha = approachAlpha;
			approachAlpha = 1.0f;

			const int number = 1;
			const int colorCounter = 42;
			const int colorOffset = 0;
			const float colorRGBMultiplier = 1.0f;

			OsuCircle::drawCircle(osu->getSkin(), m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(1.0f/5.0f), 0.0f), hitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, approachAlpha, approachAlpha, true, false);
			OsuCircle::drawHitResult(osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(2.0f/5.0f), 0.0f), OsuScore::HIT::HIT_100, 0.45f, 0.33f);
			OsuCircle::drawHitResult(osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(3.0f/5.0f), 0.0f), OsuScore::HIT::HIT_50, 0.45f, 0.66f);
			OsuCircle::drawHitResult(osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(4.0f/5.0f), 0.0f), OsuScore::HIT::HIT_MISS, 0.45f, 1.0f);
			OsuCircle::drawApproachCircle(osu->getSkin(), m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(1.0f/5.0f), 0.0f), osu->getSkin()->getComboColorForCounter(colorCounter, colorOffset), hitcircleDiameter, approachScale, approachCircleAlpha, false, false);
		}
		else if (m_iMode == 1)
		{
			const int numNumbers = 6;
			for (int i=1; i<numNumbers+1; i++)
			{
				OsuCircle::drawHitCircleNumber(skin, numberScale, overlapScale, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*((float)i/(numNumbers+1.0f)), 0.0f), i-1, 1.0f, 1.0f);
			}
		}
		else if (m_iMode == 2)
		{
			const int numNumbers = 6;
			for (int i=1; i<numNumbers+1; i++)
			{
				Vector2 pos = m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*((float)i/(numNumbers+1.0f)), 0.0f);

				g->pushTransform();
				g->scale(scoreScale, scoreScale);
				g->translate(pos.x - skin->getScore0()->getWidth()*scoreScale, pos.y);
				osu->getHUD()->drawScoreNumber(i-1, 1.0f);
				g->popTransform();
			}
		}
	}

	void onMouseUpInside() override
	{
		m_iMode++;
		m_iMode = m_iMode % 3;
	}
	// inspection
	CBASE_UI_TYPE(OsuOptionsMenuSkinPreviewElement, OPTIONSSKINPREVIEW, OsuUIElement)
private:
	int m_iMode;
};

class OsuOptionsMenuSliderPreviewElement final : public OsuUIElement
{
public:
	OsuOptionsMenuSliderPreviewElement(float xPos, float yPos, float xSize, float ySize, UString name) : OsuUIElement(xPos, yPos, xSize, ySize, std::move(name))
	{
		
		m_bDrawSliderHack = true;
		m_fPrevLength = 0.0f;
		m_vao = NULL;


	}

	void draw() override
	{
		if (!m_bVisible) return;

		/*
		g->setColor(0xffffffff);
		g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
		*/

		const float hitcircleDiameter = m_vSize.y*0.5f;
		const float numberScale = (hitcircleDiameter / (160.0f * (osu->getSkin()->isDefault12x() ? 2.0f : 1.0f))) * 1 * cv::osu::number_scale_multiplier.getFloat();
		const float overlapScale = (hitcircleDiameter / (160.0f)) * 1 * cv::osu::number_scale_multiplier.getFloat();

		const float approachScale = std::clamp<float>(1.0f + 1.5f - fmod(engine->getTime()*3, 3.0f), 0.0f, 2.5f);
		float approachAlpha = std::clamp<float>(fmod(engine->getTime()*3, 3.0f)/1.5f, 0.0f, 1.0f);

		approachAlpha = -approachAlpha*(approachAlpha-2.0f);
		approachAlpha = -approachAlpha*(approachAlpha-2.0f);

		const float approachCircleAlpha = approachAlpha;
		approachAlpha = 1.0f;

		const float length = (m_vSize.x-hitcircleDiameter);
		const int numPoints = length;
		const float pointDist = length / numPoints;

		static std::vector<Vector2> emptyVector;
		std::vector<Vector2> points;

		const bool useLegacyRenderer = (cv::osu::options_slider_preview_use_legacy_renderer.getBool() || cv::osu::force_legacy_slider_renderer.getBool());

		for (int i=0; i<numPoints; i++)
		{
			int heightAdd = i;
			if (i > numPoints/2)
				heightAdd = numPoints-i;

			float heightAddPercent = (float)heightAdd / (float)(numPoints/2.0f);
			float temp = 1.0f - heightAddPercent;
			temp *= temp;
			heightAddPercent = 1.0f - temp;

			points.emplace_back((useLegacyRenderer ? m_vPos.x : 0) + hitcircleDiameter/2 + i*pointDist, (useLegacyRenderer ? m_vPos.y : 0) + m_vSize.y/2 - hitcircleDiameter/3 + heightAddPercent*(m_vSize.y/2 - hitcircleDiameter/2));
		}

		if (points.size() > 0)
		{
			// draw regular circle with animated approach circle beneath slider
			{
				const int number = 2;
				const int colorCounter = 420;
				const int colorOffset = 0;
				const float colorRGBMultiplier = 1.0f;

				OsuCircle::drawCircle(osu->getSkin(), points[numPoints/2] + (!useLegacyRenderer ? m_vPos : Vector2(0, 0)), hitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, approachAlpha, approachAlpha, true, false);
				OsuCircle::drawApproachCircle(osu->getSkin(), points[numPoints/2] + (!useLegacyRenderer ? m_vPos : Vector2(0, 0)), osu->getSkin()->getComboColorForCounter(420, 0), hitcircleDiameter, approachScale, approachCircleAlpha, false, false);
			}

			// draw slider body
			{
				// recursive shared usage of the same RenderTarget is invalid, therefore we block slider rendering while the options menu is animating
				if (m_bDrawSliderHack)
				{
					if (useLegacyRenderer)
						OsuSliderRenderer::draw(osu, points, emptyVector, hitcircleDiameter, 0, 1, osu->getSkin()->getComboColorForCounter(420, 0));
					else
					{
						// (lazy generate vao)
						if (m_vao == NULL || length != m_fPrevLength)
						{
							m_fPrevLength = length;

							debugLog("Regenerating options menu slider preview vao ...\n");

							if (m_vao != NULL)
							{
								resourceManager->destroyResource(m_vao);
								m_vao = NULL;
							}

							if (m_vao == NULL)
								m_vao = OsuSliderRenderer::generateVAO(points, hitcircleDiameter, Vector3(0, 0, 0), false);
						}
						OsuSliderRenderer::draw(osu, m_vao, emptyVector, m_vPos, 1, hitcircleDiameter, 0, 1, osu->getSkin()->getComboColorForCounter(420, 0));
					}
				}
			}

			// and slider head/tail circles
			{
				const int number = 1;
				const int colorCounter = 420;
				const int colorOffset = 0;
				const float colorRGBMultiplier = 1.0f;

				OsuCircle::drawSliderStartCircle(osu->getSkin(), points[0] + (!useLegacyRenderer ? m_vPos : Vector2(0, 0)), hitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset, colorRGBMultiplier);
				OsuCircle::drawSliderEndCircle(osu->getSkin(), points[points.size()-1] + (!useLegacyRenderer ? m_vPos : Vector2(0, 0)), hitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, 1.0f, 1.0f, 0.0f, false, false);
			}
		}
	}

	void setDrawSliderHack(bool drawSliderHack) {m_bDrawSliderHack = drawSliderHack;}
	// inspection
	CBASE_UI_TYPE(OsuOptionsMenuSliderPreviewElement, OPTIONSSLIDERPREVIEW, OsuUIElement)
private:
	bool m_bDrawSliderHack;
	VertexArrayObject *m_vao;
	float m_fPrevLength;
;
};

class OsuOptionsMenuKeyBindLabel : public CBaseUILabel
{
public:
	OsuOptionsMenuKeyBindLabel(float xPos, float yPos, float xSize, float ySize, UString name, UString text, ConVar *cvar, CBaseUIButton *bindButton) : CBaseUILabel(xPos, yPos, xSize, ySize, std::move(name), std::move(text))
	{
		m_key = cvar;
		m_cachedKeyCode = -1;
		m_bindButton = bindButton;

		m_textColorBound = 0xffffd700;
		m_textColorUnbound = 0xffbb0000;
	}

	void update() override
	{
		CBaseUILabel::update();
		if (!m_bVisible) return;

		const KEYCODE newKeyCode = m_key->getVal<KEYCODE>();
		if (m_cachedKeyCode == newKeyCode)
			return;

		m_cachedKeyCode = newKeyCode;

		// succ
		UString labelText = env->keyCodeToString(newKeyCode);
		if (labelText.find("?") != -1)
			labelText.append(UString::format("  (%i)", m_key->getVal<KEYCODE>()));

		// handle bound/unbound
		if (m_cachedKeyCode == 0)
		{
			labelText = "<UNBOUND>";
			setTextColor(m_textColorUnbound);
		}
		else
			setTextColor(m_textColorBound);

		// update text
		setText(labelText);
	}

	void setTextColorBound(Color textColorBound) {m_textColorBound = textColorBound;}
	void setTextColorUnbound(Color textColorUnbound) {m_textColorUnbound = textColorUnbound;}

private:
	void onMouseUpInside() override
	{
		CBaseUILabel::onMouseUpInside();
		m_bindButton->click();
	}

	ConVar *m_key;
	KEYCODE m_cachedKeyCode;
	CBaseUIButton *m_bindButton;

	Color m_textColorBound;
	Color m_textColorUnbound;
};

class OsuOptionsMenuKeyBindButton : public OsuUIButton
{
public:
	OsuOptionsMenuKeyBindButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text) : OsuUIButton(xPos, yPos, xSize, ySize, std::move(name), std::move(text))
	{
		m_bDisallowLeftMouseClickBinding = false;
	}

	void setDisallowLeftMouseClickBinding(bool disallowLeftMouseClickBinding) {m_bDisallowLeftMouseClickBinding = disallowLeftMouseClickBinding;}

	[[nodiscard]] inline bool isLeftMouseClickBindingAllowed() const {return !m_bDisallowLeftMouseClickBinding;}

	// inspection
	CBASE_UI_TYPE(OsuOptionsMenuKeyBindButton, OsuUIElement::OPTIONSMENUKEYBINDBUTTON, OsuUIButton)
private:
	bool m_bDisallowLeftMouseClickBinding;
};

class OsuOptionsMenuCategoryButton : public CBaseUIButton
{
public:
	OsuOptionsMenuCategoryButton(CBaseUIElement *section, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), text)
	{
		m_section = section;
		m_bActiveCategory = false;
	}

	void drawText() override
	{
		if (m_font != NULL && m_sText.length() > 0)
		{
			g->pushClipRect(McRect(m_vPos.x+1, m_vPos.y+1, m_vSize.x-1, m_vSize.y-1));
			{
				g->setColor(m_textColor);
				g->pushTransform();
				{
					g->translate((int)(m_vPos.x + m_vSize.x/2.0f - (m_fStringWidth/2.0f)), (int)(m_vPos.y + m_vSize.y/2.0f + (m_fStringHeight/2.0f)));
					g->drawString(m_font, m_sText);
				}
				g->popTransform();
			}
			g->popClipRect();
		}
	}

	void setActiveCategory(bool activeCategory) {m_bActiveCategory = activeCategory;}

	[[nodiscard]] inline CBaseUIElement *getSection() const {return m_section;}
	[[nodiscard]] inline bool isActiveCategory() const {return m_bActiveCategory;}

	// inspection
	CBASE_UI_TYPE(OsuOptionsMenuCategoryButton, OsuUIElement::OPTIONSMENUCATEGORYBUTTON, CBaseUIButton)
private:
	CBaseUIElement *m_section;
	bool m_bActiveCategory;
};

class OsuOptionsMenuResetButton : public CBaseUIButton
{
public:
	OsuOptionsMenuResetButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), text)
	{
		m_fAnim = 1.0f;
	}

	~OsuOptionsMenuResetButton() override
	{
		anim->deleteExistingAnimation(&m_fAnim);
	}

	void draw() override
	{
		if (!m_bVisible || m_fAnim <= 0.0f) return;

		const int fullColorBlockSize = 4 * Osu::getUIScale();

		Color left = argb((int)(255*m_fAnim), 255, 233, 50);
		Color middle = argb((int)(255*m_fAnim), 255, 211, 50);
		Color right = 0x00000000;

		g->fillGradient(m_vPos.x, m_vPos.y, m_vSize.x*1.25f, m_vSize.y, middle, right, middle, right);
		g->fillGradient(m_vPos.x, m_vPos.y, fullColorBlockSize, m_vSize.y, left, middle, left, middle);
	}

	void update() override
	{
		CBaseUIButton::update();
		if (!m_bVisible || !m_bEnabled) return;

		if (isMouseInside())
		{
			osu->getTooltipOverlay()->begin();
			{
				osu->getTooltipOverlay()->addLine("Reset");
			}
			osu->getTooltipOverlay()->end();
		}
	}

private:
	void onEnabled() override
	{
		CBaseUIButton::onEnabled();
		anim->moveQuadOut(&m_fAnim, 1.0f, (1.0f - m_fAnim)*0.15f, true);
	}

	void onDisabled() override
	{
		CBaseUIButton::onDisabled();
		anim->moveQuadOut(&m_fAnim, 0.0f, m_fAnim*0.15f, true);
	}

	float m_fAnim;
};



OsuOptionsMenu::OsuOptionsMenu() : OsuScreenBackable()
{
	

	m_bFullscreen = false;
	m_fAnimation = 0.0f;

	// convar refs



















	// convar callbacks
	cv::osu::skin_use_skin_hitsounds.setCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onUseSkinsSoundSamplesChange) );
	cv::osu::options_high_quality_sliders.setCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onHighQualitySlidersConVarChange) );
	cv::osu::mania_keylayout_wizard.setCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingManiaPressedInt) );

	osu->getNotificationOverlay()->addKeyListener(this);

	m_waitingKey = NULL;
	m_resolutionLabel = NULL;
	m_fullscreenCheckbox = NULL;
	m_sliderQualitySlider = NULL;
	m_outputDeviceLabel = NULL;
	m_outputDeviceResetButton = NULL;
	m_wasapiBufferSizeSlider = NULL;
	m_wasapiPeriodSizeSlider = NULL;
	m_wasapiBufferSizeResetButton = NULL;
	m_wasapiPeriodSizeResetButton = NULL;
	m_dpiTextbox = NULL;
	m_cm360Textbox = NULL;
	m_letterboxingOffsetResetButton = NULL;
	m_skinSelectWorkshopButton = NULL;
	m_uiScaleSlider = NULL;
	m_uiScaleResetButton = NULL;
	m_notelockSelectButton = NULL;
	m_notelockSelectLabel = NULL;
	m_notelockSelectResetButton = NULL;
	m_hpDrainSelectButton = NULL;
	m_hpDrainSelectLabel = NULL;
	m_hpDrainSelectResetButton = NULL;

	m_fOsuFolderTextboxInvalidAnim = 0.0f;
	m_fVibrationStrengthExampleTimer = 0.0f;
	m_bLetterboxingOffsetUpdateScheduled = false;
	m_bWorkshopSkinSelectScheduled = false;
	m_bUIScaleChangeScheduled = false;
	m_bUIScaleScrollToSliderScheduled = false;
	m_bDPIScalingScrollToSliderScheduled = false;
	m_bWASAPIBufferChangeScheduled = false;
	m_bWASAPIPeriodChangeScheduled = false;

	m_bIsOsuFolderDialogOpen = false;

	m_iNumResetAllKeyBindingsPressed = 0;
	m_iNumResetEverythingPressed = 0;

	m_iManiaK = 0;
	m_iManiaKey = 0;

	m_fSearchOnCharKeybindHackTime = 0.0f;

	m_notelockTypes.emplace_back("None");
	m_notelockTypes.emplace_back("McOsu");
	m_notelockTypes.emplace_back("osu!stable (default)");
	m_notelockTypes.emplace_back("osu!lazer 2020");

	m_drainTypes.emplace_back("None");
	m_drainTypes.emplace_back("osu!stable (default)");
	m_drainTypes.emplace_back("osu!lazer 2020");
	m_drainTypes.emplace_back("osu!lazer 2018");

	m_container = new CBaseUIContainer(-1, 0, 0, 0, "");

	m_options = new CBaseUIScrollView(0, -1, 0, 0, "");
	m_options->setDrawFrame(true);
	m_options->setDrawBackground(true);
	m_options->setBackgroundColor(0xdd000000);
	m_options->setHorizontalScrolling(false);
	m_container->addBaseUIElement(m_options);

	m_categories = new CBaseUIScrollView(0, -1, 0, 0, "");
	m_categories->setDrawFrame(true);
	m_categories->setDrawBackground(true);
	m_categories->setBackgroundColor(0xff000000);
	m_categories->setHorizontalScrolling(false);
	m_categories->setVerticalScrolling(true);
	m_categories->setScrollResistance(30); // since all categories are always visible anyway
	m_container->addBaseUIElement(m_categories);

	m_contextMenu = new OsuUIContextMenu(50, 50, 150, 0, "", m_options);

	m_search = new OsuUISearchOverlay(0, 0, 0, 0, "");
	m_search->setOffsetRight(20);
	m_container->addBaseUIElement(m_search);

	m_spacer = new CBaseUILabel(0, 0, 1, 40, "", "");
	m_spacer->setDrawBackground(false);
	m_spacer->setDrawFrame(false);

	//**************************************************************************************************************************//

	CBaseUIElement *sectionGeneral = addSection("General");

	addSubSection("");
	OsuUIButton *downloadOsuButton = addButton("Download osu! and get some beatmaps!");
	downloadOsuButton->setTooltipText("https://osu.ppy.sh/home/download");
	downloadOsuButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onDownloadOsuClicked) );
	downloadOsuButton->setColor(0xff00ff00);
	downloadOsuButton->setTextColor(0xffffffff);

	addLabel("... or ...")->setCenterText(true);
	OsuUIButton *manuallyManageBeatmapsButton = addButton("Manually manage beatmap files?");
	manuallyManageBeatmapsButton->setTooltipText("https://steamcommunity.com/sharedfiles/filedetails/?id=880768265");
	manuallyManageBeatmapsButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onManuallyManageBeatmapsClicked) );
	manuallyManageBeatmapsButton->setColor(0xff10667b);

	addSubSection("osu! folder");
	addLabel("1) If you have an existing osu!stable installation:")->setTextColor(0xff666666);
	addLabel("2) osu!stable > Options > \"Open osu! folder\"")->setTextColor(0xff666666);
	addLabel("3) Copy & Paste the full path into the textbox:")->setTextColor(0xff666666);
	addLabel("");
	m_osuFolderTextbox = addTextbox(cv::osu::folder.getString(), &cv::osu::folder);

	addLabel("... or ...")->setCenterText(true);
	OsuUIButton *browseForOsuFolderButton = addButton("Browse to your osu! folder");
	browseForOsuFolderButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onBrowseOsuFolderClicked) );
	browseForOsuFolderButton->setColor(0xff999999);

	addLabel("");
	addLabel("osu!lazer databases are not supported.")->setTextColor(0xff770000);
	//addSpacer();
	addLabel("");
	addCheckbox("Use osu!stable osu!.db database (read-only)", "If you have an existing osu!stable installation,\nthen this will massively speed up songbrowser beatmap loading.", &cv::osu::database_enabled);
	addCheckbox("Load osu!stable collection.db (read-only)", "If you have an existing osu!stable installation,\nalso load and display your created collections from there.", &cv::osu::collections_legacy_enabled);
	addCheckbox("Load osu!stable scores.db (read-only)", "If you have an existing osu!stable installation,\nalso load and display your achieved scores from there.", &cv::osu::scores_legacy_enabled);

	addSubSection("Player (Name)");
	m_nameTextbox = addTextbox(cv::name.getString(), &cv::name);
	addSpacer();
	addCheckbox("Include Relax/Autopilot for total weighted pp/acc", "NOTE: osu! does not allow this (since these mods are unranked).\nShould relax/autopilot scores be included in the weighted pp/acc calculation?", &cv::osu::user_include_relax_and_autopilot_for_stats);
	addCheckbox("Disable osu!lazer star/pp Relax/Autopilot nerfs", "Disabled: osu!lazer algorithm default. Relax/Autopilot scores are nerfed.\nEnabled: McOsu default. All Relax/Autopilot nerfs are disabled.", &cv::osu::stars_and_pp_lazer_relax_autopilot_nerf_disabled);
	addCheckbox("Show pp instead of score in scorebrowser", "Only McOsu scores and manually imported scores will show pp.\nSee Songbrowser > User Button > Top Ranks > Gear Button > \"Import osu! scores of <NAME>\".", &cv::osu::scores_sort_by_pp);
	addCheckbox("Always enable touch device pp nerf mod", "Keep touch device pp nerf mod active even when resetting all mods.", &cv::osu::mod_touchdevice);
	addCheckbox("Show osu! scores.db user names in user switcher", "Only relevant if \"Load osu! scores.db\" is enabled.\nShould the user switcher show ALL user names from ALL scores?\n(Even from ones you got in your database because you watched a replay?)", &cv::osu::user_switcher_include_legacy_scores_for_names);

	addSubSection("Songbrowser");
	addCheckbox("Draw Strain Graph in Songbrowser", "Hold either SHIFT/CTRL to show only speed/aim strains.\nSpeed strain is red, aim strain is green.\n(See osu_hud_scrubbing_timeline_strains_*)", &cv::osu::draw_songbrowser_strain_graph);
	addCheckbox("Draw Strain Graph in Scrubbing Timeline", "Speed strain is red, aim strain is green.\n(See osu_hud_scrubbing_timeline_strains_*)", &cv::osu::draw_scrubbing_timeline_strain_graph);

	addSubSection("Window");
	addCheckbox("Pause on Focus Loss", "Should the game pause when you switch to another application?", &cv::osu::pause_on_focus_loss);

	//**************************************************************************************************************************//

	CBaseUIElement *sectionGraphics = addSection("Graphics");

	addSubSection("Renderer");
	addCheckbox("VSync", "If enabled: plz enjoy input lag.", &cv::vsync);

	addCheckbox("High Priority", "Sets the process priority to High.\nMay fix microstuttering and other weird problems.\nTry to fix your broken computer/OS/drivers first!", &cv::processpriority);

	addCheckbox("Show FPS Counter", &cv::osu::draw_fps);

	if constexpr (Env::cfg(REND::GL | REND::GLES32))
	{
		CBaseUISlider *prerenderedFramesSlider = addSlider("Max Queued Frames", 1.0f, 3.0f, &cv::r_sync_max_frames, -1.0f, true);
		prerenderedFramesSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
		prerenderedFramesSlider->setKeyDelta(1);
		addLabel("Raise for higher fps, decrease for lower latency")->setTextColor(0xff666666);
	}

	{
		addSpacer();

		addCheckbox("Unlimited FPS", &cv::fps_unlimited);

		CBaseUISlider *fpsSlider = addSlider("FPS Limiter:", 60.0f, 1000.0f, &cv::fps_max, -1.0f, true);
		fpsSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
		fpsSlider->setKeyDelta(1);

		addSubSection("Layout");
		OPTIONS_ELEMENT resolutionSelect = addButton("Select Resolution", UString::format("%ix%i", osu->getVirtScreenWidth(), osu->getVirtScreenHeight()));
		((CBaseUIButton*)resolutionSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResolutionSelect) );
		m_resolutionLabel = (CBaseUILabel*)resolutionSelect.elements[1];
		m_resolutionSelectButton = resolutionSelect.elements[0];
		m_fullscreenCheckbox = addCheckbox("Fullscreen");
		m_fullscreenCheckbox->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onFullscreenChange) );
		addCheckbox("Borderless", "May cause extra input lag if enabled.\nDepends on your operating system version/updates.", &cv::fullscreen_windowed_borderless)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onBorderlessWindowedChange) );
		addCheckbox("Keep Aspect Ratio", "Black borders instead of a stretched image.\nOnly relevant if fullscreen is enabled, and letterboxing is disabled.\nUse the two position sliders below to move the viewport around.", &cv::osu::resolution_keep_aspect_ratio);
		addCheckbox("Letterboxing", "Useful to get the low latency of fullscreen with a smaller game resolution.\nUse the two position sliders below to move the viewport around.", &cv::osu::letterboxing);
		m_letterboxingOffsetXSlider = addSlider("Horizontal position", -1.0f, 1.0f, &cv::osu::letterboxing_offset_x, 170)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeLetterboxingOffset) )->setKeyDelta(0.01f)->setAnimated(false);
		m_letterboxingOffsetYSlider = addSlider("Vertical position", -1.0f, 1.0f, &cv::osu::letterboxing_offset_y, 170)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeLetterboxingOffset) )->setKeyDelta(0.01f)->setAnimated(false);
	}

	addSubSection("UI Scaling");
	addCheckbox("DPI Scaling", UString::format("Automatically scale to the DPI of your display: %i DPI.\nScale factor = %i / 96 = %.2gx", env->getDPI(), env->getDPI(), env->getDPIScale()), &cv::osu::ui_scale_to_dpi)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onDPIScalingChange) );
	m_uiScaleSlider = addSlider("UI Scale:", 1.0f, 1.5f, &cv::osu::ui_scale);
	m_uiScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeUIScale) );
	m_uiScaleSlider->setKeyDelta(0.01f);
	m_uiScaleSlider->setAnimated(false);

	addSubSection("Detail Settings");
	addCheckbox("Mipmaps", "Reload your skin to apply! (CTRL + ALT + S)\nGenerate mipmaps for each skin element, at the cost of VRAM.\nProvides smoother visuals on lower resolutions for @2x-only skins.", &cv::osu::skin_mipmaps);
	addSpacer();
	addCheckbox("Snaking in sliders", "\"Growing\" sliders.\nSliders gradually snake out from their starting point while fading in.\nHas no impact on performance whatsoever.", &cv::osu::snaking_sliders);
	addCheckbox("Snaking out sliders", "\"Shrinking\" sliders.\nSliders will shrink with the sliderball while sliding.\nCan improve performance a tiny bit, since there will be less to draw overall.", &cv::osu::slider_shrink);
	addSpacer();
	{
		addCheckbox("Legacy Slider Renderer (!)", "WARNING: Only try enabling this on shitty old computers!\nMay or may not improve fps while few sliders are visible.\nGuaranteed lower fps while many sliders are visible!", &cv::osu::force_legacy_slider_renderer);
		addCheckbox("Higher Quality Sliders (!)", "Disable this if your fps drop too low while sliders are visible.", &cv::osu::options_high_quality_sliders)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onHighQualitySlidersCheckboxChange) );
		m_sliderQualitySlider = addSlider("Slider Quality", 0.0f, 1.0f, &cv::osu::options_slider_quality);
		m_sliderQualitySlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeSliderQuality) );
	}

	//**************************************************************************************************************************//

	CBaseUIElement *sectionAudio = addSection("Audio");

	addSubSection("Devices");
	{
		OPTIONS_ELEMENT outputDeviceSelect = addButton("Select Output Device", "Default", true);
		((CBaseUIButton*)outputDeviceSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceSelect) );
		outputDeviceSelect.resetButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceResetClicked) );

		if (Env::cfg(OS::WINDOWS, AUD::BASS, !AUD::WASAPI) && soundEngine->getTypeId() == SoundEngine::BASS)
		{
			CBaseUICheckbox *audioCompatibilityModeCheckbox = addCheckbox("Audio compatibility mode (!)", "Use legacy audio engine (higher latency but more compatible)\nWARNING: May cause hitsound delays and stuttering!", &cv::win_snd_fallback_dsound);
			audioCompatibilityModeCheckbox->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onAudioCompatibilityModeChange) );

			// HACKHACK: force manual change if user has enabled it (don't use convar callback)
			if (cv::win_snd_fallback_dsound.getBool())
				onAudioCompatibilityModeChange(audioCompatibilityModeCheckbox);
		}

		m_outputDeviceResetButton = outputDeviceSelect.resetButton;
		m_outputDeviceSelectButton = outputDeviceSelect.elements[0];
		m_outputDeviceLabel = (CBaseUILabel*)outputDeviceSelect.elements[1];
	}

	if constexpr (Env::cfg(AUD::WASAPI))
	{
		addSubSection("WASAPI");
		m_wasapiBufferSizeSlider = addSlider("Buffer Size:", 0.000f, 0.050f, &cv::win_snd_wasapi_buffer_size); // NOTE: allow 0 for shared mode windows 10 + period
		m_wasapiBufferSizeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onWASAPIBufferChange) );
		m_wasapiBufferSizeSlider->setKeyDelta(0.001f);
		m_wasapiBufferSizeSlider->setAnimated(false);
		addLabel("Windows 7: Start at 11 ms,")->setTextColor(0xff666666);
		addLabel("Windows 10: Start at 1 ms,")->setTextColor(0xff666666);
		addLabel("and if crackling: increment until fixed.")->setTextColor(0xff666666);
		addLabel("(lower is better, non-wasapi has ~40 ms minimum)")->setTextColor(0xff666666);
		addCheckbox("Exclusive Mode", "Dramatically reduces audio latency, leave this enabled.\nExclusive Mode = No other application can use audio.", &cv::win_snd_wasapi_exclusive);
		//addLabel("(On Windows 10 non-exclusive mode, try buffer = 0)")->setTextColor(0xff666666);
		//addLabel("(then, try getting the smallest possible period)")->setTextColor(0xff666666);
		addLabel("");
		addLabel("");
		addLabel("WARNING: Only if you know what you are doing")->setTextColor(0xffff0000);
		m_wasapiPeriodSizeSlider = addSlider("Period Size:", 0.0f, 0.050f, &cv::win_snd_wasapi_period_size);
		m_wasapiPeriodSizeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onWASAPIPeriodChange) );
		m_wasapiPeriodSizeSlider->setKeyDelta(0.001f);
		m_wasapiPeriodSizeSlider->setAnimated(false);
		OsuUIButton *restartSoundEngine = addButton("Restart SoundEngine");
		restartSoundEngine->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceRestart) );
		restartSoundEngine->setColor(0xff00566b);
		addLabel("");
	}

	addSubSection("Volume");
	CBaseUISlider *masterVolumeSlider = addSlider("Master:", 0.0f, 1.0f, &cv::osu::volume_master, 70.0f);
	masterVolumeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	masterVolumeSlider->setKeyDelta(0.01f);
	CBaseUISlider *inactiveVolumeSlider = addSlider("Inactive:", 0.0f, 1.0f, &cv::osu::volume_master_inactive, 70.0f);
	inactiveVolumeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	inactiveVolumeSlider->setKeyDelta(0.01f);
	CBaseUISlider *musicVolumeSlider = addSlider("Music:", 0.0f, 1.0f, &cv::osu::volume_music, 70.0f);
	musicVolumeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	musicVolumeSlider->setKeyDelta(0.01f);
	CBaseUISlider *effectsVolumeSlider = addSlider("Effects:", 0.0f, 1.0f, &cv::osu::volume_effects, 70.0f);
	effectsVolumeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	effectsVolumeSlider->setKeyDelta(0.01f);

	addSubSection("Offset Adjustment");
	CBaseUISlider *offsetSlider = addSlider("Universal Offset:", -300.0f, 300.0f, &cv::osu::universal_offset);
	offsetSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeIntMS) );
	offsetSlider->setKeyDelta(1);

	addSubSection("Songbrowser");
	addCheckbox("Apply speed/pitch mods while browsing", "Whether to always apply all mods, or keep the preview music normal.", &cv::osu::beatmap_preview_mods_live);

	//**************************************************************************************************************************//

	m_skinSection = addSection("Skin");

	addSubSection("Skin");
	addSkinPreview();
	{
		OPTIONS_ELEMENT skinSelect;
		if (Env::cfg(FEAT::STEAM) && steam->isReady())
		{
			skinSelect = addButtonButtonLabel("Local Skin ...", "Workshop ...", "default");
			m_skinSelectLocalButton = skinSelect.elements[0];
			m_skinSelectWorkshopButton = skinSelect.elements[1];
			m_skinLabel = (CBaseUILabel*)skinSelect.elements[2];

			((OsuUIButton*)m_skinSelectWorkshopButton)->setColor(0xff108fe8);
			((CBaseUIButton*)m_skinSelectWorkshopButton)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelectWorkshop) );
		}
		else
		{
			skinSelect = addButton("Select Skin", "default");
			m_skinSelectLocalButton = skinSelect.elements[0];
			m_skinLabel = (CBaseUILabel*)skinSelect.elements[1];
		}

		((CBaseUIButton*)m_skinSelectLocalButton)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelect) );

		OsuUIButton *openCurrentSkinFolderButton = addButton("Open current skin folder");
		openCurrentSkinFolderButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::openCurrentSkinFolder) );

		OPTIONS_ELEMENT skinReload = addButtonButton("Reload Skin", "Random Skin");
		((OsuUIButton*)skinReload.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinReload) );
		((OsuUIButton*)skinReload.elements[0])->setTooltipText("(CTRL + ALT + S)");
		((OsuUIButton*)skinReload.elements[1])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinRandom) );
		((OsuUIButton*)skinReload.elements[1])->setTooltipText("Temporary, does not change your configured skin (reload to reset).\nUse \"osu_skin_random 1\" to randomize on every skin reload.\nUse \"osu_skin_random_elements 1\" to mix multiple skins.\nUse \"osu_skin_export\" to export the currently active skin.");
		((OsuUIButton*)skinReload.elements[1])->setColor(0xff00566b);
	}
	addSpacer();
	CBaseUISlider *numberScaleSlider = addSlider("Number Scale:", 0.01f, 3.0f, &cv::osu::number_scale_multiplier, 135.0f);
	numberScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	numberScaleSlider->setKeyDelta(0.01f);
	CBaseUISlider *hitResultScaleSlider = addSlider("HitResult Scale:", 0.01f, 3.0f, &cv::osu::hitresult_scale, 135.0f);
	hitResultScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	hitResultScaleSlider->setKeyDelta(0.01f);
	addCheckbox("Draw Numbers", &cv::osu::draw_numbers);
	addCheckbox("Draw Approach Circles", &cv::osu::draw_approach_circles);
	addSpacer();
	addCheckbox("Ignore Beatmap Sample Volume", "Ignore beatmap timingpoint effect volumes.\nQuiet hitsounds can destroy accuracy and concentration, enabling this will fix that.", &cv::osu::ignore_beatmap_sample_volume);
	addCheckbox("Ignore Beatmap Combo Colors", &cv::osu::ignore_beatmap_combo_colors);
	addCheckbox("Use skin's sound samples", "If this is not selected, then the default skin hitsounds will be used.", &cv::osu::skin_use_skin_hitsounds);
	addCheckbox("Load HD @2x", "On very low resolutions (below 1600x900) you can disable this to get smoother visuals.", &cv::osu::skin_hd);
	addSpacer();
	addCheckbox("Draw Cursor Trail", &cv::osu::draw_cursor_trail);
	addCheckbox("Force Smooth Cursor Trail", "Usually, the presence of the cursormiddle.png skin image enables smooth cursortrails.\nThis option allows you to force enable smooth cursortrails for all skins.", &cv::osu::cursor_trail_smooth_force);
	m_cursorSizeSlider = addSlider("Cursor Size:", 0.01f, 5.0f, &cv::osu::cursor_scale, -1.0f, true);
	m_cursorSizeSlider->setAnimated(false);
	m_cursorSizeSlider->setKeyDelta(0.01f);
	addCheckbox("Automatic Cursor Size", "Cursor size will adjust based on the CS of the current beatmap.", &cv::osu::automatic_cursor_size);
	addSpacer();
	m_sliderPreviewElement = (OsuOptionsMenuSliderPreviewElement*)addSliderPreview();
	addSlider("Slider Border Size", 0.0f, 9.0f, &cv::osu::slider_border_size_multiplier)->setKeyDelta(0.01f);
	addSlider("Slider Opacity", 0.0f, 1.0f, &cv::osu::slider_alpha_multiplier, 200.0f);
	addSlider("Slider Body Opacity", 0.0f, 1.0f, &cv::osu::slider_body_alpha_multiplier, 200.0f, true);
	addSlider("Slider Body Saturation", 0.0f, 1.0f, &cv::osu::slider_body_color_saturation, 200.0f, true);
	addCheckbox("Use slidergradient.png", "Enabling this will improve performance,\nbut also block all dynamic slider (color/border) features.", &cv::osu::slider_use_gradient_image);
	addCheckbox("Use osu!lazer Slider Style", "Only really looks good if your skin doesn't \"SliderTrackOverride\" too dark.", &cv::osu::slider_osu_next_style);
	addCheckbox("Use combo color as tint for slider ball", &cv::osu::slider_ball_tint_combo_color);
	addCheckbox("Use combo color as tint for slider border", &cv::osu::slider_border_tint_combo_color);
	addCheckbox("Draw Slider End Circle", &cv::osu::slider_draw_endcircle);

	//**************************************************************************************************************************//

	CBaseUIElement *sectionInput = addSection("Input");

	addSubSection("Mouse", "scroll");

	addSlider("Sensitivity:", 0.1f, 6.0f, &cv::mouse_sensitivity)->setKeyDelta(0.01f);

	addCheckbox("Raw Input", &cv::mouse_raw_input);
	addCheckbox("Map Absolute Raw Input to Window", &cv::mouse_raw_input_absolute_to_window)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onRawInputToAbsoluteWindowChange) );
	addCheckbox("Confine Cursor (Windowed)", &cv::osu::confine_cursor_windowed);
	addCheckbox("Confine Cursor (Fullscreen)", &cv::osu::confine_cursor_fullscreen);
	addCheckbox("Confine Cursor (NEVER)", "Don't try to confine the cursor, even in gameplay.\n(Workaround for relative tablet motion quirks with Xwayland)", &cv::osu::confine_cursor_never);

	addCheckbox("Disable Mouse Wheel in Play Mode", &cv::osu::disable_mousewheel);
	addCheckbox("Disable Mouse Buttons in Play Mode", &cv::osu::disable_mousebuttons);
	addCheckbox("Cursor ripples", "The cursor will ripple outwards on clicking.", &cv::osu::draw_cursor_ripples);

	addSubSection("Tablet");
	addCheckbox("Ignore Sensitivity & Raw Input", "Only use this if nothing else works.\nIf this is enabled, then the in-game sensitivity slider will no longer work for tablets!\n(You can then instead use your tablet configuration software to change the tablet area.)", &cv::tablet_sensitivity_ignore);

	addSpacer();
	const UString keyboardSectionTags = "keyboard keys key bindings binds keybinds keybindings";
	CBaseUIElement *subSectionKeyboard = addSubSection("Keyboard", keyboardSectionTags);
	OsuUIButton *resetAllKeyBindingsButton = addButton("Reset all key bindings");
	resetAllKeyBindingsButton->setColor(0xffff0000);
	resetAllKeyBindingsButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingsResetAllPressed) );
	addSubSection("Keys - osu! Standard Mode", keyboardSectionTags);
	addKeyBindButton("Left Click", &cv::osu::keybinds::LEFT_CLICK);
	addKeyBindButton("Right Click", &cv::osu::keybinds::RIGHT_CLICK);
	addSpacer();
	addKeyBindButton("Left Click (2)", &cv::osu::keybinds::LEFT_CLICK_2);
	addKeyBindButton("Right Click (2)", &cv::osu::keybinds::RIGHT_CLICK_2);
	addSubSection("Keys - FPoSu", keyboardSectionTags);
	addKeyBindButton("Zoom", &cv::osu::keybinds::FPOSU_ZOOM);
	addSubSection("Keys - In-Game", keyboardSectionTags);
	addKeyBindButton("Game Pause", &cv::osu::keybinds::GAME_PAUSE)->setDisallowLeftMouseClickBinding(true);
	addKeyBindButton("Skip Cutscene", &cv::osu::keybinds::SKIP_CUTSCENE);
	addKeyBindButton("Toggle Scoreboard", &cv::osu::keybinds::TOGGLE_SCOREBOARD);
	addKeyBindButton("Scrubbing (+ Click Drag!)", &cv::osu::keybinds::SEEK_TIME);
	addKeyBindButton("Quick Seek -5sec <<<", &cv::osu::keybinds::SEEK_TIME_BACKWARD);
	addKeyBindButton("Quick Seek +5sec >>>", &cv::osu::keybinds::SEEK_TIME_FORWARD);
	addKeyBindButton("Increase Local Song Offset", &cv::osu::keybinds::INCREASE_LOCAL_OFFSET);
	addKeyBindButton("Decrease Local Song Offset", &cv::osu::keybinds::DECREASE_LOCAL_OFFSET);
	addKeyBindButton("Quick Retry (hold briefly)", &cv::osu::keybinds::QUICK_RETRY);
	addKeyBindButton("Quick Save", &cv::osu::keybinds::QUICK_SAVE);
	addKeyBindButton("Quick Load", &cv::osu::keybinds::QUICK_LOAD);
	addSubSection("Keys - Universal", keyboardSectionTags);
	addKeyBindButton("Save Screenshot", &cv::osu::keybinds::SAVE_SCREENSHOT);
	addKeyBindButton("Increase Volume", &cv::osu::keybinds::INCREASE_VOLUME);
	addKeyBindButton("Decrease Volume", &cv::osu::keybinds::DECREASE_VOLUME);
	addKeyBindButton("Disable Mouse Buttons", &cv::osu::keybinds::DISABLE_MOUSE_BUTTONS);
	addKeyBindButton("Boss Key (Minimize)", &cv::osu::keybinds::BOSS_KEY);
	addSubSection("Keys - Song Select", keyboardSectionTags);
	addKeyBindButton("Toggle Mod Selection Screen", &cv::osu::keybinds::TOGGLE_MODSELECT)->setTooltipText("(F1 can not be unbound. This is just an additional key.)");
	addKeyBindButton("Random Beatmap", &cv::osu::keybinds::RANDOM_BEATMAP)->setTooltipText("(F2 can not be unbound. This is just an additional key.)");
	addSubSection("Keys - Mod Select", keyboardSectionTags);
	addKeyBindButton("Easy", &cv::osu::keybinds::MOD_EASY);
	addKeyBindButton("No Fail", &cv::osu::keybinds::MOD_NOFAIL);
	addKeyBindButton("Half Time", &cv::osu::keybinds::MOD_HALFTIME);
	addKeyBindButton("Hard Rock", &cv::osu::keybinds::MOD_HARDROCK);
	addKeyBindButton("Sudden Death", &cv::osu::keybinds::MOD_SUDDENDEATH);
	addKeyBindButton("Double Time", &cv::osu::keybinds::MOD_DOUBLETIME);
	addKeyBindButton("Hidden", &cv::osu::keybinds::MOD_HIDDEN);
	addKeyBindButton("Flashlight", &cv::osu::keybinds::MOD_FLASHLIGHT);
	addKeyBindButton("Relax", &cv::osu::keybinds::MOD_RELAX);
	addKeyBindButton("Autopilot", &cv::osu::keybinds::MOD_AUTOPILOT);
	addKeyBindButton("Spunout", &cv::osu::keybinds::MOD_SPUNOUT);
	addKeyBindButton("Auto", &cv::osu::keybinds::MOD_AUTO);
	addKeyBindButton("Score V2", &cv::osu::keybinds::MOD_SCOREV2);
	///addButton("osu!mania layout")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingManiaPressed) );

	//**************************************************************************************************************************//

	CBaseUIElement *sectionGameplay = addSection("Gameplay");

	addSubSection("General");
	m_backgroundDimSlider = addSlider("Background Dim:", 0.0f, 1.0f, &cv::osu::background_dim, 220.0f);
	m_backgroundDimSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_backgroundBrightnessSlider = addSlider("Background Brightness:", 0.0f, 1.0f, &cv::osu::background_brightness, 220.0f);
	m_backgroundBrightnessSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	addSpacer();
	addCheckbox("Don't change dim level during breaks", "Makes the background basically impossible to see during breaks.\nNot recommended.", &cv::osu::background_dont_fade_during_breaks);
	addCheckbox("Show approach circle on first \"Hidden\" object", &cv::osu::show_approach_circle_on_first_hidden_object);
	addCheckbox("SuddenDeath restart on miss", "Skips the failing animation, and instantly restarts like SS/PF.", &cv::osu::mod_suddendeath_restart);
	addCheckbox("Show Skip Button during Intro", "Skip intro to first hitobject.", &cv::osu::skip_intro_enabled);
	addCheckbox("Show Skip Button during Breaks", "Skip breaks in the middle of beatmaps.", &cv::osu::skip_breaks_enabled);
	addSpacer();
	addSubSection("Mechanics", "health drain notelock lock block blocking noteblock");
	addCheckbox("Kill Player upon Failing", "Enabled: Singleplayer default. You die upon failing and the beatmap stops.\nDisabled: Multiplayer default. Allows you to keep playing even after failing.", &cv::osu::drain_kill);
	addSpacer();
	addLabel("");

	///addCheckbox("Notelock (note blocking/locking)", "NOTE: osu! has this always enabled, so leave it enabled for practicing.\n\"Protects\" you by only allowing circles to be clicked in order.", &cv::osu::note_blocking);
	OPTIONS_ELEMENT drainSelect = addButton("Select [HP Drain]", "None", true);
	((CBaseUIButton*)drainSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onHPDrainSelect) );
	m_hpDrainSelectButton = drainSelect.elements[0];
	m_hpDrainSelectLabel = (CBaseUILabel*)drainSelect.elements[1];
	m_hpDrainSelectResetButton = drainSelect.resetButton;
	m_hpDrainSelectResetButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onHPDrainSelectResetClicked) );
	addLabel("");
	addLabel("Info about different drain algorithms:")->setTextColor(0xff666666);
	addLabel("");
	addLabel("- osu!stable: Constant drain, moderately hard (default).")->setTextColor(0xff666666);
	addLabel("- osu!lazer 2020: Constant drain, very easy (too easy?).")->setTextColor(0xff666666);
	addLabel("- osu!lazer 2018: No constant drain, scales with HP.")->setTextColor(0xff666666);
	addSpacer(2);
	OPTIONS_ELEMENT notelockSelect = addButton("Select [Notelock]", "None", true);
	((CBaseUIButton*)notelockSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onNotelockSelect) );
	m_notelockSelectButton = notelockSelect.elements[0];
	m_notelockSelectLabel = (CBaseUILabel*)notelockSelect.elements[1];
	m_notelockSelectResetButton = notelockSelect.resetButton;
	m_notelockSelectResetButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onNotelockSelectResetClicked) );
	addLabel("");
	addLabel("Info about different notelock algorithms:")->setTextColor(0xff666666);
	addLabel("");
	addLabel("- McOsu: Auto miss previous circle, always.")->setTextColor(0xff666666);
	addLabel("- osu!stable: Locked until previous circle is miss.")->setTextColor(0xff666666);
	addLabel("- osu!lazer 2020: Auto miss previous circle if > time.")->setTextColor(0xff666666);
	addLabel("");
	addSpacer();
	addSubSection("Backgrounds");
	addCheckbox("Load Background Images (!)", "NOTE: Disabling this will disable ALL beatmap images everywhere!", &cv::osu::load_beatmap_background_images);
	addCheckbox("Draw Background in Beatmap", &cv::osu::draw_beatmap_background_image);
	addCheckbox("Draw Background in SongBrowser", "NOTE: You can disable this if you always want menu-background.", &cv::osu::draw_songbrowser_background_image);
	addCheckbox("Draw Background Thumbnails in SongBrowser", &cv::osu::draw_songbrowser_thumbnails);
	addCheckbox("Draw Background in Ranking/Results Screen", &cv::osu::draw_rankingscreen_background_image);
	addCheckbox("Draw menu-background in Menu", &cv::osu::draw_menu_background);
	addCheckbox("Draw menu-background in SongBrowser", "NOTE: Only applies if \"Draw Background in SongBrowser\" is disabled.", &cv::osu::draw_songbrowser_menu_background_image);
	addSpacer();
	//addCheckbox("Show pp on ranking screen", &cv::osu::rankingscreen_pp);

	addSubSection("HUD");
	addCheckbox("Draw HUD", "NOTE: You can also press SHIFT + TAB while playing to toggle this.", &cv::osu::draw_hud);
	addCheckbox("SHIFT + TAB toggles everything", "Enabled: McOsu default (toggle \"Draw HUD\")\nDisabled: osu! default (always show hiterrorbar + key overlay)", &cv::osu::hud_shift_tab_toggles_everything);
	addSpacer();
	addCheckbox("Draw Score", &cv::osu::draw_score);
	addCheckbox("Draw Combo", &cv::osu::draw_combo);
	addCheckbox("Draw Accuracy", &cv::osu::draw_accuracy);
	addCheckbox("Draw ProgressBar", &cv::osu::draw_progressbar);
	addCheckbox("Draw HitErrorBar", &cv::osu::draw_hiterrorbar);
	addCheckbox("Draw HitErrorBar UR", "Unstable Rate", &cv::osu::draw_hiterrorbar_ur);
	addCheckbox("Draw ScoreBar", "Health/HP Bar.", &cv::osu::draw_scorebar);
	addCheckbox("Draw ScoreBar-bg", "Some skins abuse this as the playfield background image.\nIt is actually just the background image for the Health/HP Bar.", &cv::osu::draw_scorebarbg);
	addCheckbox("Draw ScoreBoard", &cv::osu::draw_scoreboard);
	addCheckbox("Draw Key Overlay", &cv::osu::draw_inputoverlay);
	addCheckbox("Draw Scrubbing Timeline", &cv::osu::draw_scrubbing_timeline);
	addCheckbox("Draw Miss Window on HitErrorBar", &cv::osu::hud_hiterrorbar_showmisswindow);
	addSpacer();
	addCheckbox("Draw Stats: pp", "Realtime pp counter.\nDynamically calculates earned pp by incrementally updating the star rating.", &cv::osu::draw_statistics_pp);
	addCheckbox("Draw Stats: pp (SS)", "Max possible total pp for active mods (full combo + perfect acc).", &cv::osu::draw_statistics_perfectpp);
	addCheckbox("Draw Stats: Misses", "Number of misses.", &cv::osu::draw_statistics_misses);
	addCheckbox("Draw Stats: SliderBreaks", "Number of slider breaks.", &cv::osu::draw_statistics_sliderbreaks);
	addCheckbox("Draw Stats: Max Possible Combo", &cv::osu::draw_statistics_maxpossiblecombo);
	addCheckbox("Draw Stats: Stars*** (Until Now)", "Incrementally updates the star rating (aka \"realtime stars\").", &cv::osu::draw_statistics_livestars);
	addCheckbox("Draw Stats: Stars* (Total)", "Total stars for active mods.", &cv::osu::draw_statistics_totalstars);
	addCheckbox("Draw Stats: BPM", &cv::osu::draw_statistics_bpm);
	if constexpr (Env::cfg(AUD::SOLOUD)) // NOTE: SoLoud only, currently
	if (soundEngine->getTypeId() == SoundEngine::SOLOUD)
		addCheckbox("Draw Stats: Current BPM (Approx.)", &cv::osu::draw_statistics_detected_bpm);
	addCheckbox("Draw Stats: AR", &cv::osu::draw_statistics_ar);
	addCheckbox("Draw Stats: CS", &cv::osu::draw_statistics_cs);
	addCheckbox("Draw Stats: OD", &cv::osu::draw_statistics_od);
	addCheckbox("Draw Stats: HP", &cv::osu::draw_statistics_hp);
	addCheckbox("Draw Stats: 300 hitwindow", "Timing window for hitting a 300 (e.g. +-25ms).", &cv::osu::draw_statistics_hitwindow300);
	addCheckbox("Draw Stats: Notes Per Second", "How many clicks per second are currently required.", &cv::osu::draw_statistics_nps);
	addCheckbox("Draw Stats: Note Density", "How many objects are visible at the same time.", &cv::osu::draw_statistics_nd);
	addCheckbox("Draw Stats: Unstable Rate", &cv::osu::draw_statistics_ur);
	addCheckbox("Draw Stats: Accuracy Error", "Average hit error delta (e.g. -5ms +15ms).\nSee \"osu_hud_statistics_hitdelta_chunksize 30\",\nit defines how many recent hit deltas are averaged.", &cv::osu::draw_statistics_hitdelta);
	addSpacer();
	m_hudSizeSlider = addSlider("HUD Scale:", 0.01f, 3.0f, &cv::osu::hud_scale, 165.0f);
	m_hudSizeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudSizeSlider->setKeyDelta(0.01f);
	addSpacer();
	m_hudScoreScaleSlider = addSlider("Score Scale:", 0.01f, 3.0f, &cv::osu::hud_score_scale, 165.0f);
	m_hudScoreScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudScoreScaleSlider->setKeyDelta(0.01f);
	m_hudComboScaleSlider = addSlider("Combo Scale:", 0.01f, 3.0f, &cv::osu::hud_combo_scale, 165.0f);
	m_hudComboScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudComboScaleSlider->setKeyDelta(0.01f);
	m_hudAccuracyScaleSlider = addSlider("Accuracy Scale:", 0.01f, 3.0f, &cv::osu::hud_accuracy_scale, 165.0f);
	m_hudAccuracyScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudAccuracyScaleSlider->setKeyDelta(0.01f);
	m_hudHiterrorbarScaleSlider = addSlider("HitErrorBar Scale:", 0.01f, 3.0f, &cv::osu::hud_hiterrorbar_scale, 165.0f);
	m_hudHiterrorbarScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudHiterrorbarScaleSlider->setKeyDelta(0.01f);
	m_hudHiterrorbarURScaleSlider = addSlider("HitErrorBar UR Scale:", 0.01f, 3.0f, &cv::osu::hud_hiterrorbar_ur_scale, 165.0f);
	m_hudHiterrorbarURScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudHiterrorbarURScaleSlider->setKeyDelta(0.01f);
	m_hudProgressbarScaleSlider = addSlider("ProgressBar Scale:", 0.01f, 3.0f, &cv::osu::hud_progressbar_scale, 165.0f);
	m_hudProgressbarScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudProgressbarScaleSlider->setKeyDelta(0.01f);
	m_hudScoreBarScaleSlider = addSlider("ScoreBar Scale:", 0.01f, 3.0f, &cv::osu::hud_scorebar_scale, 165.0f);
	m_hudScoreBarScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudScoreBarScaleSlider->setKeyDelta(0.01f);
	m_hudScoreBoardScaleSlider = addSlider("ScoreBoard Scale:", 0.01f, 3.0f, &cv::osu::hud_scoreboard_scale, 165.0f);
	m_hudScoreBoardScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudScoreBoardScaleSlider->setKeyDelta(0.01f);
	m_hudInputoverlayScaleSlider = addSlider("Key Overlay Scale:", 0.01f, 3.0f, &cv::osu::hud_inputoverlay_scale, 165.0f);
	m_hudInputoverlayScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudInputoverlayScaleSlider->setKeyDelta(0.01f);
	m_statisticsOverlayScaleSlider = addSlider("Statistics Scale:", 0.01f, 3.0f, &cv::osu::hud_statistics_scale, 165.0f);
	m_statisticsOverlayScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_statisticsOverlayScaleSlider->setKeyDelta(0.01f);
	addSpacer();
	m_statisticsOverlayXOffsetSlider = addSlider("Statistics X Offset:", 0.0f, 2000.0f, &cv::osu::hud_statistics_offset_x, 165.0f, true);
	m_statisticsOverlayXOffsetSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
	m_statisticsOverlayXOffsetSlider->setKeyDelta(1.0f);
	m_statisticsOverlayYOffsetSlider = addSlider("Statistics Y Offset:", 0.0f, 1000.0f, &cv::osu::hud_statistics_offset_y, 165.0f, true);
	m_statisticsOverlayYOffsetSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
	m_statisticsOverlayYOffsetSlider->setKeyDelta(1.0f);

	addSubSection("Playfield");
	addCheckbox("Draw FollowPoints", &cv::osu::draw_followpoints);
	addCheckbox("Draw Playfield Border", "Correct border relative to the current Circle Size.", &cv::osu::draw_playfield_border);
	addSpacer();
	m_playfieldBorderSizeSlider = addSlider("Playfield Border Size:", 0.0f, 500.0f, &cv::osu::hud_playfield_border_size);
	m_playfieldBorderSizeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
	m_playfieldBorderSizeSlider->setKeyDelta(1.0f);

	addSubSection("Hitobjects");
	addCheckbox("Use Fast Hidden Fading Sliders (!)", "NOTE: osu! doesn't do this, so don't enable it for serious practicing.\nIf enabled: Fade out sliders with the same speed as circles.", &cv::osu::mod_hd_slider_fast_fade);
	addCheckbox("Use Score v2 Slider Accuracy", "Affects pp and accuracy calculations, but does not affect score.\nUse the score v2 mod if you want the 1000000 max score cap/calculation.", &cv::osu::slider_scorev2);

	//**************************************************************************************************************************//

	CBaseUIElement *sectionFposu = addSection("FPoSu (3D)");

	addSubSection("FPoSu - General");
	addCheckbox("FPoSu", (Env::cfg(OS::WINDOWS) ? "The real 3D FPS mod.\nPlay from a first person shooter perspective in a 3D environment.\nThis is only intended for mouse! (Enable \"Tablet/Absolute Mode\" for tablets.)" : "The real 3D FPS mod.\nPlay from a first person shooter perspective in a 3D environment.\nThis is only intended for mouse!"), &cv::osu::fposu::mod_fposu);
#ifdef MCOSU_FPOSU_4D_MODE_FINISHED
	addCheckbox("[Beta] 4D Mode", (Env::cfg(OS::WINDOWS) ? "Actual 3D circles instead of \"just\" a flat playfield in 3D.\nNOTE: Not compatible with \"Tablet/Absolute Mode\"." : "Actual 3D circles instead of \"just\" a flat playfield in 3D."), &cv::osu::fposu::threeD);
	addCheckbox("[Beta] 4D Mode - Spheres", "Combocolored lit 3D spheres instead of flat 3D circles.\nOnly relevant if \"[Beta] 4D Mode\" is enabled.", &cv::osu::fposu::threeD_spheres);
#endif
	addLabel("");
	addLabel("NOTE: Use CTRL + O during gameplay to get here!")->setTextColor(0xff555555);
	addLabel("");
	addLabel("LEFT/RIGHT arrow keys to precisely adjust sliders.")->setTextColor(0xff555555);
	addLabel("");
	CBaseUISlider *fposuDistanceSlider = addSlider("Distance:", 0.01f, 5.0f, &cv::osu::fposu::distance, -1.0f, true);
	fposuDistanceSlider->setKeyDelta(0.01f);
	addCheckbox("Vertical FOV", "If enabled: Vertical FOV.\nIf disabled: Horizontal FOV (default).", &cv::osu::fposu::vertical_fov);
	CBaseUISlider *fovSlider = addSlider("FOV:", 10.0f, 160.0f, &cv::osu::fposu::fov, -1.0f, true, true);
	fovSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeTwoDecimalPlaces) );
	fovSlider->setKeyDelta(0.01f);
	CBaseUISlider *zoomedFovSlider = addSlider("FOV (Zoom):", 10.0f, 160.0f, &cv::osu::fposu::zoom_fov, -1.0f, true, true);
	zoomedFovSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeTwoDecimalPlaces) );
	zoomedFovSlider->setKeyDelta(0.01f);
	addCheckbox("Zoom Key Toggle", "Enabled: Zoom key toggles zoom.\nDisabled: Zoom while zoom key is held.", &cv::osu::fposu::zoom_toggle);
	addSubSection("FPoSu - Playfield");
	addCheckbox("Curved play area", &cv::osu::fposu::curved);
	addCheckbox("Background cube", &cv::osu::fposu::cube);
	addCheckbox("Skybox", "NOTE: Overrides \"Background cube\".\nSee skybox_example.png for cubemap layout.", &cv::osu::fposu::skybox);
	addSlider("Background Opacity", 0.0f, 1.0f, &cv::osu::background_alpha)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
#ifdef MCOSU_FPOSU_4D_MODE_FINISHED
	CBaseUISlider *playfieldScaleSlider = addSlider("[Beta] 4D Mode - Scale", 0.1f, 10.0f, &cv::osu::fposu::threeD_playfield_scale, 0.0f, true);
	playfieldScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	playfieldScaleSlider->setKeyDelta(0.01f);
	CBaseUISlider *spheresAASlider = addSlider("[Beta] 4D Mode - Spheres - MSAA", 0.0f, 16.0f, &cv::osu::fposu::threeD_spheres_aa);
	//spheresAASlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeVRAntiAliasing) );
	spheresAASlider->setKeyDelta(2.0f);
	spheresAASlider->setAnimated(false);
#endif

	addSubSection("FPoSu - Mouse");
	OsuUIButton *cm360CalculatorLinkButton = addButton("https://www.mouse-sensitivity.com/");
	cm360CalculatorLinkButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onCM360CalculatorLinkClicked) );
	cm360CalculatorLinkButton->setColor(0xff10667b);
	addLabel("");
	m_dpiTextbox = addTextbox(cv::osu::fposu::mouse_dpi.getString(), "DPI:", &cv::osu::fposu::mouse_dpi);
	m_cm360Textbox = addTextbox(cv::osu::fposu::mouse_cm_360.getString(), "cm per 360:", &cv::osu::fposu::mouse_cm_360);
	addLabel("");
	addCheckbox("Invert Vertical", &cv::osu::fposu::invert_vertical);
	addCheckbox("Invert Horizontal", &cv::osu::fposu::invert_horizontal);
	addCheckbox("Tablet/Absolute Mode (!)", "WARNING: Do NOT enable this if you are using a mouse!\nIf this is enabled, then DPI and cm per 360 will be ignored!", &cv::osu::fposu::absolute_mode);
	if constexpr (Env::cfg(OS::LINUX))
	{
#ifdef MCOSU_FPOSU_4D_MODE_FINISHED
		addSubSection("[Beta] FPoSu 4D Mode - Mouse");
		addSlider("Sensitivity:", 0.1f, 6.0f, &cv::mouse_sensitivity)->setKeyDelta(0.01f);
#endif
	}

	//**************************************************************************************************************************//

	CBaseUIElement *sectionOnline = NULL;

	if constexpr (Env::cfg(FEAT::DISCORD | FEAT::STEAM)) {
		sectionOnline = addSection("Online");

		addSubSection("Integration");
		addCheckbox("Rich Presence (Discord + Steam)", "Shows your current game state in your friends' friendslists.\ne.g.: Playing Gavin G - Reach Out [Cherry Blossom's Insane]", &cv::osu::rich_presence);
	}

	//**************************************************************************************************************************//

	addSection("Bullshit");

	addSubSection("Why");
	addCheckbox("Rainbow Circles", &cv::osu::circle_rainbow);
	addCheckbox("Rainbow Sliders", &cv::osu::slider_rainbow);
	addCheckbox("Rainbow Numbers", &cv::osu::circle_number_rainbow);
	addCheckbox("SliderBreak Epilepsy", &cv::osu::slider_break_epilepsy);
	addCheckbox("Invisible Cursor", &cv::osu::hide_cursor_during_gameplay);
	addCheckbox("Draw 300s", &cv::osu::hitresult_draw_300s);

	addSection("Maintenance");
	addSubSection("Restore");
	OsuUIButton *resetAllSettingsButton = addButton("Reset all settings");
	resetAllSettingsButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResetEverythingClicked) );
	resetAllSettingsButton->setColor(0xffff0000);
	addSpacer(8);

	//**************************************************************************************************************************//

	// build categories
	addCategory(sectionGeneral, OsuIcons::GEAR);
	addCategory(sectionGraphics, OsuIcons::DESKTOP);

	addCategory(sectionAudio, OsuIcons::VOLUME_UP);
	addCategory(m_skinSection, OsuIcons::PAINTBRUSH);
	addCategory(sectionInput, OsuIcons::GAMEPAD);
	addCategory(subSectionKeyboard, OsuIcons::KEYBOARD);
	addCategory(sectionGameplay, OsuIcons::CIRCLE);
	m_fposuCategoryButton = addCategory(sectionFposu, OsuIcons::CUBE);

	if (sectionOnline != NULL)
		addCategory(sectionOnline, OsuIcons::GLOBE);

	//**************************************************************************************************************************//

	// the context menu gets added last (drawn on top of everything)
	m_options->getContainer()->addBaseUIElement(m_contextMenu);

	// HACKHACK: force current value update
	if (m_sliderQualitySlider != NULL)
		onHighQualitySlidersConVarChange("", cv::osu::options_high_quality_sliders.getString());
}

OsuOptionsMenu::~OsuOptionsMenu()
{
	SAFE_DELETE(m_container);
}

void OsuOptionsMenu::draw()
{
	const bool isAnimating = anim->isAnimating(&m_fAnimation);
	if (!m_bVisible && !isAnimating) return;

	m_sliderPreviewElement->setDrawSliderHack(!isAnimating);

	if (isAnimating)
	{
		osu->getSliderFrameBuffer()->enable();

		g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_ALPHA);
	}

	const bool isPlayingBeatmap = osu->isInPlayMode();

	// interactive sliders

	if (m_backgroundBrightnessSlider->isActive())
	{
		if (!isPlayingBeatmap)
		{
			const float brightness = std::clamp<float>(m_backgroundBrightnessSlider->getFloat(), 0.0f, 1.0f);
			const short red = std::clamp<float>(brightness * cv::osu::background_color_r.getFloat(), 0.0f, 255.0f);
			const short green = std::clamp<float>(brightness * cv::osu::background_color_g.getFloat(), 0.0f, 255.0f);
			const short blue = std::clamp<float>(brightness * cv::osu::background_color_b.getFloat(), 0.0f, 255.0f);
			if (brightness > 0.0f)
			{
				g->setColor(rgb(red, green, blue));
				g->fillRect(0, 0, osu->getVirtScreenWidth(), osu->getVirtScreenHeight());
			}
		}
	}

	if (m_backgroundDimSlider->isActive())
	{
		if (!isPlayingBeatmap)
		{
			const short dim = std::clamp<float>(m_backgroundDimSlider->getFloat(), 0.0f, 1.0f)*255.0f;
			g->setColor(argb(dim, 0, 0, 0));
			g->fillRect(0, 0, osu->getVirtScreenWidth(), osu->getVirtScreenHeight());
		}
	}

	m_container->draw();

	if (m_hudSizeSlider->isActive()
		|| m_hudComboScaleSlider->isActive()
		|| m_hudScoreScaleSlider->isActive()
		|| m_hudAccuracyScaleSlider->isActive()
		|| m_hudHiterrorbarScaleSlider->isActive()
		|| m_hudHiterrorbarURScaleSlider->isActive()
		|| m_hudProgressbarScaleSlider->isActive()
		|| m_hudScoreBarScaleSlider->isActive()
		|| m_hudScoreBoardScaleSlider->isActive()
		|| m_hudInputoverlayScaleSlider->isActive()
		|| m_statisticsOverlayScaleSlider->isActive()
		|| m_statisticsOverlayXOffsetSlider->isActive()
		|| m_statisticsOverlayYOffsetSlider->isActive())
	{
		if (!isPlayingBeatmap)
			osu->getHUD()->drawDummy();
	}
	else if (m_playfieldBorderSizeSlider->isActive())
	{
		osu->getHUD()->drawPlayfieldBorder(OsuGameRules::getPlayfieldCenter(), OsuGameRules::getPlayfieldSize(), 100);
	}
	else
		OsuScreenBackable::draw();

	if (m_cursorSizeSlider->getFloat() < 0.15f)
		mouse->drawDebug();

	/*
	if (m_sliderQualitySlider->isActive())
	{
		Vector2 startPos = Vector2(50, 50);
		Vector2 size = Vector2((int)(osu->getUIScale(250.0f)), (int)(osu->getUIScale(250.0f)));
		const float hitcircleDiameter = osu->getUIScale(75.0f);
		const float length = size.x - hitcircleDiameter;
		const float pointDist = m_osu_slider_curve_points_separation->getFloat()*2;
		const int numPoints = length / pointDist;
		std::vector<Vector2> pointsMouth;
		std::vector<Vector2> pointsEyeLeft;
		std::vector<Vector2> pointsEyeRight;
		for (int i=0; i<numPoints+1; i++)
		{
			int heightAdd = i;
			if (i > numPoints/2)
				heightAdd = numPoints-i;
			float heightAddPercent = (float)heightAdd / (float)(numPoints/2.0f);
			float temp = 1.0f - heightAddPercent;
			temp *= temp;
			heightAddPercent = 1.0f - temp;

			if (i*pointDist < length/3 - hitcircleDiameter/6)
				pointsEyeLeft.push_back(Vector2(startPos.x + hitcircleDiameter/2 + i*pointDist, startPos.y + hitcircleDiameter/2 - (i*pointDist - (length/3 - hitcircleDiameter/6))));
			else if (i*pointDist > 2*(length/3) + hitcircleDiameter/6)
				pointsEyeRight.push_back(Vector2(startPos.x + hitcircleDiameter/2 + i*pointDist, startPos.y + hitcircleDiameter/2));

			Vector2 mouthOffset = Vector2(0, size.y/2);
			pointsMouth.push_back(Vector2(startPos.x + hitcircleDiameter/2 + i*pointDist, startPos.y + hitcircleDiameter/2 + heightAddPercent*(size.y/2 - hitcircleDiameter)) + mouthOffset);
		}

		g->setColor(0xff999999);
		g->fillRect(startPos.x - 5, startPos.y - 5, size.x + 8, size.y + 10);
		g->setColor(0xffffffff);
		g->fillRect(startPos.x, startPos.y, size.x/2, size.y);
		g->setColor(0xff000000);
		g->fillRect(startPos.x + size.x/2 - 1, startPos.y, size.x/2, size.y);

		OsuSliderRenderer::draw(osu, pointsEyeLeft, hitcircleDiameter);
		OsuSliderRenderer::draw(osu, pointsEyeRight, hitcircleDiameter);
		OsuSliderRenderer::draw(osu, pointsMouth, hitcircleDiameter);
	}
	*/

	if (isAnimating)
	{
		// HACKHACK:
		if (!m_bVisible)
			m_backButton->draw();

		g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);

		osu->getSliderFrameBuffer()->disable();

		g->push3DScene(McRect(0, 0, m_options->getSize().x, m_options->getSize().y));
		{
			g->rotate3DScene(0, -(1.0f - m_fAnimation)*90, 0);
			g->translate3DScene(-(1.0f - m_fAnimation)*m_options->getSize().x*1.25f, 0, -(1.0f - m_fAnimation)*700);

			osu->getSliderFrameBuffer()->setColor(argb(m_fAnimation, 1.0f, 1.0f, 1.0f));
			osu->getSliderFrameBuffer()->draw(0, 0);
		}
		g->pop3DScene();
	}
}

void OsuOptionsMenu::update()
{
	OsuScreenBackable::update();

	// workshop background refresh takes some time, open context menu after loading is finished
	// do this even if options menu is invisible (but only show context menu if visible after finished)
	if constexpr (Env::cfg(FEAT::STEAM))
	{
		if (m_bWorkshopSkinSelectScheduled)
		{
			if (osu->getSteamWorkshop()->isReady())
			{
				m_bWorkshopSkinSelectScheduled = false;

				if (isVisible())
					onSkinSelectWorkshop3();
			}
		}
	}

	if (!m_bVisible) return;

	if (m_bDPIScalingScrollToSliderScheduled)
	{
		m_bDPIScalingScrollToSliderScheduled = false;
		m_options->scrollToElement(m_uiScaleSlider, 0, 200 * Osu::getUIScale());
	}

	if (osu->getHUD()->isVolumeOverlayBusy() || m_backButton->isActive())
		m_container->stealFocus();

	m_container->update();

	// force context menu focus
	if (m_contextMenu->isVisible())
	{
		const std::vector<CBaseUIElement*> &elements = m_options->getContainer()->getElements();
		for (int i=0; i<elements.size(); i++)
		{
			CBaseUIElement *e = (elements[i]);

			if (e == m_contextMenu)
				continue;

			e->stealFocus();
		}
	}

	// flash osu!folder textbox red if incorrect
	if (m_fOsuFolderTextboxInvalidAnim > engine->getTime())
	{
		Channel redness = std::abs(std::sin((m_fOsuFolderTextboxInvalidAnim - engine->getTime())*3))*128;
		m_osuFolderTextbox->setBackgroundColor(rgb(redness, 0, 0));
	}
	else
		m_osuFolderTextbox->setBackgroundColor(0xff000000);

	// hack to avoid entering search text while binding keys
	if (osu->getNotificationOverlay()->isVisible() && osu->getNotificationOverlay()->isWaitingForKey())
		m_fSearchOnCharKeybindHackTime = engine->getTime() + 0.1f;

	// highlight active category depending on scroll position
	if (m_categoryButtons.size() > 0)
	{
		OsuOptionsMenuCategoryButton *activeCategoryButton = m_categoryButtons[0];
		for (int i=0; i<m_categoryButtons.size(); i++)
		{
			OsuOptionsMenuCategoryButton *categoryButton = m_categoryButtons[i];
			if (categoryButton != NULL && categoryButton->getSection() != NULL)
			{
				categoryButton->setActiveCategory(false);
				categoryButton->setTextColor(0xff737373);

				if (categoryButton->getSection()->getPos().y < m_options->getSize().y*0.4)
					activeCategoryButton = categoryButton;
			}
		}
		if (activeCategoryButton != NULL)
		{
			activeCategoryButton->setActiveCategory(true);
			activeCategoryButton->setTextColor(0xffffffff);
		}
	}

	// delayed update letterboxing mouse scale/offset settings
	if (m_bLetterboxingOffsetUpdateScheduled)
	{
		if (!m_letterboxingOffsetXSlider->isActive() && !m_letterboxingOffsetYSlider->isActive())
		{
			m_bLetterboxingOffsetUpdateScheduled = false;

			cv::osu::letterboxing_offset_x.setValue(m_letterboxingOffsetXSlider->getFloat());
			cv::osu::letterboxing_offset_y.setValue(m_letterboxingOffsetYSlider->getFloat());

			// and update reset buttons as usual
			onResetUpdate(m_letterboxingOffsetResetButton);
		}
	}

	if (m_bUIScaleScrollToSliderScheduled)
	{
		m_bUIScaleScrollToSliderScheduled = false;
		m_options->scrollToElement(m_uiScaleSlider, 0, 200 * Osu::getUIScale());
	}

	// delayed UI scale change
	if (m_bUIScaleChangeScheduled)
	{
		if (!m_uiScaleSlider->isActive())
		{
			m_bUIScaleChangeScheduled = false;

			const float oldUIScale = Osu::getUIScale();

			cv::osu::ui_scale.setValue(m_uiScaleSlider->getFloat());

			const float newUIScale = Osu::getUIScale();

			// and update reset buttons as usual
			onResetUpdate(m_uiScaleResetButton);

			// additionally compensate scroll pos (but delay 1 frame)
			if (oldUIScale != newUIScale)
				m_bUIScaleScrollToSliderScheduled = true;
		}
	}

	// delayed WASAPI buffer/period change
	if (m_bWASAPIBufferChangeScheduled)
	{
		if (!m_wasapiBufferSizeSlider->isActive())
		{
			m_bWASAPIBufferChangeScheduled = false;

			cv::win_snd_wasapi_buffer_size.setValue(m_wasapiBufferSizeSlider->getFloat());

			// and update reset buttons as usual
			onResetUpdate(m_wasapiBufferSizeResetButton);
		}
	}
	if (m_bWASAPIPeriodChangeScheduled)
	{
		if (!m_wasapiPeriodSizeSlider->isActive())
		{
			m_bWASAPIPeriodChangeScheduled = false;

			cv::win_snd_wasapi_period_size.setValue(m_wasapiPeriodSizeSlider->getFloat());

			// and update reset buttons as usual
			onResetUpdate(m_wasapiPeriodSizeResetButton);
		}
	}

	// apply textbox changes on enter key
	if (m_osuFolderTextbox->hitEnter())
		updateOsuFolder();
	if (m_nameTextbox->hitEnter())
		updateName();
	if (m_dpiTextbox != NULL && m_dpiTextbox->hitEnter())
		updateFposuDPI();
	if (m_cm360Textbox != NULL && m_cm360Textbox->hitEnter())
		updateFposuCMper360();
}

void OsuOptionsMenu::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onKeyDown(e);
	if (e.isConsumed()) return;

	if (e == KEY_ESCAPE || e == cv::osu::keybinds::GAME_PAUSE.getVal<KEYCODE>())
	{
		if (m_contextMenu->isVisible())
		{
			e.consume();

			m_contextMenu->setVisible2(false);
			return;
		}
	}

	// searching text delete & escape key handling
	if (m_sSearchString.length() > 0)
	{
		switch (e.getKeyCode())
		{
		case KEY_DELETE:
		case KEY_BACKSPACE:
			if (m_sSearchString.length() > 0)
			{
				if (keyboard->isControlDown())
				{
					// delete everything from the current caret position to the left, until after the first non-space character (but including it)
					bool foundNonSpaceChar = false;
					while (m_sSearchString.length() > 0)
					{
						UString curChar = m_sSearchString.substr(m_sSearchString.length()-1, 1);

						if (foundNonSpaceChar && curChar.isWhitespaceOnly())
							break;

						if (!curChar.isWhitespaceOnly())
							foundNonSpaceChar = true;

						m_sSearchString.erase(m_sSearchString.length()-1, 1);
					}
				}
				else
					m_sSearchString = m_sSearchString.substr(0, m_sSearchString.length()-1);
				scheduleSearchUpdate();
			}
			break;
		case KEY_ESCAPE:
			m_sSearchString = "";
			scheduleSearchUpdate();
			break;
		}
	}
	else
	{
		if (e == KEY_ESCAPE || e == cv::osu::keybinds::GAME_PAUSE.getVal<KEYCODE>())
			onBack();
	}

	// paste clipboard support
	if (e == KEY_V)
	{
		if (keyboard->isControlDown())
		{
			const UString clipstring = env->getClipBoardText();
			if (clipstring.length() > 0)
			{
				m_sSearchString.append(clipstring);
				scheduleSearchUpdate();
			}
		}
	}

	e.consume();
}

void OsuOptionsMenu::onKeyUp(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onKeyUp(e);
	if (e.isConsumed()) return;
}

void OsuOptionsMenu::onChar(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onChar(e);
	if (e.isConsumed()) return;

	// handle searching
	if (e.getCharCode() < 32 || !m_bVisible || (keyboard->isControlDown() && !keyboard->isAltDown()) || m_fSearchOnCharKeybindHackTime > engine->getTime())
		return;

	KEYCODE charCode = e.getCharCode();
	UString stringChar = "";
	stringChar.insert(0, charCode);
	m_sSearchString.append(stringChar);

	scheduleSearchUpdate();

	e.consume();
}

void OsuOptionsMenu::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);

	// HACKHACK: magic
	if ((Env::cfg(OS::WINDOWS) && env->isFullscreen() && env->isFullscreenWindowedBorderless() && (int)newResolution.y == (int)env->getNativeScreenSize().y+1))
		newResolution.y--;

	if (m_resolutionLabel != NULL)
		m_resolutionLabel->setText(UString::format("%ix%i", (int)newResolution.x, (int)newResolution.y));
}

void OsuOptionsMenu::onKey(KeyboardEvent &e)
{
	// from OsuNotificationOverlay
	if (m_waitingKey != NULL)
	{
		m_waitingKey->setValue((float)e.getKeyCode());
		m_waitingKey = NULL;
	}
	else // mania layout logic
	{
		if (e.getKeyCode() != (KEYCODE)0) // if not the first call
		{
			if (m_iManiaK > -1 && m_iManiaK < 10 && m_iManiaKey > -1 && m_iManiaKey <= m_iManiaK)
				(osu->getBindings()->getMania()->begin())[m_iManiaK][m_iManiaKey]->setValue(e.getKeyCode());

			// go to next key
			m_iManiaKey++;
			if (m_iManiaKey > m_iManiaK)
			{
				m_iManiaKey = 0;
				m_iManiaK++;
			}
		}

		if (m_iManiaK > -1 && m_iManiaK < 10)
		{
			UString notificationText = UString::format("%ik:", (m_iManiaK+1));
			int curKey = 0;
			for (int i=m_iManiaK-m_iManiaKey; i<m_iManiaK; i++)
			{
				notificationText.append(" [x]");
				curKey++;
			}
			for (int i=0; i<=m_iManiaK-m_iManiaKey; i++)
			{
				if (curKey == m_iManiaKey)
					notificationText.append(" ([?])");
				else
					notificationText.append(" []");

				curKey++;
			}
			osu->getNotificationOverlay()->addNotification(notificationText, 0xffffffff, true);
		}
	}
}

void OsuOptionsMenu::setVisible(bool visible)
{
	setVisibleInt(visible, false);
}

void OsuOptionsMenu::setVisibleInt(bool visible, bool fromOnBack)
{
	if (visible != m_bVisible)
	{
		// open/close animation
		if (!m_bFullscreen)
		{
			if (!m_bVisible)
				anim->moveQuartOut(&m_fAnimation, 1.0f, 0.25f*(1.0f - m_fAnimation), true);
			else
				anim->moveQuadOut(&m_fAnimation, 0.0f, 0.25f*m_fAnimation, true);
		}

		// save even if not closed via onBack(), e.g. if closed via setVisible(false) from outside
		if (!visible && !fromOnBack)
		{
			osu->getNotificationOverlay()->stopWaitingForKey();
			save();
		}
	}

	m_bVisible = visible;

	if (visible)
		updateLayout();
	else
	{
		m_contextMenu->setVisible2(false);

		//anim->deleteExistingAnimation(&m_fAnimation);
		//m_fAnimation = 0.0f;
	}

	// usability: auto scroll to fposu settings if opening options while in fposu gamemode
	if (visible && osu->isInPlayMode() && cv::osu::fposu::mod_fposu.getBool() && !m_fposuCategoryButton->isActiveCategory())
		onCategoryClicked(m_fposuCategoryButton);

	// reset reset counters
	if (visible)
	{
		m_iNumResetAllKeyBindingsPressed = 0;
		m_iNumResetEverythingPressed = 0;
	}
}

void OsuOptionsMenu::setUsername(UString username)
{
	m_nameTextbox->setText(std::move(username));
}

bool OsuOptionsMenu::isMouseInside()
{
	return (m_backButton->isMouseInside() || m_options->isMouseInside() || m_categories->isMouseInside()) && isVisible();
}

bool OsuOptionsMenu::isBusy()
{
	return (m_backButton->isActive() || m_options->isBusy() || m_categories->isBusy()) && isVisible();
}

void OsuOptionsMenu::updateLayout()
{
	// set all elements to the current convar values, and update the reset button states
	for (int i=0; i<m_elements.size(); i++)
	{
		switch (m_elements[i].type)
		{
		case 6: // checkbox
			if (m_elements[i].cvar != NULL)
			{
				for (int e=0; e<m_elements[i].elements.size(); e++)
				{
					auto *checkboxPointer = m_elements[i].elements[e]->as<CBaseUICheckbox>();
					if (checkboxPointer != NULL)
						checkboxPointer->setChecked(m_elements[i].cvar->getBool());
				}
			}
			break;
		case 7: // slider
			if (m_elements[i].cvar != NULL)
			{
				if (m_elements[i].elements.size() == 3)
				{
					auto *sliderPointer = m_elements[i].elements[1]->as<CBaseUISlider>();
					if (sliderPointer != NULL)
					{
						// allow users to overscale certain values via the console
						if (m_elements[i].allowOverscale && m_elements[i].cvar->getFloat() > sliderPointer->getMax())
							sliderPointer->setBounds(sliderPointer->getMin(), m_elements[i].cvar->getFloat());

						// allow users to underscale certain values via the console
						if (m_elements[i].allowUnderscale && m_elements[i].cvar->getFloat() < sliderPointer->getMin())
							sliderPointer->setBounds(m_elements[i].cvar->getFloat(), sliderPointer->getMax());

						sliderPointer->setValue(m_elements[i].cvar->getFloat(), false);
						sliderPointer->fireChangeCallback();
					}
				}
			}
			break;
		case 8: // textbox
			if (m_elements[i].cvar != NULL)
			{
				if (m_elements[i].elements.size() == 1)
				{
					auto *textboxPointer = m_elements[i].elements[0]->as<CBaseUITextbox>();
					if (textboxPointer != NULL)
						textboxPointer->setText(m_elements[i].cvar->getString());
				}
				else if (m_elements[i].elements.size() == 2)
				{
					auto *textboxPointer = m_elements[i].elements[1]->as<CBaseUITextbox>();
					if (textboxPointer != NULL)
						textboxPointer->setText(m_elements[i].cvar->getString());
				}
			}
			break;
		}

		onResetUpdate(m_elements[i].resetButton);
	}

	if (m_fullscreenCheckbox != NULL)
		m_fullscreenCheckbox->setChecked(env->isFullscreen(), false);

	updateSkinNameLabel();
	updateNotelockSelectLabel();
	updateHPDrainSelectLabel();

	if (m_outputDeviceLabel != NULL)
		m_outputDeviceLabel->setText(soundEngine->getOutputDevice());

	onOutputDeviceResetUpdate();
	onNotelockSelectResetUpdate();
	onHPDrainSelectResetUpdate();

	//************************************************************************************************************************************//

	// TODO: correctly scale individual UI elements to dpiScale (depend on initial value in e.g. addCheckbox())

	OsuScreenBackable::updateLayout();

	const float dpiScale = Osu::getUIScale();

	m_container->setSize(osu->getVirtScreenSize());

	// options panel
	const float optionsScreenWidthPercent = 0.5f;
	const float categoriesOptionsPercent = 0.135f;

	int optionsWidth = (int)(osu->getVirtScreenWidth()*optionsScreenWidthPercent);
	if (!m_bFullscreen)
		optionsWidth = std::min((int)(725.0f*(1.0f - categoriesOptionsPercent)), optionsWidth) * dpiScale;

	const int categoriesWidth = optionsWidth*categoriesOptionsPercent;

	m_options->setRelPosX((!m_bFullscreen ? -1 : osu->getVirtScreenWidth()/2 - (optionsWidth + categoriesWidth)/2) + categoriesWidth);
	m_options->setSize(optionsWidth, osu->getVirtScreenHeight()+1);

	m_search->setRelPos(m_options->getRelPos());
	m_search->setSize(m_options->getSize());

	m_categories->setRelPosX(m_options->getRelPos().x - categoriesWidth);
	m_categories->setSize(categoriesWidth, osu->getVirtScreenHeight() + 1);

	// reset
	m_options->getContainer()->empty();

	// build layout
	bool enableHorizontalScrolling = false;
	int sideMargin = 25*2 * dpiScale;
	int spaceSpacing = 25 * dpiScale;
	int sectionSpacing = -15 * dpiScale; // section title to first element
	int subsectionSpacing = 15 * dpiScale; // subsection title to first element
	int sectionEndSpacing = /*70*/120 * dpiScale; // last section element to next section title
	int subsectionEndSpacing = 65 * dpiScale; // last subsection element to next subsection title
	int elementSpacing = 5 * dpiScale;
	int elementTextStartOffset = 11 * dpiScale; // e.g. labels in front of sliders
	int yCounter = sideMargin + 20 * dpiScale;
	bool inSkipSection = false;
	bool inSkipSubSection = false;
	bool sectionTitleMatch = false;
	bool subSectionTitleMatch = false;
	const std::string search = m_sSearchString.length() > 0 ? m_sSearchString.toUtf8() : "";
	for (int i=0; i<m_elements.size(); i++)
	{
		// searching logic happens here:
		// section
		//     content
		//     subsection
		//           content

		// case 1: match in content -> section + subsection + content     -> section + subsection matcher
		// case 2: match in content -> section + content                  -> section matcher
		// if match in section or subsection -> display entire section (disregard content match)
		// matcher is run through all remaining elements at every section + subsection

		if (m_sSearchString.length() > 0)
		{
			const std::string searchTags = m_elements[i].searchTags.toUtf8();

			// if this is a section
			if (m_elements[i].type == 1)
			{
				bool sectionMatch = false;

				const std::string sectionTitle = m_elements[i].elements[0]->getName().toUtf8();
				sectionTitleMatch = Osu::findIgnoreCase(sectionTitle, search);

				subSectionTitleMatch = false;
				if (inSkipSection)
					inSkipSection = false;

				for (int s=i+1; s<m_elements.size(); s++)
				{
					if (m_elements[s].type == 1) // stop at next section
						break;

					for (int e=0; e<m_elements[s].elements.size(); e++)
					{
						if (m_elements[s].elements[e]->getName().length() > 0)
						{
							std::string tags = m_elements[s].elements[e]->getName().toUtf8();

							if (Osu::findIgnoreCase(tags, search))
							{
								sectionMatch = true;
								break;
							}
						}
					}
				}

				inSkipSection = !sectionMatch;
				if (!inSkipSection)
					inSkipSubSection = false;
			}

			// if this is a subsection
			if (m_elements[i].type == 2)
			{
				bool subSectionMatch = false;

				const std::string subSectionTitle = m_elements[i].elements[0]->getName().toUtf8();
				subSectionTitleMatch = Osu::findIgnoreCase(subSectionTitle, search) || Osu::findIgnoreCase(searchTags, search);

				if (inSkipSubSection)
					inSkipSubSection = false;

				for (int s=i+1; s<m_elements.size(); s++)
				{
					if (m_elements[s].type == 2) // stop at next subsection
						break;

					for (int e=0; e<m_elements[s].elements.size(); e++)
					{
						if (m_elements[s].elements[e]->getName().length() > 0)
						{
							std::string tags = m_elements[s].elements[e]->getName().toUtf8();

							if (Osu::findIgnoreCase(tags, search))
							{
								subSectionMatch = true;
								break;
							}
						}
					}
				}

				inSkipSubSection = !subSectionMatch;
			}

			bool inSkipContent = false;
			if (!inSkipSection && !inSkipSubSection)
			{
				bool contentMatch = false;

				if (m_elements[i].type > 2)
				{
					for (int e=0; e<m_elements[i].elements.size(); e++)
					{
						if (m_elements[i].elements[e]->getName().length() > 0)
						{
							std::string tags = m_elements[i].elements[e]->getName().toUtf8();

							if (Osu::findIgnoreCase(tags, search))
							{
								contentMatch = true;
								break;
							}
						}
					}

					// if section or subsection titles match, then include all content of that (sub)section (even if content doesn't match)
					inSkipContent = !contentMatch;
				}
			}

			if ((inSkipSection || inSkipSubSection || inSkipContent) && !sectionTitleMatch && !subSectionTitleMatch)
				continue;
		}

		// add all elements of the current entry
		{
			// reset button
			if (m_elements[i].resetButton != NULL)
				m_options->getContainer()->addBaseUIElement(m_elements[i].resetButton);

			// (sub-)elements
			for (int e=0; e<m_elements[i].elements.size(); e++)
			{
				m_options->getContainer()->addBaseUIElement(m_elements[i].elements[e]);
			}
		}

		// and build the layout

		// if this element is a new section, add even more spacing
		if (i > 0 && m_elements[i].type == 1)
			yCounter += sectionEndSpacing;

		// if this element is a new subsection, add even more spacing
		if (i > 0 && m_elements[i].type == 2)
			yCounter += subsectionEndSpacing;

		const int elementWidth = optionsWidth - 2*sideMargin - 2 * dpiScale;
		const bool isKeyBindButton = (m_elements[i].type == 5);

		if (m_elements[i].resetButton != NULL)
		{
			CBaseUIButton *resetButton = m_elements[i].resetButton;
			resetButton->setSize(Vector2(35, 50) * dpiScale);
			resetButton->setRelPosY(yCounter);
			resetButton->setRelPosX(0);
		}

		for (int j=0; j<m_elements[i].elements.size(); j++)
		{
			CBaseUIElement *e = m_elements[i].elements[j];
			e->setSizeY(e->getRelSize().y * dpiScale);
		}

		if (m_elements[i].elements.size() == 1)
		{
			CBaseUIElement *e = m_elements[i].elements[0];

			int sideMarginAdd = 0;
			CBaseUILabel *labelPointer = NULL;
			if ((labelPointer = e->as<CBaseUILabel>()))
			{
				labelPointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics
				labelPointer->setSizeToContent(0, 0);
				sideMarginAdd += elementTextStartOffset;
			}

			CBaseUIButton *buttonPointer = NULL;
			if ((buttonPointer = e->as<CBaseUIButton>()))
				buttonPointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics

			CBaseUICheckbox *checkboxPointer = NULL;
			if ((checkboxPointer = e->as<CBaseUICheckbox>()))
			{
				checkboxPointer->onResized(); // HACKHACK: framework, setWidth*() does not update string metrics
				checkboxPointer->setWidthToContent(0);
				if (checkboxPointer->getSize().x > elementWidth)
					enableHorizontalScrolling = true;
				else
					e->setSizeX(elementWidth);
			}
			else
				e->setSizeX(elementWidth);

			e->setRelPosX(sideMargin + sideMarginAdd);
			e->setRelPosY(yCounter);

			yCounter += e->getSize().y;
		}
		else if (m_elements[i].elements.size() == 2 || isKeyBindButton)
		{
			CBaseUIElement *e1 = m_elements[i].elements[0];
			CBaseUIElement *e2 = m_elements[i].elements[1];

			int spacing = 15 * dpiScale;

			int sideMarginAdd = 0;
			CBaseUILabel *labelPointer = NULL;
			if ((labelPointer = e1->as<CBaseUILabel>()))
				sideMarginAdd += elementTextStartOffset;

			CBaseUIButton *buttonPointer = NULL;
			if ((buttonPointer = e1->as<CBaseUIButton>()))
				buttonPointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics

			CBaseUIButton *buttonPointer2 = NULL;
			if (buttonPointer && (buttonPointer2 = e2->as<CBaseUIButton>()))
				spacing *= 0.35f;

			if (isKeyBindButton)
			{
				CBaseUIElement *e3 = m_elements[i].elements[2];

				const float dividerMiddle = 5.0f / 8.0f;
				const float dividerEnd = 2.0f / 8.0f;

				e1->setRelPos(sideMargin, yCounter);
				e1->setSizeX(e1->getSize().y);

				e2->setRelPos(sideMargin + e1->getSize().x + 0.5f*spacing, yCounter);
				e2->setSizeX(elementWidth*dividerMiddle - spacing);

				e3->setRelPos(sideMargin + e1->getSize().x + e2->getSize().x + 1.5f*spacing, yCounter);
				e3->setSizeX(elementWidth*dividerEnd - spacing);
			}
			else
			{
				float dividerEnd = 1.0f / 2.0f;
				float dividerBegin = 1.0f - dividerEnd;

				e1->setRelPos(sideMargin + sideMarginAdd, yCounter);
				e1->setSizeX(elementWidth*dividerBegin - spacing);

				e2->setRelPos(sideMargin + e1->getSize().x + 2*spacing, yCounter);
				e2->setSizeX(elementWidth*dividerEnd - spacing);
			}

			yCounter += e1->getSize().y;
		}
		else if (m_elements[i].elements.size() == 3)
		{
			CBaseUIElement *e1 = m_elements[i].elements[0];
			CBaseUIElement *e2 = m_elements[i].elements[1];
			CBaseUIElement *e3 = m_elements[i].elements[2];

			if (m_elements[i].type == 4)
			{
				const int buttonButtonLabelOffset = 10 * dpiScale;

				const int buttonSize = elementWidth / 3 - 2*buttonButtonLabelOffset;

				CBaseUIButton *button1Pointer = NULL;
				if ((button1Pointer = e1->as<CBaseUIButton>()))
					button1Pointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics

				CBaseUIButton *button2Pointer = NULL;
				if ((button2Pointer = e2->as<CBaseUIButton>()))
					button2Pointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics

				e1->setSizeX(buttonSize);
				e2->setSizeX(buttonSize);
				e3->setSizeX(buttonSize);

				e1->setRelPos(sideMargin, yCounter);
				e2->setRelPos(e1->getRelPos().x + e1->getSize().x + buttonButtonLabelOffset, yCounter);
				e3->setRelPos(e2->getRelPos().x + e2->getSize().x + buttonButtonLabelOffset, yCounter);
			}
			else
			{
				const int labelSliderLabelOffset = 15 * dpiScale;

				// this is a big mess, because some elements rely on fixed initial widths from default strings, combined with variable font dpi on startup, will clean up whenever
				CBaseUILabel *label1Pointer = NULL;
				if ((label1Pointer = e1->as<CBaseUILabel>()))
				{
					label1Pointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics
					if (m_elements[i].label1Width > 0.0f)
						label1Pointer->setSizeX(m_elements[i].label1Width * dpiScale);
					else
						label1Pointer->setSizeX(label1Pointer->getRelSize().x * (96.0f / m_elements[i].relSizeDPI) * dpiScale);
				}

				CBaseUISlider *sliderPointer = NULL;
				if ((sliderPointer = e2->as<CBaseUISlider>()))
					sliderPointer->setBlockSize(20 * dpiScale, 20 * dpiScale);

				CBaseUILabel *label2Pointer = NULL;
				if ((label2Pointer = e3->as<CBaseUILabel>()))
				{
					label2Pointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics
					label2Pointer->setSizeX(label2Pointer->getRelSize().x * (96.0f / m_elements[i].relSizeDPI) * dpiScale);
				}

				int sliderSize = elementWidth - e1->getSize().x - e3->getSize().x;
				if (sliderSize < 100)
				{
					enableHorizontalScrolling = true;
					sliderSize = 100;
				}

				e1->setRelPos(sideMargin + elementTextStartOffset, yCounter);

				e2->setRelPos(e1->getRelPos().x + e1->getSize().x + labelSliderLabelOffset, yCounter);
				e2->setSizeX(sliderSize - 2*elementTextStartOffset - labelSliderLabelOffset*2);

				e3->setRelPos(e2->getRelPos().x + e2->getSize().x + labelSliderLabelOffset, yCounter);
			}

			yCounter += e2->getSize().y;
		}

		yCounter += elementSpacing;

		switch (m_elements[i].type)
		{
		case 0:
			yCounter += spaceSpacing;
			break;
		case 1:
			yCounter += sectionSpacing;
			break;
		case 2:
			yCounter += subsectionSpacing;
			break;
		default:
			break;
		}
	}
	m_spacer->setPosY(yCounter);
	m_options->getContainer()->addBaseUIElement(m_spacer);

	m_options->setScrollSizeToContent();
	if (!enableHorizontalScrolling)
		m_options->scrollToLeft();
	m_options->setHorizontalScrolling(enableHorizontalScrolling);

	m_options->getContainer()->addBaseUIElement(m_contextMenu);

	m_options->getContainer()->update_pos();

	const int categoryPaddingTopBottom = m_categories->getSize().y*0.15f;
	const int categoryHeight = (m_categories->getSize().y - categoryPaddingTopBottom*2) / m_categoryButtons.size();
	for (int i=0; i<m_categoryButtons.size(); i++)
	{
		OsuOptionsMenuCategoryButton *category = m_categoryButtons[i];
		category->onResized(); // HACKHACK: framework, setSize*() does not update string metrics
		category->setRelPosY(categoryPaddingTopBottom + categoryHeight*i);
		category->setSize(m_categories->getSize().x-1, categoryHeight);
	}
	m_categories->getContainer()->update_pos();
	m_categories->setScrollSizeToContent();

	m_container->update_pos();
}

void OsuOptionsMenu::onBack()
{
	osu->getNotificationOverlay()->stopWaitingForKey();

	soundEngine->play(osu->getSkin()->getMenuClick());
	save();

	if (m_bFullscreen)
		osu->toggleOptionsMenu();
	else
		setVisibleInt(false, true);
}

void OsuOptionsMenu::scheduleSearchUpdate()
{
	updateLayout();
	m_options->scrollToTop();
	m_search->setSearchString(m_sSearchString);

	if (m_contextMenu->isVisible())
		m_contextMenu->setVisible2(false);
}

void OsuOptionsMenu::updateOsuFolder()
{
	m_osuFolderTextbox->stealFocus();

	// automatically insert a slash at the end if the user forgets
	UString newOsuFolder = m_osuFolderTextbox->getText();
	newOsuFolder = newOsuFolder.trim();
	if (newOsuFolder.length() > 0)
	{
		if (newOsuFolder[newOsuFolder.length()-1] != L'/' && newOsuFolder[newOsuFolder.length()-1] != L'\\')
		{
			newOsuFolder.append("/");
			m_osuFolderTextbox->setText(newOsuFolder);
		}
	}

	// set convar to new folder
	cv::osu::folder.setValue(newOsuFolder);
}

void OsuOptionsMenu::updateName()
{
	m_nameTextbox->stealFocus();
	cv::name.setValue(m_nameTextbox->getText());
}

void OsuOptionsMenu::updateFposuDPI()
{
	if (m_dpiTextbox == NULL) return;

	m_dpiTextbox->stealFocus();

	const UString &text = m_dpiTextbox->getText();
	UString value;
	for (int i=0; i<text.length(); i++)
	{
		if (text[i] == L',')
			value.append(L'.');
		else
			value.append(text[i]);
	}
	cv::osu::fposu::mouse_dpi.setValue(value);
}

void OsuOptionsMenu::updateFposuCMper360()
{
	if (m_cm360Textbox == NULL) return;

	m_cm360Textbox->stealFocus();

	const UString &text = m_cm360Textbox->getText();
	UString value;
	for (int i=0; i<text.length(); i++)
	{
		if (text[i] == L',')
			value.append(L'.');
		else
			value.append(text[i]);
	}
	cv::osu::fposu::mouse_cm_360.setValue(value);
}

void OsuOptionsMenu::updateSkinNameLabel()
{
	if (m_skinLabel == NULL) return;
	if constexpr (Env::cfg(FEAT::STEAM))
	{
		m_skinLabel->setText(cv::osu::skin_is_from_workshop.getBool() ? cv::osu::skin_workshop_title.getString() : cv::osu::skin.getString());
		m_skinLabel->setTextColor(cv::osu::skin_is_from_workshop.getBool() ? 0xff37adff : 0xffffffff);
	}
	else
	{
		m_skinLabel->setText(cv::osu::skin.getString());
		m_skinLabel->setTextColor(0xffffffff);
	}
}

void OsuOptionsMenu::updateNotelockSelectLabel()
{
	if (m_notelockSelectLabel == NULL) return;

	m_notelockSelectLabel->setText(m_notelockTypes[std::clamp<int>(cv::osu::notelock_type.getInt(), 0, m_notelockTypes.size() - 1)]);
}

void OsuOptionsMenu::updateHPDrainSelectLabel()
{
	if (m_hpDrainSelectLabel == NULL) return;

	m_hpDrainSelectLabel->setText(m_drainTypes[std::clamp<int>(cv::osu::drain_type.getInt(), 0, m_drainTypes.size() - 1)]);
}

void OsuOptionsMenu::onFullscreenChange(CBaseUICheckbox *checkbox)
{
	if (checkbox->isChecked())
		env->enableFullscreen();
	else
		env->disableFullscreen();

	// and update reset button as usual
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onBorderlessWindowedChange(CBaseUICheckbox *checkbox)
{
	onCheckboxChange(checkbox);

	if (checkbox->isChecked() && !env->isFullscreen())
		env->enableFullscreen();

	// and update reset button as usual
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onDPIScalingChange(CBaseUICheckbox *checkbox)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				const float prevUIScale = Osu::getUIScale();

				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(checkbox->isChecked());

				onResetUpdate(m_elements[i].resetButton);

				if (Osu::getUIScale() != prevUIScale)
					m_bDPIScalingScrollToSliderScheduled = true;

				break;
			}
		}
	}
}

void OsuOptionsMenu::onRawInputToAbsoluteWindowChange(CBaseUICheckbox *checkbox)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(checkbox->isChecked());

				onResetUpdate(m_elements[i].resetButton);

				// special case: this requires a virtual mouse offset update, but since it is an engine convar we can't use callbacks
				osu->updateMouseSettings();

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSkinSelect()
{
	updateOsuFolder();

	if (osu->isSkinLoading()) return;


	UString skinFolder = cv::osu::folder.getString();
	skinFolder.append(cv::osu::folder_sub_skins.getString());
	std::vector<UString> skinFolders = env->getFoldersInFolder(skinFolder);




	/*
	std::vector<UString> skinFolders;
	std::vector<uint64_t> subscribedItems = steam->getWorkshopSubscribedItems();
	debugLog("subscribed to {} items\n", subscribedItems.size());
	for (int i=0; i<subscribedItems.size(); i++)
	{
		SteamworksInterface::WorkshopItemDetails details = steam->getWorkshopItemDetails(subscribedItems[i]);
		debugLog("item[{}] = {}, installInfo = {:s}, title = {:s}, desc = {:s}\n", i, subscribedItems[i], steam->getWorkshopItemInstallInfo(subscribedItems[i]).toUtf8(), details.title.toUtf8(), details.description.toUtf8());
		skinFolders.push_back(details.title);
	}
	*/



	if (skinFolders.size() > 0)
	{
		m_contextMenu->setPos(m_skinSelectLocalButton->getPos());
		m_contextMenu->setRelPos(m_skinSelectLocalButton->getRelPos());
		m_contextMenu->begin();

		const UString defaultText = "default";
		CBaseUIButton *buttonDefault = m_contextMenu->addButton(defaultText);
		if (defaultText == cv::osu::skin.getString())
			buttonDefault->setTextBrightColor(0xff00ff00);

		for (int i=0; i<skinFolders.size(); i++)
		{
			if (skinFolders[i] == "." || skinFolders[i] == "..") // is this universal in every file system? too lazy to check. should probably fix this in the engine and not here
				continue;

			CBaseUIButton *button = m_contextMenu->addButton(skinFolders[i]);
			if (skinFolders[i] == cv::osu::skin.getString())
				button->setTextBrightColor(0xff00ff00);
		}
		m_contextMenu->end(false, false);
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelect2) );
	}
	else
	{
		osu->getNotificationOverlay()->addNotification("Error: Couldn't find any skins", 0xffff0000);
		m_options->scrollToTop();
		m_fOsuFolderTextboxInvalidAnim = engine->getTime() + 3.0f;
	}
}

void OsuOptionsMenu::onSkinSelect2(const UString& skinName, int  /*id*/)
{
	if constexpr (Env::cfg(FEAT::STEAM))
		cv::osu::skin_is_from_workshop.setValue(0.0f);

	cv::osu::skin.setValue(skinName);

	updateSkinNameLabel();
}

void OsuOptionsMenu::onSkinSelectWorkshop()
{
	if constexpr (Env::cfg(FEAT::STEAM))
	{
		if (m_skinSelectWorkshopButton == NULL) return;

		if (osu->getSteamWorkshop()->isReady() && osu->getSteamWorkshop()->areDetailsLoaded())
		{
			onSkinSelectWorkshop3();
			return;
		}

		onSkinSelectWorkshop2();
	}
}

void OsuOptionsMenu::onSkinSelectWorkshop2()
{
	if constexpr (Env::cfg(FEAT::STEAM))
	{
		osu->getSteamWorkshop()->refresh(true);
		m_bWorkshopSkinSelectScheduled = true;

		m_contextMenu->setPos(m_skinSelectWorkshopButton->getPos());
		m_contextMenu->setRelPos(m_skinSelectWorkshopButton->getRelPos());
		m_contextMenu->begin();
		{
			m_contextMenu->addButton("(Loading, please wait ...)", -4)->setEnabled(false);
		}
		m_contextMenu->end(false, false);
	}
}

void OsuOptionsMenu::onSkinSelectWorkshop3()
{
	if constexpr (Env::cfg(FEAT::STEAM))
	{
		m_contextMenu->setPos(m_skinSelectWorkshopButton->getPos());
		m_contextMenu->setRelPos(m_skinSelectWorkshopButton->getRelPos());
		m_contextMenu->begin();
		{
			m_contextMenu->addButton(">>> Refresh <<<", -2)->setTextLeft(false);
			m_contextMenu->addButton("", -4)->setEnabled(false);

			const std::vector<OsuSteamWorkshop::SUBSCRIBED_ITEM> &subscribedItems = osu->getSteamWorkshop()->getSubscribedItems();
			if (subscribedItems.size() > 0)
			{
				for (int i=0; i<subscribedItems.size(); i++)
				{
					if (subscribedItems[i].status != OsuSteamWorkshop::SUBSCRIBED_ITEM_STATUS::INSTALLED)
						m_contextMenu->addButton("(Downloading ...)", -4)->setEnabled(false)->setTextColor(0xff888888)->setTextDarkColor(0xff000000);
					else
						m_contextMenu->addButton(subscribedItems[i].title, i);
				}
			}
			else
				m_contextMenu->addButton("(Empty. Click for Workshop!)", -3)->setTextLeft(false);
		}
		m_contextMenu->end(false, false);
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelectWorkshop4) );
	}
}

void OsuOptionsMenu::onSkinSelectWorkshop4(const UString&  /*skinName*/, int id)
{
	if constexpr (Env::cfg(FEAT::STEAM))
	{
		if (id == -2) // ">>> Refresh <<<"
		{
			onSkinSelectWorkshop2();
			return;
		}
		else if (id == -3) // "(Empty. Click for Workshop!)"
		{
			OsuMainMenu::openSteamWorkshopInGameOverlay(true);
			return;
		}

		const std::vector<OsuSteamWorkshop::SUBSCRIBED_ITEM> &subscribedItems = osu->getSteamWorkshop()->getSubscribedItems();
		if (id >= 0 && id < subscribedItems.size())
		{
			const OsuSteamWorkshop::SUBSCRIBED_ITEM &subscribedItem = subscribedItems[id];
			debugLog("installInfo = {:s}\n", subscribedItem.installInfo.toUtf8());
			if (env->directoryExists(subscribedItem.installInfo))
			{
				cv::osu::skin_is_from_workshop.setValue(1.0f);
				cv::osu::skin_workshop_title.setValue(subscribedItem.title);
				cv::osu::skin_workshop_id.setValue(UString::format("%llu", subscribedItem.id));

				cv::osu::skin.setValue(subscribedItem.installInfo);

				updateSkinNameLabel();
			}
			else
				osu->getNotificationOverlay()->addNotification("Error: Workshop skin does not exist!", 0xffff0000);
		}
	}
}

void OsuOptionsMenu::openCurrentSkinFolder()
{
	UString skinFolderToOpen = // yikes
	    osu->getSkin()->isDefaultSkin()
	        ? OsuSkin::DEFAULT_SKIN_PATH
	        : osu->getSkin()->getFilePath();
	env->openFileBrowser("skin folder", skinFolderToOpen);
}

void OsuOptionsMenu::onSkinReload()
{
	osu->reloadSkin();
}

void OsuOptionsMenu::onSkinRandom()
{
	const bool isRandomSkinEnabled = cv::osu::skin_random.getBool();

	if (!isRandomSkinEnabled)
		cv::osu::skin_random.setValue(1.0f);

	osu->reloadSkin();

	if (!isRandomSkinEnabled)
		cv::osu::skin_random.setValue(0.0f);
}

void OsuOptionsMenu::onResolutionSelect()
{
	std::vector<Vector2> resolutions;

	// 4:3
	resolutions.emplace_back(800, 600);
	resolutions.emplace_back(1024, 768);
	resolutions.emplace_back(1152, 864);
	resolutions.emplace_back(1280, 960);
	resolutions.emplace_back(1280, 1024);
	resolutions.emplace_back(1600, 1200);
	resolutions.emplace_back(1920, 1440);
	resolutions.emplace_back(2560, 1920);

	// 16:9 and 16:10
	resolutions.emplace_back(1024, 600);
	resolutions.emplace_back(1280, 720);
	resolutions.emplace_back(1280, 768);
	resolutions.emplace_back(1280, 800);
	resolutions.emplace_back(1360, 768);
	resolutions.emplace_back(1366, 768);
	resolutions.emplace_back(1440, 900);
	resolutions.emplace_back(1600, 900);
	resolutions.emplace_back(1600, 1024);
	resolutions.emplace_back(1680, 1050);
	resolutions.emplace_back(1920, 1080);
	resolutions.emplace_back(1920, 1200);
	resolutions.emplace_back(2560, 1440);
	resolutions.emplace_back(2560, 1600);
	resolutions.emplace_back(3840, 2160);
	resolutions.emplace_back(5120, 2880);
	resolutions.emplace_back(7680, 4320);

	// wtf
	resolutions.emplace_back(4096, 2160);

	// get custom resolutions
	std::vector<Vector2> customResolutions;
	std::ifstream customres("cfg/customres.cfg");
	std::string curLine;
	while (std::getline(customres, curLine))
	{
		const char *curLineChar = curLine.c_str();
		if (curLine.find("//") == std::string::npos) // ignore comments
		{
			int width = 0;
			int height = 0;
			if (sscanf(curLineChar, "%ix%i\n", &width, &height) == 2)
			{
				if (width > 319 && height > 239) // 320x240 sanity check
					customResolutions.emplace_back(width, height);
			}
		}
	}

	// native resolution at the end
	Vector2 nativeResolution = env->getNativeScreenSize();
	bool containsNativeResolution = false;
	for (int i=0; i<resolutions.size(); i++)
	{
		if (resolutions[i] == nativeResolution)
		{
			containsNativeResolution = true;
			break;
		}
	}
	if (!containsNativeResolution)
		resolutions.push_back(nativeResolution);

	// build context menu
	m_contextMenu->setPos(m_resolutionSelectButton->getPos());
	m_contextMenu->setRelPos(m_resolutionSelectButton->getRelPos());
	m_contextMenu->begin();
	for (int i=0; i<resolutions.size(); i++)
	{
		if (resolutions[i].x > nativeResolution.x || resolutions[i].y > nativeResolution.y)
			continue;

		const UString resolution = UString::format("%ix%i", (int)std::round(resolutions[i].x), (int)std::round(resolutions[i].y));
		CBaseUIButton *button = m_contextMenu->addButton(resolution);
		if (m_resolutionLabel != NULL && resolution == m_resolutionLabel->getText())
			button->setTextBrightColor(0xff00ff00);
	}
	for (int i=0; i<customResolutions.size(); i++)
	{
		m_contextMenu->addButton(UString::format("%ix%i", (int)std::round(customResolutions[i].x), (int)std::round(customResolutions[i].y)));
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResolutionSelect2) );
}

void OsuOptionsMenu::onResolutionSelect2(const UString& resolution, int  /*id*/)
{
	if (env->isFullscreen())
		cv::osu::resolution.setValue(resolution);
	else
		cv::windowed.execArgs(resolution);
}

void OsuOptionsMenu::onOutputDeviceSelect()
{
	std::vector<UString> outputDevices = soundEngine->getOutputDevices();

	// build context menu
	m_contextMenu->setPos(m_outputDeviceSelectButton->getPos());
	m_contextMenu->setRelPos(m_outputDeviceSelectButton->getRelPos());
	m_contextMenu->begin();
	for (int i=0; i<outputDevices.size(); i++)
	{
		CBaseUIButton *button = m_contextMenu->addButton(outputDevices[i]);
		if (outputDevices[i] == soundEngine->getOutputDevice())
			button->setTextBrightColor(0xff00ff00);
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceSelect2) );
}

void OsuOptionsMenu::onOutputDeviceSelect2(const UString& outputDeviceName, int  /*id*/)
{
	unsigned long prevMusicPositionMS = 0;
	if (!osu->isInPlayMode() && osu->getSelectedBeatmap() != NULL && osu->getSelectedBeatmap()->getMusic() != NULL)
		prevMusicPositionMS = osu->getSelectedBeatmap()->getMusic()->getPositionMS();

	soundEngine->setOutputDevice(outputDeviceName);
	m_outputDeviceLabel->setText(soundEngine->getOutputDevice());
	osu->getSkin()->reloadSounds();

	// and update reset button as usual
	onOutputDeviceResetUpdate();

	// start playing music again after audio device changed
	if (!osu->isInPlayMode() && osu->getSelectedBeatmap() != NULL && osu->getSelectedBeatmap()->getMusic() != NULL)
	{
		osu->getSelectedBeatmap()->unloadMusic();
		osu->getSelectedBeatmap()->select(); // (triggers preview music play)
		osu->getSelectedBeatmap()->getMusic()->setPositionMS(prevMusicPositionMS);
	}
}

void OsuOptionsMenu::onOutputDeviceResetClicked()
{
	if (soundEngine->getOutputDevices().size() > 0)
		onOutputDeviceSelect2(soundEngine->getOutputDevices()[0]);
}

void OsuOptionsMenu::onOutputDeviceResetUpdate()
{
	if (m_outputDeviceResetButton != NULL)
		m_outputDeviceResetButton->setEnabled(soundEngine->getOutputDevices().size() > 0 && soundEngine->getOutputDevice() != soundEngine->getOutputDevices()[0]);
}

void OsuOptionsMenu::onOutputDeviceRestart()
{
	if constexpr (Env::cfg(AUD::WASAPI))
		soundEngine->setOutputDeviceForce(soundEngine->getOutputDevice());
	else
		soundEngine->setOutputDevice("Default"); // NOTE: only relevant for horizon builds atm
}

void OsuOptionsMenu::onAudioCompatibilityModeChange(CBaseUICheckbox *checkbox)
{
	onCheckboxChange(checkbox);
	soundEngine->setOutputDeviceForce(soundEngine->getOutputDevice());
	checkbox->setChecked(cv::win_snd_fallback_dsound.getBool(), false);
	osu->getSkin()->reloadSounds();
}

void OsuOptionsMenu::onDownloadOsuClicked()
{
	osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	env->openURLInDefaultBrowser("https://osu.ppy.sh/home/download");
}

void OsuOptionsMenu::onManuallyManageBeatmapsClicked()
{
	osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	env->openURLInDefaultBrowser("https://steamcommunity.com/sharedfiles/filedetails/?id=880768265");
}

void OsuOptionsMenu::onBrowseOsuFolderClicked()
{
	if (!m_bIsOsuFolderDialogOpen)
	{
		m_bIsOsuFolderDialogOpen = true;
		osu->getNotificationOverlay()->addNotification("Opening file browser ...", 0xffffffff, false, 0.75f);

		env->openFolderWindow(
			[this](const std::vector<UString>& paths) {
				m_bIsOsuFolderDialogOpen = false;
				if (paths.empty()) {
					osu->getNotificationOverlay()->addNotification("No folder selected.", 0xff770000, false, 1.0f);
					return;
				}

				// use the first selected path
				UString newPath = paths[0];
				cv::osu::folder.setValue(newPath);
				m_osuFolderTextbox->setText(newPath);
				updateOsuFolder();
			},
			cv::osu::folder.getString()
		);
	}
}

void OsuOptionsMenu::onCM360CalculatorLinkClicked()
{
	osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	env->openURLInDefaultBrowser("https://www.mouse-sensitivity.com/");
}

void OsuOptionsMenu::onNotelockSelect()
{
	// build context menu
	m_contextMenu->setPos(m_notelockSelectButton->getPos());
	m_contextMenu->setRelPos(m_notelockSelectButton->getRelPos());
	m_contextMenu->begin(m_notelockSelectButton->getSize().x);
	{
		for (int i=0; i<m_notelockTypes.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(m_notelockTypes[i], i);
			if (i == cv::osu::notelock_type.getInt())
				button->setTextBrightColor(0xff00ff00);
		}
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onNotelockSelect2) );
}

void OsuOptionsMenu::onNotelockSelect2(const UString&  /*notelockType*/, int id)
{
	cv::osu::notelock_type.setValue(id);
	updateNotelockSelectLabel();

	// and update the reset button as usual
	onNotelockSelectResetUpdate();
}

void OsuOptionsMenu::onNotelockSelectResetClicked()
{
	if (m_notelockTypes.size() > 1 && (size_t)cv::osu::notelock_type.getDefaultFloat() < m_notelockTypes.size())
		onNotelockSelect2(m_notelockTypes[(size_t)cv::osu::notelock_type.getDefaultFloat()], (int)cv::osu::notelock_type.getDefaultFloat());
}

void OsuOptionsMenu::onNotelockSelectResetUpdate()
{
	if (m_notelockSelectResetButton != NULL)
		m_notelockSelectResetButton->setEnabled(cv::osu::notelock_type.getInt() != (int)cv::osu::notelock_type.getDefaultFloat());
}

void OsuOptionsMenu::onHPDrainSelect()
{
	// build context menu
	m_contextMenu->setPos(m_hpDrainSelectButton->getPos());
	m_contextMenu->setRelPos(m_hpDrainSelectButton->getRelPos());
	m_contextMenu->begin(m_hpDrainSelectButton->getSize().x);
	{
		for (int i=0; i<m_drainTypes.size(); i++)
		{
			CBaseUIButton *button = m_contextMenu->addButton(m_drainTypes[i], i);
			if (i == cv::osu::drain_type.getInt())
				button->setTextBrightColor(0xff00ff00);
		}
	}
	m_contextMenu->end(false, false);
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onHPDrainSelect2) );
}

void OsuOptionsMenu::onHPDrainSelect2(const UString&  /*hpDrainType*/, int id)
{
	cv::osu::drain_type.setValue(id);
	updateHPDrainSelectLabel();

	// and update the reset button as usual
	onHPDrainSelectResetUpdate();
}

void OsuOptionsMenu::onHPDrainSelectResetClicked()
{
	if (m_drainTypes.size() > 1 && (size_t)cv::osu::drain_type.getDefaultFloat() < m_drainTypes.size())
		onHPDrainSelect2(m_drainTypes[(size_t)cv::osu::drain_type.getDefaultFloat()], (size_t)cv::osu::drain_type.getDefaultFloat());
}

void OsuOptionsMenu::onHPDrainSelectResetUpdate()
{
	if (m_hpDrainSelectResetButton != NULL)
		m_hpDrainSelectResetButton->setEnabled(cv::osu::drain_type.getInt() != (int)cv::osu::drain_type.getDefaultFloat());
}

void OsuOptionsMenu::onCheckboxChange(CBaseUICheckbox *checkbox)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(checkbox->isChecked());

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChange(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f); // round to 2 decimal places

				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					labelPointer->setText(m_elements[i].cvar->getString());
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeOneDecimalPlace(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*10.0f)/10.0f); // round to 1 decimal place

				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					labelPointer->setText(m_elements[i].cvar->getString());
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeTwoDecimalPlaces(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f); // round to 2 decimal places

				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					labelPointer->setText(m_elements[i].cvar->getString());
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeOneDecimalPlaceMeters(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*10.0f)/10.0f); // round to 1 decimal place

				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					labelPointer->setText(UString::format("%.1f m", m_elements[i].cvar->getFloat()));
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeInt(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat())); // round to int

				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					labelPointer->setText(m_elements[i].cvar->getString());
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeIntMS(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat())); // round to int

				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					UString text = m_elements[i].cvar->getString();
					text.append(" ms");
					labelPointer->setText(text);
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeFloatMS(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(slider->getFloat());

				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					UString text = UString::format("%i", (int)std::round(m_elements[i].cvar->getFloat()*1000.0f));
					text.append(" ms");
					labelPointer->setText(text);
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangePercent(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f);

				if (m_elements[i].elements.size() == 3)
				{
					int percent = std::round(m_elements[i].cvar->getFloat()*100.0f);

					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					labelPointer->setText(UString::format("%i%%", percent));
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onKeyBindingButtonPressed(CBaseUIButton *button)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == button)
			{
				if (m_elements[i].cvar != NULL)
				{
					m_waitingKey = m_elements[i].cvar;

					UString notificationText = "Press new key for ";
					notificationText.append(button->getText());
					notificationText.append(":");

					const bool waitForKey = true;
					osu->getNotificationOverlay()->addNotification(notificationText, 0xffffffff, waitForKey);
					if (button->isType<OsuOptionsMenuKeyBindButton>())
						osu->getNotificationOverlay()->setDisallowWaitForKeyLeftClick(!static_cast<OsuOptionsMenuKeyBindButton*>(button)->isLeftMouseClickBindingAllowed());
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onKeyUnbindButtonPressed(CBaseUIButton *button)
{
	soundEngine->play(osu->getSkin()->getCheckOff());

	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == button)
			{
				if (m_elements[i].cvar != NULL)
				{
					m_elements[i].cvar->setValue(0.0f);
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onKeyBindingsResetAllPressed(CBaseUIButton * /*button*/)
{
	m_iNumResetAllKeyBindingsPressed++;

	const int numRequiredPressesUntilReset = 4;
	const int remainingUntilReset = numRequiredPressesUntilReset - m_iNumResetAllKeyBindingsPressed;

	if (m_iNumResetAllKeyBindingsPressed > (numRequiredPressesUntilReset-1))
	{
		m_iNumResetAllKeyBindingsPressed = 0;

		for (ConVar *bind : *(osu->getBindings()->get()))
		{
			bind->setValue(bind->getDefaultFloat());
		}

		osu->getNotificationOverlay()->addNotification("All key bindings have been reset.", 0xff00ff00);
	}
	else
	{
		if (remainingUntilReset > 1)
			osu->getNotificationOverlay()->addNotification(UString::format("Press %i more times to confirm.", remainingUntilReset));
		else
			osu->getNotificationOverlay()->addNotification(UString::format("Press %i more time to confirm!", remainingUntilReset), 0xffffff00);
	}
}

void OsuOptionsMenu::onKeyBindingManiaPressedInt()
{
	onKeyBindingManiaPressed(NULL);
}

void OsuOptionsMenu::onKeyBindingManiaPressed(CBaseUIButton * /*button*/)
{
	m_waitingKey = NULL;
	m_iManiaK = 0;
	m_iManiaKey = 0;

	KeyboardEvent e(0);
	onKey(e);
}

void OsuOptionsMenu::onSliderChangeSliderQuality(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
				{
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f); // round to 2 decimal places
				}

				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();

					int percent = std::round((slider->getPercent()) * 100.0f);
					UString text = UString::format(percent > 49 ? "%i !" : "%i", percent);
					labelPointer->setText(text);
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeLetterboxingOffset(CBaseUISlider *slider)
{
	m_bLetterboxingOffsetUpdateScheduled = true;

	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				const float newValue = std::round(slider->getFloat()*100.0f)/100.0f;

				if (m_elements[i].elements.size() == 3)
				{
					const int percent = std::round(newValue*100.0f);

					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					labelPointer->setText(UString::format("%i%%", percent));
				}

				m_letterboxingOffsetResetButton = m_elements[i].resetButton; // HACKHACK: disgusting

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeUIScale(CBaseUISlider *slider)
{
	m_bUIScaleChangeScheduled = true;

	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				const float newValue = std::round(slider->getFloat()*100.0f)/100.0f;

				if (m_elements[i].elements.size() == 3)
				{
					const int percent = std::round(newValue*100.0f);

					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					labelPointer->setText(UString::format("%i%%", percent));
				}

				m_uiScaleResetButton = m_elements[i].resetButton; // HACKHACK: disgusting

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onWASAPIBufferChange(CBaseUISlider *slider)
{
	m_bWASAPIBufferChangeScheduled = true;

	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					UString text = UString::format("%i", (int)std::round(slider->getFloat()*1000.0f));
					text.append(" ms");
					labelPointer->setText(text);
				}

				m_wasapiBufferSizeResetButton = m_elements[i].resetButton; // HACKHACK: disgusting

				break;
			}
		}
	}
}

void OsuOptionsMenu::onWASAPIPeriodChange(CBaseUISlider *slider)
{
	m_bWASAPIPeriodChangeScheduled = true;

	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].elements.size() == 3)
				{
					auto *labelPointer = m_elements[i].elements[2]->as<CBaseUILabel>();
					UString text = UString::format("%i", (int)std::round(slider->getFloat()*1000.0f));
					text.append(" ms");
					labelPointer->setText(text);
				}

				m_wasapiPeriodSizeResetButton = m_elements[i].resetButton; // HACKHACK: disgusting

				break;
			}
		}
	}
}

void OsuOptionsMenu::onUseSkinsSoundSamplesChange(const UString & /*oldValue*/, const UString & /*newValue*/)
{
	osu->reloadSkin();
}

void OsuOptionsMenu::onHighQualitySlidersCheckboxChange(CBaseUICheckbox *checkbox)
{
	onCheckboxChange(checkbox);

	// special case: if the checkbox is clicked and enabled via the UI, force set the quality to 100
	if (checkbox->isChecked())
		m_sliderQualitySlider->setValue(1.0f, false);
}

void OsuOptionsMenu::onHighQualitySlidersConVarChange(const UString & /*oldValue*/, const UString &newValue)
{
	const bool enabled = newValue.toFloat() > 0;
	for (int i=0; i<m_elements.size(); i++)
	{
		bool contains = false;
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == m_sliderQualitySlider)
			{
				contains = true;
				break;
			}
		}

		if (contains)
		{
			// HACKHACK: show/hide quality slider, this is ugly as fuck
			// TODO: containers use setVisible() on all elements. there needs to be a separate API for hiding elements while inside any kind of container
			for (int e=0; e<m_elements[i].elements.size(); e++)
			{
				m_elements[i].elements[e]->setEnabled(enabled);

				auto *sliderPointer = m_elements[i].elements[e]->as<OsuUISlider>();
				auto *labelPointer = m_elements[i].elements[e]->as<CBaseUILabel>();

				if (sliderPointer != NULL)
					sliderPointer->setFrameColor(enabled ? 0xffffffff : 0xff000000);
				if (labelPointer != NULL)
					labelPointer->setTextColor(enabled ? 0xffffffff : 0xff000000);
			}

			// reset value if disabled
			if (!enabled)
			{
				m_sliderQualitySlider->setValue(m_elements[i].cvar->getDefaultFloat(), false);
				m_elements[i].cvar->setValue(m_elements[i].cvar->getDefaultFloat());
			}

			onResetUpdate(m_elements[i].resetButton);

			break;
		}
	}
}

void OsuOptionsMenu::onCategoryClicked(CBaseUIButton *button)
{
	// reset search
	m_sSearchString = "";
	scheduleSearchUpdate();

	// scroll to category
	const auto *categoryButton = button->as<const OsuOptionsMenuCategoryButton>();
	if (categoryButton != NULL)
		m_options->scrollToElement(categoryButton->getSection(), 0, 100 * Osu::getUIScale());
}

void OsuOptionsMenu::onResetUpdate(CBaseUIButton *button)
{
	if (button == NULL) return;

	for (int i=0; i<m_elements.size(); i++)
	{
		if (m_elements[i].resetButton == button && m_elements[i].cvar != NULL)
		{
			switch (m_elements[i].type)
			{
			case 6: // checkbox
				m_elements[i].resetButton->setEnabled(m_elements[i].cvar->getBool() != (bool)m_elements[i].cvar->getDefaultFloat());
				break;
			case 7: // slider
				m_elements[i].resetButton->setEnabled(m_elements[i].cvar->getFloat() != m_elements[i].cvar->getDefaultFloat());
				break;
			}

			break;
		}
	}
}

void OsuOptionsMenu::onResetClicked(CBaseUIButton *button)
{
	if (button == NULL) return;

	for (int i=0; i<m_elements.size(); i++)
	{
		if (m_elements[i].resetButton == button && m_elements[i].cvar != NULL)
		{
			switch (m_elements[i].type)
			{
			case 6: // checkbox
				for (int e=0; e<m_elements[i].elements.size(); e++)
				{
					auto *checkboxPointer = m_elements[i].elements[e]->as<CBaseUICheckbox>();
					if (checkboxPointer != NULL)
						checkboxPointer->setChecked((bool)m_elements[i].cvar->getDefaultFloat());
				}
				break;
			case 7: // slider
				if (m_elements[i].elements.size() == 3)
				{
					auto *sliderPointer = m_elements[i].elements[1]->as<CBaseUISlider>();
					if (sliderPointer != NULL)
					{
						sliderPointer->setValue(m_elements[i].cvar->getDefaultFloat(), false);
						sliderPointer->fireChangeCallback();
					}
				}
				break;
			}

			break;
		}
	}

	onResetUpdate(button);
}

void OsuOptionsMenu::onResetEverythingClicked(CBaseUIButton * /*button*/)
{
	m_iNumResetEverythingPressed++;

	const int numRequiredPressesUntilReset = 4;
	const int remainingUntilReset = numRequiredPressesUntilReset - m_iNumResetEverythingPressed;

	if (m_iNumResetEverythingPressed > (numRequiredPressesUntilReset-1))
	{
		m_iNumResetEverythingPressed = 0;

		// first reset all settings
		for (size_t i=0; i<m_elements.size(); i++)
		{
			OsuOptionsMenuResetButton *resetButton = m_elements[i].resetButton;
			if (resetButton != NULL && resetButton->isEnabled())
				resetButton->click();
		}

		// and then all key bindings (since these don't use the yellow reset button system)
		for (ConVar *bind : *(osu->getBindings()->get()))
		{
			bind->setValue(bind->getDefaultFloat());
		}

		osu->getNotificationOverlay()->addNotification("All settings have been reset.", 0xff00ff00);
	}
	else
	{
		if (remainingUntilReset > 1)
			osu->getNotificationOverlay()->addNotification(UString::format("Press %i more times to confirm.", remainingUntilReset));
		else
			osu->getNotificationOverlay()->addNotification(UString::format("Press %i more time to confirm!", remainingUntilReset), 0xffffff00);
	}
}

void OsuOptionsMenu::addSpacer(unsigned num)
{
	OPTIONS_ELEMENT e;
	e.type = 0;
	e.cvar = NULL;
	m_elements.insert(m_elements.end(), num, e);
}

CBaseUILabel *OsuOptionsMenu::addSection(const UString& text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	//label->setTextColor(0xff58dafe);
	label->setFont(osu->getTitleFont());
	label->setSizeToContent(0, 0);
	label->setTextJustification(CBaseUILabel::TEXT_JUSTIFICATION_RIGHT);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 1;
	e.cvar = NULL;
	m_elements.push_back(e);

	return label;
}

CBaseUILabel *OsuOptionsMenu::addSubSection(const UString& text, UString searchTags)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	label->setFont(osu->getSubTitleFont());
	label->setSizeToContent(0, 0);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 2;
	e.cvar = NULL;
	e.searchTags = std::move(searchTags);
	m_elements.push_back(e);

	return label;
}

CBaseUILabel *OsuOptionsMenu::addLabel(const UString& text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	label->setSizeToContent(0, 0);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 3;
	e.cvar = NULL;
	m_elements.push_back(e);

	return label;
}

OsuUIButton *OsuOptionsMenu::addButton(const UString& text)
{
	OsuUIButton *button = new OsuUIButton(0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	OPTIONS_ELEMENT e;
	e.elements.push_back(button);
	e.type = 4;
	m_elements.push_back(e);

	return button;
}

OsuOptionsMenu::OPTIONS_ELEMENT OsuOptionsMenu::addButton(const UString& text, const UString& labelText, bool withResetButton)
{
	OsuUIButton *button = new OsuUIButton(0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 50, labelText, labelText);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	if (withResetButton)
	{
		e.resetButton = new OsuOptionsMenuResetButton(0, 0, 35, 50, "", "");
	}
	e.elements.push_back(button);
	e.elements.push_back(label);
	e.type = 4;
	m_elements.push_back(e);

	return e;
}

OsuOptionsMenu::OPTIONS_ELEMENT OsuOptionsMenu::addButtonButton(const UString& text1, const UString& text2)
{
	OsuUIButton *button = new OsuUIButton(0, 0, m_options->getSize().x, 50, text1, text1);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	OsuUIButton *button2 = new OsuUIButton(0, 0, m_options->getSize().x, 50, text2, text2);
	button2->setColor(0xff0e94b5);
	button2->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button2);

	OPTIONS_ELEMENT e;
	e.elements.push_back(button);
	e.elements.push_back(button2);
	e.type = 4;
	m_elements.push_back(e);

	return e;
}

OsuOptionsMenu::OPTIONS_ELEMENT OsuOptionsMenu::addButtonButtonLabel(const UString& text1, const UString& text2, const UString& labelText, bool withResetButton)
{
	OsuUIButton *button = new OsuUIButton(0, 0, m_options->getSize().x, 50, text1, text1);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	OsuUIButton *button2 = new OsuUIButton(0, 0, m_options->getSize().x, 50, text2, text2);
	button2->setColor(0xff0e94b5);
	button2->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button2);

	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 50, labelText, labelText);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	if (withResetButton)
	{
		e.resetButton = new OsuOptionsMenuResetButton(0, 0, 35, 50, "", "");
	}
	e.elements.push_back(button);
	e.elements.push_back(button2);
	e.elements.push_back(label);
	e.type = 4;
	m_elements.push_back(e);

	return e;
}

OsuOptionsMenuKeyBindButton *OsuOptionsMenu::addKeyBindButton(const UString& text, ConVar *cvar)
{
	///UString unbindIconString; unbindIconString.insert(0, OsuIcons::UNDO);
	OsuUIButton *unbindButton = new OsuUIButton(0, 0, m_options->getSize().x, 50, text, "");
	unbindButton->setTooltipText("Unbind");
	unbindButton->setColor(0x77ff0000);
	unbindButton->setUseDefaultSkin();
	unbindButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyUnbindButtonPressed) );
	///unbindButton->setFont(osu->getFontIcons());
	m_options->getContainer()->addBaseUIElement(unbindButton);

	OsuOptionsMenuKeyBindButton *bindButton = new OsuOptionsMenuKeyBindButton(0, 0, m_options->getSize().x, 50, text, text);
	bindButton->setColor(0xff0e94b5);
	bindButton->setUseDefaultSkin();
	bindButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	m_options->getContainer()->addBaseUIElement(bindButton);

	OsuOptionsMenuKeyBindLabel *label = new OsuOptionsMenuKeyBindLabel(0, 0, m_options->getSize().x, 50, "", "", cvar, bindButton);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(unbindButton);
	e.elements.push_back(bindButton);
	e.elements.push_back(label);
	e.type = 5;
	e.cvar = cvar;
	m_elements.push_back(e);

	return bindButton;
}

CBaseUICheckbox *OsuOptionsMenu::addCheckbox(UString text, ConVar *cvar)
{
	return addCheckbox(text, "", cvar);
}

CBaseUICheckbox *OsuOptionsMenu::addCheckbox(const UString& text, const UString& tooltipText, ConVar *cvar)
{
	OsuUICheckbox *checkbox = new OsuUICheckbox(0, 0, m_options->getSize().x, 50, text, text);
	checkbox->setDrawFrame(false);
	checkbox->setDrawBackground(false);

	if (tooltipText.length() > 0)
		checkbox->setTooltipText(tooltipText);

	if (cvar != NULL)
	{
		checkbox->setChecked(cvar->getBool());
		checkbox->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onCheckboxChange) );
	}

	m_options->getContainer()->addBaseUIElement(checkbox);

	OPTIONS_ELEMENT e;
	if (cvar != NULL)
	{
		e.resetButton = new OsuOptionsMenuResetButton(0, 0, 35, 50, "", "");
		e.resetButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResetClicked) );
	}
	e.elements.push_back(checkbox);
	e.type = 6;
	e.cvar = cvar;
	m_elements.push_back(e);

	return checkbox;
}

OsuUISlider *OsuOptionsMenu::addSlider(const UString& text, float min, float max, ConVar *cvar, float label1Width, bool allowOverscale, bool allowUnderscale)
{
	OsuUISlider *slider = new OsuUISlider(0, 0, 100, 50, text);
	slider->setAllowMouseWheel(false);
	slider->setBounds(min, max);
	slider->setLiveUpdate(true);
	if (cvar != NULL)
	{
		slider->setValue(cvar->getFloat(), false);
		slider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChange) );
	}
	m_options->getContainer()->addBaseUIElement(slider);

	CBaseUILabel *label1 = new CBaseUILabel(0, 0, m_options->getSize().x, 50, text, text);
	label1->setDrawFrame(false);
	label1->setDrawBackground(false);
	label1->setWidthToContent();
	if (label1Width > 1)
		label1->setSizeX(label1Width);
	label1->setRelSizeX(label1->getSize().x);
	m_options->getContainer()->addBaseUIElement(label1);

	CBaseUILabel *label2 = new CBaseUILabel(0, 0, m_options->getSize().x, 50, "", "8.81");
	label2->setDrawFrame(false);
	label2->setDrawBackground(false);
	label2->setWidthToContent();
	label2->setRelSizeX(label2->getSize().x);
	m_options->getContainer()->addBaseUIElement(label2);

	OPTIONS_ELEMENT e;
	if (cvar != NULL)
	{
		e.resetButton = new OsuOptionsMenuResetButton(0, 0, 35, 50, "", "");
		e.resetButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResetClicked) );
	}
	e.elements.push_back(label1);
	e.elements.push_back(slider);
	e.elements.push_back(label2);
	e.type = 7;
	e.cvar = cvar;
	e.label1Width = label1Width;
	e.relSizeDPI = label1->getFont()->getDPI();
	e.allowOverscale = allowOverscale;
	e.allowUnderscale = allowUnderscale;
	m_elements.push_back(e);

	return slider;
}

CBaseUITextbox *OsuOptionsMenu::addTextbox(UString text, ConVar *cvar)
{
	CBaseUITextbox *textbox = new CBaseUITextbox(0, 0, m_options->getSize().x, 40, "");
	textbox->setText(std::move(text));
	m_options->getContainer()->addBaseUIElement(textbox);

	OPTIONS_ELEMENT e;
	e.elements.push_back(textbox);
	e.type = 8;
	e.cvar = cvar;
	m_elements.push_back(e);

	return textbox;
}

CBaseUITextbox *OsuOptionsMenu::addTextbox(UString text, const UString& labelText, ConVar *cvar)
{
	CBaseUITextbox *textbox = new CBaseUITextbox(0, 0, m_options->getSize().x, 40, "");
	textbox->setText(std::move(text));
	m_options->getContainer()->addBaseUIElement(textbox);

	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 40, labelText, labelText);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	label->setWidthToContent();
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.elements.push_back(textbox);
	e.type = 8;
	e.cvar = cvar;
	m_elements.push_back(e);

	return textbox;
}

CBaseUIElement *OsuOptionsMenu::addSkinPreview()
{
	CBaseUIElement *skinPreview = new OsuOptionsMenuSkinPreviewElement(0, 0, 0, 200, "skincirclenumberhitresultpreview");
	m_options->getContainer()->addBaseUIElement(skinPreview);

	OPTIONS_ELEMENT e;
	e.elements.push_back(skinPreview);
	e.type = 9;
	e.cvar = NULL;
	m_elements.push_back(e);

	return skinPreview;
}

CBaseUIElement *OsuOptionsMenu::addSliderPreview()
{
	CBaseUIElement *sliderPreview = new OsuOptionsMenuSliderPreviewElement(0, 0, 0, 200, "skinsliderpreview");
	m_options->getContainer()->addBaseUIElement(sliderPreview);

	OPTIONS_ELEMENT e;
	e.elements.push_back(sliderPreview);
	e.type = 9;
	e.cvar = NULL;
	m_elements.push_back(e);

	return sliderPreview;
}

OsuOptionsMenuCategoryButton *OsuOptionsMenu::addCategory(CBaseUIElement *section, wchar_t icon)
{
	UString iconString; iconString.insert(0, icon);
	OsuOptionsMenuCategoryButton *button = new OsuOptionsMenuCategoryButton(section, 0, 0, 50, 50, "", iconString);
	button->setFont(osu->getFontIcons());
	button->setDrawBackground(false);
	button->setDrawFrame(false);
	button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onCategoryClicked) );
	m_categories->getContainer()->addBaseUIElement(button);
	m_categoryButtons.push_back(button);

	return button;
}

void OsuOptionsMenu::save()
{
	if (!cv::osu::options_save_on_back.getBool())
	{
		debugLog("DEACTIVATED SAVE!!!! @ {:f}\n", engine->getTime());
		return;
	}

	updateOsuFolder();
	updateName();
	updateFposuDPI();
	updateFposuCMper360();

	debugLog("Osu: Saving user config file ...\n");

	UString userConfigFile = UString::fmt("cfg/{:s}", OSU_CONFIG_FILE_NAME);

	// manual commands (e.g. fullscreen, windowed, osu_resolution); meaning commands which are not necessarily visible in the options menu, or which need special
	// handling & ordering
	std::vector<ConVar *> manualConCommands;
	std::vector<ConVar *> manualConVars;
	std::vector<ConVar *> removeConCommands;

	manualConVars.push_back(&cv::osu::songbrowser_sortingtype);
	manualConVars.push_back(&cv::osu::songbrowser_scores_sortingtype);
	manualConVars.push_back(&cv::osu::notelock_type);
	manualConVars.push_back(&cv::osu::drain_type);

	removeConCommands.push_back(&cv::monitor);
	removeConCommands.push_back(&cv::windowed);
	removeConCommands.push_back(&cv::osu::skin);
	removeConCommands.push_back(&cv::osu::skin_workshop_title);
	removeConCommands.push_back(&cv::osu::skin_workshop_id);
	removeConCommands.push_back(&cv::osu::skin_is_from_workshop);
	removeConCommands.push_back(&cv::snd_output_device);

	if (m_fullscreenCheckbox != NULL)
	{
		if (m_fullscreenCheckbox->isChecked())
		{
			manualConCommands.push_back(&cv::fullscreen);
			if (cv::osu::resolution_enabled.getBool())
				manualConVars.push_back(&cv::osu::resolution);
			else
				removeConCommands.push_back(&cv::osu::resolution);
		}
		else
		{
			removeConCommands.push_back(&cv::fullscreen);
			removeConCommands.push_back(&cv::osu::resolution);
		}
	}

	// read existing config file and filter lines
	std::vector<UString> keepLines;
	{
		McFile in(userConfigFile, McFile::TYPE::READ);
		if (!in.canRead())
			debugLog("Osu Error: Couldn't read user config file! (tried: {:s})\n", userConfigFile);
		else
		{
			UString line;
			while (!(line = in.readLine()).isEmpty())
			{
				bool keepLine = true;

				// extract the convar name (first word) from the line
				const int firstSpaceIndex = line.findChar(' ');
				const int endOfConVarNameIndex = (firstSpaceIndex != -1 ? firstSpaceIndex : line.length());
				const UString lineConVarName = line.substr(0, endOfConVarNameIndex);

				// check against elements convars
				for (const auto &element : m_elements)
				{
					if (element.cvar != NULL && element.cvar->getName().equalsIgnoreCase(lineConVarName))
					{
						keepLine = false;
						break;
					}
				}

				// check against manual commands
				if (keepLine)
				{
					for (const auto &cmd : manualConCommands)
					{
						if (cmd->getName().equalsIgnoreCase(lineConVarName))
						{
							keepLine = false;
							break;
						}
					}
				}

				// check against manual convars
				if (keepLine)
				{
					for (const auto &var : manualConVars)
					{
						if (var->getName().equalsIgnoreCase(lineConVarName))
						{
							keepLine = false;
							break;
						}
					}
				}

				// check against remove commands
				if (keepLine)
				{
					for (const auto &cmd : removeConCommands)
					{
						if (cmd->getName().equalsIgnoreCase(lineConVarName))
						{
							keepLine = false;
							break;
						}
					}
				}

				if (keepLine)
					keepLines.push_back(line);
			}
		}
	}

	// write new config file
	McFile out(userConfigFile, McFile::TYPE::WRITE);
	if (!out.canWrite())
	{
		engine->showMessageError("Osu Error", "Couldn't write user config file!");
		return;
	}

	// write user stuff back
	for (const auto &line : keepLines)
		out.writeLine(line);

	out.writeLine("");

	// write manual commands
	for (const auto &cmd : manualConCommands)
		out.writeLine(cmd->getName());

	// write manual convars
	for (const auto &var : manualConVars)
		out.writeLine(UString::fmt("{:s} {:s}", var->getName(), var->getString()));

	// hardcoded entries
	out.writeLine(UString::fmt("monitor {:d}", env->getMonitor()));

	if (soundEngine->getOutputDevice() != "Default" && !soundEngine->getOutputDevice().isEmpty())
		out.writeLine(UString::fmt("snd_output_device {:s}", soundEngine->getOutputDevice()));

	if (m_fullscreenCheckbox != NULL && !m_fullscreenCheckbox->isChecked())
		out.writeLine(UString::fmt("windowed {:d}x{:d}", engine->getScreenWidth(), engine->getScreenHeight()));

	// write options elements convars
	for (const auto &element : m_elements)
	{
		if (element.cvar != NULL)
			out.writeLine(UString::fmt("{:s} {:s}", element.cvar->getName(), element.cvar->getString()));
	}

	// write skin-related convars
	out.writeLine(UString::fmt("osu_skin_mipmaps {:s}", cv::osu::skin_mipmaps.getString()));

	if constexpr (Env::cfg(FEAT::STEAM))
	{
		if (cv::osu::skin_is_from_workshop.getBool())
		{
			out.writeLine(UString::fmt("osu_skin_is_from_workshop {:s}", cv::osu::skin_is_from_workshop.getString()));
			out.writeLine(UString::fmt("osu_skin_workshop_title {:s}", cv::osu::skin_workshop_title.getString()));
			out.writeLine(UString::fmt("osu_skin_workshop_id {:s}", cv::osu::skin_workshop_id.getString()));
		}
	}

	out.writeLine(UString::fmt("osu_skin {:s}", cv::osu::skin.getString()));
	if (!out.canWrite())
		debugLog("WARNING: partially written user config file {:s}.", userConfigFile);
}

void OsuOptionsMenu::openAndScrollToSkinSection()
{
	const bool wasVisible = isVisible();
	if (!wasVisible)
		osu->toggleOptionsMenu();

	if (!m_skinSelectLocalButton->isVisible() || !wasVisible)
		m_options->scrollToElement(m_skinSection, 0, 100 * Osu::getUIScale());
}
