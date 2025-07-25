//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		smooth kinetic scrolling container
//
// $NoKeywords: $
//===============================================================================//

// TODO: refactor the spaghetti parts, this can be done way more elegantly

#include "CBaseUIScrollView.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "ConVar.h"

#include "CBaseUIContainer.h"
namespace cv {
ConVar ui_scrollview_resistance("ui_scrollview_resistance", 5.0f, FCVAR_NONE, "how many pixels you have to pull before you start scrolling");
ConVar ui_scrollview_scrollbarwidth("ui_scrollview_scrollbarwidth", 15.0f, FCVAR_NONE);
ConVar ui_scrollview_kinetic_energy_multiplier("ui_scrollview_kinetic_energy_multiplier", 24.0f, FCVAR_NONE, "afterscroll delta multiplier");
ConVar ui_scrollview_kinetic_approach_time("ui_scrollview_kinetic_approach_time", 0.075f, FCVAR_NONE, "approach target afterscroll delta over this duration");
ConVar ui_scrollview_mousewheel_multiplier("ui_scrollview_mousewheel_multiplier", 3.5f, FCVAR_NONE);
ConVar ui_scrollview_mousewheel_overscrollbounce("ui_scrollview_mousewheel_overscrollbounce", true, FCVAR_NONE);
}

CBaseUIScrollView::CBaseUIScrollView(float xPos, float yPos, float xSize, float ySize, const UString& name) : CBaseUIElement(xPos, yPos, xSize, ySize, name)
{
	m_bDrawFrame = true;
	m_bDrawBackground = true;
	m_bDrawScrollbars = true;
	m_bClipping = true;

	m_backgroundColor = 0xff000000;
	m_frameColor = 0xffffffff;
	m_frameBrightColor = 0;
	m_frameDarkColor = 0;
	m_scrollbarColor = 0xaaffffff;

	m_fScrollMouseWheelMultiplier = 1.0f;
	m_fScrollbarSizeMultiplier = 1.0f;

	m_bScrolling = false;
	m_bScrollbarScrolling = false;
	m_vScrollPos = Vector2(1, 1);
	m_vVelocity = Vector2(0, 0);
	m_vScrollSize = Vector2(1, 1);

	m_bVerticalScrolling = true;
	m_bHorizontalScrolling = true;
	m_bScrollbarIsVerticalScrolling = false;

	m_bAutoScrollingX = m_bAutoScrollingY = false;
	m_iPrevScrollDeltaX =  0;
	m_bBlockScrolling = false;

	m_bScrollResistanceCheck = false;
	m_iScrollResistance = cv::ui_scrollview_resistance.getInt(); // TODO: dpi handling

	m_container = new CBaseUIContainer(xPos, yPos, xSize, ySize, name);
}

CBaseUIScrollView::~CBaseUIScrollView()
{
	clear();
	SAFE_DELETE(m_container);
}

void CBaseUIScrollView::clear()
{
	m_container->clear();

	anim->deleteExistingAnimation(&m_vKineticAverage.x);
	anim->deleteExistingAnimation(&m_vKineticAverage.y);

	anim->deleteExistingAnimation(&m_vScrollPos.y);
	anim->deleteExistingAnimation(&m_vScrollPos.x);

	anim->deleteExistingAnimation(&m_vVelocity.x);
	anim->deleteExistingAnimation(&m_vVelocity.y);

	m_vScrollSize.x = m_vScrollSize.y = 0;
	m_vScrollPos.x = m_vScrollPos.y = 0;
	m_vVelocity.x = m_vVelocity.y = 0;

	m_container->setPos(m_vPos); // TODO: wtf is this doing here
}

