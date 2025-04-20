//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		sound engine base class
//
// $NoKeywords: $snd
//===============================================================================//

#include "SoundEngine.h"
#include "BassSoundEngine.h"
#include "SDLSoundEngine.h"

#include "ConVar.h"
#include "Engine.h"

// this shouldn't be here, but McKay hardcoded it in places where it breaks if it doesnt exist...
ConVar win_snd_fallback_dsound("win_snd_fallback_dsound", false, FCVAR_NONE, "use DirectSound instead of WASAPI");

ConVar snd_output_device("snd_output_device", "Default", FCVAR_NONE);
ConVar snd_restart("snd_restart");
ConVar snd_freq("snd_freq", 44100, FCVAR_NONE, "output sampling rate in Hz");
ConVar snd_restrict_play_frame("snd_restrict_play_frame", true, FCVAR_NONE,
                               "only allow one new channel per frame for overlayable sounds (prevents lag and earrape)");
ConVar snd_change_check_interval("snd_change_check_interval", 0.0f, FCVAR_NONE,
                                 "check for output device changes every this many seconds. 0 = disabled (default)");

void _volume(UString oldValue, UString newValue)
{
	engine->getSound()->setVolume(newValue.toFloat());
}

ConVar _volume_("volume", 1.0f, FCVAR_NONE, _volume);

SoundEngine::SoundEngine()
{
	m_bReady = false;
	m_fPrevOutputDeviceChangeCheckTime = 0.0f;
	m_outputDeviceChangeCallback = nullptr;
	m_fVolume = 1.0f;
}

SoundEngine::~SoundEngine() = default;

SoundEngine *SoundEngine::createSoundEngine()
{
#ifdef MCENGINE_FEATURE_BASS
	return new BassSoundEngine();
#elif defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_SDL_MIXER)
	return new SDLSoundEngine();
#else
#error No sound engine backend available!
#endif
}

void SoundEngine::setOnOutputDeviceChange(std::function<void()> callback)
{
	m_outputDeviceChangeCallback = callback;
}
