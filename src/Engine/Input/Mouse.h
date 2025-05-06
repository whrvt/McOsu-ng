//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		mouse wrapper
//
// $NoKeywords: $mouse
//===============================================================================//

#pragma once
#ifndef MOUSE_H
#define MOUSE_H

#include "Cursors.h"
#include "InputDevice.h"
#include "MouseListener.h"

#include <array>

class Mouse : public InputDevice
{
public:
	Mouse();
	virtual ~Mouse() { ; }

	virtual void draw(Graphics *g);
	void drawDebug(Graphics *g);
	virtual void update();

	// event handling
	void addListener(MouseListener *mouseListener, bool insertOnTop = false);
	void removeListener(MouseListener *mouseListener);

	// input handling
	void onPosChange(Vector2 pos);
	void onMotion(float x, float y, float xRel, float yRel, bool isRawInput);
	void onWheelVertical(int delta);
	void onWheelHorizontal(int delta);
	void onButtonChange(int button, bool down);

	// button-specific handlers for compatibility with existing code
	void onLeftChange(bool leftDown) { onButtonChange(BUTTON_LEFT, leftDown); }
	void onMiddleChange(bool middleDown) { onButtonChange(BUTTON_MIDDLE, middleDown); }
	void onRightChange(bool rightDown) { onButtonChange(BUTTON_RIGHT, rightDown); }
	void onButton4Change(bool button4down) { onButtonChange(BUTTON_X1, button4down); }
	void onButton5Change(bool button5down) { onButtonChange(BUTTON_X2, button5down); }

	// position/coordinate handling
	void setPos(Vector2 pos);
	void setOffset(Vector2 offset);
	void setScale(Vector2 scale) { m_vScale = scale; }

	// cursor control
	void setCursorType(CURSORTYPE cursorType);
	void setCursorVisible(bool cursorVisible);
	bool isCursorVisible();

	// state getters
	inline Vector2 getPos() const { return m_vPos; }
	inline Vector2 getRealPos() const { return m_vPosWithoutOffset; }
	inline Vector2 getActualPos() const { return m_vActualPos; }
	inline Vector2 getDelta() const { return m_vDelta; }
	inline Vector2 getRawDelta() const { return m_vRawDelta; }

	inline Vector2 getOffset() const { return m_vOffset; }
	inline Vector2 getScale() const { return m_vScale; }

	// button state accessors
	inline bool isLeftDown() const { return m_bMouseButtonDown[BUTTON_LEFT]; }
	inline bool isMiddleDown() const { return m_bMouseButtonDown[BUTTON_MIDDLE]; }
	inline bool isRightDown() const { return m_bMouseButtonDown[BUTTON_RIGHT]; }
	inline bool isButton4Down() const { return m_bMouseButtonDown[BUTTON_X1]; }
	inline bool isButton5Down() const { return m_bMouseButtonDown[BUTTON_X2]; }

	inline int getWheelDeltaVertical() const { return m_iWheelDeltaVertical; }
	inline int getWheelDeltaHorizontal() const { return m_iWheelDeltaHorizontal; }

	void resetWheelDelta();

	// input mode control
	bool isInAbsoluteMode() const { return m_bAbsolute; }
	void setAbsoluteMode(bool absolute) { m_bAbsolute = absolute; }

private:
	void setPosXY(float x, float y);
	void updateFakelagBuffer();

	// button mapping constants for internal array indexing
	enum ButtonIndex
	{
		BUTTON_NONE = 0,
		BUTTON_LEFT = 1,
		BUTTON_MIDDLE = 2,
		BUTTON_RIGHT = 3,
		BUTTON_X1 = 4,
		BUTTON_X2 = 5,
		BUTTON_COUNT = 6 // size of our button array
	};

	// position state
	Vector2 m_vPos;              // position with offset applied
	Vector2 m_vPosWithoutOffset; // position without offset
	Vector2 m_vDelta;            // movement delta in the current frame
	Vector2 m_vRawDelta;         // movement delta in the current frame, without consideration for clipping or sensitivity
	Vector2 m_vActualPos;        // final cursor position after all transformations

	// mode tracking
	bool m_bLastFrameHadMotion; // whether setPos was called in the previous frame
	bool m_bAbsolute;                 // whether using absolute input (tablets)
	bool m_bVirtualDesktop;           // whether using virtual desktop coordinates

	// button state (using our internal button index)
	std::array<bool, BUTTON_COUNT> m_bMouseButtonDown;

	// wheel state
	int m_iWheelDeltaVertical;
	int m_iWheelDeltaHorizontal;
	int m_iWheelDeltaVerticalActual;
	int m_iWheelDeltaHorizontalActual;

	// listeners
	std::vector<MouseListener *> m_listeners;

	// transform parameters
	Vector2 m_vOffset; // offset applied to coordinates
	Vector2 m_vScale;  // scale applied to coordinates

	// input delay buffer (fakelag)
	struct FAKELAG_PACKET
	{
		float time;
		Vector2 pos;
	};
	std::vector<FAKELAG_PACKET> m_fakelagBuffer;
	Vector2 m_vFakeLagPos;
};

#endif
