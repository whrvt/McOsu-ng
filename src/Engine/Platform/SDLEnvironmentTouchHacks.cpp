//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		Legacy SDL2 touch/joystick support compatibility quarantine file
//
// $NoKeywords: $sdltouch
//===============================================================================//

#include "SDLEnvironment.h"

#ifdef MCENGINE_FEATURE_SDL
#include "Mouse.h"
#include "ConsoleBox.h"

// everything below this point is untested, deprecated since SDL2->SDL3 transition

[[maybe_unused]] static inline bool isGamescopeInt()
{
	const char *xdgCurrentDesktop = std::getenv("XDG_CURRENT_DESKTOP");
	if (xdgCurrentDesktop != NULL)
	{
		const std::string stdXdgCurrentDesktop(xdgCurrentDesktop);
		return (stdXdgCurrentDesktop == "gamescope");
	}
	return false;
}

void SDLEnvironment::handleTouchEvent(SDL_Event event, Uint32 eventtype, Vector2 *mousePos)
{
	static const bool isGamescope = isGamescopeInt();
	static const bool isSteamDeckDoubletouchWorkaroundEnabled = (m_bIsSteamDeck && isGamescope && m_cvSdl_steamdeck_doubletouch_workaround->getBool());
	static std::vector<SDL_FingerID> touchingFingerIds;
	static SDL_TouchID currentTouchId = 0;

	// sanity, ensure any lost SDL_FINGERUP events (even though this should be impossible) don't kill cursor movement for the rest of the session
	{
		int fingerCount;
		if (SDL_GetTouchFingers(currentTouchId, &fingerCount) && fingerCount < 1) // FIXME: this is wrong, probably (SDL2->SDL3)
			touchingFingerIds.clear();
	}

	switch (eventtype)
	{
		case SDL_EVENT_FINGER_DOWN :
		{
			if (sdlDebug())
				debugLog("SDL_FINGERDOWN: touchId = %i, fingerId = %i, x = %f, y = %f\n",
					(int) event.tfinger.touchID,
					(int) event.tfinger.fingerID,
					event.tfinger.x,
					event.tfinger.y);

			setWasLastMouseInputTouch(true);

			currentTouchId = event.tfinger.touchID;

			bool isFingerIdAlreadyTouching = false;
			for (const SDL_FingerID &touchingFingerId : touchingFingerIds)
			{
				if (touchingFingerId == event.tfinger.fingerID)
				{
					isFingerIdAlreadyTouching = true;
					break;
				}
			}

			if (!isFingerIdAlreadyTouching || isSteamDeckDoubletouchWorkaroundEnabled)
			{
				touchingFingerIds.push_back(event.tfinger.fingerID);

				if (!isSteamDeckDoubletouchWorkaroundEnabled || isFingerIdAlreadyTouching)
				{
					if (touchingFingerIds.size() < (isSteamDeckDoubletouchWorkaroundEnabled ? 3 : 2))
					{
						*mousePos = Vector2(event.tfinger.x, event.tfinger.y) * m_engine->getScreenSize();
						setMousePos(mousePos->x, mousePos->y);
						m_engine->getMouse()->onPosChange(*mousePos);

						if (m_engine->getMouse()->isLeftDown())
							m_engine->onMouseLeftChange(false);

						m_engine->onMouseLeftChange(true);
					}
					else
					{
						if (m_engine->getMouse()->isLeftDown())
							m_engine->onMouseLeftChange(false);

						m_engine->onMouseLeftChange(true);
					}
				}
			}
		}
		break;

		case SDL_EVENT_FINGER_UP :
		{
			if (sdlDebug())
				debugLog("SDL_FINGERUP: touchId = %i, fingerId = %i, x = %f, y = %f\n",
					(int) event.tfinger.touchID,
					(int) event.tfinger.fingerID,
					event.tfinger.x,
					event.tfinger.y);

			setWasLastMouseInputTouch(true);

			currentTouchId = event.tfinger.touchID;

			// NOTE: also removes the finger from the touchingFingerIds list
			bool wasFingerIdAlreadyTouching = false;
			{
				size_t numFingerIdTouches = 0;
				for (size_t i=0; i<touchingFingerIds.size(); i++)
				{
					if (touchingFingerIds[i] == event.tfinger.fingerID)
					{
						wasFingerIdAlreadyTouching = true;
						numFingerIdTouches++;

						if (isSteamDeckDoubletouchWorkaroundEnabled)
							continue;

						touchingFingerIds.erase(touchingFingerIds.begin() + i);
						i--;
					}
				}

				if (isSteamDeckDoubletouchWorkaroundEnabled)
				{
					// cleanup on "last" release (the second one)
					if (numFingerIdTouches > 1)
					{
						for (size_t i=0; i<touchingFingerIds.size(); i++)
						{
							if (touchingFingerIds[i] == event.tfinger.fingerID)
							{
								touchingFingerIds.erase(touchingFingerIds.begin() + i);
								i--;
							}
						}
					}
				}
			}

			if (wasFingerIdAlreadyTouching)
			{
				if (event.tfinger.fingerID == touchingFingerIds[0])
					m_engine->onMouseLeftChange(false);
			}
		}
		break;

		case SDL_EVENT_FINGER_MOTION :
		{
			if (sdlDebug())
				debugLog("SDL_FINGERMOTION: touchId = %i, fingerId = %i, x = %f, y = %f, dx = %f, dy = %f\n",
					(int) event.tfinger.touchID,
					(int) event.tfinger.fingerID,
					event.tfinger.x,
					event.tfinger.y,
					event.tfinger.dx,
					event.tfinger.dy);

			setWasLastMouseInputTouch(true);

			currentTouchId = event.tfinger.touchID;

			bool isFingerIdTouching = false;
			for (size_t i=0; i<touchingFingerIds.size(); i++)
			{
				if (touchingFingerIds[i] == event.tfinger.fingerID)
				{
					isFingerIdTouching = true;
					break;
				}
			}

			if (isFingerIdTouching)
			{
				if (event.tfinger.fingerID == touchingFingerIds[0])
				{
					*mousePos = Vector2(event.tfinger.x, event.tfinger.y) * m_engine->getScreenSize();
					setMousePos(mousePos->x, mousePos->y);
					m_engine->getMouse()->onPosChange(*mousePos);
				}
			}
		}
		break;
	}
}

