//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		skin images/drawables
//
// $NoKeywords: $osuskimg
//===============================================================================//

#include "OsuSkinImage.h"

#include "Engine.h"
#include "Environment.h"
#include "ResourceManager.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"

ConVar osu_skin_animation_fps_override("osu_skin_animation_fps_override", -1.0f, FCVAR_NONE);

ConVar *OsuSkinImage::m_osu_skin_mipmaps_ref = NULL;

OsuSkinImage::OsuSkinImage(OsuSkin *skin, UString skinElementName, Vector2 baseSizeForScaling2x, float osuSize, UString animationSeparator, bool ignoreDefaultSkin)
    : m_skin(skin), m_vBaseSizeForScaling2x(baseSizeForScaling2x), m_fOsuSize(osuSize), m_bReady(false), m_iCurMusicPos(0), m_iFrameCounter(0),
      m_iFrameCounterUnclamped(0), m_fLastFrameTime(0.0f), m_iBeatmapAnimationTimeStartOffset(0), m_bIsMissingTexture(false), m_bIsFromDefaultSkin(false),
      m_fDrawClipWidthPercent(1.0f)
{
	if (m_osu_skin_mipmaps_ref == nullptr)
		m_osu_skin_mipmaps_ref = convar->getConVarByName("osu_skin_mipmaps");

	// attempt to load skin elements and fallback to default
	if (!load(skinElementName, animationSeparator, true) && !ignoreDefaultSkin)
		load(skinElementName, animationSeparator, false);

	// fallback to missing texture if nothing loaded
	if (m_images.empty())
	{
		m_bIsMissingTexture = true;

		IMAGE missingTexture;
		missingTexture.img = m_skin->getMissingTexture();
		missingTexture.scale = 2;
		m_images.push_back(missingTexture);
	}

	// set animation framerate
	if (m_skin->getAnimationFramerate() > 0.0f)
		m_fFrameDuration = 1.0f / m_skin->getAnimationFramerate();
	else if (!m_images.empty())
		m_fFrameDuration = 1.0f / static_cast<float>(m_images.size());
}

bool OsuSkinImage::loadSingleImage(const UString &elementName, bool ignoreDefaultSkin)
{
	const UString skinPath = m_skin->getFilePath();

	// build all possible paths
	const UString userHd = buildImagePath(skinPath, elementName, true);
	const UString userNormal = buildImagePath(skinPath, elementName, false);
	const UString defaultHd = buildDefaultImagePath(elementName, true);
	const UString defaultNormal = buildDefaultImagePath(elementName, false);

	// check existence once
	const bool existsUserHd = m_skin->skinFileExists(userHd);
	const bool existsUserNormal = m_skin->skinFileExists(userNormal);
	const bool existsDefaultHd = m_skin->skinFileExists(defaultHd);
	const bool existsDefaultNormal = m_skin->skinFileExists(defaultNormal);

	// loading candidates in priority order
	struct LoadCandidate
	{
		const UString &path;
		bool exists;
		float scale;
		bool isDefault;
	};

	const std::initializer_list<LoadCandidate> candidates = {
	    {.path = userHd,        .exists = existsUserHd && OsuSkin::m_osu_skin_hd->getBool(),                          .scale = 2.0f, .isDefault = false},
	    {.path = userNormal,    .exists = existsUserNormal,	                                                       .scale = 1.0f, .isDefault = false},
	    {.path = defaultHd,     .exists = existsDefaultHd && OsuSkin::m_osu_skin_hd->getBool() && !ignoreDefaultSkin, .scale = 2.0f, .isDefault = true },
	    {.path = defaultNormal, .exists = existsDefaultNormal && !ignoreDefaultSkin,                                  .scale = 1.0f, .isDefault = true }
    };

	for (const auto &candidate : candidates)
	{
		if (!candidate.exists)
			continue;

		IMAGE image;

		if (OsuSkin::m_osu_skin_async->getBool())
			resourceManager->requestNextLoadAsync();

		image.img = resourceManager->loadImageAbsUnnamed(candidate.path, m_osu_skin_mipmaps_ref->getBool());
		image.scale = candidate.scale;

		m_images.push_back(image);

		// handle export paths; add primary path first, then secondary if it exists
		m_filepathsForExport.push_back(candidate.path);
		if (!candidate.isDefault)
		{
			// add the other resolution if it exists
			if (candidate.scale == 2.0f && existsUserNormal)
				m_filepathsForExport.push_back(userNormal);
			else if (candidate.scale == 1.0f && existsUserHd)
				m_filepathsForExport.push_back(userHd);
		}
		else
		{
			// add the other default resolution if it exists
			if (candidate.scale == 2.0f && existsDefaultNormal)
				m_filepathsForExport.push_back(defaultNormal);
			else if (candidate.scale == 1.0f && existsDefaultHd)
				m_filepathsForExport.push_back(defaultHd);
		}

		// note if we loaded from default skin
		if (candidate.isDefault)
			m_bIsFromDefaultSkin = true;

		return true;
	}

	return false;
}

