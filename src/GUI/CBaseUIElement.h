//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		the base class for all UI Elements
//
// $NoKeywords: $buie
//===============================================================================//

#pragma once
#ifndef CBASEUIELEMENT_H
#define CBASEUIELEMENT_H

#include "cbase.h"
#include "KeyboardListener.h"

#define ELEMENT_BODY_BASE(T, v, o) v T *setPos(float xPos, float yPos) o {if (m_vPos.x != xPos || m_vPos.y != yPos) {m_vPos.x = xPos - m_vSize.x * m_vAnchor.x; m_vPos.y = yPos - m_vSize.y * m_vAnchor.y; onMoved();} return this;} \
	v T *setPosX(float xPos) o {if (m_vPos.x != xPos) {m_vPos.x = xPos - m_vSize.x * m_vAnchor.x; onMoved();} return this;} \
	v T *setPosY(float yPos) o {if (m_vPos.y != yPos) {m_vPos.y = yPos - m_vSize.y * m_vAnchor.y; onMoved();} return this;} \
	v T *setPos(Vector2 position) o {if (m_vPos != position) {m_vPos = position - m_vSize * m_vAnchor; onMoved();} return this;} \
	\
	v T *setPosAbsolute(float xPos, float yPos) o {if (m_vPos.x != xPos || m_vPos.y != yPos) {m_vPos.x = xPos; m_vPos.y = yPos; onMoved();} return this;} \
	v T *setPosAbsoluteX(float xPos) o {if (m_vPos.x != xPos) {m_vPos.x = xPos; onMoved();} return this;} \
	v T *setPosAbsoluteY(float yPos) o {if (m_vPos.y != yPos) {m_vPos.y = yPos; onMoved();} return this;} \
	v T *setPosAbsolute(Vector2 position) o {if (m_vPos != position) {m_vPos = position; onMoved();} return this;} \
	\
	v T *setRelPos(float xPos, float yPos) o {if (m_vmPos.x != xPos || m_vmPos.y != yPos) {m_vmPos.x = xPos - m_vSize.x * m_vAnchor.x; m_vmPos.y = yPos - m_vSize.y * m_vAnchor.y; updateLayout();} return this;} \
	v T *setRelPosX(float xPos) o {if (m_vmPos.x != xPos) {m_vmPos.x = xPos - m_vSize.x * m_vAnchor.x; updateLayout();} return this;} \
	v T *setRelPosY(float yPos) o {if (m_vmPos.y != yPos) {m_vmPos.y = yPos - m_vSize.x * m_vAnchor.y; updateLayout();} return this;} \
	v T *setRelPos(Vector2 position) o {if (m_vmPos != position) {m_vmPos = position - m_vSize * m_vAnchor; updateLayout();} return this;} \
	\
	v T *setRelPosAbsolute(float xPos, float yPos) o {if (m_vmPos.x != xPos || m_vmPos.y != yPos) {m_vmPos.x = xPos; m_vmPos.y = yPos; updateLayout();} return this;} \
	v T *setRelPosAbsoluteX(float xPos) o {if (m_vmPos.x != xPos) {m_vmPos.x = xPos; updateLayout();} return this;} \
	v T *setRelPosAbsoluteY(float yPos) o {if (m_vmPos.y != yPos) {m_vmPos.y = yPos; updateLayout();} return this;} \
	v T *setRelPosAbsolute(Vector2 position) o {if (m_vmPos != position) {m_vmPos = position; updateLayout();} return this;} \
	\
	v T *setSize(float xSize, float ySize) o {if (m_vSize.x != xSize || m_vSize.y != ySize) {m_vPos.x += (m_vSize.x - xSize) * m_vAnchor.x; m_vPos.y += (m_vSize.y - ySize) * m_vAnchor.y; m_vSize.x = xSize; m_vSize.y = ySize; onResized(); onMoved();} return this;} \
	v T *setSizeX(float xSize) o {if (m_vSize.x != xSize) {m_vPos.x += (m_vSize.x - xSize) * m_vAnchor.x; m_vSize.x = xSize; onResized(); onMoved();} return this;} \
	v T *setSizeY(float ySize) o {if (m_vSize.y != ySize) {m_vPos.y += (m_vSize.y - ySize) * m_vAnchor.y; m_vSize.y = ySize; onResized(); onMoved();} return this;} \
	v T *setSize(Vector2 size) o {if (m_vSize != size) {m_vPos += (m_vSize - size) * m_vAnchor; m_vSize = size; onResized(); onMoved();} return this;} \
	\
	v T *setSizeAbsolute(float xSize, float ySize) o {if (m_vSize.x != xSize || m_vSize.y != ySize) {m_vSize.x = xSize; m_vSize.y = ySize; onResized();} return this;} \
	v T *setSizeAbsoluteX(float xSize) o {if (m_vSize.x != xSize) {m_vSize.x = xSize; onResized();} return this;} \
	v T *setSizeAbsoluteY(float ySize) o {if (m_vSize.y != ySize) {m_vSize.y = ySize; onResized();} return this;} \
	v T *setSizeAbsolute(Vector2 size) o {if (m_vSize != size) {m_vSize = size; onResized();} return this;} \
	\
	v T *setRelSize(float xSize, float ySize) o {if(m_vmSize.x != xSize || m_vmSize.y != ySize) {m_vmPos.x += (m_vmSize.x - xSize) * m_vAnchor.x; m_vmPos.y += (m_vmSize.y - ySize) * m_vAnchor.y; m_vmSize.x = xSize; m_vmSize.y = ySize; updateLayout();} return this;} \
	v T *setRelSizeX(float xSize) o {if (m_vmSize.x != xSize) {m_vmPos.x += (m_vmSize.x - xSize) * m_vAnchor.x; m_vmSize.x = xSize; updateLayout();} return this;} \
	v T *setRelSizeY(float ySize) o {if (m_vmSize.y != ySize) {m_vmPos.y += (m_vmSize.y - ySize) * m_vAnchor.y; m_vmSize.y = ySize; updateLayout();} return this;} \
	v T *setRelSize(Vector2 size) o {if (m_vmSize != size) {m_vmPos += (m_vmSize - size) * m_vAnchor; m_vmSize = size; updateLayout();} return this;} \
	\
	v T *setRelSizeAbsolute(float xSize, float ySize) o {if (m_vmSize.x != xSize || m_vmSize.y != ySize) {m_vmSize.x = xSize; m_vmSize.y = ySize; updateLayout();} return this;} \
	v T *setRelSizeAbsoluteX(float xSize) o {if (m_vmSize.x != xSize) {m_vmSize.x = xSize; updateLayout();} return this;} \
	v T *setRelSizeAbsoluteY(float ySize) o {if (m_vmSize.y != ySize) {m_vmSize.y = ySize; updateLayout();} return this;} \
	v T *setRelSizeAbsolute(Vector2 size) o {if (m_vmSize != size) {m_vmSize = size; updateLayout();} return this;} \
	\
	v T *setAnchor(float xAnchor, float yAnchor) o {if (m_vAnchor.x != xAnchor || m_vAnchor.y != yAnchor){m_vmPos.x -= m_vmSize.x * (xAnchor - m_vAnchor.x); m_vmPos.y -= m_vmSize.y * (yAnchor - m_vAnchor.y); m_vPos.x -= m_vSize.x * (xAnchor - m_vAnchor.x); m_vPos.y -= m_vSize.y * (yAnchor - m_vAnchor.y); m_vAnchor.x = xAnchor; m_vAnchor.y = yAnchor; if (m_parent != nullptr) updateLayout(); onMoved();} return this;} \
	v T *setAnchorX(float xAnchor) o {if (m_vAnchor.x != xAnchor){m_vmPos.x -= m_vmSize.x * (xAnchor - m_vAnchor.x); m_vPos.x -= m_vSize.x * (xAnchor - m_vAnchor.x); m_vAnchor.x = xAnchor; if (m_parent != nullptr) updateLayout(); onMoved();} return this;} \
	v T *setAnchorY(float yAnchor) o {if (m_vAnchor.y != yAnchor){m_vmPos.y -= m_vmSize.y * (yAnchor - m_vAnchor.y); m_vPos.y -= m_vSize.y * (yAnchor - m_vAnchor.y); m_vAnchor.y = yAnchor; if (m_parent != nullptr) updateLayout(); onMoved();} return this;} \
	v T *setAnchor(Vector2 anchor) o {if (m_vAnchor != anchor){m_vmPos -= m_vmSize * (anchor - m_vAnchor); m_vPos -= m_vSize * (anchor - m_vAnchor); m_vAnchor = anchor; if (m_parent != nullptr) updateLayout(); onMoved();} return this;} \
	\
	v T *setAnchorAbsolute(float xAnchor, float yAnchor) o {if (m_vAnchor.x != xAnchor || m_vAnchor.y != yAnchor){m_vAnchor.x = xAnchor, m_vAnchor.y = yAnchor;} return this;} \
	v T *setAnchorAbsoluteX(float xAnchor) o {if (m_vAnchor.x != xAnchor) {m_vAnchor.x = xAnchor;} return this;} \
	v T *setAnchorAbsoluteY(float yAnchor) o {if (m_vAnchor.y != yAnchor) {m_vAnchor.y = yAnchor;} return this;} \
	v T *setAnchorAbsolute(Vector2 anchor) o {if (m_vAnchor != anchor) {m_vAnchor = anchor;} return this;} \
	\
	v T *setVisible(bool visible) o {m_bVisible = visible; return this;} \
	v T *setActive(bool active) o {m_bActive = active; return this;} \
	v T *setKeepActive(bool keepActive) o {m_bKeepActive = keepActive; return this;} \
	v T *setDrawManually(bool drawManually) o {m_bDrawManually = drawManually; return this;} \
	v T *setPositionManually(bool positionManually) o {m_bPositionManually = positionManually; return this;} \
	v T *setEnabled(bool enabled) o {if (enabled != m_bEnabled) {m_bEnabled = enabled; if (m_bEnabled) {onEnabled();} else {onDisabled();}} return this;} \
	v T *setBusy(bool busy) o {m_bBusy = busy; return this;} \
	v T *setName(UString name) o {m_sName = name; return this;} \
	v T *setParent(CBaseUIElement *parent) o {m_parent = parent; return this;} \
	v T *setScaleByHeightOnly(bool scaleByHeightOnly) o {m_bScaleByHeightOnly = scaleByHeightOnly; return this;}

