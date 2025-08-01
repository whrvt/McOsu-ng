//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		pause menu (while playing)
//
// $NoKeywords: $
//===============================================================================//

#include "OsuPauseMenu.h"

#include <utility>

#include "Engine.h"
#include "Environment.h"
#include "SoundEngine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "Keyboard.h"
#include "ConVar.h"
#include "Mouse.h"

#include "CBaseUIContainer.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuKeyBindings.h"
#include "OsuModSelector.h"
#include "OsuOptionsMenu.h"
#include "OsuHUD.h"

#include "OsuUIPauseMenuButton.h"
namespace cv::osu {
ConVar pause_dim_background("osu_pause_dim_background", true, FCVAR_NONE);
ConVar pause_dim_alpha("osu_pause_dim_alpha", 0.58f, FCVAR_NONE);
ConVar pause_anim_duration("osu_pause_anim_duration", 0.15f, FCVAR_NONE);
}

OsuPauseMenu::OsuPauseMenu() : OsuScreen()
{
	m_bScheduledVisibility = false;
	m_bScheduledVisibilityChange = false;

	m_selectedButton = NULL;
	m_fWarningArrowsAnimStartTime = 0.0f;
	m_fWarningArrowsAnimAlpha = 0.0f;
	m_fWarningArrowsAnimX = 0.0f;
	m_fWarningArrowsAnimY = 0.0f;
	m_bInitialWarningArrowFlyIn = true;

	m_bContinueEnabled = true;
	m_bClick1Down = false;
	m_bClick2Down = false;

	m_fDimAnim = 0.0f;

	m_container = new CBaseUIContainer(0, 0, osu->getVirtScreenWidth(), osu->getVirtScreenHeight(), "");

	OsuUIPauseMenuButton *continueButton = addButton([]() -> Image *{return osu->getSkin()->getPauseContinue();});
	OsuUIPauseMenuButton *retryButton = addButton([]() -> Image *{return osu->getSkin()->getPauseRetry();});
	OsuUIPauseMenuButton *backButton = addButton([]() -> Image *{return osu->getSkin()->getPauseBack();});

	continueButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onContinueClicked) );
	retryButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onRetryClicked) );
	backButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onBackClicked) );

	updateLayout();
}

OsuPauseMenu::~OsuPauseMenu()
{
	SAFE_DELETE(m_container);
}

void OsuPauseMenu::draw()
{
	const bool isAnimating = anim->isAnimating(&m_fDimAnim);
	if (!m_bVisible && !isAnimating) return;

	if (m_bVisible || isAnimating)
	{
		// draw dim
		if (cv::osu::pause_dim_background.getBool())
		{
			g->setColor(argb(m_fDimAnim * cv::osu::pause_dim_alpha.getFloat(), 0.078f, 0.078f, 0.078f));
			g->fillRect(0, 0, osu->getVirtScreenWidth(), osu->getVirtScreenHeight());
		}

		// draw background image
		if ((m_bVisible || isAnimating))
		{
			Image *image = NULL;
			if (m_bContinueEnabled)
				image = osu->getSkin()->getPauseOverlay();
			else
				image = osu->getSkin()->getFailBackground();

			if (image != osu->getSkin()->getMissingTexture())
			{
				const float scale = Osu::getImageScaleToFillResolution(image, osu->getVirtScreenSize());
				const Vector2 centerTrans = (osu->getVirtScreenSize() / 2);

				g->setColor(argb(m_fDimAnim, 1.0f, 1.0f, 1.0f));
				g->pushTransform();
				{
					g->scale(scale, scale);
					g->translate((int)centerTrans.x, (int)centerTrans.y);
					g->drawImage(image);
				}
				g->popTransform();
			}
		}

		// draw buttons
		for (int i=0; i<m_buttons.size(); i++)
		{
			m_buttons[i]->setAlpha(1.0f - (1.0f - m_fDimAnim)*(1.0f - m_fDimAnim)*(1.0f - m_fDimAnim));
		}
		m_container->draw();

		// draw selection arrows
		if (m_selectedButton != NULL)
		{
			const Color arrowColor = rgb(0, 114, 255);
			float animation = fmod((float)(engine->getTime()-m_fWarningArrowsAnimStartTime)*3.2f, 2.0f);
			if (animation > 1.0f)
				animation = 2.0f - animation;

			animation =  -animation*(animation-2); // quad out
			const float offset = osu->getUIScale(20.0f + 45.0f*animation);

			g->setColor(arrowColor);
			g->setAlpha(m_fWarningArrowsAnimAlpha * m_fDimAnim);
			osu->getHUD()->drawWarningArrow(Vector2(m_fWarningArrowsAnimX, m_fWarningArrowsAnimY) + Vector2(0, m_selectedButton->getSize().y/2) - Vector2(offset, 0), false, false);
			osu->getHUD()->drawWarningArrow(Vector2(osu->getVirtScreenWidth() - m_fWarningArrowsAnimX, m_fWarningArrowsAnimY) + Vector2(0, m_selectedButton->getSize().y/2) + Vector2(offset, 0), true, false);
		}
	}

	if (!m_bVisible) return;

	/*
	g->setColor(0xffff0000);
	g->drawLine(0, osu->getVirtScreenHeight()/3.0f, osu->getVirtScreenWidth(), osu->getVirtScreenHeight()/3.0f);
	g->drawLine(0, (osu->getVirtScreenHeight()/3.0f)*2, osu->getVirtScreenWidth(), (osu->getVirtScreenHeight()/3.0f)*2);
	g->drawLine(0, (osu->getVirtScreenHeight()/3.0f)*3, osu->getVirtScreenWidth(), (osu->getVirtScreenHeight()/3.0f)*3);
	*/
}

