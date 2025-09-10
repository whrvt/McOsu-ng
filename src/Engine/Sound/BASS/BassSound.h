//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		BASS-specific sound implementation
//
// $NoKeywords: $snd $bass
//===============================================================================//

#pragma once
#ifndef BASS_SOUND_H
#define BASS_SOUND_H

#include "Sound.h"
#ifdef MCENGINE_FEATURE_BASS
#include "BassManager.h"

class BassSoundEngine;

class BassSound final : public Sound
{
	friend class BassSoundEngine;

public:
	BassSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~BassSound() override;

	BassSound &operator=(const BassSound &) = delete;
	BassSound &operator=(BassSound &&) = delete;

	BassSound(const BassSound &) = delete;
	BassSound(BassSound &&) = delete;

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

	// inspection
	SOUND_TYPE(BassSound, BASS, Sound)
private:
	void init() override;
	void initAsync() override;
	void destroy() override;

	// helpers
	bool setBassAttribute(DWORD attrib, float value, const char *debugName = nullptr);
	void updatePlayInterpolationTime(double positionSeconds);
	void cleanupWasapiStreams();
	SOUNDHANDLE createWasapiChannel();

	inline void setPrevPosition(unsigned long prevPosition) { m_iPrevPosition = prevPosition; }
	[[nodiscard]] constexpr unsigned long getPrevPosition() const { return m_iPrevPosition; }

	// TODO: get rid of these "BACKUP" things
	SOUNDHANDLE m_HSTREAM;
	SOUNDHANDLE m_HSTREAMBACKUP;
	SOUNDHANDLE m_HCHANNEL;
	SOUNDHANDLE m_HCHANNELBACKUP;

	// BASS custom
	float m_fActualSpeedForDisabledPitchCompensation;

	// BASS WASAPI
	char *m_wasapiSampleBuffer;
	unsigned long long m_iWasapiSampleBufferSize;
	std::vector<SOUNDHANDLE> m_danglingWasapiStreams;
	unsigned long m_iPrevPosition;
};

#else
class BassSound : public Sound
{};
#endif
#endif