#define ELEMENT_BODY_VIRTUAL(T) ELEMENT_BODY_BASE(T, virtual, )
#define ELEMENT_BODY(T) ELEMENT_BODY_BASE(T, , override)

#define CBASE_UI_TYPE(ClassName, TypeID, ParentClass) \
	static constexpr TypeId TYPE_ID = TypeID; \
	[[nodiscard]] TypeId getTypeId() const override { return TYPE_ID; } \
	[[nodiscard]] bool isTypeOf(TypeId typeId) const override { \
		return typeId == TYPE_ID || ParentClass::isTypeOf(typeId); \
	}

class CBaseUIElement : public KeyboardListener
{
public:
	using TypeId = uint8_t;
	enum elemType : TypeId {
		BASE = 0,
		BOXSHADOW,
		BUTTON,
		CONTAINER,
		CONTAINERBASE,
		IMAGE,
		LABEL,
		SCROLLVIEW,
		SLIDER,
		TEXTBOX,
		TEXTFIELD,
		TEXTOBJECT,
		WINDOW,
		CANVAS,
		CHECKBOX,
		CONTAINERBOX,
		CONTAINERHBOX,
		CONTAINERVBOX,
		IMAGEBUTTON,
		ENGINE_TYPES_START,
		// reserved range for derived classes
		ENGINE_TYPES_END = 99,
		APP_TYPES_START = 100,
		RESERVED_END = 255
	};
public:
	CBaseUIElement(float xPos=0, float yPos=0, float xSize=0, float ySize=0, UString name="");
	virtual ~CBaseUIElement() {;}

