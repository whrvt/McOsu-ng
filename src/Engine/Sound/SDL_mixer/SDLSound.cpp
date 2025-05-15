//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		SDL_mixer-specific sound implementation
//
// $NoKeywords: $snd $sdl
//===============================================================================//

#include "SDLSound.h"
#include "SDLSoundEngine.h"

#if defined(MCENGINE_FEATURE_SDL_MIXER)

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "ResourceManager.h"

extern ConVar debug_snd;
extern ConVar snd_speed_compensate_pitch;
extern ConVar snd_play_interp_duration;
extern ConVar snd_play_interp_ratio;
extern ConVar snd_wav_file_min_size;

SDLSound::SDLSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan) : Sound(filepath, stream, threeD, loop, prescan)
{
	m_HCHANNEL = 0;
	m_mixChunkOrMixMusic = NULL;

	m_fLastRawSDLPosition = 0.0;
	m_fLastSDLPositionTime = 0.0;
	m_fSDLPositionRate = 1.0; // default to 1x rate (position units per second)
}

SDLSound::~SDLSound()
{
    destroy();
}

void SDLSound::init()
{
	if (m_sFilePath.length() < 2 || !(m_bAsyncReady.load()))
		return;
	m_bReady = m_bAsyncReady.load();
}

void SDLSound::initAsync()
{
	if (ResourceManager::debug_rm->getBool())
		debugLog("Resource Manager: Loading %s\n", m_sFilePath.toUtf8());

	// HACKHACK: workaround for malformed WAV files
	{
		const int minWavFileSize = snd_wav_file_min_size.getInt();
		if (minWavFileSize > 0)
		{
			UString fileExtensionLowerCase = env->getFileExtensionFromFilePath(m_sFilePath);
			fileExtensionLowerCase.lowerCase();
			if (fileExtensionLowerCase == "wav")
			{
				McFile wavFile(m_sFilePath);
				if (wavFile.getFileSize() < (size_t)minWavFileSize)
				{
					debugLog("Sound: Ignoring malformed/corrupt WAV file (%i) %s\n", (int)wavFile.getFileSize(), m_sFilePath.toUtf8());
					return;
				}
			}
		}
	}

	if (m_bStream)
		m_mixChunkOrMixMusic = Mix_LoadMUS(m_sFilePath.toUtf8());
	else
		m_mixChunkOrMixMusic = Mix_LoadWAV(m_sFilePath.toUtf8());

	if (m_mixChunkOrMixMusic == NULL)
		debugLog(m_bStream ? "Sound Error: Mix_LoadMUS() error %s on file %s\n" : "Sound Error: Mix_LoadWAV() error %s on file %s\n", SDL_GetError(),
		         m_sFilePath.toUtf8());

	m_bAsyncReady = (m_mixChunkOrMixMusic != NULL);
}

SDLSound::SOUNDHANDLE SDLSound::getHandle()
{
	return m_HCHANNEL;
}

void SDLSound::destroy()
{
	if (!m_bReady)
		return;

	m_bReady = false;

	if (m_bStream)
		Mix_FreeMusic((Mix_Music *)m_mixChunkOrMixMusic);
	else
		Mix_FreeChunk((Mix_Chunk *)m_mixChunkOrMixMusic);

	m_mixChunkOrMixMusic = NULL;
}

void SDLSound::setPosition(double percent)
{
	if (!m_bReady)
		return;

	percent = std::clamp<double>(percent, 0.0, 1.0);

	if (m_bStream)
	{
		const double length = Mix_MusicDuration((Mix_Music *)m_mixChunkOrMixMusic);
		if (length > 0.0)
		{
			Mix_RewindMusic();

			const double targetPositionS = length * percent;
			if (!Mix_SetMusicPosition(targetPositionS) && targetPositionS > 0.1)
				debugLog("Mix_SetMusicPosition(%.2f) failed! SDL Error: %s\n", targetPositionS, SDL_GetError());

			const double targetPositionMS = targetPositionS * 1000.0;

			// reset interpolation state
			m_fLastRawSDLPosition = targetPositionMS;
			m_fLastSDLPositionTime = engine->getTime();
			m_fSDLPositionRate = 1000.0 * getSpeed();

			// NOTE: Mix_SetMusicPosition()/scrubbing is inaccurate depending on the underlying decoders, approach in 1 second increments
			double positionMS = targetPositionMS;
			double actualPositionMS = getPositionMS();
			double deltaMS = actualPositionMS - targetPositionMS;
			int loopCounter = 0;
			while (std::abs(deltaMS) > 1.1 * 1000.0)
			{
				positionMS -= std::signbit(deltaMS) * 1000.0;

				if (!Mix_SetMusicPosition(positionMS / 1000.0) && (positionMS / 1000.0) > 0.1)
					debugLog("Mix_SetMusicPosition(%.2f) failed! SDL Error: %s\n", positionMS / 1000.0, SDL_GetError());

				// update our internal state after each position adjustment
				m_fLastRawSDLPosition = positionMS;
				m_fLastSDLPositionTime = engine->getTime();

				actualPositionMS = getPositionMS();
				deltaMS = actualPositionMS - targetPositionMS;

				loopCounter++;
				if (loopCounter > 10000)
					break;
			}
		}
		else if (almostEqual(percent, 0.0))
			Mix_RewindMusic();
		else if (length < 0.5)
			debugLog("Mix_MusicDuration failed! length: %.2f\n", length);
	}
}