void CBaseUIScrollView::draw()
{
	if (!m_bVisible) return;

	// draw background
	if (m_bDrawBackground)
	{
		g->setColor( m_backgroundColor );
		g->fillRect(m_vPos.x + 1, m_vPos.y + 1, m_vSize.x - 1, m_vSize.y - 1);
	}

	// draw base frame
	if (m_bDrawFrame)
	{
		if (m_frameDarkColor != 0 || m_frameBrightColor != 0)
			g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y, m_frameDarkColor, m_frameBrightColor, m_frameBrightColor, m_frameDarkColor);
		else
		{
			g->setColor(m_frameColor);
			g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
		}
	}

	// draw elements & scrollbars
	if (m_bClipping)
	{
		g->pushClipRect(McRect(m_vPos.x + 1, m_vPos.y + 2, m_vSize.x - 1, m_vSize.y - 1));
	}
	{
		m_container->draw();

		if (m_bDrawScrollbars)
		{
			// vertical
			if (m_bVerticalScrolling && m_vScrollSize.y > m_vSize.y)
			{
				g->setColor(m_scrollbarColor);
				if (((m_bScrollbarScrolling && m_bScrollbarIsVerticalScrolling) || m_verticalScrollbar.contains(mouse->getPos())) && !m_bScrolling)
					g->setAlpha(1.0f);

				g->fillRect(m_verticalScrollbar.getX(), m_verticalScrollbar.getY(), m_verticalScrollbar.getWidth(), m_verticalScrollbar.getHeight());
				//g->fillRoundedRect(m_verticalScrollbar.getX(), m_verticalScrollbar.getY(), m_verticalScrollbar.getWidth(), m_verticalScrollbar.getHeight(), m_verticalScrollbar.getWidth()/2);
			}

			// horizontal
			if (m_bHorizontalScrolling && m_vScrollSize.x > m_vSize.x)
			{
				g->setColor(m_scrollbarColor);
				if (((m_bScrollbarScrolling && !m_bScrollbarIsVerticalScrolling) || m_horizontalScrollbar.contains(mouse->getPos())) && !m_bScrolling)
					g->setAlpha(1.0f);

				g->fillRect(m_horizontalScrollbar.getX(), m_horizontalScrollbar.getY(), m_horizontalScrollbar.getWidth(), m_horizontalScrollbar.getHeight());
				//g->fillRoundedRect(m_horizontalScrollbar.getX(), m_horizontalScrollbar.getY(), m_horizontalScrollbar.getWidth(), m_horizontalScrollbar.getHeight(), m_horizontalScrollbar.getHeight()/2);
			}
		}
	}
	if (m_bClipping)
	{
		g->popClipRect();
	}
}

