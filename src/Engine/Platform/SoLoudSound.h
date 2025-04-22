//========== Copyright (c) 2025, WH, All rights reserved. ============//
//
// Purpose:		SoLoud-specific sound implementation
//
// $NoKeywords: $snd $soloud
//===============================================================================//

#pragma once
#ifndef SOLOUD_SOUND_H
#define SOLOUD_SOUND_H

#include "Sound.h"
#ifdef MCENGINE_FEATURE_SOLOUD

#include <soloud/soloud.h>
#include <soloud/soloud_file.h>
#include <soloud/soloud_wav.h>
#include <soloud/soloud_wavstream.h>

class SoLoudSoundEngine;

class SoLoudSound : public Sound
{
	friend class SoLoudSoundEngine;

public:
	SoLoudSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~SoLoudSound() override;

	// sound interface implementation
	void setPosition(double percent) override;
	void setPositionMS(unsigned long ms, bool internal) override;
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

private:
	void init() override;
	void initAsync() override;
	void destroy() override;

	// get the engine instance (will try to set m_engine if it's null)
	SoLoudSoundEngine *getSoLoudEngine();

	// helper methods to access specific implementations
	inline SoLoud::Wav *asWav() { return m_bStream ? nullptr : static_cast<SoLoud::Wav *>(m_audioSource); }

	inline SoLoud::WavStream *asWavStream() { return m_bStream ? static_cast<SoLoud::WavStream *>(m_audioSource) : nullptr; }

	// execute engine command if handle and engine are valid with return value
	template <typename ReturnType = bool, typename... Args>
	ReturnType sndEngine(ReturnType defaultValue, ReturnType (SoLoudSoundEngine::*command)(unsigned int, Args...), Args... args)
	{
		if (m_handle == 0)
			return defaultValue;

		SoLoudSoundEngine *engine = getSoLoudEngine();
		if (!engine)
			return defaultValue;

		return (engine->*command)(m_handle, args...);
	}

	// void version (no return value)
	template <typename... Args> void sndEngine(void (SoLoudSoundEngine::*command)(unsigned int, Args...), Args... args)
	{
		if (m_handle == 0)
			return;

		SoLoudSoundEngine *engine = getSoLoudEngine();
		if (!engine)
			return;

		(engine->*command)(m_handle, args...);
	}

	// SoLoud specific members
	SoLoud::AudioSource *m_audioSource; // base class pointer to either Wav or WavStream
	unsigned int m_handle;

	// cache the engine pointer to avoid repeated dynamic_cast
	SoLoudSoundEngine *m_engine;

	// track values since SoLoud doesn't have direct getters for some properties
	float m_speed;
	float m_pitch;
	float m_frequency;
	unsigned long m_prevPosition;
};

#endif
#endif
