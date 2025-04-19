//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		sound wrapper, either streamed or preloaded
//
// $NoKeywords: $snd
//===============================================================================//

#pragma once
#ifndef SOUND_H
#define SOUND_H

#include "Resource.h"
#include "SoundEngine.h"

#if defined(_GNU_SOURCE) && defined(__linux__) && defined(MCENGINE_FEATURE_BASS)
#include <dlfcn.h>
#else
#define DL_CALL_FCT(fctp, args) ((*(fctp)) args)
#endif

class SoundEngine;

class Sound : public Resource
{
	friend class SoundEngine;

public:
	typedef unsigned long SOUNDHANDLE;

public:
	Sound(UString filepath, bool stream, bool threeD, bool loop, bool prescan);
	virtual ~Sound() {destroy();}

	void setPosition(double percent);
	void setPositionMS(unsigned long ms) {setPositionMS(ms, false);}
	void setVolume(float volume);
	void setSpeed(float speed);
	void setPitch(float pitch);
	void setFrequency(float frequency);
	void setPan(float pan);
	void setLoop(bool loop);
	void setOverlayable(bool overlayable) {m_bIsOverlayable = overlayable;}
	void setLastPlayTime(double lastPlayTime) {m_fLastPlayTime = lastPlayTime;}

	SOUNDHANDLE getHandle();
	float getPosition();
	unsigned long getPositionMS();
	unsigned long getLengthMS();
	float getSpeed();
	float getPitch();
	float getFrequency();

	inline double getLastPlayTime() const {return m_fLastPlayTime;}

	bool isPlaying();
	bool isFinished();

	inline bool isStream() const {return m_bStream;}
	inline bool is3d() const {return m_bIs3d;}
	inline bool isLooped() const {return m_bIsLooped;}
	inline bool isOverlayable() const {return m_bIsOverlayable;}

	void rebuild(UString newFilePath);

	// ILLEGAL:
	void setHandle(SOUNDHANDLE handle) {m_HCHANNEL = handle;}
	void setPrevPosition(unsigned long prevPosition) {m_iPrevPosition = prevPosition;}
	inline unsigned long getPrevPosition() const {return m_iPrevPosition;}

#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_SDL_MIXER)
	inline void *getMixChunkOrMixMusic() const {return m_mixChunkOrMixMusic;}
#endif

private:
	virtual void init();
	virtual void initAsync();
	virtual void destroy();

	void setPositionMS(unsigned long ms, bool internal);

	SOUNDHANDLE m_HSTREAM;
	SOUNDHANDLE m_HSTREAMBACKUP;
	SOUNDHANDLE m_HCHANNEL;
	SOUNDHANDLE m_HCHANNELBACKUP;

	bool m_bStream;
	bool m_bIs3d;
	bool m_bIsLooped;
	bool m_bPrescan;
	bool m_bIsOverlayable;

	float m_fVolume;
	double m_fLastPlayTime;

	// bass custom
	float m_fActualSpeedForDisabledPitchCompensation;

	// bass wasapi
	char *m_wasapiSampleBuffer;
	unsigned long long m_iWasapiSampleBufferSize;
	std::vector<SOUNDHANDLE> m_danglingWasapiStreams;
	unsigned long m_iPrevPosition;

	// sdl mixer
#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_SDL_MIXER)
	void *m_mixChunkOrMixMusic;
    double m_fLastRawSDLPosition;      // last raw position reported by Mix_GetMusicPosition
    double m_fLastSDLPositionTime;     // engine time when the last position was obtained
    double m_fSDLPositionRate;         // estimated playback rate (position units per second)
#endif

#if defined(MCENGINE_FEATURE_BASS)
    inline HSTREAM _BASS_FX_TempoCreate(DWORD chan, DWORD flags) const {return DL_CALL_FCT((SoundEngine::m_BASS_FX_TempoCreate), (chan, flags));}
#endif
};

#endif