void CBaseUIScrollView::update()
{
	CBaseUIElement::update();
	if (!m_bVisible) return;

	const bool wasContainerBusyBeforeUpdate = m_container->isBusy();

	m_container->update();

	if (m_bBusy)
	{
		const Vector2 deltaToAdd = (mouse->getPos() - m_vMouseBackup2);
		//debugLog("+ ({:f}, {:f})\n", deltaToAdd.x, deltaToAdd.y);

		anim->moveQuadOut(&m_vKineticAverage.x, deltaToAdd.x, cv::ui_scrollview_kinetic_approach_time.getFloat(), true);
		anim->moveQuadOut(&m_vKineticAverage.y, deltaToAdd.y, cv::ui_scrollview_kinetic_approach_time.getFloat(), true);

		m_vMouseBackup2 = mouse->getPos();
	}

	// scrolling logic
	if (m_bActive && !m_bBlockScrolling && (m_bVerticalScrolling || m_bHorizontalScrolling) && m_bEnabled)
	{
		if (!m_bScrollResistanceCheck)
		{
			m_bScrollResistanceCheck = true;
			m_vMouseBackup3 = mouse->getPos();
		}

		// get pull strength
		int diff = std::abs(mouse->getPos().x - m_vMouseBackup3.x);
		if (std::abs(mouse->getPos().y - m_vMouseBackup3.y) > diff)
			diff = std::abs(mouse->getPos().y - m_vMouseBackup3.y);

		// if we are above our resistance, try to steal the focus and enable scrolling for us
		if (m_container->isActive() && diff > m_iScrollResistance && !m_container->isBusy())
			m_container->stealFocus();

		// handle scrollbar scrolling start
		if (m_verticalScrollbar.contains(mouse->getPos()) && !m_bScrollbarScrolling && !m_bScrolling)
		{
			// NOTE: scrollbar dragging always force steals focus
			if (!wasContainerBusyBeforeUpdate)
			{
				m_container->stealFocus();

				m_vMouseBackup.y = mouse->getPos().y - m_verticalScrollbar.getMaxY();
				m_bScrollbarScrolling = true;
				m_bScrollbarIsVerticalScrolling = true;
			}
		}
		else if (m_horizontalScrollbar.contains(mouse->getPos()) && !m_bScrollbarScrolling && !m_bScrolling)
		{
			// NOTE: scrollbar dragging always force steals focus
			if (!wasContainerBusyBeforeUpdate)
			{
				m_container->stealFocus();

				m_vMouseBackup.x = mouse->getPos().x - m_horizontalScrollbar.getMaxX();
				m_bScrollbarScrolling = true;
				m_bScrollbarIsVerticalScrolling = false;
			}
		}
		else if (!m_bScrolling && !m_bScrollbarScrolling && !m_container->isBusy() && !m_container->isActive())
		{
			if (!m_container->isBusy())
			{
				// if we have successfully stolen the focus or the container is no longer busy, start scrolling
				m_bScrollbarIsVerticalScrolling = false;

				m_vMouseBackup = mouse->getPos();
				m_vScrollPosBackup = m_vScrollPos + (mouse->getPos() - m_vMouseBackup3);
				m_bScrolling = true;
				m_bAutoScrollingX = false;
				m_bAutoScrollingY = false;

				anim->deleteExistingAnimation(&m_vScrollPos.x);
				anim->deleteExistingAnimation(&m_vScrollPos.y);

				anim->deleteExistingAnimation(&m_vVelocity.x);
				anim->deleteExistingAnimation(&m_vVelocity.y);
			}
		}
	}
	else if (m_bScrolling || m_bScrollbarScrolling) // we were scrolling, stop it
	{
		m_bScrolling = false;
		m_bActive = false;

		Vector2 delta = m_vKineticAverage;

		// calculate remaining kinetic energy
		if (!m_bScrollbarScrolling)
			m_vVelocity = cv::ui_scrollview_kinetic_energy_multiplier.getFloat() * delta * (engine->getFrameTime() != 0.0 ? 1.0/engine->getFrameTime() : 60.0)/60.0 + m_vScrollPos;

		//debugLog("kinetic = ({:f}, {:f}), velocity = ({:f}, {:f}), frametime = {:f}\n", delta.x, delta.y, m_vVelocity.x, m_vVelocity.y, engine->getFrameTime());

		m_bScrollbarScrolling = false;
	}
	else
		m_bScrollResistanceCheck = false;

	// handle mouse wheel scrolling
	if (!keyboard->isAltDown() && m_bMouseInside && m_bEnabled)
	{
		if (mouse->getWheelDeltaVertical() != 0)
			scrollY(mouse->getWheelDeltaVertical() * m_fScrollMouseWheelMultiplier * cv::ui_scrollview_mousewheel_multiplier.getFloat());
		if (mouse->getWheelDeltaHorizontal() != 0)
			scrollX(-mouse->getWheelDeltaHorizontal() * m_fScrollMouseWheelMultiplier * cv::ui_scrollview_mousewheel_multiplier.getFloat());
	}

	// handle drag scrolling and rubber banding
	if (m_bScrolling && m_bActive)
	{
		if (m_bVerticalScrolling)
			m_vScrollPos.y = m_vScrollPosBackup.y + (mouse->getPos().y - m_vMouseBackup.y);
		if (m_bHorizontalScrolling)
			m_vScrollPos.x = m_vScrollPosBackup.x + (mouse->getPos().x - m_vMouseBackup.x);

		m_container->setPos(m_vPos + m_vScrollPos);
	}
	else // no longer scrolling, smooth the remaining velocity
	{
		m_vKineticAverage.zero();

		// rubber banding + kinetic scrolling

		// TODO: fix amount being dependent on fps due to double animation time indirection

		// y axis
		if (!m_bAutoScrollingY && m_bVerticalScrolling)
		{
			if (std::round(m_vScrollPos.y) > 1) // rubber banding, top
			{
				anim->moveQuadOut(&m_vVelocity.y, 1, 0.05f, 0.0f, true);
				anim->moveQuadOut(&m_vScrollPos.y, m_vVelocity.y, 0.2f, 0.0f, true);
			}
			else if (std::round(std::abs(m_vScrollPos.y) + m_vSize.y) > m_vScrollSize.y && std::round(m_vScrollPos.y) < 1) // rubber banding, bottom
			{
				anim->moveQuadOut(&m_vVelocity.y, (m_vScrollSize.y > m_vSize.y ? -m_vScrollSize.y : 1) + (m_vScrollSize.y > m_vSize.y ? m_vSize.y : 0), 0.05f, 0.0f, true);
				anim->moveQuadOut(&m_vScrollPos.y, m_vVelocity.y, 0.2f, 0.0f, true);
			}
			else if (std::round(m_vVelocity.y) != 0 && std::round(m_vScrollPos.y) != std::round(m_vVelocity.y)) // kinetic scrolling
				anim->moveQuadOut(&m_vScrollPos.y, m_vVelocity.y, 0.35f, 0.0f, true);
		}

		// x axis
		if (!m_bAutoScrollingX && m_bHorizontalScrolling)
		{
			if (std::round(m_vScrollPos.x) > 1) // rubber banding, left
			{
				anim->moveQuadOut(&m_vVelocity.x, 1, 0.05f, 0.0f, true);
				anim->moveQuadOut(&m_vScrollPos.x, m_vVelocity.x, 0.2f, 0.0f, true);
			}
			else if (std::round(std::abs(m_vScrollPos.x) + m_vSize.x) > m_vScrollSize.x && std::round(m_vScrollPos.x) < 1) // rubber banding, right
			{
				anim->moveQuadOut(&m_vVelocity.x, (m_vScrollSize.x > m_vSize.x ? -m_vScrollSize.x : 1) + (m_vScrollSize.x > m_vSize.x ? m_vSize.x : 0), 0.05f, 0.0f, true);
				anim->moveQuadOut(&m_vScrollPos.x, m_vVelocity.x, 0.2f, 0.0f, true);
			}
			else if (std::round(m_vVelocity.x) != 0 && std::round(m_vScrollPos.x) != std::round(m_vVelocity.x)) // kinetic scrolling
				anim->moveQuadOut(&m_vScrollPos.x, m_vVelocity.x, 0.35f, 0.0f, true);
		}
	}

	// handle scrollbar scrolling movement
	if (m_bScrollbarScrolling)
	{
		m_vVelocity.x = m_vVelocity.y = 0;
		if (m_bScrollbarIsVerticalScrolling)
		{
			const float percent = std::clamp<float>((mouse->getPos().y - m_vPos.y - m_verticalScrollbar.getWidth() - m_verticalScrollbar.getHeight() - m_vMouseBackup.y - 1) / (m_vSize.y - 2*m_verticalScrollbar.getWidth()), 0.0f, 1.0f);
			scrollToYInt(-m_vScrollSize.y*percent, true, false);
		}
		else
		{
			const float percent = std::clamp<float>((mouse->getPos().x - m_vPos.x - m_horizontalScrollbar.getHeight() - m_horizontalScrollbar.getWidth() - m_vMouseBackup.x - 1) / (m_vSize.x - 2*m_horizontalScrollbar.getHeight()), 0.0f, 1.0f);
			scrollToXInt(-m_vScrollSize.x*percent, true, false);
		}
	}

	// position update during scrolling
	if (anim->isAnimating(&m_vScrollPos.y) || anim->isAnimating(&m_vScrollPos.x))
		m_container->setPos(m_vPos.x + std::round(m_vScrollPos.x), m_vPos.y + std::round(m_vScrollPos.y));

	// update scrollbars
	if (anim->isAnimating(&m_vScrollPos.y) || anim->isAnimating(&m_vScrollPos.x) || m_bScrolling || m_bScrollbarScrolling)
		updateScrollbars();

	// HACKHACK: if an animation was started and ended before any setpos could get fired, manually update the position
	if (m_container->getPos() != (m_vPos + Vector2(std::round(m_vScrollPos.x), std::round(m_vScrollPos.y))))
	{
		m_container->setPos(m_vPos.x + std::round(m_vScrollPos.x), m_vPos.y + std::round(m_vScrollPos.y));
		updateScrollbars();
	}

	// only draw visible elements
	updateClipping();
}

