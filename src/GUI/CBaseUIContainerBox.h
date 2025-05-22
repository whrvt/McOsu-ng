/*
 * CBaseUIContainerBox.h
 *
 *  Created on: Jun 1, 2017
 *      Author: Psy
 */

#pragma once
#ifndef GUI_CBASEUICONTAINERBOX_H_
#define GUI_CBASEUICONTAINERBOX_H_

#include "CBaseUIContainerBase.h"

class CBaseUIContainerBox : public CBaseUIContainerBase
{
public:
	CBaseUIContainerBox(float xPos=0, float yPos=0, UString name="");
	~CBaseUIContainerBox() override;

	CONTAINER_BODY(CBaseUIContainerBox)

	// inspection
	CBASE_UI_TYPE(CBaseUIContainerBox, CONTAINERBOX, CBaseUIContainerBase)

protected:
	void updateLayout() override;
	void updateElement(CBaseUIElement *element) override;
};

#endif /* GUI_CBASEUICONTAINERBOX_H_ */
