//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		mouse wrapper
//
// $NoKeywords: $mouse
//===============================================================================//

#include "Mouse.h"

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "ResourceManager.h"

ConVar debug_mouse("debug_mouse", false, FCVAR_CHEAT);
ConVar debug_mouse_clicks("debug_mouse_clicks", false, FCVAR_NONE);
ConVar mouse_sensitivity("mouse_sensitivity", 1.0f, FCVAR_NONE);
ConVar mouse_raw_input("mouse_raw_input", false, FCVAR_NONE);
ConVar mouse_raw_input_absolute_to_window("mouse_raw_input_absolute_to_window", false, FCVAR_NONE);
ConVar mouse_fakelag("mouse_fakelag", 0.000f, FCVAR_NONE, "delay all mouse movement by this many seconds (e.g. 0.1 = 100 ms delay)");
ConVar tablet_sensitivity_ignore("tablet_sensitivity_ignore", false, FCVAR_NONE);
ConVar win_ink_workaround("win_ink_workaround", false, FCVAR_NONE);

Mouse::Mouse() : InputDevice()
{
	m_bMouseButtonDown.fill(false);

	m_iWheelDeltaVertical = 0;
	m_iWheelDeltaHorizontal = 0;
	m_iWheelDeltaVerticalActual = 0;
	m_iWheelDeltaHorizontalActual = 0;

	m_bSetPosWasCalledLastFrame = false;
	m_bAbsolute = false;
	m_bVirtualDesktop = false;

	m_vOffset = Vector2(0, 0);
	m_vScale = Vector2(1, 1);
	m_vActualPos = m_vPosWithoutOffset = m_vPos = env->getMousePos();

	m_vFakeLagPos = m_vPos;
}

void Mouse::draw(Graphics *g)
{
	if (!debug_mouse.getBool())
		return;

	drawDebug(g);

	// green rect = virtual cursor pos
	g->setColor(0xff00ff00);
	float size = 20.0f;
	g->drawRect(m_vActualPos.x - size / 2, m_vActualPos.y - size / 2, size, size);

	// red rect = real cursor pos
	g->setColor(0xffff0000);
	Vector2 envPos = env->getMousePos();
	g->drawRect(envPos.x - size / 2, envPos.y - size / 2, size, size);

	// red = cursor clip
	if (env->isCursorClipped())
	{
		McRect cursorClip = env->getCursorClip();
		g->drawRect(cursorClip.getMinX(), cursorClip.getMinY(), cursorClip.getWidth() - 1, cursorClip.getHeight() - 1);
	}

	// green = scaled & offset virtual area
	const Vector2 scaledOffset = m_vOffset;
	const Vector2 scaledEngineScreenSize = engine->getScreenSize() * m_vScale;
	g->setColor(0xff00ff00);
	g->drawRect(-scaledOffset.x, -scaledOffset.y, scaledEngineScreenSize.x, scaledEngineScreenSize.y);
}

