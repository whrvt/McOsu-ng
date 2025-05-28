//======= Copyright (c) 2014, PG, 2024, CW, 2025, WH All rights reserved. =======//
//
// Purpose:		handles sounds using BASS library
//
// $NoKeywords: $snd $bass
//===============================================================================//

#pragma once
#ifndef BASS_SOUNDENGINE2_H
#define BASS_SOUNDENGINE2_H

#include "SoundEngine.h"
#if defined(MCENGINE_FEATURE_BASS) && defined(MCENGINE_NEOSU_BASS_PORT_FINISHED)
#include "BassManager.h"
#include "Sound.h"
#include <algorithm>

class BassSoundEngine2 final : public SoundEngine
{
friend class BassSound2;
private:
	enum class OutputDriver : uint8_t
	{
		NONE,
		BASS,        // directsound/wasapi non-exclusive mode/alsa
		BASS_WASAPI, // exclusive mode
		BASS_ASIO,   // exclusive move
	};

	struct OUTPUT_DEVICE_BASS
	{
		int id{0};
		bool enabled{true};
		bool isDefault{true};
		UString name{"No sound"};
		OutputDriver driver{OutputDriver::NONE};
	};

public:
	BassSoundEngine2();
	~BassSoundEngine2() override;

	void restart() override;

	void update() override;

	bool play(Sound *snd, float pan = 0.0f, float pitch = 1.0f) override;
	bool play3d(Sound *, Vector3) override { return false; }
	void pause(Sound *snd) override;
	void stop(Sound *snd) override;

	void setOutputDevice(UString device) override { setOutputDevice({.name = device}); }
	void setOutputDeviceForce(UString outputDeviceName) override { setOutputDevice({.name = outputDeviceName}); }
	void setVolume(float volume) override;
	void set3dPosition(Vector3, Vector3, Vector3) override { ; }

	void setOutputDevice(OUTPUT_DEVICE_BASS device);
	bool isASIO() { return m_currentOutputDevice.driver == OutputDriver::BASS_ASIO; }
	bool isWASAPI() { return m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI; }
	bool hasExclusiveOutput();

	OUTPUT_DEVICE_BASS getDefaultDevice();
	OUTPUT_DEVICE_BASS getWantedDevice();
	std::vector<OUTPUT_DEVICE_BASS> getOutputDevicesStruct();
	std::vector<UString> getOutputDevices() override
	{
		std::vector<OUTPUT_DEVICE_BASS> devices = getOutputDevicesStruct(); // calls the OUTPUT_DEVICE version
		std::vector<UString> names;
		names.reserve(devices.size());
		std::ranges::transform(devices, std::back_inserter(names), [](const OUTPUT_DEVICE_BASS &device) { return device.name; });
		return names;
	}

	[[nodiscard]] inline const UString &getOutputDeviceName() const { return m_currentOutputDevice.name; }
	[[nodiscard]] inline float getVolume() const { return m_fVolume; }

	SOUNDHANDLE g_bassOutputMixer{0};

	SoundEngineType *getSndEngine() override { return this; }
	[[nodiscard]] const SoundEngineType *getSndEngine() const override { return this; }

private:
	void bassfree();
	void updateOutputDevices(bool printInfo);
	void updateOutputDevices(bool, bool printInfo) override { return updateOutputDevices(printInfo); }
	bool initializeOutputDeviceStruct(OUTPUT_DEVICE_BASS device);
	bool initializeOutputDevice(int id = -1, bool = false) override { return initializeOutputDeviceStruct({.id = id}); };

	std::vector<OUTPUT_DEVICE_BASS> m_vOutputDevices;

	OUTPUT_DEVICE_BASS m_currentOutputDevice{};
	UString m_sCurrentOutputDevice{m_currentOutputDevice.name};

	bool init_bass_mixer(OUTPUT_DEVICE_BASS device);

	float m_fVolume = 1.0f;

	void onFreqChanged(UString oldValue, UString newValue);

	void _RESTART_SOUND_ENGINE_ON_CHANGE(UString oldValue, UString newValue);
#ifdef MCENGINE_PLATFORM_WINDOWS
	DWORD ASIO_clamp(BASS_ASIO_INFO info, DWORD buflen);
#endif
};

#else
class BassSoundEngine2 : public SoundEngine
{};
#endif
#endif