void OsuPauseMenu::update()
{
	if (!m_bVisible) return;

	// update and focus handling
	m_container->update();

	if (osu->getOptionsMenu()->isMouseInside())
		m_container->stealFocus();

	if (m_bScheduledVisibilityChange)
	{
		m_bScheduledVisibilityChange = false;
		setVisible(m_bScheduledVisibility);
	}

	if (anim->isAnimating(&m_fWarningArrowsAnimX))
		m_fWarningArrowsAnimStartTime = engine->getTime();
}

void OsuPauseMenu::onContinueClicked()
{
	if (!m_bContinueEnabled) return;
	if (anim->isAnimating(&m_fDimAnim)) return;

	soundEngine->play(osu->getSkin()->getMenuHit());

	if (osu->getSelectedBeatmap() != NULL)
		osu->getSelectedBeatmap()->pause();

	scheduleVisibilityChange(false);
}

void OsuPauseMenu::onRetryClicked()
{
	if (anim->isAnimating(&m_fDimAnim)) return;

	soundEngine->play(osu->getSkin()->getMenuHit());

	if (osu->getSelectedBeatmap() != NULL)
		osu->getSelectedBeatmap()->restart();

	scheduleVisibilityChange(false);
}

void OsuPauseMenu::onBackClicked()
{
	if (anim->isAnimating(&m_fDimAnim)) return;

	soundEngine->play(osu->getSkin()->getMenuHit());

	if (osu->getSelectedBeatmap() != NULL)
		osu->getSelectedBeatmap()->stop();

	scheduleVisibilityChange(false);
}

void OsuPauseMenu::onSelectionChange()
{
	if (m_selectedButton != NULL)
	{
		if (m_bInitialWarningArrowFlyIn)
		{
			m_bInitialWarningArrowFlyIn = false;

			m_fWarningArrowsAnimY = m_selectedButton->getPos().y;
			m_fWarningArrowsAnimX = m_selectedButton->getPos().x - osu->getUIScale(170.0f);

			anim->moveLinear(&m_fWarningArrowsAnimAlpha, 1.0f, 0.3f);
			anim->moveQuadIn(&m_fWarningArrowsAnimX, m_selectedButton->getPos().x, 0.3f);
		}
		else
			m_fWarningArrowsAnimX = m_selectedButton->getPos().x;

		anim->moveQuadOut(&m_fWarningArrowsAnimY, m_selectedButton->getPos().y, 0.1f);

		soundEngine->play(osu->getSkin()->getMenuClick());
	}
}

