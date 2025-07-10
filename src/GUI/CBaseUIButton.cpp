//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		a simple button
//
// $NoKeywords: $button
//===============================================================================//

#include "CBaseUIButton.h"

#include <utility>

#include "Engine.h"
#include "Mouse.h"
#include "ResourceManager.h"

CBaseUIButton::CBaseUIButton(float xPos, float yPos, float xSize, float ySize, UString name, const UString& text) : CBaseUIElement(xPos, yPos, xSize, ySize, std::move(name))
{
	m_font = resourceManager->getFont("FONT_DEFAULT");

	// settings
	m_bDrawFrame = true;
	m_bDrawBackground = true;
	m_bTextLeft = false;

	// colors
	m_frameColor = rgb(255, 255, 255);
	m_backgroundColor = rgb(0, 0, 0);
	m_textColor = rgb(255, 255, 255);
	m_textBrightColor = m_textDarkColor = argb(0, 0, 0, 0);

	setText(text);
}

void CBaseUIButton::draw()
{
	if (!m_bVisible || !isVisibleOnScreen()) return;

	// draw background
	if (m_bDrawBackground)
	{
		g->setColor(m_backgroundColor);
		g->fillRect(m_vPos.x + 1, m_vPos.y + 1, m_vSize.x - 1, m_vSize.y - 1);
	}

	// draw frame
	if (m_bDrawFrame)
	{
		/*
		///const float lineThickness = std::round((float)m_font->getDPI() / 96.0f); // NOTE: abusing font dpi
		const float lineThickness = 9.0f;

		const float lineOffset = std::floor(lineThickness / 2.0f);

		g->setColor(m_frameColor);
		g->setAlpha(0.5f);
		g->fillRect(m_vPos.x - lineOffset, m_vPos.y - lineOffset, lineThickness, m_vSize.y);
		g->fillRect(m_vPos.x + lineThickness - lineOffset, m_vPos.y - lineOffset, m_vSize.x - lineThickness, lineThickness);
		g->fillRect(m_vPos.x + m_vSize.x - lineOffset, m_vPos.y - lineOffset, lineThickness, m_vSize.y);
		g->fillRect(m_vPos.x - lineOffset, m_vPos.y + m_vSize.y - lineOffset, m_vSize.x + lineThickness, lineThickness);
		*/

		g->setColor(m_frameColor);
		g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
	}

	// draw hover rects
	const int hoverRectOffset = std::round(3.0f * ((float)m_font->getDPI() / 96.0f)); // NOTE: abusing font dpi
	g->setColor(m_frameColor);
	if (m_bMouseInside && m_bEnabled && (m_bActive || !mouse->isLeftDown()))
		drawHoverRect(hoverRectOffset);

	if (m_bActive && m_bEnabled)
		drawHoverRect(hoverRectOffset * 2);

	// draw text
	drawText();
}

void CBaseUIButton::drawText()
{
	if (m_font == NULL || !isVisibleOnScreen() || m_sText.length() < 1) return;
	//debugLog("busy {} visible {} isVisible {} x {:.2f} y {:.2f} w {:.2f} h {:.2f}\n", m_bBusy, m_bVisible, isVisible(), m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
	const int shadowOffset = std::round(1.0f * ((float)m_font->getDPI() / 96.0f)); // NOTE: abusing font dpi

	g->pushClipRect(McRect(m_vPos.x + 1, m_vPos.y + 1, m_vSize.x - 1, m_vSize.y - 1));
	{
		g->setColor(m_textColor);
		g->pushTransform();
		{
			g->translate((int)(m_vPos.x + m_vSize.x/2.0f - m_fStringWidth/2.0f), (int)(m_vPos.y + m_vSize.y/2.0f + m_fStringHeight/2.0f));

			// shadow
			g->translate(shadowOffset, shadowOffset);
			{
				if (m_textDarkColor != 0)
					g->setColor(m_textDarkColor);
				else
					g->setColor(Colors::invert(m_textColor));
			}
			g->drawString(m_font, m_sText);

			// top
			g->translate(-shadowOffset, -shadowOffset);
			{
				if (m_textBrightColor != 0)
					g->setColor(m_textBrightColor);
				else
					g->setColor(m_textColor);
			}
			g->drawString(m_font, m_sText);
		}
		g->popTransform();
	}
	g->popClipRect();
}

void CBaseUIButton::drawHoverRect(int distance)
{
	g->drawLine(m_vPos.x, m_vPos.y - distance, m_vPos.x + m_vSize.x + 1, m_vPos.y - distance);
	g->drawLine(m_vPos.x, m_vPos.y + m_vSize.y + distance, m_vPos.x + m_vSize.x + 1, m_vPos.y + m_vSize.y + distance);
	g->drawLine(m_vPos.x - distance, m_vPos.y, m_vPos.x - distance, m_vPos.y + m_vSize.y + 1);
	g->drawLine(m_vPos.x + m_vSize.x + distance, m_vPos.y, m_vPos.x + m_vSize.x + distance, m_vPos.y + m_vSize.y + 1);
}

void CBaseUIButton::onMouseUpInside()
{
	onClicked();
}

void CBaseUIButton::onClicked()
{
	if (m_clickCallback != NULL)
		m_clickCallback(this);

	if (m_clickVoidCallback != NULL)
		m_clickVoidCallback();
}

void CBaseUIButton::updateStringMetrics()
{
	if (m_font != NULL)
	{
		m_fStringHeight = m_font->getHeight();

		if (m_bTextLeft)
			m_fStringWidth = m_vSize.x - 4; // TODO: this is broken af, why is it like this? where is this even used/needed
		else
			m_fStringWidth = m_font->getStringWidth(m_sText);
	}
}