void CBaseUIScrollView::onKeyUp(KeyboardEvent &e)
{
	m_container->onKeyUp(e);
}

void CBaseUIScrollView::onKeyDown(KeyboardEvent &e)
{
	m_container->onKeyDown(e);
}

void CBaseUIScrollView::onChar(KeyboardEvent &e)
{
	m_container->onChar(e);
}

void CBaseUIScrollView::scrollY(int delta, bool animated)
{
	if (!m_bVerticalScrolling || delta == 0 || m_bScrolling || m_vSize.y >= m_vScrollSize.y || m_container->isBusy()) return;

	const bool allowOverscrollBounce = cv::ui_scrollview_mousewheel_overscrollbounce.getBool();

	// keep velocity (partially animated/finished scrolls should not get lost, especially multiple scroll() calls in quick succession)
	const float remainingVelocity = m_vScrollPos.y - m_vVelocity.y;
	if (animated && m_bAutoScrollingY)
		delta -= remainingVelocity;

	// calculate new target
	float target = m_vScrollPos.y + delta;
	m_bAutoScrollingY = animated;

	// clamp target
	{
		if (target > 1)
		{
			if (!allowOverscrollBounce)
				target = 1;

			m_bAutoScrollingY = !allowOverscrollBounce;
		}

		if (std::abs(target) + m_vSize.y > m_vScrollSize.y)
		{
			if (!allowOverscrollBounce)
				target = (m_vScrollSize.y > m_vSize.y ? -m_vScrollSize.y : m_vScrollSize.y) + (m_vScrollSize.y > m_vSize.y ? m_vSize.y : 0);

			m_bAutoScrollingY = !allowOverscrollBounce;
		}
	}

	// TODO: fix very slow autoscroll when 1 scroll event goes to >= top or >= bottom
	// TODO: fix overscroll dampening user action when direction flips (while rubber banding)

	// apply target
	anim->deleteExistingAnimation(&m_vVelocity.y);
	if (animated)
	{
		anim->moveQuadOut(&m_vScrollPos.y, target, 0.15f, 0.0f, true);

		m_vVelocity.y = target;
	}
	else
	{
		anim->deleteExistingAnimation(&m_vScrollPos.y);

		m_vScrollPos.y = target;
		m_vVelocity.y = m_vScrollPos.y - remainingVelocity;
	}
}

