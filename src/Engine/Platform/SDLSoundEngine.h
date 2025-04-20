//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		handles sounds using SDL_mixer library
//
// $NoKeywords: $snd $sdl
//===============================================================================//

#pragma once
#ifndef SDL_SOUNDENGINE_H
#define SDL_SOUNDENGINE_H

#include "SoundEngine.h"
#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_SDL_MIXER)

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

class SDLSound;

class SDLSoundEngine : public SoundEngine
{
public:
	SDLSoundEngine();
	~SDLSoundEngine() override;

	void restart() override;
	void update() override;

	bool play(Sound *snd, float pan = 0.0f, float pitch = 1.0f) override;
	bool play3d(Sound *snd, Vector3 pos) override;
	void pause(Sound *snd) override;
	void stop(Sound *snd) override;

	void setOutputDevice(UString outputDeviceName) override;
	void setOutputDeviceForce(UString outputDeviceName) override;
	void setVolume(float volume) override;
	void set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp) override;

	std::vector<UString> getOutputDevices() override;

	// SDL-specific methods
	void setMixChunkSize(int mixChunkSize) { m_iMixChunkSize = mixChunkSize; }
	void setVolumeMixMusic(float volumeMixMusic) { m_fVolumeMixMusic = volumeMixMusic; }
	inline int getMixChunkSize() const { return m_iMixChunkSize; }
	inline float getVolumeMixMusic() const { return m_fVolumeMixMusic; }

private:
	void updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo) override;
	bool initializeOutputDevice(int id = -1) override;

	int m_iMixChunkSize;
	float m_fVolumeMixMusic;
};

#endif
#endif