	ELEMENT_BODY_VIRTUAL(CBaseUIElement)

	// main
	virtual void draw(Graphics *g) = 0;
	virtual void update();

	// keyboard input
	void onKeyUp(KeyboardEvent &) override {;}
	void onKeyDown(KeyboardEvent &) override {;}
	void onChar(KeyboardEvent &) override {;}

	// getters
	[[nodiscard]] inline const Vector2& getPos() const {return m_vPos;}
	[[nodiscard]] inline const Vector2& getSize() const {return m_vSize;}
	[[nodiscard]] inline UString getName() const {return m_sName;}
	[[nodiscard]] inline const Vector2& getRelPos() const {return m_vmPos;}
	[[nodiscard]] inline const Vector2& getRelSize() const {return m_vmSize;}
	[[nodiscard]] inline const Vector2& getAnchor() const {return m_vAnchor;}
	[[nodiscard]] inline CBaseUIElement *getParent() const {return m_parent;}

	virtual bool isActive() {return m_bActive || isBusy();}
	virtual bool isVisible() {return m_bVisible;}
	virtual bool isEnabled() {return m_bEnabled;}
	virtual bool isBusy() {return m_bBusy && isVisible();}
	virtual bool isDrawnManually() {return m_bDrawManually;}
	virtual bool isPositionedManually() {return m_bPositionManually;}
	virtual bool isMouseInside() {return m_bMouseInside && isVisible();}
	virtual bool isScaledByHeightOnly() {return m_bScaleByHeightOnly;}