void Mouse::drawDebug(Graphics *g)
{
	Vector2 pos = getPos();

	g->setColor(0xff000000);
	g->drawLine(pos.x - 1, pos.y - 1, 0 - 1, pos.y - 1);
	g->drawLine(pos.x - 1, pos.y - 1, engine->getScreenWidth() - 1, pos.y - 1);
	g->drawLine(pos.x - 1, pos.y - 1, pos.x - 1, 0 - 1);
	g->drawLine(pos.x - 1, pos.y - 1, pos.x - 1, engine->getScreenHeight() - 1);

	g->setColor(0xffffffff);
	g->drawLine(pos.x, pos.y, 0, pos.y);
	g->drawLine(pos.x, pos.y, engine->getScreenWidth(), pos.y);
	g->drawLine(pos.x, pos.y, pos.x, 0);
	g->drawLine(pos.x, pos.y, pos.x, engine->getScreenHeight());

	float rectSizePercent = 0.05f;
	float aspectRatio = (float)engine->getScreenWidth() / (float)engine->getScreenHeight();
	Vector2 rectSize = Vector2(engine->getScreenWidth(), engine->getScreenHeight() * aspectRatio) * rectSizePercent;

	g->setColor(0xff000000);
	g->drawRect(pos.x - rectSize.x / 2.0f - 1, pos.y - rectSize.y / 2.0f - 1, rectSize.x, rectSize.y);

	g->setColor(0xffffffff);
	g->drawRect(pos.x - rectSize.x / 2.0f, pos.y - rectSize.y / 2.0f, rectSize.x, rectSize.y);

	McFont *posFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
	UString posString = UString::format("[%i, %i]", (int)pos.x, (int)pos.y);
	float stringWidth = posFont->getStringWidth(posString);
	float stringHeight = posFont->getHeight();
	Vector2 textOffset = Vector2(pos.x + rectSize.x / 2.0f + stringWidth + 5 > engine->getScreenWidth() ? -rectSize.x / 2.0f - stringWidth - 5 : rectSize.x / 2.0f + 5,
	                             (pos.y + rectSize.y / 2.0f + stringHeight > engine->getScreenHeight()) ? -rectSize.y / 2.0f - stringHeight : rectSize.y / 2.0f + stringHeight);

	g->pushTransform();
	g->translate(pos.x + textOffset.x, pos.y + textOffset.y);
	g->drawString(posFont, UString::format("[%i, %i]", (int)pos.x, (int)pos.y));
	g->popTransform();
}

void Mouse::update()
{
	m_vDelta.zero();

	resetWheelDelta();

	Vector2 envPos = env->getMousePos();

	// if setPos was called last frame, the mouse was already moved
	if (!m_bSetPosWasCalledLastFrame)
	{
		Vector2 nextPos = envPos;

		// apply sensitivity if enabled AND not in raw input mode
		// (since raw input already has sensitivity applied, by the SDL transform callback in SDLEnvironment.cpp)
		// TODO: fix non-rawinput sensitivity <1 having the cursor clipped sooner than when it reaches the bounds of the screen
		bool applyMouseSensitivity = (mouse_sensitivity.getFloat() != 1.0f) && !mouse_raw_input.getBool();
		if (applyMouseSensitivity && !tablet_sensitivity_ignore.getBool())
		{
			// apply sensitivity around screen center as the pivot point
			Vector2 center = Vector2(engine->getScreenSize().x / 2, engine->getScreenSize().y / 2);
			nextPos = center + ((nextPos - center) * mouse_sensitivity.getFloat());
		}

		// calculate delta
		// the clip rect is also handled by SDL (setCursorClip), no need to clamp it again here
		m_vDelta = nextPos - m_vPosWithoutOffset;

		// update internal position state
		onPosChange(nextPos);
	}

	m_bSetPosWasCalledLastFrame = false;

	if (unlikely(mouse_fakelag.getBool()))
		updateFakelagBuffer();
}

void Mouse::onMotion(float x, float y, float xRel, float yRel, bool isRawInput)
{
	if (isRawInput)
	{
		// the transform is now applied by SDL before we get the values
		// so we can use xRel and yRel directly without applying sensitivity
		Vector2 nextPos = m_vPosWithoutOffset + Vector2(xRel, yRel);

		m_bAbsolute = false;
		onPosChange(nextPos);
	}
	else
	{
		// Handle absolute mode as before
		m_bAbsolute = true;
		onPosChange(Vector2(x, y));
	}
}

void Mouse::resetWheelDelta()
{
	m_iWheelDeltaVertical = m_iWheelDeltaVerticalActual;
	m_iWheelDeltaVerticalActual = 0;

	m_iWheelDeltaHorizontal = m_iWheelDeltaHorizontalActual;
	m_iWheelDeltaHorizontalActual = 0;
}

void Mouse::onPosChange(Vector2 pos)
{
	m_vPosWithoutOffset = pos;
	m_vPos = (m_vOffset + pos);
	m_vActualPos = m_vPos;

	setPosXY(m_vPos.x, m_vPos.y);
}