void OsuPauseMenu::onKeyDown(KeyboardEvent &e)
{
	OsuScreen::onKeyDown(e); // only used for options menu
	if (!m_bVisible || e.isConsumed()) return;

	if (e == cv::osu::keybinds::LEFT_CLICK.getVal<KEYCODE>() || e == cv::osu::keybinds::RIGHT_CLICK.getVal<KEYCODE>()
	 || e == cv::osu::keybinds::LEFT_CLICK_2.getVal<KEYCODE>() || e == cv::osu::keybinds::RIGHT_CLICK_2.getVal<KEYCODE>())
	{
		bool fireButtonClick = false;
		if ((e == cv::osu::keybinds::LEFT_CLICK.getVal<KEYCODE>() || e == cv::osu::keybinds::LEFT_CLICK_2.getVal<KEYCODE>()) && !m_bClick1Down)
		{
			m_bClick1Down = true;
			fireButtonClick = true;
		}
		if ((e == cv::osu::keybinds::RIGHT_CLICK.getVal<KEYCODE>() || e == cv::osu::keybinds::RIGHT_CLICK_2.getVal<KEYCODE>()) && !m_bClick2Down)
		{
			m_bClick2Down = true;
			fireButtonClick = true;
		}
		if (fireButtonClick)
		{
			for (int i=0; i<m_buttons.size(); i++)
			{
				if (m_buttons[i]->isMouseInside())
				{
					m_buttons[i]->click();
					break;
				}
			}
		}
	}

	// handle arrow keys selection
	if (m_buttons.size() > 0)
	{
		if (!keyboard->isAltDown() && e == KEY_DOWN)
		{
			OsuUIPauseMenuButton *nextSelectedButton = m_buttons[0];

			// get first visible button
			for (int i=0; i<m_buttons.size(); i++)
			{
				if (!m_buttons[i]->isVisible())
					continue;

				nextSelectedButton = m_buttons[i];
				break;
			}

			// next selection logic
			bool next = false;
			for (int i=0; i<m_buttons.size(); i++)
			{
				if (!m_buttons[i]->isVisible())
					continue;

				if (next)
				{
					nextSelectedButton = m_buttons[i];
					break;
				}
				if (m_selectedButton == m_buttons[i])
					next = true;
			}
			m_selectedButton = nextSelectedButton;
			onSelectionChange();
		}

		if (!keyboard->isAltDown() && e == KEY_UP)
		{
			OsuUIPauseMenuButton *nextSelectedButton = m_buttons[m_buttons.size()-1];

			// get first visible button
			for (int i=m_buttons.size()-1; i>=0; i--)
			{
				if (!m_buttons[i]->isVisible())
					continue;

				nextSelectedButton = m_buttons[i];
				break;
			}

			// next selection logic
			bool next = false;
			for (int i=m_buttons.size()-1; i>=0; i--)
			{
				if (!m_buttons[i]->isVisible())
					continue;

				if (next)
				{
					nextSelectedButton = m_buttons[i];
					break;
				}
				if (m_selectedButton == m_buttons[i])
					next = true;
			}
			m_selectedButton = nextSelectedButton;
			onSelectionChange();
		}

		if (m_selectedButton != NULL && (e == KEY_ENTER || e == KEY_NUMPAD_ENTER))
			m_selectedButton->click();
	}

	// consume ALL events, except for a few special binds which are allowed through (e.g. for unpause or changing the local offset in Osu.cpp)
	if (e != KEY_ESCAPE && e != cv::osu::keybinds::GAME_PAUSE.getVal<KEYCODE>() && e != cv::osu::keybinds::INCREASE_LOCAL_OFFSET.getVal<KEYCODE>() && e != cv::osu::keybinds::DECREASE_LOCAL_OFFSET.getVal<KEYCODE>())
		e.consume();
}

void OsuPauseMenu::onKeyUp(KeyboardEvent &e)
{
	if (e == cv::osu::keybinds::LEFT_CLICK.getVal<KEYCODE>() || e == cv::osu::keybinds::LEFT_CLICK_2.getVal<KEYCODE>())
		m_bClick1Down = false;

	if (e == cv::osu::keybinds::RIGHT_CLICK.getVal<KEYCODE>() || e == cv::osu::keybinds::RIGHT_CLICK_2.getVal<KEYCODE>())
		m_bClick2Down = false;
}

