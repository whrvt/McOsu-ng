//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoLoud-specific sound implementation
//
// $NoKeywords: $snd $soloud
//================================================================================//

#pragma once
#ifndef SOLOUD_SOUND_H
#define SOLOUD_SOUND_H

#include "Sound.h"
#ifdef MCENGINE_FEATURE_SOLOUD

#include <soloud/soloud.h>
#include <soloud/soloud_file.h>
#include <soloud/soloud_wav.h>
#include <soloud/soloud_wavstream.h>

#include "SoundTouchFilter.h"

class SoLoudSoundEngine;

class SoLoudSound : public Sound
{
	friend class SoLoudSoundEngine;

public:
	SoLoudSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~SoLoudSound() override;

	// Sound interface implementation
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

	// Helper method to get the engine instance (cached after first call)
	SoLoudSoundEngine *getSoLoudEngine();

	// Helper methods to access specific implementations
	[[nodiscard]] inline SoLoud::Wav *asWav() const { return m_bStream ? nullptr : static_cast<SoLoud::Wav *>(m_audioSource); }
	[[nodiscard]] inline SoLoud::WavStream *asWavStream() const { return m_bStream ? static_cast<SoLoud::WavStream *>(m_audioSource) : nullptr; }

	// Filter management methods
	SoLoud::SoundTouchFilter *getOrCreateFilter();
	bool updateFilterParameters();

	// Current parameter values
	float m_speed;     // Playback speed factor (1.0 = normal)
	float m_pitch;     // Pitch factor (1.0 = normal)
	float m_frequency; // Sample rate in Hz

	// SoLoud specific members
	SoLoud::AudioSource *m_audioSource; // Base class pointer to either Wav or WavStream
	SoLoud::SoundTouchFilter *m_filter; // SoundTouch filter instance
	unsigned int m_handle;              // Current voice handle

	// Flags
	bool m_usingFilter;       // Whether we're currently using a filter

	// nightcore things
	float m_fActualSpeedForDisabledPitchCompensation;

	// position interp
	double m_fLastRawSoLoudPosition;  // last raw position reported in milliseconds
	double m_fLastSoLoudPositionTime; // engine time when the last position was obtained
	double m_fSoLoudPositionRate;     // estimated playback rate (position units per second)

	// Cache the engine pointer to avoid repeated dynamic_cast
	SoLoudSoundEngine *m_engine;
};

#endif
#endif
