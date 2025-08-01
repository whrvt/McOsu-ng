//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		a container for UI elements
//
// $NoKeywords: $
//===============================================================================//

#include "CBaseUIContainer.h"

#include <utility>
#include "Engine.h"

CBaseUIContainer::CBaseUIContainer(float Xpos, float Ypos, float Xsize, float Ysize, UString name) : CBaseUIElement(Xpos, Ypos, Xsize, Ysize, std::move(name))
{
}

CBaseUIContainer::~CBaseUIContainer()
{
	clear();
}

void CBaseUIContainer::clear()
{
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		SAFE_DELETE(m_vElements[i]);
	}
	m_vElements = std::vector<CBaseUIElement*>();
}

void CBaseUIContainer::empty()
{
	m_vElements = std::vector<CBaseUIElement*>();
}

CBaseUIContainer *CBaseUIContainer::addBaseUIElement(CBaseUIElement *element, float xPos, float yPos)
{
	if (element == NULL) return this;

	element->setRelPos(xPos, yPos);
	element->setPos(m_vPos + element->getRelPos());
	m_vElements.push_back(element);

	return this;
}

CBaseUIContainer *CBaseUIContainer::addBaseUIElement(CBaseUIElement *element)
{
	if (element == NULL) return this;

	element->setRelPos(element->getPos().x, element->getPos().y);
	element->setPos(m_vPos + element->getRelPos());
	m_vElements.push_back(element);

	return this;
}

CBaseUIContainer *CBaseUIContainer::addBaseUIElementBack(CBaseUIElement *element, float xPos, float yPos)
{
	if (element == NULL) return this;

	element->setRelPos(xPos, yPos);
	element->setPos(m_vPos + element->getRelPos());
	m_vElements.insert(m_vElements.begin(), element);

	return this;
}


CBaseUIContainer *CBaseUIContainer::addBaseUIElementBack(CBaseUIElement *element)
{
	if (element == NULL) return this;

	element->setRelPos(element->getPos().x, element->getPos().y);
	element->setPos(m_vPos + element->getRelPos());
	m_vElements.insert(m_vElements.begin(), element);

	return this;
}

CBaseUIContainer *CBaseUIContainer::insertBaseUIElement(CBaseUIElement *element, CBaseUIElement *index)
{
	if (element == NULL || index == NULL) return this;

	element->setRelPos(element->getPos().x, element->getPos().y);
	element->setPos(m_vPos + element->getRelPos());
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (m_vElements[i] == index)
		{
			m_vElements.insert(m_vElements.begin() + std::clamp<int>(i, 0, m_vElements.size()), element);
			return this;
		}
	}

	debugLog("Warning: couldn't find index\n");

	return this;
}

CBaseUIContainer *CBaseUIContainer::insertBaseUIElementBack(CBaseUIElement *element, CBaseUIElement *index)
{
	if (element == NULL || index == NULL) return this;

	element->setRelPos(element->getPos().x, element->getPos().y);
	element->setPos(m_vPos + element->getRelPos());
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (m_vElements[i] == index)
		{
			m_vElements.insert(m_vElements.begin() + std::clamp<int>(i+1, 0, m_vElements.size()), element);
			return this;
		}
	}

	debugLog("Warning: couldn't find index\n");

	return this;
}

CBaseUIContainer *CBaseUIContainer::removeBaseUIElement(CBaseUIElement *element)
{
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (m_vElements[i] == element)
		{
			m_vElements.erase(m_vElements.begin()+i);
			return this;
		}
	}

	debugLog("Warning: couldn't find element\n");

	return this;
}

CBaseUIContainer *CBaseUIContainer::deleteBaseUIElement(CBaseUIElement *element)
{
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (m_vElements[i] == element)
		{
			SAFE_DELETE(element);
			m_vElements.erase(m_vElements.begin()+i);
			return this;
		}
	}

	debugLog("Warning: couldn't find element\n");

	return this;
}

CBaseUIElement *CBaseUIContainer::getBaseUIElement(const UString& name)
{
	MC_UNROLL
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (m_vElements[i]->getName() == name)
			return m_vElements[i];
	}
	debugLog("CBaseUIContainer ERROR: GetBaseUIElement() \"{:s}\" does not exist!!!\n",name.toUtf8());
	return NULL;
}

void CBaseUIContainer::draw()
{
	if (!m_bVisible) return;

	MC_UNROLL
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (!m_vElements[i]->isDrawnManually())
			m_vElements[i]->draw();
	}
}

void CBaseUIContainer::draw_debug()
{
	g->setColor(0xffffffff);
	g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y);
	g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x, m_vPos.y+m_vSize.y);
	g->drawLine(m_vPos.x, m_vPos.y+m_vSize.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
	g->drawLine(m_vPos.x+m_vSize.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
}

void CBaseUIContainer::update()
{
	CBaseUIElement::update();
	if (!m_bVisible) return;

	MC_UNROLL
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		m_vElements[i]->update();
	}
}

void CBaseUIContainer::update_pos()
{
	MC_UNROLL
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (!m_vElements[i]->isPositionedManually())
			m_vElements[i]->setPos(m_vPos + m_vElements[i]->getRelPos());
	}
}

void CBaseUIContainer::update_pos(CBaseUIElement *element)
{
	if (element != NULL)
		element->setPos(m_vPos + element->getRelPos());
}

void CBaseUIContainer::onKeyUp(KeyboardEvent &e)
{
	for (auto & m_vElement : m_vElements)
	{
		if (m_vElement->isVisible())
			m_vElement->onKeyUp(e);
	}
}
void CBaseUIContainer::onKeyDown(KeyboardEvent &e)
{
	for (auto & m_vElement : m_vElements)
	{
		if (m_vElement->isVisible())
			m_vElement->onKeyDown(e);
	}
}

void CBaseUIContainer::onChar(KeyboardEvent &e)
{
	for (auto & m_vElement : m_vElements)
	{
		if (m_vElement->isVisible())
			m_vElement->onChar(e);
	}
}

void CBaseUIContainer::onFocusStolen()
{
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		m_vElements[i]->stealFocus();
	}
}

void CBaseUIContainer::onEnabled()
{
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		m_vElements[i]->setEnabled(true);
	}
}

void CBaseUIContainer::onDisabled()
{
	for (size_t i=0; i<m_vElements.size(); i++)
	{
		m_vElements[i]->setEnabled(false);
	}
}

void CBaseUIContainer::onMouseDownOutside()
{
	onFocusStolen();
}

bool CBaseUIContainer::isBusy()
{
	if (!m_bVisible)
		return false;

	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (m_vElements[i]->isBusy())
			return true;
	}

	return false;
}

bool CBaseUIContainer::isActive()
{
	if (!m_bVisible)
		return false;

	for (size_t i=0; i<m_vElements.size(); i++)
	{
		if (m_vElements[i]->isActive())
			return true;
	}

	return false;
}