void SDLEnvironment::handleJoystickEvent(SDL_Event event, Uint32 eventtype)
{
	static bool xDown = false;

	static bool zlDown = false;
	static bool zrDown = false;

	static bool hatUpDown = false;
	static bool hatDownDown = false;
	static bool hatLeftDown = false;
	static bool hatRightDown = false;

	switch (eventtype)
	{
		case SDL_EVENT_JOYSTICK_BUTTON_DOWN :
		if (sdlDebug())
			debugLog("SDL_JOYBUTTONDOWN: joystickId = %i, button = %i\n", (int)event.jbutton.which, (int)event.jbutton.button);

		if (event.jbutton.button == 0) // KEY_A
		{
			m_engine->onMouseLeftChange(true);

			if (m_engine->getConsoleBox()->isActive())
			{
				m_engine->onKeyboardKeyDown(SDL_SCANCODE_RETURN);
				m_engine->onKeyboardKeyUp(SDL_SCANCODE_RETURN);
			}
		}
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 10 : event.jbutton.button == 7) || event.jbutton.button == 1) // KEY_PLUS/KEY_START || KEY_B
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_ESCAPE);
		else if (event.jbutton.button == 2) // KEY_X
		{
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_X);
			xDown = true;
		}
		else if (event.jbutton.button == 3) // KEY_Y
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_Y);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 21 : false) || (Env::cfg(OS::HORIZON) ? event.jbutton.button == 13 : false)) // right stick up || dpad up
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_UP);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 23 : false) || (Env::cfg(OS::HORIZON) ? event.jbutton.button == 15 : false)) // right stick down || dpad down
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_DOWN);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 20 : false) || (Env::cfg(OS::HORIZON) ? event.jbutton.button == 12 : false)) // right stick left || dpad left
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_LEFT);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 22 : false) || (Env::cfg(OS::HORIZON) ? event.jbutton.button == 14 : false)) // right stick right || dpad right
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_RIGHT);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 6 : event.jbutton.button == 4)) // KEY_L
			m_engine->onKeyboardKeyDown((Env::cfg(OS::HORIZON) ? SDL_SCANCODE_L : SDL_SCANCODE_BACKSPACE));
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 7 : event.jbutton.button == 5)) // KEY_R
			m_engine->onKeyboardKeyDown((Env::cfg(OS::HORIZON) ? SDL_SCANCODE_R : SDL_SCANCODE_LSHIFT));
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 8 : false)) // KEY_ZL
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_Z);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 9 : false)) // KEY_ZR
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_V);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 11 : event.jbutton.button == 6)) // KEY_MINUS/KEY_SELECT
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_F1);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 4 : event.jbutton.button == 9)) // left stick press
		{
			// toggle options (CTRL + O)
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_LCTRL);
			m_engine->onKeyboardKeyDown(SDL_SCANCODE_O);
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_LCTRL);
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_O);
		}
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 5 : event.jbutton.button == 10)) // right stick press
		{
			if (xDown)
			{
				// toggle console
				m_engine->onKeyboardKeyDown(SDL_SCANCODE_LSHIFT);
				m_engine->onKeyboardKeyDown(SDL_SCANCODE_F1);
				m_engine->onKeyboardKeyUp(SDL_SCANCODE_LSHIFT);
				m_engine->onKeyboardKeyUp(SDL_SCANCODE_F1);
			}
			else
			{
#ifdef __SWITCH__
				((HorizonSDLEnvironment*)environment)->showKeyboard();
#endif
			}
		}
		break;

	case SDL_EVENT_JOYSTICK_BUTTON_UP :
		if (event.jbutton.button == 0) // KEY_A
			m_engine->onMouseLeftChange(false);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 10 : event.jbutton.button == 7) || event.jbutton.button == 1) // KEY_PLUS/KEY_START || KEY_B
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_ESCAPE);
		else if (event.jbutton.button == 2) // KEY_X
		{
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_X);
			xDown = false;
		}
		else if (event.jbutton.button == 3) // KEY_Y
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_Y);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 21 : false) || (Env::cfg(OS::HORIZON) ? event.jbutton.button == 13 : false)) // right stick up || dpad up
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_UP);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 23 : false) || (Env::cfg(OS::HORIZON) ? event.jbutton.button == 15 : false)) // right stick down || dpad down
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_DOWN);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 20 : false) || (Env::cfg(OS::HORIZON) ? event.jbutton.button == 12 : false)) // right stick left || dpad left
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_LEFT);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 22 : false) || (Env::cfg(OS::HORIZON) ? event.jbutton.button == 14 : false)) // right stick right || dpad right
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_RIGHT);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 6 : event.jbutton.button == 4)) // KEY_L
			m_engine->onKeyboardKeyUp((Env::cfg(OS::HORIZON) ? SDL_SCANCODE_L : SDL_SCANCODE_BACKSPACE));
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 7 : event.jbutton.button == 5)) // KEY_R
			m_engine->onKeyboardKeyUp((Env::cfg(OS::HORIZON) ? SDL_SCANCODE_R : SDL_SCANCODE_LSHIFT));
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 8 : false)) // KEY_ZL
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_Z);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 9 : false)) // KEY_ZR
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_V);
		else if ((Env::cfg(OS::HORIZON) ? event.jbutton.button == 11 : event.jbutton.button == 6)) // KEY_MINUS/KEY_SELECT
			m_engine->onKeyboardKeyUp(SDL_SCANCODE_F1);
		break;

	case SDL_EVENT_JOYSTICK_AXIS_MOTION :
		//debugLog("joyaxismotion: stick %i : axis = %i, value = %i\n", (int)event.jaxis.which, (int)event.jaxis.axis, (int)event.jaxis.value);
		// left stick
		if (event.jaxis.axis == 1 || event.jaxis.axis == 0)
		{
			if (event.jaxis.axis == 0)
				m_fJoystick0XPercent = clamp<float>((float)event.jaxis.value / 32767.0f, -1.0f, 1.0f);
			else
				m_fJoystick0YPercent = clamp<float>((float)event.jaxis.value / 32767.0f, -1.0f, 1.0f);
		}
		if constexpr (!Env::cfg(OS::HORIZON))
		{
			// ZL/ZR
			if (event.jaxis.axis == 2 || event.jaxis.axis == 5)
			{
				if (event.jaxis.axis == 2)
				{
					const float threshold = m_cvSdl_joystick_zl_threshold->getFloat();
					const float percent = clamp<float>((float)event.jaxis.value / 32767.0f, -1.0f, 1.0f);
					const bool wasZlDown = zlDown;
					zlDown = !(threshold <= 0.0f ? percent <= threshold : percent >= threshold);
					if (zlDown != wasZlDown)
					{
						if (zlDown)
							m_engine->onKeyboardKeyDown(SDL_SCANCODE_KP_MINUS);
						else
							m_engine->onKeyboardKeyUp(SDL_SCANCODE_KP_MINUS);
					}
				}
				else
				{
					const float threshold = m_cvSdl_joystick_zr_threshold->getFloat();
					const float percent = clamp<float>((float)event.jaxis.value / 32767.0f, -1.0f, 1.0f);
					const bool wasZrDown = zrDown;
					zrDown = !(threshold <= 0.0f ? percent <= threshold : percent >= threshold);
					if (zrDown != wasZrDown)
					{
						if (zrDown)
							m_engine->onKeyboardKeyDown(SDL_SCANCODE_KP_PLUS);
						else
							m_engine->onKeyboardKeyUp(SDL_SCANCODE_KP_PLUS);
					}
				}
			}
		}
		break;

	case SDL_EVENT_JOYSTICK_HAT_MOTION :
		//debugLog("joyhatmotion: hat %i : value = %i\n", (int)event.jhat.hat, (int)event.jhat.value);
		if constexpr (!Env::cfg(OS::HORIZON))
		{
			const bool wasHatUpDown = hatUpDown;
			const bool wasHatDownDown = hatDownDown;
			const bool wasHatLeftDown = hatLeftDown;
			const bool wasHatRightDown = hatRightDown;

			hatUpDown = (event.jhat.value == SDL_HAT_UP);
			hatDownDown = (event.jhat.value == SDL_HAT_DOWN);
			hatLeftDown = (event.jhat.value == SDL_HAT_LEFT);
			hatRightDown = (event.jhat.value == SDL_HAT_RIGHT);

			if (hatUpDown != wasHatUpDown)
			{
				if (hatUpDown)
					m_engine->onKeyboardKeyDown(SDL_SCANCODE_UP);
				else
					m_engine->onKeyboardKeyUp(SDL_SCANCODE_UP);
			}

			if (hatDownDown != wasHatDownDown)
			{
				if (hatDownDown)
					m_engine->onKeyboardKeyDown(SDL_SCANCODE_DOWN);
				else
					m_engine->onKeyboardKeyUp(SDL_SCANCODE_DOWN);
			}

			if (hatLeftDown != wasHatLeftDown)
			{
				if (hatLeftDown)
					m_engine->onKeyboardKeyDown(SDL_SCANCODE_LEFT);
				else
					m_engine->onKeyboardKeyUp(SDL_SCANCODE_LEFT);
			}

			if (hatRightDown != wasHatRightDown)
			{
				if (hatRightDown)
					m_engine->onKeyboardKeyDown(SDL_SCANCODE_RIGHT);
				else
					m_engine->onKeyboardKeyUp(SDL_SCANCODE_RIGHT);
			}
		}
		break;
	}
}

