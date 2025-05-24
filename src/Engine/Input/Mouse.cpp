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

Mouse::Mouse() : InputDevice()
{
	m_bMouseButtonDown.fill(false);

	m_iWheelDeltaVertical = 0;
	m_iWheelDeltaHorizontal = 0;
	m_iWheelDeltaVerticalActual = 0;
	m_iWheelDeltaHorizontalActual = 0;

	m_bLastFrameHadMotion = false;
	m_bAbsolute = false;
	m_bVirtualDesktop = false;
	m_bIsRawInput = false;

	m_vOffset = Vector2(0, 0);
	m_vScale = Vector2(1, 1);
	m_vDelta.zero();
	m_vRawDelta.zero();
	m_vActualPos = m_vPosWithoutOffset = m_vPos = env->getMousePos();

	m_vFakeLagPos = m_vPos;

	m_fSensitivity = 1.0f;
	mouse_raw_input.setCallback(fastdelegate::MakeDelegate(this, &Mouse::onRawInputChanged));
	mouse_sensitivity.setCallback(fastdelegate::MakeDelegate(this, &Mouse::onSensitivityChanged));
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

	McFont *posFont = resourceManager->getFont("FONT_DEFAULT");
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
	resetWheelDelta();

	// if onMotion wasn't called last frame, there is no motion delta
	if (!m_bLastFrameHadMotion)
	{
		m_vDelta.zero();
		m_vRawDelta.zero();
	}
	else
	{
		// center the OS cursor if it's close to the screen edges, for non-raw input with sensitivity <1.0
		// the reason for trying hard to avoid env->setMousePos is because setting the OS cursor position can take a long time
		// so we try to use the virtual cursor position as much as possible and only update the OS cursor when it's going to go somewhere we don't want
		if (!m_bAbsolute)
		{
			const bool clipped = env->isCursorClipped();
			const McRect clipRect = clipped ? env->getCursorClip() : McRect{{0,0}, engine->getScreenSize()};
			const Vector2 center = clipRect.getCenter();
			const Vector2 realPosNudgedOut = env->getMousePos().nudge(center, 10.0f);
			if (!clipRect.contains(realPosNudgedOut))
			{
				if (clipped)
					env->setMousePos(center);
				else if (!env->isCursorVisible()) // FIXME: this is crazy. for windowed mode, need to "pop out" the OS cursor
					env->setMousePos(m_vPosWithoutOffset.nudge(center, 0.1f));
			}
		}

		onPosChange(m_vPosWithoutOffset);
	}

	m_bLastFrameHadMotion = false;

	if (unlikely(mouse_fakelag.getBool()))
		updateFakelagBuffer();
}

void Mouse::onMotion(float x, float y, float xRel, float yRel, bool preTransformed)
{
	Vector2 newRel{xRel, yRel}, newAbs{x, y};
	auto sens = m_fSensitivity;

	m_bAbsolute = true; // assume we don't have to lock the cursor

	const bool osCursorVisible = (env->isCursorVisible() || !env->isCursorInWindow() || !engine->hasFocus());

 	// rawinput has sensitivity pre-applied
	// this entire block may be skipped if: (preTransformed || (sens == 1 && !clipped))
	if (!preTransformed && !osCursorVisible)
	{
		// need to apply sensitivity
		if (!almostEqual(sens, 1.0f))
		{
			// need to lock the OS cursor to the center of the screen if rawinput is disabled, otherwise it can exit the screen rect before the virtual cursor does
			// don't do it here because we don't want the event loop to make more external calls than necessary,
			// just set a flag to do it on the engine update loop
			if (sens < 0.995f)
				m_bAbsolute = false;
			newRel *= sens;
			if (newRel.length() > 50.0f) // don't allow obviously bogus values
				newRel.zero();
			newAbs = m_vPosWithoutOffset + newRel;
		}
		if (env->isCursorClipped())
		{
			const McRect clipRect = env->getCursorClip();

			// clamp the final position to the clip rect
			newAbs.x = std::clamp<float>(newAbs.x, clipRect.getMinX(), clipRect.getMaxX());
			newAbs.y = std::clamp<float>(newAbs.y, clipRect.getMinY(), clipRect.getMaxY());
		}
	}

	m_vRawDelta = newRel / sens; // rawdelta doesn't include sensitivity or clipping
	m_vDelta = newAbs - m_vPosWithoutOffset;
	m_vPosWithoutOffset = newAbs;

	m_bLastFrameHadMotion = true;
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

	for (auto & m_listener : m_listeners)
	{
		m_listener->onWheelVertical(delta);
	}
}

void Mouse::onWheelHorizontal(int delta)
{
	m_iWheelDeltaHorizontalActual += delta;

	for (auto & m_listener : m_listeners)
	{
		m_listener->onWheelHorizontal(delta);
	}
}

void Mouse::onButtonChange(MouseButton::Index button, bool down)
{
	if (button < 1 || button >= BUTTON_COUNT)
		return;

	if (debug_mouse_clicks.getBool())
		debugLog("Mouse::onButtonChange({}, {})\n", (int)button, (int)down);

	m_bMouseButtonDown[button] = down;

	// notify listeners
	for (auto & m_listener : m_listeners)
	{
		m_listener->onButtonChange(button, down);
	}
}

void Mouse::setPos(Vector2 newPos)
{
	m_bLastFrameHadMotion = true;

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

CURSORTYPE Mouse::getCursorType()
{
	return env->getCursor();
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

void Mouse::onRawInputChanged(float newval)
{
	m_bIsRawInput = !!static_cast<int>(newval);
	env->notifyWantRawInput(m_bIsRawInput); // request environment to change the real OS cursor state (may or may not take effect immediately)
}

void Mouse::onSensitivityChanged(float newSens)
{
	m_fSensitivity = newSens;
}
