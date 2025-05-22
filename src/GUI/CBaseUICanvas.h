/*
 * CBaseUICanvas.h
 *
 *  Created on: May 28, 2017
 *      Author: Psy
 */

#pragma once
#ifndef GUI_WINDOWS_CBASEUICANVAS_H_
#define GUI_WINDOWS_CBASEUICANVAS_H_

#include "cbase.h"
#include "CBaseUIContainerBase.h"

/*
 * UI Canvas Container
 * Scales any slotted containers or elements by the size of the canvas, useful for resolution scaling
 * The size/position of UI elements slotted should 0.0 to 1.0 as a percentage of the total screen area
 * Set scaleByHeightOnly per element to avoid stretching/squashing on aspect ratio changes. Uses a 16:9 (Widescreen) aspect ratio for assumed desired width
 */

class CBaseUICanvas : public CBaseUIContainerBase
{
public:
	CBaseUICanvas(float xPos=0, float yPos=0, float xSize=0, float ySize=0, UString name="");
	~CBaseUICanvas() override;

	CONTAINER_BODY(CBaseUICanvas)

	// main
	virtual void drawDebug(Graphics *g, Color color=rgb(255,0,0));

	// inspection
	CBASE_UI_TYPE(CBaseUICanvas, CANVAS, CBaseUIContainerBase)
protected:
	// events
	void onMoved() override;
	void onResized() override;
	void updateLayout() override;
	void updateElement(CBaseUIElement *element) override;
};

#endif /* GUI_WINDOWS_CBASEUICANVAS_H_ */
