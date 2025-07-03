//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		Interpolated position tracking for audio playback.
//
// $NoKeywords: $snd $audioclock
//================================================================================//

#pragma once

/**
 * PlaybackInterpolator provides smooth interpolated position tracking for audio playback.
 *
 * This class implements rate-based interpolation to smooth out position reporting
 * from audio backends that may have irregular or jittery position updates. It
 * estimates the actual playback rate based on position changes and interpolates
 * between updates to provide smooth, predictable position values.
 */
class PlaybackInterpolator
{
public:
	PlaybackInterpolator() = default;

	/**
	 * Update with current raw position and get interpolated position.
	 * Call this regularly (every frame) with the raw position from your audio backend.
	 *
	 * @param rawPositionMS Raw position from audio backend in milliseconds
	 * @param currentTime Current engine time in seconds
	 * @param playbackSpeed Current playback speed multiplier (1.0 = normal)
	 * @param isLooped Whether the audio is looped
	 * @param lengthMS Total length of audio in milliseconds (required for loop handling)
	 * @param isPlaying Whether the audio is currently playing
	 * @return Interpolated position in milliseconds
	 */
	unsigned long update(double rawPositionMS, double currentTime, double playbackSpeed, bool isLooped = false, unsigned long long lengthMS = 0, bool isPlaying = true);

	/**
	 * Reset interpolation state.
	 * Call this when seeking or starting playback to reset the interpolation.
	 *
	 * @param rawPositionMS Current raw position in milliseconds
	 * @param currentTime Current engine time in seconds
	 * @param playbackSpeed Current playback speed multiplier
	 */
	inline void reset(double rawPositionMS, double currentTime, double playbackSpeed)
	{
		m_dLastRawPosition = rawPositionMS;
		m_dLastPositionTime = currentTime;
		m_iEstimatedRate = 1000.0 * playbackSpeed;
		m_dLastInterpolatedPosition = static_cast<unsigned long>(rawPositionMS);
	}

	/**
	 * Get the last calculated interpolated position without updating.
	 * @return Last interpolated position in milliseconds
	 */
	[[nodiscard]] unsigned long getLastInterpolatedPositionMS() const { return m_dLastInterpolatedPosition; }

private:
	double m_dLastRawPosition{0.0};          // last raw position in milliseconds
	double m_dLastPositionTime{0.0};         // engine time when last position was obtained
	double m_iEstimatedRate{1000.0};         // estimated playback rate (ms per second)
	unsigned long m_dLastInterpolatedPosition{0}; // last calculated interpolated position
};
