//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		context menu, dropdown style
//
// $NoKeywords: $
//===============================================================================//

#pragma once
#ifndef OSUUICONTEXTMENU_H
#define OSUUICONTEXTMENU_H

#include "OsuUIElement.h"

class CBaseUIContainer;
class CBaseUIScrollView;

class Osu;

class OsuUIContextMenuButton;
class OsuUIContextMenuTextbox;

class OsuUIContextMenu final : public OsuUIElement
{
public:
	static void clampToBottomScreenEdge(OsuUIContextMenu *menu);
	static void clampToRightScreenEdge(OsuUIContextMenu *menu);

public:
	OsuUIContextMenu(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "", CBaseUIScrollView *parent = NULL);
	~OsuUIContextMenu() override;

	void draw() override;
	void update() override;

	void onKeyUp(KeyboardEvent &e) override;
	void onKeyDown(KeyboardEvent &e) override;
	void onChar(KeyboardEvent &e) override;

	typedef fastdelegate::FastDelegate2<UString, int> ButtonClickCallback;
	void setClickCallback(ButtonClickCallback clickCallback) {m_clickCallback = clickCallback;}

	void begin(int minWidth = 0, bool bigStyle = false);
	OsuUIContextMenuButton *addButtonJustified(UString text, bool left = true, int id = -1);
	inline OsuUIContextMenuButton *addButton(UString text, int id = -1) {return addButtonJustified(text, true, id);};

	OsuUIContextMenuTextbox *addTextbox(UString text, int id = -1);

	void end(bool invertAnimation, bool clampUnderflowAndOverflowAndEnableScrollingIfNecessary);

	void setVisible2(bool visible2);

	bool isVisible() override {return m_bVisible && m_bVisible2;}

	// inspection
	CBASE_UI_TYPE(OsuUIContextMenu, UICONTEXTMENU, OsuUIElement)
private:
	void onResized() override;
	void onMoved() override;
	void onMouseDownOutside() override;
	void onFocusStolen() override;

	void onClick(CBaseUIButton *button);
	void onHitEnter(OsuUIContextMenuTextbox *textbox);

	CBaseUIScrollView *m_container;
	CBaseUIScrollView *m_parent;

	OsuUIContextMenuTextbox *m_containedTextbox;

	ButtonClickCallback m_clickCallback;

	int m_iYCounter;
	int m_iWidthCounter;

	bool m_bVisible2;
	float m_fAnimation;
	bool m_bInvertAnimation;

	bool m_bBigStyle;
	bool m_bClampUnderflowAndOverflowAndEnableScrollingIfNecessary;

	std::vector<CBaseUIElement*> m_selfDeletionCrashWorkaroundScheduledElementDeleteHack;
};

class OsuUIContextMenuButton : public CBaseUIButton
{
public:
	OsuUIContextMenuButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text, int id);
	virtual ~OsuUIContextMenuButton() {;}

	virtual void update();

	[[nodiscard]] inline int getID() const {return m_iID;}

	void setTooltipText(UString text);

private:
	int m_iID;

	std::vector<UString> m_tooltipTextLines;
};

class OsuUIContextMenuTextbox : public CBaseUITextbox
{
public:
	OsuUIContextMenuTextbox(float xPos, float yPos, float xSize, float ySize, UString name, int id);
	virtual ~OsuUIContextMenuTextbox() {;}

	[[nodiscard]] inline int getID() const {return m_iID;}

private:
	int m_iID;
};

#endif