void CBaseUIScrollView::scrollX(int delta, bool animated)
{
	if (!m_bHorizontalScrolling || delta == 0 || m_bScrolling || m_vSize.x >= m_vScrollSize.x || m_container->isBusy()) return;

	// TODO: fix all of this shit with the code from scrollY() above

	// stop any movement
	if (animated)
		m_vVelocity.x = 0;

	// keep velocity
	if (m_bAutoScrollingX && animated)
		delta += (delta > 0 ? ( m_iPrevScrollDeltaX < 0 ? 0 : std::abs(delta - m_iPrevScrollDeltaX) ) : ( m_iPrevScrollDeltaX > 0 ? 0 : -std::abs(delta - m_iPrevScrollDeltaX) ));

	// calculate target respecting the boundaries
	float target = m_vScrollPos.x + delta;
	if (target > 1)
		target = 1;
	if (std::abs(target)+m_vSize.x > m_vScrollSize.x)
		target = (m_vScrollSize.x > m_vSize.x ? -m_vScrollSize.x : m_vScrollSize.x) + (m_vScrollSize.x > m_vSize.x ? m_vSize.x : 0);

	m_bAutoScrollingX = animated;
	m_iPrevScrollDeltaX = delta;

	if (animated)
		anim->moveQuadOut(&m_vScrollPos.x, target, 0.15f, 0.0f, true);
	else
	{
		const float remainingVelocity = m_vScrollPos.x - m_vVelocity.x;

		m_vScrollPos.x += delta;
		m_vVelocity.x = m_vScrollPos.x - remainingVelocity;

		anim->deleteExistingAnimation(&m_vScrollPos.x);
	}
}

void CBaseUIScrollView::scrollToX(int scrollPosX, bool animated)
{
	scrollToXInt(scrollPosX, animated);
}

void CBaseUIScrollView::scrollToY(int scrollPosY, bool animated)
{
	scrollToYInt(scrollPosY, animated);
}

