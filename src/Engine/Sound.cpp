//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		sound wrapper base class
//
// $NoKeywords: $snd
//===============================================================================//

#include "Sound.h"
#include "BassSound.h"
#include "ConVar.h"
#include "SDLSound.h"

ConVar debug_snd("debug_snd", false, FCVAR_NONE);
ConVar snd_speed_compensate_pitch("snd_speed_compensate_pitch", true, FCVAR_NONE, "automatically keep pitch constant if speed changes");
ConVar snd_play_interp_duration("snd_play_interp_duration", 0.75f, FCVAR_NONE,
                                "smooth over freshly started channel position jitter with engine time over this duration in seconds");
ConVar snd_play_interp_ratio("snd_play_interp_ratio", 0.50f, FCVAR_NONE,
                             "percentage of snd_play_interp_duration to use 100% engine time over audio time (some devices report 0 for very long)");
ConVar snd_wav_file_min_size(
    "snd_wav_file_min_size", 51, FCVAR_NONE,
    "minimum file size in bytes for WAV files to be considered valid (everything below will fail to load), this is a workaround for BASS crashes");

Sound::Sound(UString filepath, bool stream, bool threeD, bool loop, bool prescan) : Resource(filepath)
{
	m_bStream = stream;
	m_bIs3d = threeD;
	m_bIsLooped = loop;
	m_bPrescan = prescan;
	m_bIsOverlayable = false;

	m_fVolume = 1.0f;
	m_fLastPlayTime = -1.0f;
}

Sound::~Sound() = default;

Sound *Sound::createSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan)
{
#ifdef MCENGINE_FEATURE_BASS
	return new BassSound(filepath, stream, threeD, loop, prescan);
#elif defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_SDL_MIXER)
	return new SDLSound(filepath, stream, threeD, loop, prescan);
#else
#error No sound backend available!
#endif
}
