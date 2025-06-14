//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		global fps independent animations
//
// $NoKeywords: $anim
//===============================================================================//

#include "AnimationHandler.h"

#include "Engine.h"
#include "ConVar.h"

namespace cv {
ConVar debug_anim("debug_anim", false, FCVAR_NONE);
}

AnimationHandler *anim = NULL;

AnimationHandler::AnimationHandler()
{
	anim = this;
}

AnimationHandler::~AnimationHandler()
{
	m_vAnimations.clear();

	anim = NULL;
}

void AnimationHandler::update()
{
	for (size_t i=0; i<m_vAnimations.size(); i++)
	{
		// start animation
		Animation &animation = m_vAnimations[i];
		if (engine->getTime() < animation.m_fStartTime)
			continue;
		else if (!animation.m_bStarted)
		{
			// after our delay, take the current value as startValue, then start animating to the target
			animation.m_fStartValue = *animation.m_fBase;
			animation.m_bStarted = true;
		}

		// calculate percentage
		float percent = std::clamp<float>((engine->getTime() - animation.m_fStartTime) / (animation.m_fDuration), 0.0f, 1.0f);

		if (cv::debug_anim.getBool())
			debugLog("animation #{}, percent = {:f}\n", i, percent);

		// check if finished
		if (percent >= 1.0f)
		{
			*animation.m_fBase = animation.m_fTarget;

			if (cv::debug_anim.getBool())
				debugLog("removing animation #{}, dtime = {:f}\n", i, engine->getTime() - animation.m_fStartTime);

			m_vAnimations.erase(m_vAnimations.begin() + i);
			i--;

			continue;
		}

		// modify percentage
		switch (animation.m_animType)
		{
		case ANIMATION_TYPE::MOVE_SMOOTH_END:
			percent = std::clamp<float>(1.0f - std::pow(1.0f - percent, animation.m_fFactor), 0.0f, 1.0f);
			if ((int)(percent*(animation.m_fTarget - animation.m_fStartValue) + animation.m_fStartValue) == (int)animation.m_fTarget)
				percent = 1.0f;
			break;

		case ANIMATION_TYPE::MOVE_QUAD_IN:
			percent = percent*percent;
			break;

		case ANIMATION_TYPE::MOVE_QUAD_OUT:
			percent = -percent*(percent - 2.0f);
			break;

		case ANIMATION_TYPE::MOVE_QUAD_INOUT:
			if ((percent *= 2.0f) < 1.0f)
				percent = 0.5f*percent*percent;
			else
			{
				percent -= 1.0f;
				percent = -0.5f * ((percent)*(percent - 2.0f) - 1.0f);
			}
			break;

		case ANIMATION_TYPE::MOVE_CUBIC_IN:
			percent = percent*percent*percent;
			break;

		case ANIMATION_TYPE::MOVE_CUBIC_OUT:
			percent = percent - 1.0f;
			percent = percent*percent*percent + 1.0f;
			break;

		case ANIMATION_TYPE::MOVE_QUART_IN:
			percent = percent*percent*percent*percent;
			break;

		case ANIMATION_TYPE::MOVE_QUART_OUT:
			percent = percent - 1.0f;
			percent = 1.0f - percent*percent*percent*percent;
			break;
		default: // MOVE_LINEAR unhandled
			break;
		}

		// set new value
		*animation.m_fBase = animation.m_fStartValue + percent*(animation.m_fTarget - animation.m_fStartValue);
	}

	// TODO: prevent this from happening
	if (cv::debug_anim.getBool() && (m_vAnimations.size() > 512))
		debugLog("WARNING: AnimationHandler has {} animations!\n", m_vAnimations.size());

	//printf("AnimStackSize = %i\n", m_vAnimations.size());
}

void AnimationHandler::moveLinear(float *base, float target, float duration, float delay, bool overrideExisting)
{
	addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_LINEAR);
}

void AnimationHandler::moveQuadIn(float *base, float target, float duration, float delay, bool overrideExisting)
{
	addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUAD_IN);
}

void AnimationHandler::moveQuadOut(float *base, float target, float duration, float delay, bool overrideExisting)
{
	addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUAD_OUT);
}

void AnimationHandler::moveQuadInOut(float *base, float target, float duration, float delay, bool overrideExisting)
{
	addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUAD_INOUT);
}

void AnimationHandler::moveCubicIn(float *base, float target, float duration, float delay, bool overrideExisting)
{
	addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_CUBIC_IN);
}

void AnimationHandler::moveCubicOut(float *base, float target, float duration, float delay, bool overrideExisting)
{
	addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_CUBIC_OUT);
}

void AnimationHandler::moveQuartIn(float *base, float target, float duration, float delay, bool overrideExisting)
{
	addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUART_IN);
}

void AnimationHandler::moveQuartOut(float *base, float target, float duration, float delay, bool overrideExisting)
{
	addAnimation(base, target, duration, delay, overrideExisting, ANIMATION_TYPE::MOVE_QUART_OUT);
}

void AnimationHandler::moveSmoothEnd(float *base, float target, float duration, int smoothFactor, float delay)
{
	addAnimation(base, target, duration, delay, true, ANIMATION_TYPE::MOVE_SMOOTH_END, smoothFactor);
}

void AnimationHandler::addAnimation(float *base, float target, float duration, float delay, bool overrideExisting, AnimationHandler::ANIMATION_TYPE type, float smoothFactor)
{
	if (base == NULL) return;

	if (overrideExisting)
		overrideExistingAnimation(base);

	Animation anim;

	anim.m_fBase = base;
	anim.m_fTarget = target;
	anim.m_fDuration = duration;
	anim.m_fStartValue = *base;
	anim.m_fStartTime = engine->getTime() + delay;
	anim.m_animType = type;
	anim.m_fFactor = smoothFactor;
	anim.m_bStarted = (delay == 0.0f);

	m_vAnimations.push_back(anim);
}

void AnimationHandler::overrideExistingAnimation(float *base)
{
	deleteExistingAnimation(base);
}

void AnimationHandler::deleteExistingAnimation(float *base)
{
	for (size_t i=0; i<m_vAnimations.size(); i++)
	{
		if (m_vAnimations[i].m_fBase == base)
		{
			m_vAnimations.erase(m_vAnimations.begin() + i);
			i--;
		}
	}
}

float AnimationHandler::getRemainingDuration(float *base) const
{
	for (size_t i=0; i<m_vAnimations.size(); i++)
	{
		if (m_vAnimations[i].m_fBase == base)
			return std::max(0.0f, (m_vAnimations[i].m_fStartTime + m_vAnimations[i].m_fDuration) - (float)engine->getTime());
	}

	return 0.0f;
}

bool AnimationHandler::isAnimating(float *base) const
{
	for (size_t i=0; i<m_vAnimations.size(); i++)
	{
		if (m_vAnimations[i].m_fBase == base)
			return true;
	}

	return false;
}
