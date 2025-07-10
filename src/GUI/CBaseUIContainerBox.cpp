/*
 * CBaseUIContainerBox.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Psy
 */

#include "CBaseUIContainerBox.h"

#include <utility>

CBaseUIContainerBox::CBaseUIContainerBox(float xPos, float yPos, UString name) : CBaseUIContainerBase(std::move(name))
{
	m_vPos.x = xPos;
	m_vPos.y = yPos;
	m_vmPos = m_vPos;
}

CBaseUIContainerBox::~CBaseUIContainerBox()
{
}

void CBaseUIContainerBox::updateLayout()
{
	if (m_parent != nullptr)
		m_parent->updateLayout();

	for (size_t i=0; i<m_vElements.size(); i++)
	{
		m_vElements[i]->setPosAbsolute(m_vElements[i]->getRelPos() + m_vPos);
	}
}

void CBaseUIContainerBox::updateElement(CBaseUIElement *element)
{
	if (element && element->as<CBaseUIContainerBase>())
	{
		updateLayout();
		return;
	}

	element->setPosAbsolute(element->getRelPos() + m_vPos);
}
