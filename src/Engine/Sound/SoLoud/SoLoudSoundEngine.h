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

#include "SoLoudManager.h"

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

	SoundEngineType *getSndEngine() override { return this; }
	[[nodiscard]] const SoundEngineType *getSndEngine() const override { return this; }

private:
	void updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo) override;
	bool initializeOutputDevice(int id = -1, bool force = false) override;

	SL* m_slManager;
};

#else
class SoLoudSoundEngine : public SoundEngine
{};
#endif
#endif
