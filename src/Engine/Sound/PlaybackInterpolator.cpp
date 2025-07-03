//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		Interpolated position tracking for audio playback.
//
// $NoKeywords: $snd $audioclock
//================================================================================//

#include "PlaybackInterpolator.h"
#include <algorithm>
#include <cmath>

unsigned long PlaybackInterpolator::update(double rawPositionMS, double currentTime, double playbackSpeed, bool isLooped, unsigned long long lengthMS,
                                           bool isPlaying)
{
	// reset state if not initialized or not playing
	if (m_dLastPositionTime <= 0.0 || !isPlaying)
	{
		reset(rawPositionMS, currentTime, playbackSpeed);
		return m_dLastInterpolatedPosition;
	}

	// update rate estimate when position changes
	if (m_dLastRawPosition != rawPositionMS)
	{
		const double timeDelta = currentTime - m_dLastPositionTime;

		// only update rate if enough time has passed (5ms minimum)
		if (timeDelta > 0.005)
		{
			double newRate = 1000.0;

			if (rawPositionMS >= m_dLastRawPosition)
			{
				// normal forward movement
				newRate = (rawPositionMS - m_dLastRawPosition) / timeDelta;
			}
			else if (isLooped && lengthMS > 0)
			{
				// handle loop wraparound
				double length = static_cast<double>(lengthMS);
				double wrappedChange = (length - m_dLastRawPosition) + rawPositionMS;
				newRate = wrappedChange / timeDelta;
			}
			else
			{
				// backward movement (seeking), keep current rate
				newRate = m_iEstimatedRate;
			}

			// sanity check against expected rate (allow 20% deviation)
			const double expectedRate = 1000.0 * playbackSpeed;
			if (newRate < expectedRate * 0.8 || newRate > expectedRate * 1.2)
			{
				newRate = expectedRate * 0.7 + newRate * 0.3; // blend back toward expected
			}

			// smooth the rate estimate
			m_iEstimatedRate = m_iEstimatedRate * 0.6 + newRate * 0.4;
		}

		m_dLastRawPosition = rawPositionMS;
		m_dLastPositionTime = currentTime;
	}
	else
	{
		// gradual adjustment when position hasn't changed for a while
		const double timeSinceLastChange = currentTime - m_dLastPositionTime;
		if (timeSinceLastChange > 0.1)
		{
			const double expectedRate = 1000.0 * playbackSpeed;
			m_iEstimatedRate = m_iEstimatedRate * 0.95 + expectedRate * 0.05;
		}
	}

	// interpolate position based on estimated rate
	const double timeSinceLastReading = currentTime - m_dLastPositionTime;
	const double interpolatedPosition = m_dLastRawPosition + (timeSinceLastReading * m_iEstimatedRate);

	// handle looping
	if (isLooped && lengthMS > 0)
	{
		double length = static_cast<double>(lengthMS);
		if (interpolatedPosition >= length)
		{
			m_dLastInterpolatedPosition = static_cast<unsigned long>(fmod(interpolatedPosition, length));
			return m_dLastInterpolatedPosition;
		}
	}

	m_dLastInterpolatedPosition = static_cast<unsigned long>(std::max(0.0, interpolatedPosition));
	return m_dLastInterpolatedPosition;
}
