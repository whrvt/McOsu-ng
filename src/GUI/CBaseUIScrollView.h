//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		smooth kinetic scrolling container
//
// $NoKeywords: $
//===============================================================================//

// TODO: refactor the spaghetti parts, this can be done way more elegantly

#pragma once
#ifndef CBASEUISCROLLVIEW_H
#define CBASEUISCROLLVIEW_H

#include "CBaseUIElement.h"

class CBaseUIContainer;

class CBaseUIScrollView : public CBaseUIElement
{
public:
	CBaseUIScrollView(float xPos=0, float yPos=0, float xSize=0, float ySize=0, const UString& name="");
	~CBaseUIScrollView() override;

	void clear();

	ELEMENT_BODY(CBaseUIScrollView)

	void draw() override;
	void update() override;

	void onKeyUp(KeyboardEvent &e) override;
	void onKeyDown(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	// scrolling
	void scrollY(int delta, bool animated = true);
	void scrollX(int delta, bool animated = true);
	void scrollToY(int scrollPosY, bool animated = true);
	void scrollToX(int scrollPosX, bool animated = true);
	void scrollToElement(CBaseUIElement *element, int xOffset = 0, int yOffset = 0, bool animated = true);

	void scrollToLeft();
	void scrollToRight();
	void scrollToBottom();
	void scrollToTop();

	// set
	CBaseUIScrollView *setDrawBackground(bool drawBackground) {m_bDrawBackground = drawBackground; return this;}
	CBaseUIScrollView *setDrawFrame(bool drawFrame) {m_bDrawFrame = drawFrame; return this;}
	CBaseUIScrollView *setDrawScrollbars(bool drawScrollbars) {m_bDrawScrollbars = drawScrollbars; return this;}
	CBaseUIScrollView *setClipping(bool clipping) {m_bClipping = clipping; return this;}

	CBaseUIScrollView *setBackgroundColor(Color backgroundColor) {m_backgroundColor = backgroundColor; return this;}
	CBaseUIScrollView *setFrameColor(Color frameColor) {m_frameColor = frameColor; return this;}
	CBaseUIScrollView *setFrameBrightColor(Color frameBrightColor) {m_frameBrightColor = frameBrightColor; return this;}
	CBaseUIScrollView *setFrameDarkColor(Color frameDarkColor) {m_frameDarkColor = frameDarkColor; return this;}
	CBaseUIScrollView *setScrollbarColor(Color scrollbarColor) {m_scrollbarColor = scrollbarColor; return this;}

	CBaseUIScrollView *setHorizontalScrolling(bool horizontalScrolling) {m_bHorizontalScrolling = horizontalScrolling; return this;}
	CBaseUIScrollView *setVerticalScrolling(bool verticalScrolling) {m_bVerticalScrolling = verticalScrolling; return this;}
	CBaseUIScrollView *setScrollSizeToContent(int border = 5);
	CBaseUIScrollView *setScrollResistance(int scrollResistanceInPixels) {m_iScrollResistance = scrollResistanceInPixels; return this;}

	CBaseUIScrollView *setBlockScrolling(bool block) {m_bBlockScrolling = block; return this;} // means: disable scrolling, not scrolling in 'blocks'

	void setScrollMouseWheelMultiplier(float scrollMouseWheelMultiplier) {m_fScrollMouseWheelMultiplier = scrollMouseWheelMultiplier;}
	void setScrollbarSizeMultiplier(float scrollbarSizeMultiplier) {m_fScrollbarSizeMultiplier = scrollbarSizeMultiplier;}

	// get
	[[nodiscard]] inline CBaseUIContainer *getContainer() const {return m_container;}
	[[nodiscard]] inline float getScrollPosY() const {return m_vScrollPos.y;}
	[[nodiscard]] inline float getScrollPosX() const {return m_vScrollPos.x;}
	[[nodiscard]] inline Vector2 getScrollSize() const {return m_vScrollSize;}
	[[nodiscard]] inline Vector2 getVelocity() const {return (m_vScrollPos - m_vVelocity);}

	[[nodiscard]] inline bool isScrolling() const {return m_bScrolling;}
	bool isBusy() override;

	// events
	void onResized() override;
	void onMouseDownOutside() override;
	void onMouseDownInside() override;
	void onMouseUpInside() override;
	void onMouseUpOutside() override;

	void onFocusStolen() override;
	void onEnabled() override;
	void onDisabled() override;

	// inspection
	CBASE_UI_TYPE(CBaseUIScrollView, SCROLLVIEW, CBaseUIElement)
protected:
	void onMoved() override;

private:
	void updateClipping();
	void updateScrollbars();

	void scrollToYInt(int scrollPosY, bool animated = true, bool slow = true);
	void scrollToXInt(int scrollPosX, bool animated = true, bool slow = true);

	// main container
	CBaseUIContainer *m_container;

	// vars
	bool m_bDrawFrame;
	bool m_bDrawBackground;
	bool m_bDrawScrollbars;
	bool m_bClipping;

	Color m_backgroundColor;
	Color m_frameColor;
	Color m_frameBrightColor;
	Color m_frameDarkColor;
	Color m_scrollbarColor;

	Vector2 m_vScrollPos;
	Vector2 m_vScrollPosBackup;
	Vector2 m_vMouseBackup;

	float m_fScrollMouseWheelMultiplier;
	float m_fScrollbarSizeMultiplier;
	McRect m_verticalScrollbar;
	McRect m_horizontalScrollbar;

	// scroll logic
	bool m_bScrolling;
	bool m_bScrollbarScrolling;
	bool m_bScrollbarIsVerticalScrolling;
	bool m_bBlockScrolling;
	bool m_bHorizontalScrolling;
	bool m_bVerticalScrolling;
	Vector2 m_vScrollSize;
	Vector2 m_vMouseBackup2;
	Vector2 m_vMouseBackup3;
	Vector2 m_vVelocity;
	Vector2 m_vKineticAverage;

	bool m_bAutoScrollingX;
	bool m_bAutoScrollingY;
	int m_iPrevScrollDeltaX;

	bool m_bScrollResistanceCheck;
	int m_iScrollResistance;
};

#endif
