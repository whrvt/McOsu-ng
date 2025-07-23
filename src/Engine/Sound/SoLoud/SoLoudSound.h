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

#include "Engine.h"

// fwd decls to avoid include external soloud headers here
namespace SoLoud
{
class Soloud;
class AudioSource;
class SLFXStream;
} // namespace SoLoud

// defined in SoLoudSoundEngine
extern std::unique_ptr<SoLoud::Soloud> soloud;

class SoLoudSound final : public Sound
{
	friend class SoLoudSoundEngine;

public:
	SoLoudSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan)
	    : Sound(std::move(filepath), stream, threeD, loop, prescan)
	{
	}

	~SoLoudSound() override;

	SoLoudSound &operator=(const SoLoudSound &) = delete;
	SoLoudSound &operator=(SoLoudSound &&) = delete;

	SoLoudSound(const SoLoudSound &) = delete;
	SoLoudSound(SoLoudSound &&) = delete;

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
	inline float getFrequency() override { return m_fFrequency; }

	float getBPM() override;

	bool isPlaying() override;
	bool isFinished() override;

	void rebuild(UString newFilePath) override;

	// inspection
	SOUND_TYPE(SoLoudSound, SOLOUD, Sound)
private:
	void init() override;
	void initAsync() override;
	void destroy() override;

	// helpers to access Wav/SLFXStream internals
	[[nodiscard]] double getSourceLengthInSeconds() const;
	[[nodiscard]] double getStreamPositionInSeconds() const;

	// current playback parameters
	float m_fSpeed{1.f};         // speed factor (1.0 = normal)
	float m_fPitch{1.f};         // pitch factor (1.0 = normal)
	float m_fFrequency{44100.f}; // sample rate in Hz

	// SoLoud-specific members
	SoLoud::AudioSource *m_audioSource{nullptr}; // base class pointer, could be either SLFXStream or Wav
	SOUNDHANDLE m_handle{0};                     // current voice (i.e. "Sound") handle
};

#else
class SoLoudSound : public Sound
{};
#endif
#endif
