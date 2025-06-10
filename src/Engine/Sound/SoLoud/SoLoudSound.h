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
#include "SoLoudFX.h"

extern SoLoud::Soloud *soloud;

class SoLoudSound final : public Sound
{
	friend class SoLoudSoundEngine;

public:
	SoLoudSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~SoLoudSound() override;

	// Sound interface implementation
	void setPosition(double percent) override;
	void setPositionMS(unsigned long ms) override;
	void setVolume(float volume) override;
	void setSpeed(float speed) override;
	void setPitch(float pitch) override;
	void setFrequency(float frequency) override;
	void setPan(float pan) override;
	void setLoop(bool loop) override;
	void setOverlayable(bool overlayable) override;

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

	// inspection
	SOUND_TYPE(SoLoudSound, SOLOUD, Sound)
private:
	void init() override;
	void initAsync() override;
	void destroy() override;

	[[nodiscard]] inline double getSourceLengthInSeconds() const
	{
		if (!m_audioSource)
			return 0.0;
		if (m_bStream)
			return static_cast<SoLoud::SLFXStream *>(m_audioSource)->getLength();
		else
			return static_cast<SoLoud::Wav *>(m_audioSource)->getLength();
	}

	[[nodiscard]] inline double getStreamPositionInSeconds() const
	{
		if (!m_audioSource)
			return m_fLastSoLoudPositionTime;
		if (m_bStream)
			return static_cast<SoLoud::SLFXStream *>(m_audioSource)->getRealStreamPosition();
		else
			return m_handle ? soloud->getStreamPosition(m_handle) : m_fLastSoLoudPositionTime;
	}

	// current playback parameters
	float m_speed;     // speed factor (1.0 = normal)
	float m_pitch;     // pitch factor (1.0 = normal)
	float m_frequency; // sample rate in Hz

	// SoLoud-specific members
	SoLoud::AudioSource *m_audioSource; // base class pointer, could be either SLFXStream or Wav
	SOUNDHANDLE m_handle;               // current voice (i.e. "Sound") handle

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