void CBaseUIScrollView::scrollToYInt(int scrollPosY, bool animated, bool slow)
{
	if (!m_bVerticalScrolling || m_bScrolling) return;

	float upperBounds = 1;
	float lowerBounds = -m_vScrollSize.y + m_vSize.y;
	if (lowerBounds >= upperBounds)
		lowerBounds = upperBounds;

	const float targetY = std::clamp<float>(scrollPosY, lowerBounds, upperBounds);

	m_vVelocity.y = targetY;

	if (animated)
	{
		m_bAutoScrollingY = true;
		anim->moveQuadOut(&m_vScrollPos.y, targetY, (slow ? 0.15f : 0.035f), 0.0f, true);
	}
	else
	{
		anim->deleteExistingAnimation(&m_vScrollPos.y);
		m_vScrollPos.y = targetY;
	}
}

void CBaseUIScrollView::scrollToXInt(int scrollPosX, bool animated, bool slow)
{
	if (!m_bHorizontalScrolling || m_bScrolling) return;

	float upperBounds = 1;
	float lowerBounds = -m_vScrollSize.x + m_vSize.x;
	if (lowerBounds >= upperBounds)
		lowerBounds = upperBounds;

	const float targetX = std::clamp<float>(scrollPosX, lowerBounds, upperBounds);

	m_vVelocity.x = targetX;

	if (animated)
	{
		m_bAutoScrollingX = true;
		anim->moveQuadOut(&m_vScrollPos.x, targetX, (slow ? 0.15f : 0.035f), 0.0f, true);
	}
	else
	{
		anim->deleteExistingAnimation(&m_vScrollPos.x);
		m_vScrollPos.x = targetX;
	}
}

void CBaseUIScrollView::scrollToElement(CBaseUIElement *element, int  /*xOffset*/, int yOffset, bool animated)
{
	const std::vector<CBaseUIElement*> &elements = m_container->getElements();
	for (size_t i=0; i<elements.size(); i++)
	{
		if (elements[i] == element)
		{
			scrollToY(-element->getRelPos().y + yOffset, animated);
			return;
		}
	}
}

void CBaseUIScrollView::updateClipping()
{
	const std::vector<CBaseUIElement*> &elements = m_container->getElements();
	const McRect me = McRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);

	for (size_t i=0; i<elements.size(); i++)
	{
		CBaseUIElement *e = elements[i];

		const McRect elementBounds = McRect(e->getPos().x, e->getPos().y, e->getSize().x, e->getSize().y);
		if (me.intersects(elementBounds))
		{
			if (!e->isVisible())
				e->setVisible(true);
		}
		else if (e->isVisible())
			e->setVisible(false);
	}
}

void CBaseUIScrollView::updateScrollbars()
{
	// update vertical scrollbar
	if (m_bVerticalScrolling && m_vScrollSize.y > m_vSize.y)
	{
		const float verticalBlockWidth = cv::ui_scrollview_scrollbarwidth.getInt();

		const float rawVerticalPercent = (m_vScrollPos.y > 0 ? -m_vScrollPos.y : std::abs(m_vScrollPos.y)) / (m_vScrollSize.y - m_vSize.y);
		float overscroll = 1.0f;
		if (rawVerticalPercent > 1.0f)
			overscroll = 1.0f - (rawVerticalPercent - 1.0f) * 0.95f;
		else if (rawVerticalPercent < 0.0f)
			overscroll = 1.0f - std::abs(rawVerticalPercent) * 0.95f;

		const float verticalPercent = std::clamp<float>(rawVerticalPercent, 0.0f, 1.0f);

		const float verticalHeightPercent = (m_vSize.y - (verticalBlockWidth * 2)) / m_vScrollSize.y;
		const float verticalBlockHeight = std::clamp<float>(std::max(verticalHeightPercent * m_vSize.y, verticalBlockWidth) * overscroll, verticalBlockWidth, m_vSize.y);

		m_verticalScrollbar = McRect(m_vPos.x + m_vSize.x - (verticalBlockWidth * m_fScrollbarSizeMultiplier), m_vPos.y + (verticalPercent * (m_vSize.y - (verticalBlockWidth * 2) - verticalBlockHeight) + verticalBlockWidth + 1), (verticalBlockWidth * m_fScrollbarSizeMultiplier), verticalBlockHeight);
	}

	// update horizontal scrollbar
	if (m_bHorizontalScrolling && m_vScrollSize.x > m_vSize.x)
	{
		const float horizontalPercent = std::clamp<float>((m_vScrollPos.x > 0 ? -m_vScrollPos.x : std::abs(m_vScrollPos.x)) / (m_vScrollSize.x - m_vSize.x), 0.0f, 1.0f);
		const float horizontalBlockWidth = cv::ui_scrollview_scrollbarwidth.getInt();
		const float horizontalHeightPercent = (m_vSize.x - (horizontalBlockWidth * 2)) / m_vScrollSize.x;
		const float horizontalBlockHeight = std::max(horizontalHeightPercent * m_vSize.x, horizontalBlockWidth);

		m_horizontalScrollbar = McRect(m_vPos.x + (horizontalPercent * (m_vSize.x - (horizontalBlockWidth * 2) - horizontalBlockHeight) + horizontalBlockWidth + 1), m_vPos.y + m_vSize.y - horizontalBlockWidth, horizontalBlockHeight, horizontalBlockWidth);
	}
}

