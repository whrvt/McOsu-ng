//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser button base class
//
// $NoKeywords: $osusbb
//===============================================================================//

#pragma once
#ifndef OSUUISONGBROWSERBUTTON_H
#define OSUUISONGBROWSERBUTTON_H

#include <utility>

#include "OsuUIElement.h"

class Osu;
class OsuDatabaseBeatmap;
class OsuSongBrowser2;
class OsuUIContextMenu;

class CBaseUIScrollView;

class OsuUISongBrowserButton : public CBaseUIButton
{
public:
	OsuUISongBrowserButton(OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name);
	~OsuUISongBrowserButton() override;
	void deleteAnimations();

	void draw() override;
	void update() override;

	virtual void updateLayoutEx();

	OsuUISongBrowserButton *setVisible(bool visible) override;

	void select(bool fireCallbacks = true, bool autoSelectBottomMostChild = true, bool wasParentSelected = true);
	void deselect();

	void resetAnimations();

	void setTargetRelPosY(float targetRelPosY);
	void setChildren(std::vector<OsuUISongBrowserButton*> children) {m_children = std::move(children);}
	void setOffsetPercent(float offsetPercent) {m_fOffsetPercent = offsetPercent;}
	void setHideIfSelected(bool hideIfSelected) {m_bHideIfSelected = hideIfSelected;}
	void setIsSearchMatch(bool isSearchMatch) {m_bIsSearchMatch = isSearchMatch;}

	[[nodiscard]] Vector2 getActualOffset() const;
	[[nodiscard]] inline Vector2 getActualSize() const {return m_vSize - 2*getActualOffset();}
	[[nodiscard]] inline Vector2 getActualPos() const {return m_vPos + getActualOffset();}
	[[nodiscard]] inline std::vector<OsuUISongBrowserButton*> &getChildren() {return m_children;}
	[[nodiscard]] inline const std::vector<OsuUISongBrowserButton*> &getChildren() const {return m_children;}
	[[nodiscard]] inline int getSortHack() const {return m_iSortHack;}

	[[nodiscard]] virtual OsuDatabaseBeatmap *getDatabaseBeatmap() const {return NULL;}
	[[nodiscard]] virtual Color getActiveBackgroundColor() const;
	[[nodiscard]] virtual Color getInactiveBackgroundColor() const;

	[[nodiscard]] inline bool isSelected() const {return m_bSelected;}
	[[nodiscard]] inline bool isHiddenIfSelected() const {return m_bHideIfSelected;}
	[[nodiscard]] inline bool isSearchMatch() const {return m_bIsSearchMatch.load();}

	// inspection
	CBASE_UI_TYPE(OsuUISongBrowserButton, OsuUIElement::UISONGBROWSERBUTTON, CBaseUIButton)

protected:
	void drawMenuButtonBackground();

	virtual void onSelected(bool  /*wasSelected*/, bool  /*autoSelectBottomMostChild*/, bool  /*wasParentSelected*/) {;}
	virtual void onRightMouseUpInside() {;}

	CBaseUIScrollView *m_view;
	OsuSongBrowser2 *m_songBrowser;
	OsuUIContextMenu *m_contextMenu;

	McFont *m_font;
	McFont *m_fontBold;

	bool m_bSelected;

	std::vector<OsuUISongBrowserButton*> m_children;

private:
	static int marginPixelsX;
	static int marginPixelsY;
	static float lastHoverSoundTime;
	static int sortHackCounter;

	enum class MOVE_AWAY_STATE : uint8_t
	{
		MOVE_CENTER,
		MOVE_UP,
		MOVE_DOWN
	};

	void onClicked() override;
	void onMouseInside() override;
	void onMouseOutside() override;

	void setMoveAwayState(MOVE_AWAY_STATE moveAwayState, bool animate = true);

	bool m_bRightClick;
	bool m_bRightClickCheck;

	float m_fTargetRelPosY;
	float m_fScale;
	float m_fOffsetPercent;
	float m_fHoverOffsetAnimation;
	float m_fHoverMoveAwayAnimation;
	float m_fCenterOffsetAnimation;
	float m_fCenterOffsetVelocityAnimation;

	int m_iSortHack;
	std::atomic<bool> m_bIsSearchMatch;

	bool m_bHideIfSelected;

	MOVE_AWAY_STATE m_moveAwayState;
};

#endif
