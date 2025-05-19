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

#include "Engine.h"
#include "SoundTouchFilter.h"

class SoLoudSound : public Sound
{
	friend class SoLoudSoundEngine;

public:
	SoLoudSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~SoLoudSound() override;

	// Sound interface implementation
	void setPosition(double percent) override;
	void setPositionMS(unsigned long ms, bool internal = false) override;
	void setVolume(float volume) override;
	void setSpeed(float speed) override;
	void setPitch(float pitch) override;
	void setFrequency(float frequency) override;
	void setPan(float pan) override;
	void setLoop(bool loop) override;
	void setOverlayable(bool overlayable) override;

	SOUNDHANDLE getHandle() override;
	constexpr SoundType *getSound() override { return this; }
	[[nodiscard]] constexpr const SoundType *getSound() const override { return this; }
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

	// similar idea to the ugly "m_MixChunkOrMixMusic" casting thing in the SDL_mixer implementation
	// WavStreams are for beatmap audio, streamed from disk, Wavs are for other (shorter) audio samples, loaded entirely into memory
	[[nodiscard]] inline SoLoud::Wav *asWav() const { return m_bStream ? nullptr : static_cast<SoLoud::Wav *>(m_audioSource); }
	[[nodiscard]] inline SoLoud::WavStream *asWavStream() const { return m_bStream ? static_cast<SoLoud::WavStream *>(m_audioSource) : nullptr; }

	// pitch/tempo filter management methods
	[[nodiscard]] inline SoLoud::SoundTouchFilter *getFilterInstance() const { return m_filter; }
	bool updateFilterParameters();

	// current playback parameters
	float m_speed;     // speed factor (1.0 = normal)
	float m_pitch;     // pitch factor (1.0 = normal)
	float m_frequency; // sample rate in Hz

	// SoLoud-specific members
	SoLoud::AudioSource *m_audioSource; // base class pointer, could be either Wav or WavStream
	SoLoud::SoundTouchFilter *m_filter; // SoundTouch filter instance
	unsigned int m_handle;              // current voice (i.e. "Sound") handle

	// nightcore things (TODO: this might not be needed)
	float m_fActualSpeedForDisabledPitchCompensation;

	// position interp
	double m_fLastRawSoLoudPosition;  // last raw position reported in milliseconds
	double m_fLastSoLoudPositionTime; // engine time when the last position was obtained
	double m_fSoLoudPositionRate;     // estimated playback rate (position units per second)
};

#else
class SoLoudSound : public Sound
{};
#endif
#endif
