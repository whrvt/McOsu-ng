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

class SoundEngine;

class Sound : public Resource
{
public:
	typedef unsigned long SOUNDHANDLE;

public:
	Sound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	~Sound() override;

	// Factory method to create the appropriate sound object
	static Sound *createSound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);

	// Public interface
	virtual void setPosition(double percent) = 0;
	virtual void setPositionMS(unsigned long ms) { setPositionMS(ms, false); }
	virtual void setVolume(float volume) = 0;
	virtual void setSpeed(float speed) = 0;
	virtual void setPitch(float pitch) = 0;
	virtual void setFrequency(float frequency) = 0;
	virtual void setPan(float pan) = 0;
	virtual void setLoop(bool loop) = 0;
	virtual void setOverlayable(bool overlayable) { m_bIsOverlayable = overlayable; }
	virtual void setLastPlayTime(double lastPlayTime) { m_fLastPlayTime = lastPlayTime; }

	virtual SOUNDHANDLE getHandle() = 0;
	virtual float getPosition() = 0;
	virtual unsigned long getPositionMS() = 0;
	virtual unsigned long getLengthMS() = 0;
	virtual float getSpeed() = 0;
	virtual float getPitch() = 0;
	virtual float getFrequency() = 0;

	virtual bool isPlaying() = 0;
	virtual bool isFinished() = 0;

	[[nodiscard]] inline double getLastPlayTime() const { return m_fLastPlayTime; }
	[[nodiscard]] inline bool isStream() const { return m_bStream; }
	[[nodiscard]] inline bool is3d() const { return m_bIs3d; }
	[[nodiscard]] inline bool isLooped() const { return m_bIsLooped; }
	[[nodiscard]] inline bool isOverlayable() const { return m_bIsOverlayable; }

	virtual void rebuild(UString newFilePath) = 0;

protected:
	void init() override = 0;
	void initAsync() override = 0;
	void destroy() override = 0;

	virtual void setPositionMS(unsigned long ms, bool internal) = 0;

	bool m_bStream;
	bool m_bIs3d;
	bool m_bIsLooped;
	bool m_bPrescan;
	bool m_bIsOverlayable;

	float m_fVolume;
	double m_fLastPlayTime;
};

#endif