bool OsuSkinImage::load(UString skinElementName, UString animationSeparator, bool ignoreDefaultSkin)
{
	// try animated loading first (element + separator + "0")
	UString animatedStartName = UString::fmt("{}{}{}", skinElementName, animationSeparator, "0");

	if (loadSingleImage(animatedStartName, ignoreDefaultSkin))
	{
		// continue loading animation frames until we hit a missing one
		for (int frame = 1; frame < 512; ++frame) // sanity limit
		{
			UString frameName = UString::fmt("{}{}{}", skinElementName, animationSeparator, frame);
			if (!loadSingleImage(frameName, ignoreDefaultSkin))
				break; // stop on first missing frame
		}
	}
	else
	{
		// try non-animated loading
		loadSingleImage(skinElementName, ignoreDefaultSkin);
	}

	return !m_images.empty();
}

OsuSkinImage::~OsuSkinImage()
{
	for (int i=0; i<m_images.size(); i++)
	{
		if (m_images[i].img != m_skin->getMissingTexture())
			resourceManager->destroyResource(m_images[i].img);
	}
	m_images.clear();

	m_filepathsForExport.clear();
}

void OsuSkinImage::draw(Vector2 pos, float scale)
{
	if (m_images.size() < 1) return;

	scale *= getScale(); // auto scale to current resolution

	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(pos.x, pos.y);

		Image *img = getImageForCurrentFrame().img;

		if (m_fDrawClipWidthPercent == 1.0f)
			g->drawImage(img);
		else if (img->isReady())
		{
			const float realWidth = img->getWidth();
			const float realHeight = img->getHeight();

			const float width = realWidth * m_fDrawClipWidthPercent;
			const float height = realHeight;

			const float x = -realWidth/2;
			const float y = -realHeight/2;

			VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

			vao.addVertex(x, y);
			vao.addTexcoord(0, 0);

			vao.addVertex(x, (y + height));
			vao.addTexcoord(0, 1);

			vao.addVertex((x + width), (y + height));
			vao.addTexcoord(m_fDrawClipWidthPercent, 1);

			vao.addVertex((x + width), y);
			vao.addTexcoord(m_fDrawClipWidthPercent, 0);

			img->bind();
			{
				g->drawVAO(&vao);
			}
			img->unbind();
		}
	}
	g->popTransform();
}

void OsuSkinImage::drawRaw(Vector2 pos, float scale)
{
	if (m_images.size() < 1) return;

	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(pos.x, pos.y);

		Image *img = getImageForCurrentFrame().img;

		if (m_fDrawClipWidthPercent == 1.0f)
			g->drawImage(img);
		else if (img->isReady())
		{
			const float realWidth = img->getWidth();
			const float realHeight = img->getHeight();

			const float width = realWidth * m_fDrawClipWidthPercent;
			const float height = realHeight;

			const float x = -realWidth/2;
			const float y = -realHeight/2;

			VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

			vao.addVertex(x, y);
			vao.addTexcoord(0, 0);

			vao.addVertex(x, (y + height));
			vao.addTexcoord(0, 1);

			vao.addVertex((x + width), (y + height));
			vao.addTexcoord(m_fDrawClipWidthPercent, 1);

			vao.addVertex((x + width), y);
			vao.addTexcoord(m_fDrawClipWidthPercent, 0);

			img->bind();
			{
				g->drawVAO(&vao);
			}
			img->unbind();
		}
	}
	g->popTransform();
}