void OsuPauseMenu::onChar(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	e.consume();
}

void OsuPauseMenu::scheduleVisibilityChange(bool visible)
{
	if (visible != m_bVisible)
	{
		m_bScheduledVisibilityChange = true;
		m_bScheduledVisibility = visible;
	}

	// HACKHACK:
	if (!visible)
		setContinueEnabled(true);
}

void OsuPauseMenu::updateLayout()
{
	const float height = (osu->getVirtScreenHeight()/(float)m_buttons.size());
	const float half = (m_buttons.size()-1)/2.0f;

	float maxWidth = 0.0f;
	float maxHeight = 0.0f;
	for (int i=0; i<m_buttons.size(); i++)
	{
		Image *img = m_buttons[i]->getImage();
		if (img == NULL)
			img = osu->getSkin()->getMissingTexture();

		const float scale = osu->getUIScale(256) / (411.0f * (osu->getSkin()->isPauseContinue2x() ? 2.0f : 1.0f));

		m_buttons[i]->setBaseScale(scale, scale);
		m_buttons[i]->setSize(img->getWidth()*scale, img->getHeight()*scale);

		if (m_buttons[i]->getSize().x > maxWidth)
			maxWidth = m_buttons[i]->getSize().x;
		if (m_buttons[i]->getSize().y > maxHeight)
			maxHeight = m_buttons[i]->getSize().y;
	}

	for (int i=0; i<m_buttons.size(); i++)
	{
		Vector2 newPos = Vector2(osu->getVirtScreenWidth()/2.0f - maxWidth/2, (i+1)*height - height/2.0f - maxHeight/2.0f);

		const float pinch = std::max(0.0f, (height/2.0f - maxHeight/2.0f));
		if ((float)i < half)
			newPos.y += pinch;
		else if ((float)i > half)
			newPos.y -= pinch;

		m_buttons[i]->setPos(newPos);
		m_buttons[i]->setSize(maxWidth, maxHeight);
	}

	onSelectionChange();
}

void OsuPauseMenu::onResolutionChange(Vector2 newResolution)
{
	m_container->setSize(newResolution);
	updateLayout();
}

void OsuPauseMenu::setVisible(bool visible)
{
	m_bVisible = visible;

	if (osu->isInPlayMode() && osu->getSelectedBeatmap() != NULL)
		setContinueEnabled(!osu->getSelectedBeatmap()->hasFailed());
	else
		setContinueEnabled(true);

	// HACKHACK: force disable mod selection screen in case it was open and the beatmap ended/failed
	osu->getModSelector()->setVisible(false);

	// HACKHACK: workaround for BaseUI framework deficiency (missing mouse events. if a mouse button is being held, and then suddenly a BaseUIElement gets put under it and set visible, and then the mouse button is released, that "incorrectly" fires onMouseUpInside/onClicked/etc.)
	if (visible)
		m_container->stealFocus();

	// reset
	m_selectedButton = NULL;
	m_bInitialWarningArrowFlyIn = true;
	m_fWarningArrowsAnimAlpha = 0.0f;
	m_bScheduledVisibility = visible;
	m_bScheduledVisibilityChange = false;

	if (m_bVisible)
		updateLayout();

	osu->updateConfineCursor();
	osu->updateWindowsKeyDisable();

	anim->moveQuadOut(&m_fDimAnim, (m_bVisible ? 1.0f : 0.0f), cv::osu::pause_anim_duration.getFloat() * (m_bVisible ? 1.0f - m_fDimAnim : m_fDimAnim), true);
}

void OsuPauseMenu::setContinueEnabled(bool continueEnabled)
{
	m_bContinueEnabled = continueEnabled;
	if (m_buttons.size() > 0)
		m_buttons[0]->setVisible(m_bContinueEnabled);
}

OsuUIPauseMenuButton *OsuPauseMenu::addButton(std::function<Image*()> getImageFunc)
{
	OsuUIPauseMenuButton *button = new OsuUIPauseMenuButton(std::move(getImageFunc), 0, 0, 0, 0, "");
	m_container->addBaseUIElement(button);
	m_buttons.push_back(button);
	return button;
}
