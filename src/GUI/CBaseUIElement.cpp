//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		the base class for all UI Elements
//
// $NoKeywords: $buie
//===============================================================================//

#include "CBaseUIElement.h"

#include <utility>

#include "Engine.h"
#include "Mouse.h"

CBaseUIElement::CBaseUIElement(float xPos, float yPos, float xSize, float ySize, UString name)
{
	// pos, size, name
	m_vPos.x = xPos;
	m_vPos.y = yPos;
	m_vmPos.x = m_vPos.x;
	m_vmPos.y = m_vPos.y;
	m_vSize.x = xSize;
	m_vSize.y = ySize;
	m_vmSize.x = m_vSize.x;
	m_vmSize.y = m_vSize.y;
	m_vAnchor.x = 0;
	m_vAnchor.y = 0;
	m_sName = std::move(name);
	m_parent = nullptr;

	// attributes
	m_bVisible = true;
	m_bActive = false;
	m_bBusy = false;
	m_bEnabled = true;

	m_bKeepActive = false;
	m_bDrawManually = false;
	m_bPositionManually = false;
	m_bMouseInside = false;

	// container options
	m_bScaleByHeightOnly = false;

	m_bMouseInsideCheck = false;
	m_bMouseUpCheck = false;
}

bool CBaseUIElement::isVisibleOnScreen()
{
	if (!isVisible())
		return false;
	const McRect visrect{
	    {0, 0},
        engine->getScreenSize()
    };
	const Vector2 visrectCenter{visrect.getCenter()};
	const Vector2 elemPosNudgedIn{
	    Vector2{m_vPos.x, m_vPos.y}
        .nudge(visrectCenter, -5.0f)
    };
	return visrect.contains(elemPosNudgedIn);
}

void CBaseUIElement::update()
{
	if (!m_bVisible || !m_bEnabled)
		return;

	// check if mouse is inside element
	McRect temp = McRect(m_vPos.x + 1, m_vPos.y + 1, m_vSize.x - 1, m_vSize.y - 1);
	if (temp.contains(mouse->getPos()))
	{
		if (!m_bMouseInside)
		{
			m_bMouseInside = true;
			onMouseInside();
		}
	}
	else
	{
		if (m_bMouseInside)
		{
			m_bMouseInside = false;
			onMouseOutside();
		}
	}

	if (mouse->isLeftDown())
	{
		m_bMouseUpCheck = true;

		// onMouseDownOutside
		if (!m_bMouseInside && !m_bMouseInsideCheck)
		{
			m_bMouseInsideCheck = true;
			onMouseDownOutside();
		}

		// onMouseDownInside
		if (m_bMouseInside && !m_bMouseInsideCheck)
		{
			m_bActive = true;
			m_bMouseInsideCheck = true;
			onMouseDownInside();
		}
	}
	else
	{
		if (m_bMouseUpCheck)
		{
			if (m_bActive)
			{
				if (m_bMouseInside)
					onMouseUpInside();
				else
					onMouseUpOutside();

				if (!m_bKeepActive)
					m_bActive = false;
			}
		}

		m_bMouseInsideCheck = false;
		m_bMouseUpCheck = false;
	}
}