void SDLSound::setPositionMS(unsigned long ms, bool internal)
{
	if (!m_bReady || ms > getLengthMS())
		return;

	if (m_bStream)
	{
		Mix_RewindMusic();

		if (!Mix_SetMusicPosition((double)ms / 1000.0) && (double)ms / 1000.0 > 0.1)
			debugLog("Mix_SetMusicPosition(%.2f) failed! SDL Error: %s\n", (double)ms / 1000.0, SDL_GetError());

		// reset interpolation state
		m_fLastRawSDLPosition = ms;
		m_fLastSDLPositionTime = engine->getTime();
		m_fSDLPositionRate = 1000.0 * getSpeed();

		// NOTE: Mix_SetMusicPosition()/scrubbing is inaccurate depending on the underlying decoders, approach in 1 second increments
		const double targetPositionMS = (double)ms;
		double positionMS = targetPositionMS;
		double actualPositionMS = getPositionMS();
		double deltaMS = actualPositionMS - targetPositionMS;
		int loopCounter = 0;
		while (std::abs(deltaMS) > 1.1 * 1000.0)
		{
			positionMS -= std::signbit(deltaMS) * 1000.0;

			if (!Mix_SetMusicPosition(positionMS / 1000.0) && (positionMS / 1000.0) > 0.1)
				debugLog("Mix_SetMusicPosition(%.2f) failed! SDL Error: %s\n", positionMS / 1000.0, SDL_GetError());

			// update our internal state after each position adjustment
			m_fLastRawSDLPosition = positionMS;
			m_fLastSDLPositionTime = engine->getTime();

			actualPositionMS = getPositionMS();
			deltaMS = actualPositionMS - targetPositionMS;

			loopCounter++;
			if (loopCounter > 10000)
				break;
		}
	}
}

void SDLSound::setVolume(float volume)
{
	if (!m_bReady)
		return;

	m_fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

	if (m_bStream)
	{
		SDLSoundEngine *soundEngine = engine->getSound()->getSndEngine();
		if (soundEngine != nullptr)
		{
			soundEngine->setVolumeMixMusic(m_fVolume);
			Mix_VolumeMusic((int)(m_fVolume * soundEngine->getVolume() * MIX_MAX_VOLUME));
		}
	}
	else
		Mix_VolumeChunk((Mix_Chunk *)m_mixChunkOrMixMusic, (int)(m_fVolume * MIX_MAX_VOLUME));
}

void SDLSound::setSpeed(float speed)
{
	// Not directly supported by SDL_mixer
	// However, we track this for our position interpolation
	if (!m_bReady)
		return;

	speed = std::clamp<float>(speed, 0.05f, 50.0f);

	// Update our rate for position interpolation
	if (m_bStream && isPlaying())
	{
		m_fSDLPositionRate = 1000.0 * speed;
	}
}

void SDLSound::setPitch(float pitch)
{
	// Not directly supported by SDL_mixer
	if (!m_bReady)
		return;
}

void SDLSound::setFrequency(float frequency)
{
	// Not directly supported by SDL_mixer
	if (!m_bReady)
		return;
}

void SDLSound::setPan(float pan)
{
	if (!m_bReady)
		return;

	pan = std::clamp<float>(pan, -1.0f, 1.0f);

	if (!m_bStream)
	{
		const float rangeHalfLimit = 96.0f; // NOTE: trying to match BASS behavior
		const int left = (int)std::lerp(rangeHalfLimit / 2.0f, 254.0f - rangeHalfLimit / 2.0f, 1.0f - ((pan + 1.0f) / 2.0f));
		Mix_SetPanning(getHandle(), left, 254 - left);
	}
}

void SDLSound::setLoop(bool loop)
{
	if (!m_bReady)
		return;
	m_bIsLooped = loop;
}

