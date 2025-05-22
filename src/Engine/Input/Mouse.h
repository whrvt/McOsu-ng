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

class Mouse final : public InputDevice
{
public:
	Mouse();
	~Mouse() override { ; }

	void draw(Graphics *g) override;
	void update() override;

	void drawDebug(Graphics *g);

	// event handling
	void addListener(MouseListener *mouseListener, bool insertOnTop = false);
	void removeListener(MouseListener *mouseListener);

	// input handling
	void onPosChange(Vector2 pos);
	void onMotion(float x, float y, float xRel, float yRel, bool preTransformed);
	void onWheelVertical(int delta);
	void onWheelHorizontal(int delta);
	void onButtonChange(MouseButton::Index button, bool down);

	// position/coordinate handling
	void setPos(Vector2 pos);
	void setOffset(Vector2 offset);
	void setScale(Vector2 scale) { m_vScale = scale; }

	// cursor control
	CURSORTYPE getCursorType();
	void setCursorType(CURSORTYPE cursorType);
	void setCursorVisible(bool cursorVisible);
	bool isCursorVisible();

	// state getters
	[[nodiscard]] inline Vector2 getPos() const { return m_vPos; }
	[[nodiscard]] inline Vector2 getRealPos() const { return m_vPosWithoutOffset; }
	[[nodiscard]] inline Vector2 getActualPos() const { return m_vActualPos; }
	[[nodiscard]] inline Vector2 getDelta() const { return m_vDelta; }
	[[nodiscard]] inline Vector2 getRawDelta() const { return m_vRawDelta; }

	[[nodiscard]] inline Vector2 getOffset() const { return m_vOffset; }
	[[nodiscard]] inline Vector2 getScale() const { return m_vScale; }
	[[nodiscard]] inline float getSensitivity() const { return m_fSensitivity; }

	// button state accessors
	[[nodiscard]] inline bool isLeftDown() const { return m_bMouseButtonDown[BUTTON_LEFT]; }
	[[nodiscard]] inline bool isMiddleDown() const { return m_bMouseButtonDown[BUTTON_MIDDLE]; }
	[[nodiscard]] inline bool isRightDown() const { return m_bMouseButtonDown[BUTTON_RIGHT]; }
	[[nodiscard]] inline bool isButton4Down() const { return m_bMouseButtonDown[BUTTON_X1]; }
	[[nodiscard]] inline bool isButton5Down() const { return m_bMouseButtonDown[BUTTON_X2]; }

	[[nodiscard]] inline int getWheelDeltaVertical() const { return m_iWheelDeltaVertical; }
	[[nodiscard]] inline int getWheelDeltaHorizontal() const { return m_iWheelDeltaHorizontal; }

	void resetWheelDelta();

	// input mode control
	[[nodiscard]] inline bool isInAbsoluteMode() const { return m_bAbsolute; }
	inline void setAbsoluteMode(bool absolute) { m_bAbsolute = absolute; }

	[[nodiscard]] inline bool isRawInput() const { return m_bIsRawInput; } // "desired" rawinput state, NOT actual OS raw input state!
private:
	void setPosXY(float x, float y);
	void updateFakelagBuffer();

	// callbacks
	void onSensitivityChanged(float newSens);
	void onRawInputChanged(float newVal);

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
	bool m_bIsRawInput;			// whether raw input is active
	float m_fSensitivity;

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
