//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		handles sounds using SoLoud library
//
// $NoKeywords: $snd $soloud
//================================================================================//

#pragma once
#ifndef SOLOUD_SOUNDENGINE_H
#define SOLOUD_SOUNDENGINE_H

#include "SoundEngine.h"
#ifdef MCENGINE_FEATURE_SOLOUD

#include <soloud/soloud.h>

class SoLoudSound;

class SoLoudSoundEngine : public SoundEngine
{
public:
	SoLoudSoundEngine();
	~SoLoudSoundEngine() override;

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

	// playback functions specific to SoLoudSound
	bool playSound(SoLoudSound *soloudSound, float pan, float pitch, bool is3d = false, Vector3 *pos = nullptr);
	unsigned int playSoundWithFilter(SoLoudSound *soloudSound, float pan, float volume);
	unsigned int playDirectSound(SoLoudSound *soloudSound, float pan, float pitch, float volume);
	unsigned int play3dSound(SoLoudSound *soloudSound, Vector3 pos, float volume);

	// SoLoud-specific sound manip methods
	void stopSound(unsigned int handle);
	void seekSound(unsigned int handle, double positionSeconds);
	void setPauseSound(unsigned int handle, bool pause);
	void setVolumeSound(unsigned int handle, float volume);
	void setRelativePlaySpeedSound(unsigned int handle, float speed);
	void setSampleRateSound(unsigned int handle, float sampleRate);
	void setPanSound(unsigned int handle, float pan);
	void setLoopingSound(unsigned int handle, bool loop);

	// SoLoud-specific sound info methods
	float getStreamPositionSound(unsigned int handle);
	float getSampleRateSound(unsigned int handle);
	bool isValidVoiceHandleSound(unsigned int handle);
	bool getPauseSound(unsigned int handle);

	SoundEngineType* getSndEngine() override {return this;}
	[[nodiscard]] const SoundEngineType* getSndEngine() const override {return this;}

private:
	void updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo) override;
	bool initializeOutputDevice(int id = -1, bool force = false) override;

	// SoLoud engine instance
	SoLoud::Soloud m_engine;
};

#else
class SoLoudSoundEngine : public SoundEngine{};
#endif
#endif
