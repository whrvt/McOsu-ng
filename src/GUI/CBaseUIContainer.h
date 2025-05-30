//================ Copyright (c) 2011, PG, All rights reserved. =================//
//
// Purpose:		a container for UI elements
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef CBASEUICONTAINER_H
#define CBASEUICONTAINER_H

#include "CBaseUIElement.h"

class CBaseUIContainer : public CBaseUIElement
{
public:
	CBaseUIContainer(float xPos=0, float yPos=0, float xSize=0, float ySize=0, UString name="");
	~CBaseUIContainer() override;

	ELEMENT_BODY(CBaseUIContainer)

	void clear();
	void empty();

	void draw_debug();
	void draw() override;
	void update() override;

	void onKeyUp(KeyboardEvent &e) override;
	void onKeyDown(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	CBaseUIContainer *addBaseUIElement(CBaseUIElement *element, float xPos, float yPos);
	CBaseUIContainer *addBaseUIElement(CBaseUIElement *element);
	CBaseUIContainer *addBaseUIElementBack(CBaseUIElement *element, float xPos, float yPos);
	CBaseUIContainer *addBaseUIElementBack(CBaseUIElement *element);

	CBaseUIContainer *insertBaseUIElement(CBaseUIElement *element, CBaseUIElement *index);
	CBaseUIContainer *insertBaseUIElementBack(CBaseUIElement *element, CBaseUIElement *index);

	CBaseUIContainer *removeBaseUIElement(CBaseUIElement *element);
	CBaseUIContainer *deleteBaseUIElement(CBaseUIElement *element);

	CBaseUIElement *getBaseUIElement(UString name);

	[[nodiscard]] inline const std::vector<CBaseUIElement*> &getElements() const {return m_vElements;}

	void onMoved() override {update_pos();}
	void onResized() override {update_pos();}

	bool isBusy() override;
	bool isActive() override;

	void onMouseDownOutside() override;

	void onFocusStolen() override;
	void onEnabled() override;
	void onDisabled() override;

	void update_pos();
	void update_pos(CBaseUIElement *element);

	// inspection
	CBASE_UI_TYPE(CBaseUIContainer, CONTAINER, CBaseUIElement)
protected:
	std::vector<CBaseUIElement*> m_vElements;
};

#endif
