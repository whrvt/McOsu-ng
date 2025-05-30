//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		a simple button
//
// $NoKeywords: $button
//===============================================================================//

#pragma once
#ifndef CBASEUIBUTTON_H
#define CBASEUIBUTTON_H

#include "CBaseUIElement.h"

class McFont;

class CBaseUIButton : public CBaseUIElement
{
public:
	CBaseUIButton(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "", UString text = "");
	~CBaseUIButton() override {;}

	ELEMENT_BODY(CBaseUIButton);

	void draw() override;

	void click() {onClicked();}

	// callbacks, either void or with ourself as the argument
	typedef fastdelegate::FastDelegate0<> ButtonClickVoidCallback;
	CBaseUIButton *setClickCallback(ButtonClickVoidCallback clickCallback) {m_clickVoidCallback = clickCallback; return this;}
	typedef fastdelegate::FastDelegate1<CBaseUIButton*> ButtonClickCallback;
	CBaseUIButton *setClickCallback(ButtonClickCallback clickCallback) {m_clickCallback = clickCallback; return this;}

	// set
	CBaseUIButton *setDrawFrame(bool drawFrame) {m_bDrawFrame = drawFrame; return this;}
	CBaseUIButton *setDrawBackground(bool drawBackground) {m_bDrawBackground = drawBackground; return this;}
	CBaseUIButton *setTextLeft(bool textLeft) {m_bTextLeft = textLeft; updateStringMetrics(); return this;}

	CBaseUIButton *setFrameColor(Color frameColor) {m_frameColor = frameColor; return this;}
	CBaseUIButton *setBackgroundColor(Color backgroundColor) {m_backgroundColor = backgroundColor; return this;}
	CBaseUIButton *setTextColor(Color textColor) {m_textColor = textColor;m_textBrightColor = m_textDarkColor = 0; return this;}
	CBaseUIButton *setTextBrightColor(Color textBrightColor) {m_textBrightColor = textBrightColor; return this;}
	CBaseUIButton *setTextDarkColor(Color textDarkColor) {m_textDarkColor = textDarkColor; return this;}

	CBaseUIButton *setText(UString text) {m_sText = text; updateStringMetrics(); return this;}
	CBaseUIButton *setFont(McFont *font) {m_font = font; updateStringMetrics(); return this;}

	CBaseUIButton *setSizeToContent(int horizontalBorderSize = 1, int verticalBorderSize = 1) {setSize(m_fStringWidth+2*horizontalBorderSize, m_fStringHeight + 2*verticalBorderSize); return this;}
	CBaseUIButton *setWidthToContent(int horizontalBorderSize = 1) {setSizeX(m_fStringWidth+2*horizontalBorderSize); return this;}

	// get
	[[nodiscard]] inline Color getFrameColor() const {return m_frameColor;}
	[[nodiscard]] inline Color getBackgroundColor() const {return m_backgroundColor;}
	[[nodiscard]] inline Color getTextColor() const {return m_textColor;}
	[[nodiscard]] inline UString getText() const {return m_sText;}
	[[nodiscard]] inline McFont *getFont() const {return m_font;}
	[[nodiscard]] inline ButtonClickCallback getClickCallback() const {return m_clickCallback;}
	[[nodiscard]] inline bool isTextLeft() const {return m_bTextLeft;}

	// events
	void onMouseUpInside() override;
	void onResized() override {updateStringMetrics();}

	// inspection
	CBASE_UI_TYPE(CBaseUIButton, BUTTON, CBaseUIElement)
protected:
	virtual void onClicked();

	virtual void drawText();

	void drawHoverRect(int distance);

	void updateStringMetrics();

	bool m_bDrawFrame;
	bool m_bDrawBackground;
	bool m_bTextLeft;

	Color m_frameColor;
	Color m_backgroundColor;
	Color m_textColor;
	Color m_textBrightColor;
	Color m_textDarkColor;

	McFont *m_font;
	UString m_sText;
	float m_fStringWidth;
	float m_fStringHeight;

	ButtonClickVoidCallback m_clickVoidCallback;
	ButtonClickCallback m_clickCallback;
};

#endif
