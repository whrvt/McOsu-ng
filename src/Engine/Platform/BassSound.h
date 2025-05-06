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


class BassSoundEngine;

class BassSound : public Sound
{
	friend class BassSoundEngine;

public:
	BassSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~BassSound() override;

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
	constexpr SoundType* getSound() override {return this;}
	[[nodiscard]] constexpr const SoundType* getSound() const override {return this;}
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

	SOUNDHANDLE m_HSTREAM;
	SOUNDHANDLE m_HSTREAMBACKUP;
	SOUNDHANDLE m_HCHANNEL;
	SOUNDHANDLE m_HCHANNELBACKUP;

	// bass custom
	float m_fActualSpeedForDisabledPitchCompensation;

	// bass wasapi
	char *m_wasapiSampleBuffer;
	unsigned long long m_iWasapiSampleBufferSize;
	std::vector<SOUNDHANDLE> m_danglingWasapiStreams;
	unsigned long m_iPrevPosition;
};

#else
class BassSound : public Sound{};
#endif
#endif