void OsuSkinImage::update(bool useEngineTimeForAnimations, long curMusicPos)
{
	if (m_images.size() < 1) return;

	m_iCurMusicPos = curMusicPos;

	const float frameDurationInSeconds = (osu_skin_animation_fps_override.getFloat() > 0.0f ? (1.0f / osu_skin_animation_fps_override.getFloat()) : m_fFrameDuration);

	if (useEngineTimeForAnimations)
	{
		// as expected
		if (engine->getTime() >= m_fLastFrameTime)
		{
			m_fLastFrameTime = engine->getTime() + frameDurationInSeconds;

			m_iFrameCounter++;
			m_iFrameCounterUnclamped++;
			m_iFrameCounter = m_iFrameCounter % m_images.size();
		}
	}
	else
	{
		// when playing a beatmap, objects start the animation at frame 0 exactly when they first become visible (this wouldn't work with the engine time method)
		// therefore we need an offset parameter in the same time-space as the beatmap (m_iBeatmapTimeAnimationStartOffset), and we need the beatmap time (curMusicPos) as a relative base
		// m_iBeatmapAnimationTimeStartOffset must be set by all hitobjects live while drawing (e.g. to their m_iTime-m_iObjectTime), since we don't have any animation state saved in the hitobjects!
		m_iFrameCounter = std::max((int)((curMusicPos - m_iBeatmapAnimationTimeStartOffset) / (long)(frameDurationInSeconds*1000.0f)), 0); // freeze animation on frame 0 on negative offsets
		m_iFrameCounterUnclamped = m_iFrameCounter;
		m_iFrameCounter = m_iFrameCounter % m_images.size(); // clamp and wrap around to the number of frames we have
	}
}

void OsuSkinImage::setAnimationTimeOffset(long offset)
{
	m_iBeatmapAnimationTimeStartOffset = offset;
	update(false, m_iCurMusicPos); // force update
}

void OsuSkinImage::setAnimationFrameForce(int frame)
{
	if (m_images.size() < 1) return;
	m_iFrameCounter = frame % m_images.size();
	m_iFrameCounterUnclamped = m_iFrameCounter;
}

void OsuSkinImage::setAnimationFrameClampUp()
{
	if (m_images.size() > 0 && m_iFrameCounterUnclamped > m_images.size()-1)
		m_iFrameCounter = m_images.size() - 1;
}

Vector2 OsuSkinImage::getSize()
{
	if (m_images.size() > 0)
		return getImageForCurrentFrame().img->getSize() * getScale();
	else
		return getSizeBase();
}

Vector2 OsuSkinImage::getSizeBase()
{
	return m_vBaseSizeForScaling2x * getResolutionScale();
}

Vector2 OsuSkinImage::getSizeBaseRaw()
{
	return m_vBaseSizeForScaling2x * getImageForCurrentFrame().scale;
}

Vector2 OsuSkinImage::getImageSizeForCurrentFrame()
{
	return getImageForCurrentFrame().img->getSize();
}

float OsuSkinImage::getScale()
{
	return getImageScale() * getResolutionScale();
}

float OsuSkinImage::getImageScale()
{
	if (m_images.size() > 0)
		return m_vBaseSizeForScaling2x.x / getSizeBaseRaw().x; // allow overscale and underscale
	else
		return 1.0f;
}

float OsuSkinImage::getResolutionScale()
{
	return Osu::getImageScale(m_vBaseSizeForScaling2x, m_fOsuSize);
}

bool OsuSkinImage::isReady()
{
	if (m_bReady) return true;

	for (int i=0; i<m_images.size(); i++)
	{
		if (resourceManager->isLoadingResource(m_images[i].img))
			return false;
	}

	m_bReady = true;
	return m_bReady;
}

OsuSkinImage::IMAGE OsuSkinImage::getImageForCurrentFrame()
{
	if (m_images.size() > 0)
		return m_images[m_iFrameCounter % m_images.size()];
	else
	{
		IMAGE image;

		image.img = m_skin->getMissingTexture();
		image.scale = 1.0f;

		return image;
	}
}

// file path construction helpers
UString OsuSkinImage::buildImagePath(const UString& skinPath, const UString& elementName, bool hd) const
{
	return UString::fmt("{}{}{}.png", skinPath, elementName, hd ? "@2x" : "");
}

// TODO: rework this nonsense, the default paths should only be created once
UString OsuSkinImage::buildDefaultImagePath(const UString &elementName, bool hd) const
{
	return UString::fmt("{}{}{}.png", OsuSkin::DEFAULT_SKIN_PATH, elementName, hd ? "@2x" : "");
}
