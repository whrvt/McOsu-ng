//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoundTouch filter for independent pitch/speed control (for use with SoLoud)
//
// $NoKeywords: $snd $soloud $soundtouch
//================================================================================//

#pragma once
#ifndef SOUNDTOUCH_FILTER_H
#define SOUNDTOUCH_FILTER_H

#include "cbase.h"

#ifdef MCENGINE_FEATURE_SOLOUD
#include <SoundTouch.h>
#include <soloud/soloud.h>
#include <soloud/soloud_wav.h>
#include <soloud/soloud_wavstream.h>

namespace SoLoud
{
class SoundTouchFilterInstance;

/**
 * SoundTouchFilter - time-stretching and pitch-shifting filter, using the SoundTouch library
 *
 * the filter allows independent control of playback speed and pitch, making it possible
 * to speed up audio without changing the pitch, or to change pitch without affecting speed.
 */
class SoundTouchFilter : public AudioSource
{
public:
	SoundTouchFilter();
	virtual ~SoundTouchFilter();

	// the audio source to be processed
	result setSource(AudioSource *aSource);

	// create a new instance of the filter with current settings
	virtual AudioSourceInstance *createInstance() override;

	void setSpeedFactor(float aSpeed); // 1.0 = normal, 2.0 = double speed, etc.
	void setPitchFactor(float aPitch); // 1.0 = normal, 2.0 = one octave up, 0.5 = one octave down

	float getSpeedFactor() const;
	float getPitchFactor() const;

protected:
	friend class SoundTouchFilterInstance;

	AudioSource *mSource; // source to be processed
	float mSpeedFactor;   // current speed factor (tempo)
	float mPitchFactor;   // current pitch factor
};

/**
 * SoundTouchFilterInstance - instance of the SoundTouch filter that processes audio
 *
 * handles the audio processing through SoundTouch, incl. converting between
 * SoLoud's non-interleaved format and SoundTouch's interleaved format.
 */
class SoundTouchFilterInstance : public AudioSourceInstance
{
public:
	SoundTouchFilterInstance(SoundTouchFilter *aParent);
	~SoundTouchFilterInstance() override;

	// core audio processing method
	unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize) override;

	// playback state management
	bool hasEnded() override;
	result seek(time aSeconds, float *mScratch, unsigned int mScratchSize) override;
	unsigned int rewind() override;

protected:
	// buffer management
	void ensureBufferSize(unsigned int samples);
	void ensureInterleavedBufferSize(unsigned int samples);

	// member variables
	SoundTouchFilter *mParent;            // parent filter
	AudioSourceInstance *mSourceInstance; // source instance to process
	soundtouch::SoundTouch *mSoundTouch;  // soundTouch processor

	float mSoundTouchSpeed; // currently set soundtouch playback parameters for the instance
	float mSoundTouchPitch;

	// buffers for format conversion
	float *mBuffer;           // non-interleaved audio buffer (SoLoud format)
	unsigned int mBufferSize; // size in samples

	float *mInterleavedBuffer;           // interleaved audio buffer (SoundTouch format)
	unsigned int mInterleavedBufferSize; // size in samples

	// DEBUG
	void logBufferData(const char *label, float *buffer, unsigned int channels, unsigned int samples, bool isInterleaved, unsigned int maxToLog = 16);

	unsigned int mProcessingCounter;     // counter for logspam avoidance
	unsigned int mTotalSamplesProcessed; // total samples processed since creation (or last seek/rewind)
};

} // namespace SoLoud

#endif // MCENGINE_FEATURE_SOLOUD
#endif // SOUNDTOUCH_FILTER_H
