//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoundTouch filter for independent pitch/speed control (for use with SoLoud)
//
// $NoKeywords: $snd $soloud $soundtouch
//================================================================================//

#pragma once
#ifndef SOLOUD_FX_H
#define SOLOUD_FX_H

#include "BaseEnvironment.h"

#ifdef MCENGINE_FEATURE_SOLOUD
#include <memory>
#include <soloud/soloud.h>

namespace soundtouch
{
class SoundTouch;
}

namespace SoLoud
{
class WavStream;
class File;
class SLFXStream;
class SoundTouchFilterInstance;

/**
 * SoundTouchFilterInstance - instance of the SoundTouch filter that processes audio
 *
 * handles the audio processing through SoundTouch, incl. converting between
 * SoLoud's non-interleaved format and SoundTouch's interleaved format.
 * includes special handling for OGG files to maintain frame integrity.
 */
class SoundTouchFilterInstance : public AudioSourceInstance
{
	friend class SLFXStream;
public:
	SoundTouchFilterInstance(SLFXStream *aParent);
	~SoundTouchFilterInstance() override;

	// core audio processing method
	unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize) override;

	// playback state management
	bool hasEnded() override;
	result seek(time aSeconds, float *mScratch, unsigned int mScratchSize) override;
	result rewind() override;

	// accurate position tracking
	[[nodiscard]] time getRealStreamPosition() const;
private:
	// buffer management
	void ensureBufferSize(unsigned int samples);
	void ensureInterleavedBufferSize(unsigned int samples);

	// calculate target buffer level for consistent processing
	unsigned int calculateTargetBufferLevel(unsigned int aSamplesToRead, bool logThis);

	// feed SoundTouch using fixed-size chunks
	void feedSoundTouch(unsigned int targetBufferLevel, bool logThis);

	// buffer synchronization and positioning
	void reSynchronize();
	void updateSTLatency();

	void requestSettingUpdate(float speed, float pitch);

	// member variables
	SLFXStream *mParent;                  // parent filter
	AudioSourceInstance *mSourceInstance; // source instance to process
	soundtouch::SoundTouch *mSoundTouch;  // soundtouch processor

	// soundtouch setting cache for calculting offset trailing behind source stream
	unsigned int mInitialSTLatencySamples;
	unsigned int mSTOutputSequenceSamples;

	// this is derived from the above, it doesn't change very often so it makes sense to keep it cached as well
	double mSTLatencySeconds;

	float mSoundTouchSpeed;
	float mSoundTouchPitch;

	// to be used to signal the instance that the soundtouch parameters need to be adjusted at the next best time
	bool mNeedsSettingUpdate;

	// buffers for format conversion
	float *mBuffer;           // temporary read buffer from source (non-interleaved)
	unsigned int mBufferSize; // size in samples

	float *mInterleavedBuffer;           // interleaved audio buffer for SoundTouch
	unsigned int mInterleavedBufferSize; // size in samples

	// debugging and tracking
	unsigned int mProcessingCounter; // counter for logspam avoidance
};

/**
 * SLFXStream - streaming audio source with built-in SoundTouch processing
 *
 * combines WavStream functionality with SoundTouch time-stretching and pitch-shifting.
 * provides the same loading interface as WavStream but with additional methods for
 * independent control of playback speed and pitch.
 */
class SLFXStream : public AudioSource
{
public:
	SLFXStream();
	~SLFXStream() override;

	// SoundTouch control interface
	void setSpeedFactor(float aSpeed); // 1.0 = normal, 2.0 = double speed, etc.
	void setPitchFactor(float aPitch); // 1.0 = normal, 2.0 = one octave up, 0.5 = one octave down

	[[nodiscard]] float getSpeedFactor() const;
	[[nodiscard]] float getPitchFactor() const;

	// AudioSource interface
	AudioSourceInstance *createInstance() override;

	// WavStream-compatible loading interface
	result load(const char *aFilename);
	result loadMem(const unsigned char *aData, unsigned int aDataLen, bool aCopy = false, bool aTakeOwnership = true);
	result loadToMem(const char *aFilename);
	result loadFile(File *aFile);
	result loadFileToMem(File *aFile);

	// utility methods
	double getLength();

	// accurate position access for active instance
	time getRealStreamPosition() const;

protected:
	friend class SoundTouchFilterInstance;

	float mSpeedFactor; // current speed factor (tempo)
	float mPitchFactor; // current pitch factor

	// internal audio stream
	std::unique_ptr<WavStream> mSource;

	// track the active instance for position queries
	mutable SoundTouchFilterInstance *mActiveInstance;
};

} // namespace SoLoud

#endif // MCENGINE_FEATURE_SOLOUD
#endif // SOLOUD_FX_H
