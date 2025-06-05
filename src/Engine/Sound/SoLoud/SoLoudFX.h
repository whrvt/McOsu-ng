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
public:
	SoundTouchFilterInstance(SLFXStream *aParent);
	~SoundTouchFilterInstance() override;

	// core audio processing method
	unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize) override;

	// playback state management
	bool hasEnded() override;
	result seek(time aSeconds, float *mScratch, unsigned int mScratchSize) override;
	result rewind() override;

private:
	// buffer management
	void ensureBufferSize(unsigned int samples);
	void ensureInterleavedBufferSize(unsigned int samples);
	void ensureOggFrameBuffer(unsigned int samples);

	// make sure buffers are filled to ensure position data is synced with the stream source
	void primeBuffers();
	// handle any internal behavior related to playback position changes
	void reSynchronize();
	void setAutoOffset();

	// OGG-specific handling methods
	[[nodiscard]] bool isOggSource() const;
	void feedSoundTouchFromOggFrames(unsigned int targetBufferSize, bool logThis);
	void feedSoundTouchStandard(unsigned int targetBufferSize, bool logThis);

	// member variables
	SLFXStream *mParent;                  // parent filter
	AudioSourceInstance *mSourceInstance; // source instance to process
	soundtouch::SoundTouch *mSoundTouch;  // soundTouch processor

	float mSoundTouchSpeed; // currently set soundtouch playback parameters for the instance
	float mSoundTouchPitch;

	// buffers for format conversion
	float *mBuffer;           // non-interleaved audio buffer (SoLoud format)
	unsigned int mBufferSize; // size in samples

	float *mInterleavedBuffer;           // interleaved audio buffer (SoundTouch format)
	unsigned int mInterleavedBufferSize; // size in samples

	// OGG frame buffering for proper alignment
	float *mOggFrameBuffer;           // frame-aligned buffer for OGG sources
	unsigned int mOggFrameBufferSize; // size in samples
	unsigned int mOggSamplesInBuffer; // current number of samples in OGG buffer

	// debugging
	unsigned int mProcessingCounter;     // counter for logspam avoidance
	unsigned int mTotalSamplesProcessed; // total samples processed since creation (or last seek/rewind)
};

// a combined WavStream + SoundTouch filter audio source that acts like a regular WavStream on the outside, with extra methods to set rate/pitch
class SLFXStream : public AudioSource
{
	/**
	 * SoundTouchFilter - time-stretching and pitch-shifting filter, using the SoundTouch library
	 *
	 * the filter allows independent control of playback speed and pitch, making it possible
	 * to speed up audio without changing the pitch, or to change pitch without affecting speed.
	 */
public:
	SLFXStream();
	~SLFXStream() override;

	// the audio source to be processed
	result setSource(AudioSource *aSource);

	// SoundTouch control interface
	void setSpeedFactor(float aSpeed); // 1.0 = normal, 2.0 = double speed, etc.
	void setPitchFactor(float aPitch); // 1.0 = normal, 2.0 = one octave up, 0.5 = one octave down

	[[nodiscard]] float getSpeedFactor() const;
	[[nodiscard]] float getPitchFactor() const;

	// create a new instance of the filter with current settings
	AudioSourceInstance *createInstance() override;

protected:
	friend class SoundTouchFilterInstance;

	AudioSource *mSource; // source to be processed
	float mSpeedFactor;   // current speed factor (tempo)
	float mPitchFactor;   // current pitch factor

	/**
	 * SLFXStream - high-level streaming audio source with built-in SoundTouch processing
	 *
	 * combines WavStream and SoundTouchFilter into a single, easy-to-use interface.
	 * provides the same loading methods as WavStream but automatically handles
	 * pitch/speed control through an internal SoundTouch filter.
	 */
public:
	// WavStream-compatible loading interface
	result load(const char *aFilename);
	result loadMem(const unsigned char *aData, unsigned int aDataLen, bool aCopy = false, bool aTakeOwnership = true);
	result loadToMem(const char *aFilename);
	result loadFile(File *aFile);
	result loadFileToMem(File *aFile);

	// utility methods
	double getLength();

private:
	WavStream *mWavStream; // internal audio stream

	// initialization helper
	void initializeFilter();
};

} // namespace SoLoud

#endif // MCENGINE_FEATURE_SOLOUD
#endif // SOLOUD_FX_H
