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

#define SOUND_ENGINE_TYPE(ClassName, TypeID, ParentClass) \
	static constexpr TypeId TYPE_ID = TypeID; \
	[[nodiscard]] TypeId getTypeId() const override { return TYPE_ID; } \
	[[nodiscard]] bool isTypeOf(TypeId typeId) const override { \
		return typeId == TYPE_ID || ParentClass::isTypeOf(typeId); \
	}

typedef uint32_t SOUNDHANDLE;

class Sound;

class SoundEngine
{
public:
	using TypeId = uint8_t;
	enum SndEngineType : TypeId {
		BASS,
		SOLOUD,
		SDL
	};
public:
	SoundEngine();
	virtual ~SoundEngine() = default;

	// Factory method to create the appropriate sound engine
	static SoundEngine *createSoundEngine(SndEngineType type = BASS);

	// Public interface
	virtual void restart() = 0;
	virtual void update() = 0;

	virtual bool play(Sound *snd, float pan = 0.0f, float pitch = 1.0f) = 0;
	virtual bool play3d(Sound *snd, Vector3 pos) = 0;
	virtual void pause(Sound *snd) = 0;
	virtual void stop(Sound *snd) = 0;

	typedef fastdelegate::FastDelegate0<> AudioOutputChangedCallback;
	virtual void setOnOutputDeviceChange(AudioOutputChangedCallback callback);

	virtual void setOutputDevice(UString outputDeviceName) = 0;
	virtual void setOutputDeviceForce(UString outputDeviceName) = 0;
	virtual void setVolume(float volume) = 0;
	virtual void set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp) = 0;

	virtual std::vector<UString> getOutputDevices() = 0;

	[[nodiscard]] inline bool shouldDetectBPM() const { return m_bBPMDetectEnabled; }
	inline void setBPMDetection(bool enabled) { m_bBPMDetectEnabled = enabled; }

	[[nodiscard]] inline const UString &getOutputDevice() const { return m_sCurrentOutputDevice; }
	[[nodiscard]] inline float getVolume() const { return m_fVolume; }

	[[nodiscard]] virtual bool isReady() { return m_bReady; }

	// type inspection
	[[nodiscard]] virtual TypeId getTypeId() const = 0;
	[[nodiscard]] virtual bool isTypeOf(TypeId /*type_id*/) const { return false; }
	template<typename T>
	[[nodiscard]] bool isType() const { return isTypeOf(T::TYPE_ID); }
	template<typename T>
	T* as() { return isType<T>() ? static_cast<T*>(this) : nullptr; }
	template<typename T>
	const T* as() const { return isType<T>() ? static_cast<const T*>(this) : nullptr; }
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
	AudioOutputChangedCallback m_outputDeviceChangeCallback;
	std::vector<OUTPUT_DEVICE> m_outputDevices;

	int m_iCurrentOutputDevice;
	UString m_sCurrentOutputDevice;

	float m_fVolume;

	bool m_bBPMDetectEnabled;
};

// convenience conversion macro to get the sound handle, extra args are any extra conditions to check for besides general state validity
// just minor boilerplate reduction
#define GETHANDLE(T, ...) \
	[&]() -> std::pair<T *, SOUNDHANDLE> { \
		SOUNDHANDLE retHandle = 0; \
		T *retSound = nullptr; \
		if (m_bReady && snd && snd->isReady() __VA_OPT__(&&(__VA_ARGS__)) && (retSound = snd->as<T>())) \
			retHandle = retSound->getHandle(); \
		return {retSound, retHandle}; \
	}()

#endif