float SDLSound::getPosition()
{
	if (!m_bReady)
		return 0.0f;

	if (m_bStream)
	{
		const double length = Mix_MusicDuration((Mix_Music *)m_mixChunkOrMixMusic);
		if (length > 0.0)
		{
			const double position = Mix_GetMusicPosition((Mix_Music *)m_mixChunkOrMixMusic);
			if (position > -0.0)
				return (float)(position / length);
			else if (position < -0.5)
			{
				static int once = 0;
				if (!once++)
					debugLog("unsupported codec for Mix_GetMusicPosition! (returned %.2f)\n", position);
			}
		}
		else if (length < -0.5)
		{
			static int once = 0;
			if (!once++)
				debugLog("Mix_MusicDuration failed! (returned %.2f)\n", length);
		}
	}

	return 0.0f;
}

unsigned long SDLSound::getPositionMS()
{
	if (!m_bReady)
		return 0;

	if (m_bStream)
	{
		const double currentTime = engine->getTime();
		const double rawPosition = Mix_GetMusicPosition((Mix_Music *)m_mixChunkOrMixMusic);

		if (rawPosition >= 0.0)
		{
			const double rawPositionMS = rawPosition * 1000.0;

			// if this is our first reading or if we're not playing, just use the raw value
			if (m_fLastSDLPositionTime <= 0.0 || !isPlaying())
			{
				m_fLastRawSDLPosition = rawPositionMS;
				m_fLastSDLPositionTime = currentTime;
				m_fSDLPositionRate = 1000.0 * getSpeed(); // initialize rate
				return (unsigned long)rawPositionMS;
			}

			// if the position changed, update our rate estimate
			if (m_fLastRawSDLPosition != rawPositionMS)
			{
				const double timeDelta = currentTime - m_fLastSDLPositionTime;

				// only update rate if enough time has passed to avoid division by very small numbers
				if (timeDelta > 0.01)
				{
					// calculate new rate (change in position / change in time)
					// account for possibility of wrapping (looped sound) by ensuring positive rate
					double newRate = (rawPositionMS > m_fLastRawSDLPosition) ? (rawPositionMS - m_fLastRawSDLPosition) / timeDelta : m_fSDLPositionRate;

					// smooth the rate transition
					m_fSDLPositionRate = m_fSDLPositionRate * 0.7 + newRate * 0.3;
				}

				// store the new raw position and time
				m_fLastRawSDLPosition = rawPositionMS;
				m_fLastSDLPositionTime = currentTime;
			}

			// calculate the interpolated position based on time elapsed since last raw reading
			const double timeSinceLastReading = currentTime - m_fLastSDLPositionTime;
			const double interpolatedPosition = m_fLastRawSDLPosition + (timeSinceLastReading * m_fSDLPositionRate);

			return static_cast<unsigned long>(interpolatedPosition);
		}
		else if (rawPosition < -0.5)
		{
			static int once = 0;
			if (!once++)
				debugLog("unsupported codec for Mix_GetMusicPosition! (returned %.2f)\n", rawPosition);
		}
	}

	return 0;
}

unsigned long SDLSound::getLengthMS()
{
	if (!m_bReady)
		return 0;

	if (m_bStream)
	{
		const double length = Mix_MusicDuration((Mix_Music *)m_mixChunkOrMixMusic);
		if (length > -0.0)
			return (unsigned long)(length * 1000.0);
		else if (length < -0.5)
		{
			static int once = 0;
			if (!once++)
				debugLog("Mix_MusicDuration failed! (returned %.2f)\n", length);
		}
	}

	return 0;
}

float SDLSound::getSpeed()
{
	if (!m_bReady)
		return 1.0f;
	// SDL doesn't support speed directly, but we track it for our position interpolation
	return m_fSDLPositionRate / 1000.0f;
}

float SDLSound::getPitch()
{
	if (!m_bReady)
		return 1.0f;
	// Not supported by SDL_mixer
	return 1.0f;
}

float SDLSound::getFrequency()
{
	const float default_freq = convar->getConVarByName("snd_freq")->getFloat();
	if (!m_bReady)
		return default_freq;

	// Not supported by SDL_mixer
	return default_freq;
}

bool SDLSound::isPlaying()
{
	if (!m_bReady)
		return false;
	return (m_bStream ? Mix_PlayingMusic() && !Mix_PausedMusic() : Mix_Playing(m_HCHANNEL) && !Mix_Paused(m_HCHANNEL));
}

bool SDLSound::isFinished()
{
	if (!m_bReady)
		return false;
	return (m_bStream ? !Mix_PlayingMusic() && !Mix_PausedMusic() : !Mix_Playing(m_HCHANNEL) && !Mix_Paused(m_HCHANNEL));
}

void SDLSound::rebuild(UString newFilePath)
{
	m_sFilePath = newFilePath;
	reload();
}

#endif // defined(MCENGINE_FEATURE_SDL_MIXER)
