//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		handles sounds using BASS library
//
// $NoKeywords: $snd $bass
//===============================================================================//

#pragma once
#ifndef BASS_SOUNDENGINE_H
#define BASS_SOUNDENGINE_H

#include "SoundEngine.h"
#ifdef MCENGINE_FEATURE_BASS

#include "BassSound.h"

class BassSoundEngine final : public SoundEngine
{
public:
	BassSoundEngine();
	~BassSoundEngine() override;

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

	SOUND_ENGINE_TYPE(BassSoundEngine, BASS, SoundEngine)
private:
	void updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo) override;
	bool initializeOutputDevice(int id = -1, bool force = false) override;

	// helpers
	bool initializeBass(int id, bool canReinitInsteadOfFreeInit);
	bool initializeWasapi(int id);
	void applyRuntimeConfigs(bool &needsReinit);
	bool playChannelWithAttributes(BassSound *bassSound, SOUNDHANDLE handle, float pan, float pitch);

	void onFreqChanged(UString oldValue, UString newValue);

	friend class BassSound;
};

#else
class BassSoundEngine : public SoundEngine
{};
#endif
#endif
