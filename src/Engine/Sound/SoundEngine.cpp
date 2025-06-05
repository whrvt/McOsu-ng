//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		sound engine base class
//
// $NoKeywords: $snd
//===============================================================================//

#include "BassSoundEngine.h"
#include "BassSoundEngine2.h"
#include "SDLSoundEngine.h"
#include "SoLoudSoundEngine.h"
#include "SoundEngine.h"

#include "ConVar.h"
#include "Engine.h"

namespace cv {
ConVar volume("volume", 1.0f, FCVAR_NONE, [](float newValue){soundEngine ? soundEngine->setVolume(newValue) : (void)0;});

ConVar snd_output_device("snd_output_device", "Default", FCVAR_NONE);
ConVar snd_restart("snd_restart");
ConVar snd_freq("snd_freq", 44100, FCVAR_NONE, "output sampling rate in Hz");
ConVar snd_restrict_play_frame("snd_restrict_play_frame", true, FCVAR_NONE,
                               "only allow one new channel per frame for overlayable sounds (prevents lag and earrape)");
ConVar snd_change_check_interval("snd_change_check_interval", 0.0f, FCVAR_NONE,
                                 "check for output device changes every this many seconds. 0 = disabled (default)");

// HACK: THIS SHOULD NOT BE HERE, TO BE BE REMOVED ONCE REFERENCES FROM GAME CODE ARE REMOVED
ConVar win_snd_fallback_dsound("win_snd_fallback_dsound", false, FCVAR_NONE, "use DirectSound instead of WASAPI");

ConVar win_snd_wasapi_buffer_size("win_snd_wasapi_buffer_size", 0.011f, FCVAR_NONE,
                                  "buffer size/length in seconds (e.g. 0.011 = 11 ms), directly responsible for audio delay and crackling");
ConVar win_snd_wasapi_period_size("win_snd_wasapi_period_size", 0.0f, FCVAR_NONE,
                                  "interval between OutputWasapiProc calls in seconds (e.g. 0.016 = 16 ms) (0 = use default)");
ConVar win_snd_wasapi_exclusive("win_snd_wasapi_exclusive", true, FCVAR_NONE, "whether to use exclusive device mode to further reduce latency");
ConVar win_snd_wasapi_shared_volume_affects_device("win_snd_wasapi_shared_volume_affects_device", false, FCVAR_NONE,
                                                   "if in shared mode, whether to affect device volume globally or use separate session volume (default)");
}

SoundEngine::SoundEngine()
{
	m_bReady = false;
	m_fPrevOutputDeviceChangeCheckTime = 0.0f;
	m_outputDeviceChangeCallback = nullptr;
	m_fVolume = 1.0f;
	m_iCurrentOutputDevice = -1;
}

SoundEngine::~SoundEngine() = default;

SoundEngine *SoundEngine::createSoundEngine()
{
#ifdef MCENGINE_FEATURE_BASS
	return new BassSoundEngine();
#elif defined(MCENGINE_FEATURE_SDL_MIXER)
	return new SDLSoundEngine();
#elif defined(MCENGINE_FEATURE_SOLOUD)
	return new SoLoudSoundEngine();
#else
#error No sound engine backend available!
#endif
}

void SoundEngine::setOnOutputDeviceChange(AudioOutputChangedCallback callback)
{
	m_outputDeviceChangeCallback = callback;
}
