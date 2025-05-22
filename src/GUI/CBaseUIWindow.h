//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		base class for windows
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef CBASEUIWINDOW_H
#define CBASEUIWINDOW_H

#include "CBaseUIElement.h"

class CBaseUIButton;
class CBaseUIContainer;
class CBaseUIBoxShadow;

class RenderTarget;

class CBaseUIWindow : public CBaseUIElement
{
public:
	CBaseUIWindow(float xPos=0, float yPos=0, float xSize=0, float ySize=0, UString name="");
	~CBaseUIWindow() override;

	ELEMENT_BODY(CBaseUIWindow)

	void draw(Graphics *g) override;
	virtual void drawCustomContent(Graphics *) {;}
	void update() override;

	void onKeyDown(KeyboardEvent &e) override;
	void onKeyUp(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	// actions
	void close();
	void open();

	void minimize();

	// BETA: mimic native window
	CBaseUIWindow *enableCoherenceMode();

	// set
	CBaseUIWindow *setSizeToContent(int horizontalBorderSize = 1, int verticalBorderSize = 1);
	CBaseUIWindow *setTitleBarHeight(int height) {m_iTitleBarHeight = height; updateTitleBarMetrics(); return this;}
	CBaseUIWindow *setTitle(UString text);
	CBaseUIWindow *setTitleFont(McFont *titleFont) {m_titleFont = titleFont; updateTitleBarMetrics(); return this;}
	CBaseUIWindow *setResizeLimit(int maxWidth, int maxHeight) {m_vResizeLimit = Vector2(maxWidth, maxHeight); return this;}
	CBaseUIWindow *setResizeable(bool resizeable) {m_bResizeable = resizeable; return this;}
	CBaseUIWindow *setDrawTitleBarLine(bool drawTitleBarLine) {m_bDrawTitleBarLine = drawTitleBarLine; return this;}
	CBaseUIWindow *setDrawFrame(bool drawFrame) {m_bDrawFrame = drawFrame; return this;}
	CBaseUIWindow *setDrawBackground(bool drawBackground) {m_bDrawBackground = drawBackground; return this;}
	CBaseUIWindow *setRoundedRectangle(bool roundedRectangle) {m_bRoundedRectangle = roundedRectangle; return this;}

	CBaseUIWindow *setBackgroundColor(Color backgroundColor) {m_backgroundColor = backgroundColor; return this;}
	CBaseUIWindow *setFrameColor(Color frameColor) {m_frameColor = frameColor; return this;}
	CBaseUIWindow *setFrameBrightColor(Color frameBrightColor) {m_frameBrightColor = frameBrightColor; return this;}
	CBaseUIWindow *setFrameDarkColor(Color frameDarkColor) {m_frameDarkColor = frameDarkColor; return this;}
	CBaseUIWindow *setTitleColor(Color titleColor) {m_titleColor = titleColor; return this;}

	// get
	bool isBusy() override;
	bool isActive() override;
	[[nodiscard]] inline bool isMoving() const {return m_bMoving;}
	[[nodiscard]] inline bool isResizing() const {return m_bResizing;}
	[[nodiscard]] inline CBaseUIContainer *getContainer() const {return m_container;}
	[[nodiscard]] inline CBaseUIContainer *getTitleBarContainer() const {return m_titleBarContainer;}
	inline int getTitleBarHeight() {return m_iTitleBarHeight;}

	// events
	void onMouseDownInside() override;
	void onMouseUpInside() override;
	void onMouseUpOutside() override;

	void onMoved() override;
	void onResized() override;

	virtual void onResolutionChange(Vector2 newResolution);

	void onEnabled() override;
	void onDisabled() override;

	// inspection
	CBASE_UI_TYPE(CBaseUIWindow, WINDOW, CBaseUIElement)

protected:
	void updateTitleBarMetrics();
	void udpateResizeAndMoveLogic(bool captureMouse);
	void updateWindowLogic();

	virtual void onClosed();

	inline CBaseUIButton *getCloseButton() {return m_closeButton;}
	inline CBaseUIButton *getMinimizeButton() {return m_minimizeButton;}

private:
	// colors
	Color m_frameColor;
	Color m_frameBrightColor;
	Color m_frameDarkColor;
	Color m_backgroundColor;
	Color m_titleColor;

	// window properties
	bool m_bIsOpen;
	bool m_bAnimIn;
	bool m_bResizeable;
	bool m_bCoherenceMode;
	float m_fAnimation;

	bool m_bDrawFrame;
	bool m_bDrawBackground;
	bool m_bRoundedRectangle;

	// title bar
	bool m_bDrawTitleBarLine;
	CBaseUIContainer *m_titleBarContainer;
	McFont *m_titleFont;
	float m_fTitleFontWidth;
	float m_fTitleFontHeight;
	int m_iTitleBarHeight;
	UString m_sTitle;

	CBaseUIButton *m_closeButton;
	CBaseUIButton *m_minimizeButton;

	// main container
	CBaseUIContainer *m_container;

	// moving
	bool m_bMoving;
	Vector2 m_vMousePosBackup;
	Vector2 m_vLastPos;

	// resizing
	Vector2 m_vResizeLimit;
	bool m_bResizing;
	int m_iResizeType;
	Vector2 m_vLastSize;

	// test features
	// RenderTarget *m_rt;
	// CBaseUIBoxShadow *m_shadow;
};

#endif
