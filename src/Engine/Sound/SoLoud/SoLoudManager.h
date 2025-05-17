//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		singleton manager for SoLoud instance and operations
//
// $NoKeywords: $snd $soloud
//================================================================================//

#pragma once

#ifndef SOLOUD_MANAGER_H
#define SOLOUD_MANAGER_H

#include "EngineFeatures.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include <soloud/soloud.h>

#include <memory>

class SL
{
private:
	static std::unique_ptr<SoLoud::Soloud> m_engine;

public:
	SL();

	// convenience methods that just forward to SoLoud
	static inline SoLoud::result init(unsigned int aFlags = SoLoud::Soloud::CLIP_ROUNDOFF, unsigned int aBackend = SoLoud::Soloud::AUTO,
	                                  unsigned int aSamplerate = SoLoud::Soloud::AUTO, unsigned int aBufferSize = SoLoud::Soloud::AUTO, unsigned int aChannels = 2)
	{
		return m_engine->init(aFlags, aBackend, aSamplerate, aBufferSize, aChannels);
	}
	static inline void deinit() { m_engine->deinit(); }
	static inline const char *getBackendString() { return m_engine->getBackendString(); }
	static inline unsigned int getBackendID() { return m_engine->getBackendId(); }
	static inline unsigned int getBackendSamplerate() { return m_engine->getBackendSamplerate(); }
	static inline unsigned int getBackendChannels() { return m_engine->getBackendChannels(); }
	static inline unsigned int getBackendBuffersize() { return m_engine->getBackendBufferSize(); }
	static inline unsigned int play(SoLoud::AudioSource &aSound, float aVolume = -1.0f) { return m_engine->play(aSound, aVolume); }
	static inline unsigned int play3d(SoLoud::AudioSource &aSound, float aPosX, float aPosY, float aPosZ, float aVelX = 0.0f, float aVelY = 0.0f, float aVelZ = 0.0f,
	                                  float aVolume = -1.0f)
	{
		return m_engine->play3d(aSound, aPosX, aPosY, aPosZ, aVelX, aVelY, aVelZ, aVolume);
	}
	static inline void stop(unsigned int aVoiceHandle) { m_engine->stop(aVoiceHandle); }
	static inline void seek(unsigned int aVoiceHandle, SoLoud::time aSeconds) { m_engine->seek(aVoiceHandle, aSeconds); }
	static inline void setPause(unsigned int aVoiceHandle, bool aPause) { m_engine->setPause(aVoiceHandle, aPause); }
	static inline void setVolume(unsigned int aVoiceHandle, float aVolume) { m_engine->setVolume(aVoiceHandle, aVolume); }
	static inline void setRelativePlaySpeed(unsigned int aVoiceHandle, float aSpeed) { m_engine->setRelativePlaySpeed(aVoiceHandle, aSpeed); }
	static inline void setSamplerate(unsigned int aVoiceHandle, float aSamplerate) { m_engine->setSamplerate(aVoiceHandle, aSamplerate); }
	static inline void setPan(unsigned int aVoiceHandle, float aPan) { m_engine->setPan(aVoiceHandle, aPan); }
	static inline void setLooping(unsigned int aVoiceHandle, bool aLoop) { m_engine->setLooping(aVoiceHandle, aLoop); }
	static inline SoLoud::time getStreamPosition(unsigned int aVoiceHandle) { return m_engine->getStreamPosition(aVoiceHandle); }
	static inline float getSamplerate(unsigned int aVoiceHandle) { return m_engine->getSamplerate(aVoiceHandle); }
	static inline bool isValidVoiceHandle(unsigned int aVoiceHandle) { return m_engine->isValidVoiceHandle(aVoiceHandle); }
	static inline bool getPause(unsigned int aVoiceHandle) { return m_engine->getPause(aVoiceHandle); }
	static inline void setGlobalVolume(float aVolume) { m_engine->setGlobalVolume(aVolume); }
	static inline float getGlobalVolume() { return m_engine->getGlobalVolume(); }
	static inline void set3dListenerPosition(float aPosX, float aPosY, float aPosZ) { m_engine->set3dListenerPosition(aPosX, aPosY, aPosZ); }
	static inline void set3dListenerAt(float aAtX, float aAtY, float aAtZ) { m_engine->set3dListenerAt(aAtX, aAtY, aAtZ); }
	static inline void set3dListenerUp(float aUpX, float aUpY, float aUpZ) { m_engine->set3dListenerUp(aUpX, aUpY, aUpZ); }
	static inline void update3dAudio() { m_engine->update3dAudio(); }
	static inline void setProtectVoice(unsigned int aVoiceHandle, bool aProtect) { m_engine->setProtectVoice(aVoiceHandle, aProtect); }
	static inline bool getProtectVoice(unsigned int aVoiceHandle) { return m_engine->getProtectVoice(aVoiceHandle); }
	static inline SoLoud::result setMaxActiveVoiceCount(unsigned int aVoiceCount) { return m_engine->setMaxActiveVoiceCount(aVoiceCount); }
	static inline unsigned int getMaxActiveVoiceCount() { return m_engine->getMaxActiveVoiceCount(); }
};

#else
class SL
{};
#endif

#endif
