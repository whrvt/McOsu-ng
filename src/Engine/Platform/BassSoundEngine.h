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

#include <bass.h>
#include <bass_fx.h>

#ifdef MCENGINE_FEATURE_BASS_WASAPI
#include <bassmix.h>
#include <basswasapi.h>
#endif

class BassSound;
class SoundEngineThread;

class BassSoundEngine : public SoundEngine
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

private:
	void updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo) override;
	bool initializeOutputDevice(int id = -1) override;

	void onFreqChanged(UString oldValue, UString newValue);

	void *m_bassfx_handle;

	friend class BassSound;
	static inline HSTREAM (*m_BASS_FX_TempoCreate)(DWORD, DWORD);
	uint32_t m_iBASSVersion;

#ifdef MCENGINE_FEATURE_MULTITHREADING
	SoundEngineThread *m_thread;
#endif
};

#endif
#endif
