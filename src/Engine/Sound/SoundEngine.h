//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		sound engine base class
//
// $NoKeywords: $snd
//===============================================================================//

#pragma once
#ifndef SOUNDENGINE_H
#define SOUNDENGINE_H

#include "cbase.h"

class Sound;

class SoundEngine
{
public:
	SoundEngine();
	virtual ~SoundEngine();

	// Factory method to create the appropriate sound engine
	static SoundEngine *createSoundEngine();

	// Public interface
	virtual void restart() = 0;
	virtual void update() = 0;

	virtual bool play(Sound *snd, float pan = 0.0f, float pitch = 1.0f) = 0;
	virtual bool play3d(Sound *snd, Vector3 pos) = 0;
	virtual void pause(Sound *snd) = 0;
	virtual void stop(Sound *snd) = 0;

	virtual void setOnOutputDeviceChange(std::function<void()> callback);

	virtual void setOutputDevice(UString outputDeviceName) = 0;
	virtual void setOutputDeviceForce(UString outputDeviceName) = 0;
	virtual void setVolume(float volume) = 0;
	virtual void set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp) = 0;

	virtual std::vector<UString> getOutputDevices() = 0;

	inline const UString &getOutputDevice() const { return m_sCurrentOutputDevice; }
	inline float getVolume() const { return m_fVolume; }

	virtual SoundEngineType* getSndEngine() = 0;
	[[nodiscard]] virtual const SoundEngineType* getSndEngine() const = 0;

protected:
	struct OUTPUT_DEVICE
	{
		int id;
		bool enabled;
		bool isDefault;
		UString name;
	};

	virtual void updateOutputDevices(bool handleOutputDeviceChanges, bool printInfo) = 0;
	virtual bool initializeOutputDevice(int id = -1, bool force = false) = 0;

	bool m_bReady;
	float m_fPrevOutputDeviceChangeCheckTime;
	std::function<void()> m_outputDeviceChangeCallback;
	std::vector<OUTPUT_DEVICE> m_outputDevices;

	int m_iCurrentOutputDevice;
	UString m_sCurrentOutputDevice;

	float m_fVolume;
};

#endif
