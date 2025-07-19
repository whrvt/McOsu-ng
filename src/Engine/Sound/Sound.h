//========== Copyright (c) 2014, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		sound wrapper base class
//
// $NoKeywords: $snd
//===============================================================================//

#pragma once

#ifndef SOUND_H
#define SOUND_H

#include "Resource.h"
#include "PlaybackInterpolator.h"

#define SOUND_TYPE(ClassName, TypeID, ParentClass) \
	static constexpr TypeId TYPE_ID = TypeID; \
	[[nodiscard]] TypeId getTypeId() const override { return TYPE_ID; } \
	[[nodiscard]] bool isTypeOf(TypeId typeId) const override { \
		return typeId == TYPE_ID || ParentClass::isTypeOf(typeId); \
	}

class SoundEngine;

typedef uint32_t SOUNDHANDLE;

class Sound : public Resource
{
public:
	using TypeId = uint8_t;
	enum SndType : TypeId {
		BASS,
		SOLOUD,
		SDL
	};
public:
	Sound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);

	// Factory method to create the appropriate sound object
	static Sound *createSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);

	// Public interface
	virtual void setPosition(double percent) = 0;
	virtual void setPositionMS(unsigned long ms) = 0;
	virtual void setVolume(float volume) = 0;
	virtual void setSpeed(float speed) = 0;
	virtual void setPitch(float pitch) = 0;
	virtual void setFrequency(float frequency) = 0;
	virtual void setPan(float pan) = 0;
	virtual void setLoop(bool loop) = 0;
	virtual void setOverlayable(bool overlayable) { m_bIsOverlayable = overlayable; }
	inline void setLastPlayTime(double lastPlayTime) { m_fLastPlayTime = lastPlayTime; }

	virtual SOUNDHANDLE getHandle() = 0;

	virtual float getPosition() = 0;
	virtual unsigned long getPositionMS() = 0;
	virtual unsigned long getLengthMS() = 0;
	virtual float getSpeed() = 0;
	virtual float getPitch() = 0;
	virtual float getFrequency() = 0;

	[[nodiscard]] virtual inline float getBPM() { return m_fCurrentBPM; } // non-const for possible caching

	virtual bool isPlaying() = 0;
	virtual bool isFinished() = 0;

	[[nodiscard]] constexpr double getLastPlayTime() const { return m_fLastPlayTime; }
	[[nodiscard]] constexpr bool isStream() const { return m_bStream; }
	[[nodiscard]] constexpr bool is3d() const { return m_bIs3d; }
	[[nodiscard]] constexpr bool isLooped() const { return m_bIsLooped; }
	[[nodiscard]] constexpr bool isOverlayable() const { return m_bIsOverlayable; }

	virtual void rebuild(UString newFilePath) = 0;

	// type inspection
	[[nodiscard]] Type getResType() const final { return SOUND; }

	Sound *asSound() final { return this; }
	[[nodiscard]] const Sound *asSound() const final { return this; }

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
	void init() override = 0;
	void initAsync() override = 0;
	void destroy() override = 0;

	bool m_bStream;
	bool m_bIs3d;
	bool m_bIsLooped;
	bool m_bPrescan;
	bool m_bIsOverlayable;

	bool m_bIgnored;

	float m_fVolume;
	double m_fLastPlayTime;

	float m_fCurrentBPM;

	PlaybackInterpolator m_interpolator;

private:
	static bool isValidAudioFile(const UString& filePath, const UString &fileExt);
};

#endif
