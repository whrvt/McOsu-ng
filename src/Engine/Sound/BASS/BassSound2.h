//======= Copyright (c) 2014, PG, 2024, CW, 2025, WH All rights reserved. =======//
//
// Purpose:		BASS-specific sound implementation
//
// $NoKeywords: $snd $bass
//===============================================================================//

#pragma once
#ifndef BASS_SOUND2_H
#define BASS_SOUND2_H

#include "Sound.h"
#if defined(MCENGINE_FEATURE_BASS) && defined(MCENGINE_NEOSU_BASS_PORT_FINISHED)

#include "BassSoundEngine2.h"

class BassSound2 final : public Sound
{
	friend class BassSoundEngine2;
public:
	BassSound2(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~BassSound2() override { destroy(); }

	std::vector<HCHANNEL> getActiveChannels();
	HCHANNEL getChannel();

	void setPosition(double percent) override;
	void setPositionMS(unsigned long ms) override;

	void setVolume(float volume) override;
	void setSpeed(float speed) override;
	void setFrequency(float frequency) override;
	void setPan(float pan) override;
	void setLoop(bool loop) override;
	void setPitch(float pitch) override;
	inline SOUNDHANDLE getHandle() override {return getChannel();};

	float getPosition() override;
	unsigned long getPositionMS() override;
	unsigned long getLengthMS() override;
	float getSpeed() override;
	float getPitch() override;
	float getFrequency() override;

	bool isPlaying() override;
	bool isFinished() override;

	[[nodiscard]] inline bool isStream() const override { return m_bStream; }
	[[nodiscard]] inline bool isLooped() const override { return m_bIsLooped; }
	[[nodiscard]] inline bool isOverlayable() const override { return m_bIsOverlayable; }

	constexpr SoundType* getSound() override {return this;}
	[[nodiscard]] constexpr const SoundType* getSound() const override {return this;}

	void rebuild(UString newFilePath) override;

private:
	void setPositionMS_fast(unsigned int ms);
	void setPositionMS(unsigned long ms, bool internal) override { setPositionMS(ms); }

	[[nodiscard]] inline float getPan() { return m_fPan; }

	void init() override;
	void initAsync() override;
	void destroy() override;

	SOUNDHANDLE m_stream{0};
	SOUNDHANDLE m_sample{0};

	std::vector<HCHANNEL> m_vMixerChannels;

	bool m_bStarted{false};
	bool m_bPaused{false};
	bool m_bStream;
	bool m_bIsLooped;
	bool m_bIsOverlayable;

	float m_fPan;
	float m_fSpeed;
	float m_fVolume;
	double m_dLastPlayTime{0.0};
	double m_dChannelCreationTime{0.0};
	unsigned long m_iPausePositionMS{0};
	unsigned int m_dLength{};
};

#else
class BassSound2 : public Sound
{};
#endif
#endif