void Mouse::setPosXY(float x, float y)
{
	if (mouse_fakelag.getFloat() > 0.0f)
	{
		FAKELAG_PACKET p;
		p.time = engine->getTime() + mouse_fakelag.getFloat();
		p.pos = Vector2(x, y);
		m_fakelagBuffer.push_back(p);

		updateFakelagBuffer();
	}
	else
	{
		m_vPos.x = x;
		m_vPos.y = y;
	}
}

void Mouse::updateFakelagBuffer()
{
	float engineTime = engine->getTime();
	for (size_t i = 0; i < m_fakelagBuffer.size(); i++)
	{
		if (engineTime >= m_fakelagBuffer[i].time)
		{
			m_vFakeLagPos = m_fakelagBuffer[i].pos;

			m_fakelagBuffer.erase(m_fakelagBuffer.begin() + i);
			i--;
		}
	}

	m_vPos = m_vFakeLagPos;
}

void Mouse::onWheelVertical(int delta)
{
	m_iWheelDeltaVerticalActual += delta;

	for (size_t i = 0; i < m_listeners.size(); i++)
	{
		m_listeners[i]->onWheelVertical(delta);
	}
}

void Mouse::onWheelHorizontal(int delta)
{
	m_iWheelDeltaHorizontalActual += delta;

	for (size_t i = 0; i < m_listeners.size(); i++)
	{
		m_listeners[i]->onWheelHorizontal(delta);
	}
}

void Mouse::onButtonChange(int button, bool down)
{
	if (button < 1 || button >= BUTTON_COUNT)
		return;

	if (debug_mouse_clicks.getBool())
		debugLog("Mouse::onButtonChange(%i, %i)\n", button, (int)down);

	m_bMouseButtonDown[button] = down;

	// notify listeners
	for (size_t i = 0; i < m_listeners.size(); i++)
	{
		switch (button)
		{
		case BUTTON_LEFT:
			m_listeners[i]->onLeftChange(down);
			break;
		case BUTTON_MIDDLE:
			m_listeners[i]->onMiddleChange(down);
			break;
		case BUTTON_RIGHT:
			m_listeners[i]->onRightChange(down);
			break;
		case BUTTON_X1:
			m_listeners[i]->onButton4Change(down);
			break;
		case BUTTON_X2:
			m_listeners[i]->onButton5Change(down);
			break;
		}
	}
}

void Mouse::setPos(Vector2 newPos)
{
	m_bSetPosWasCalledLastFrame = true;

	setPosXY(newPos.x, newPos.y);
	env->setMousePos(newPos.x, newPos.y);
}

void Mouse::setOffset(Vector2 offset)
{
	Vector2 oldOffset = m_vOffset;
	m_vOffset = offset;

	// update position to maintain visual position after offset change
	Vector2 posAdjustment = m_vOffset - oldOffset;
	m_vPos += posAdjustment;
	m_vActualPos += posAdjustment;
}

void Mouse::setCursorType(CURSORTYPE cursorType)
{
	env->setCursor(cursorType);
}

void Mouse::setCursorVisible(bool cursorVisible)
{
	env->setCursorVisible(cursorVisible);
}

bool Mouse::isCursorVisible()
{
	return env->isCursorVisible();
}

void Mouse::addListener(MouseListener *mouseListener, bool insertOnTop)
{
	if (mouseListener == NULL)
	{
		engine->showMessageError("Mouse Error", "addListener(NULL)!");
		return;
	}

	if (insertOnTop)
		m_listeners.insert(m_listeners.begin(), mouseListener);
	else
		m_listeners.push_back(mouseListener);
}

void Mouse::removeListener(MouseListener *mouseListener)
{
	for (size_t i = 0; i < m_listeners.size(); i++)
	{
		if (m_listeners[i] == mouseListener)
		{
			m_listeners.erase(m_listeners.begin() + i);
			i--;
		}
	}
}