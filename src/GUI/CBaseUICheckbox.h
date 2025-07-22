//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		a simple checkbox
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef CBASEUICHECKBOX_H
#define CBASEUICHECKBOX_H

#include "CBaseUIButton.h"

class CBaseUICheckbox : public CBaseUIButton
{
public:
	CBaseUICheckbox(float xPos=0, float yPos=0, float xSize=0, float ySize=0, UString name="", const UString& text="");
	~CBaseUICheckbox() override {;}

	ELEMENT_BODY(CBaseUICheckbox)

	void draw() override;

	[[nodiscard]] inline float getBlockSize() const {return m_vSize.y/2;}
	[[nodiscard]] inline float getBlockBorder() const {return m_vSize.y/4;}
	[[nodiscard]] inline bool isChecked() const {return m_bChecked;}

	CBaseUICheckbox *setChecked(bool checked, bool fireChangeEvent = true);
	CBaseUICheckbox *setSizeToContent(int horizontalBorderSize = 1, int verticalBorderSize = 1);
	CBaseUICheckbox *setWidthToContent(int horizontalBorderSize = 1);

	using CheckboxChangeCallback = SA::delegate<void(CBaseUICheckbox*)> ;
	CBaseUICheckbox *setChangeCallback( const CheckboxChangeCallback& clickCallback ) {m_changeCallback = clickCallback; return this;}

	// inspection
	CBASE_UI_TYPE(CBaseUICheckbox, CHECKBOX, CBaseUIButton)
protected:
	virtual void onPressed();

	bool m_bChecked;
	CheckboxChangeCallback m_changeCallback;
};


#endif