	// actions
	void stealFocus() {m_bMouseInsideCheck = true; m_bActive = false; onFocusStolen();}
	virtual void updateLayout() {if(m_parent != nullptr) m_parent->updateLayout();}

	// type inspection
	[[nodiscard]] virtual TypeId getTypeId() const = 0;
	[[nodiscard]] virtual bool isTypeOf(TypeId) const { return false; }
	template<typename T>
	[[nodiscard]] bool isType() const { return isTypeOf(T::TYPE_ID); }
	template<typename T>
	T* as() { return isType<T>() ? static_cast<T*>(this) : nullptr; }
	template<typename T>
	const T* as() const { return isType<T>() ? static_cast<const T*>(this) : nullptr; }
protected:
	// events
	virtual void onResized() {;}
	virtual void onMoved() {;}

	virtual void onFocusStolen() {;}
	virtual void onEnabled() {;}
	virtual void onDisabled() {;}

	virtual void onMouseInside() {;}
	virtual void onMouseOutside() {;}
	virtual void onMouseDownInside() {;}
	virtual void onMouseDownOutside() {;}
	virtual void onMouseUpInside() {;}
	virtual void onMouseUpOutside() {;}

	// vars
	UString m_sName;
	CBaseUIElement *m_parent;

	// attributes
	bool m_bVisible;
	bool m_bActive;			// we are doing something, e.g. textbox is blinking and ready to receive input
	bool m_bBusy;			// we demand the focus to be kept on us, e.g. click-drag scrolling in a scrollview
	bool m_bEnabled;

	bool m_bKeepActive;		// once clicked, don't lose m_bActive, we have to manually release it (e.g. textbox)
	bool m_bDrawManually;
	bool m_bPositionManually;
	bool m_bMouseInside;

	// container options
	bool m_bScaleByHeightOnly;

	// position and size
	Vector2 m_vPos;
	Vector2 m_vmPos;
	Vector2 m_vSize;
	Vector2 m_vmSize;
	Vector2 m_vAnchor;		// the point of transformation

private:
	bool m_bMouseInsideCheck;
	bool m_bMouseUpCheck;
};

#endif