CBaseUIScrollView *CBaseUIScrollView::setScrollSizeToContent(int border)
{
	m_vScrollSize.zero();

	const std::vector<CBaseUIElement*> &elements = m_container->getElements();
	for (size_t i=0; i<elements.size(); i++)
	{
		const CBaseUIElement *e = elements[i];

		const float x = e->getRelPos().x + e->getSize().x;
		const float y = e->getRelPos().y + e->getSize().y;

		if (x > m_vScrollSize.x)
			m_vScrollSize.x = x;
		if (y > m_vScrollSize.y)
			m_vScrollSize.y = y;
	}

	m_vScrollSize.x += border;
	m_vScrollSize.y += border;

	m_container->setSize(m_vScrollSize);

	// TODO: duplicate code, ref onResized(), but can't call onResized() due to possible endless recursion if setScrollSizeToContent() within onResized()
	// HACKHACK: shit code
	if (m_bVerticalScrolling && m_vScrollSize.y < m_vSize.y && m_vScrollPos.y != 1)
		scrollToY(1);
	if (m_bHorizontalScrolling && m_vScrollSize.x < m_vSize.x && m_vScrollPos.x != 1)
		scrollToX(1);

	updateScrollbars();

	return this;
}

void CBaseUIScrollView::scrollToLeft()
{
	scrollToX(0);
}

void CBaseUIScrollView::scrollToRight()
{
	scrollToX(-m_vScrollSize.x);
}

void CBaseUIScrollView::scrollToBottom()
{
	scrollToY(-m_vScrollSize.y);
}

void CBaseUIScrollView::scrollToTop()
{
	scrollToY(0);
}

void CBaseUIScrollView::onMouseDownOutside()
{
	m_container->stealFocus();
}

void CBaseUIScrollView::onMouseDownInside()
{
	m_bBusy = true;

	m_vMouseBackup2 = mouse->getPos(); // to avoid spastic movement at scroll start
}

void CBaseUIScrollView::onMouseUpInside()
{
	m_bBusy = false;
}

void CBaseUIScrollView::onMouseUpOutside()
{
	m_bBusy = false;
}

void CBaseUIScrollView::onFocusStolen()
{
	m_bActive = false;
	m_bScrolling = false;
	m_bScrollbarScrolling = false;
	m_bBusy = false;

	// forward focus steal to container
	m_container->stealFocus();
}

void CBaseUIScrollView::onEnabled()
{
	m_container->setEnabled(true);
}

void CBaseUIScrollView::onDisabled()
{
	m_bActive = false;
	m_bScrolling = false;
	m_bScrollbarScrolling = false;
	m_bBusy = false;

	m_container->setEnabled(false);
}

void CBaseUIScrollView::onResized()
{
	m_container->setSize(m_vScrollSize);

	// TODO: duplicate code
	// HACKHACK: shit code
	if (m_bVerticalScrolling && m_vScrollSize.y < m_vSize.y && m_vScrollPos.y != 1)
		scrollToY(1);
	if (m_bHorizontalScrolling && m_vScrollSize.x < m_vSize.x && m_vScrollPos.x != 1)
		scrollToX(1);

	updateScrollbars();
}

void CBaseUIScrollView::onMoved()
{
	m_container->setPos(m_vPos + m_vScrollPos);

	m_vMouseBackup2 = mouse->getPos(); // to avoid spastic movement after we are moved

	updateScrollbars();
}

bool CBaseUIScrollView::isBusy()
{
	return (m_container->isBusy() || m_bScrolling || m_bBusy) && m_bVisible;
}