void SDLEnvironment::handleJoystickMouse(Vector2 *mousePos)
{
	// apply deadzone
	float joystick0XPercent = m_fJoystick0XPercent;
	float joystick0YPercent = m_fJoystick0YPercent;
	{
		const float joystick0DeadzoneX = m_cvSdl_joystick0_deadzone->getFloat();
		const float joystick0DeadzoneY = m_cvSdl_joystick0_deadzone->getFloat();

		if (joystick0DeadzoneX > 0.0f && joystick0DeadzoneX < 1.0f)
		{
			const float deltaAbs = (std::abs(m_fJoystick0XPercent) - joystick0DeadzoneX);
			joystick0XPercent = (deltaAbs > 0.0f ? (deltaAbs / (1.0f - joystick0DeadzoneX)) * (float)signbit(m_fJoystick0XPercent) : 0.0f);
		}

		if (joystick0DeadzoneY > 0.0f && joystick0DeadzoneY < 1.0f)
		{
			const float deltaAbs = (std::abs(m_fJoystick0YPercent) - joystick0DeadzoneY);
			joystick0YPercent = (deltaAbs > 0.0f ? (deltaAbs / (1.0f - joystick0DeadzoneY)) * (float)signbit(m_fJoystick0YPercent) : 0.0f);
		}
	}

	if (m_bHasFocus && !m_bMinimized && (joystick0XPercent != 0.0f || joystick0YPercent != 0.0f))
	{
		const float hardcodedMultiplier = 1000.0f;
		const Vector2 hardcodedResolution = Vector2(1280, 720);
		Vector2 joystickDelta = Vector2(joystick0XPercent * m_cvSdl_joystick_mouse_sensitivity->getFloat(), joystick0YPercent * m_cvSdl_joystick_mouse_sensitivity->getFloat()) * m_engine->getFrameTime() * hardcodedMultiplier;
		joystickDelta *= m_engine->getScreenSize().x/hardcodedResolution.x > m_engine->getScreenSize().y/hardcodedResolution.y ?
							m_engine->getScreenSize().y/hardcodedResolution.y : m_engine->getScreenSize().x/hardcodedResolution.x; // normalize

		*mousePos += joystickDelta;
		mousePos->x = clamp<float>(mousePos->x, 0.0f, m_engine->getScreenSize().x - 1);
		mousePos->y = clamp<float>(mousePos->y, 0.0f, m_engine->getScreenSize().y - 1);

		setWasLastMouseInputTouch(false);

		setMousePos(mousePos->x, mousePos->y);
		m_engine->getMouse()->onPosChange(*mousePos);
	}
}

#endif
