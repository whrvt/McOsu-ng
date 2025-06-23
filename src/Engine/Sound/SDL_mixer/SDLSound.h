//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		SDL_mixer-specific sound implementation
//
// $NoKeywords: $snd $sdl
//===============================================================================//

#pragma once
#ifndef SDL_SOUND_H
#define SDL_SOUND_H

#include "Sound.h"

#if defined(MCENGINE_FEATURE_SDL_MIXER)

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

class SDLSoundEngine;

class SDLSound final : public Sound
{
	friend class SDLSoundEngine;

public:
	SDLSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~SDLSound() override;

	SDLSound &operator=(const SDLSound &) = delete;
	SDLSound &operator=(SDLSound &&) = delete;

	SDLSound(const SDLSound &) = delete;
	SDLSound(SDLSound &&) = delete;

	// Sound interface implementation
	void setPosition(double percent) override;
	void setPositionMS(unsigned long ms) override;
	void setVolume(float volume) override;
	void setSpeed(float speed) override;
	void setPitch(float pitch) override;
	void setFrequency(float frequency) override;
	void setPan(float pan) override;
	void setLoop(bool loop) override;

	SOUNDHANDLE getHandle() override;

	float getPosition() override;
	unsigned long getPositionMS() override;
	unsigned long getLengthMS() override;
	float getSpeed() override;
	float getPitch() override;
	float getFrequency() override;

	bool isPlaying() override;
	bool isFinished() override;

	void rebuild(UString newFilePath) override;
	[[nodiscard]] inline void *getMixChunkOrMixMusic() const { return m_mixChunkOrMixMusic; }

	// inspection
	SOUND_TYPE(SDLSound, SDL, Sound)
private:
	void init() override;
	void initAsync() override;
	void destroy() override;

	void *m_mixChunkOrMixMusic;
	double m_fLastRawSDLPosition;  // last raw position reported by Mix_GetMusicPosition
	double m_fLastSDLPositionTime; // engine time when the last position was obtained
	double m_fSDLPositionRate;     // estimated playback rate (position units per second)

	// Common SOUNDHANDLE for the interface
	inline void setHandle(SOUNDHANDLE handle) {m_HCHANNEL = handle;}
	SOUNDHANDLE m_HCHANNEL;
};

#else
class SDLSound : public Sound{};
#endif
#endif
