//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		skin images/drawables
//
// $NoKeywords: $osuskimg
//===============================================================================//

#pragma once
#ifndef OSUSKINIMAGE_H
#define OSUSKINIMAGE_H

#include "cbase.h"

class OsuSkin;

class Image;

class OsuSkinImage
{
public:
	struct IMAGE
	{
		Image *img;
		float scale;
	};

public:
	OsuSkinImage(OsuSkin *skin, const UString& skinElementName, Vector2 baseSizeForScaling2x, float osuSize, const UString& animationSeparator = "-", bool ignoreDefaultSkin = false);
	virtual ~OsuSkinImage();

	virtual void draw(Vector2 pos, float scale = 1.0f); // for objects scaled automatically to the current resolution
	virtual void drawRaw(Vector2 pos, float scale); // for objects which scale depending on external factors (e.g. hitobjects, depending on the diameter defined by the CS)
	virtual void update(bool useEngineTimeForAnimations = true, long curMusicPos = 0);

	void setAnimationFramerate(float fps) {m_fFrameDuration = 1.0f / std::clamp<float>(fps, 1.0f, 9999.0f);}
	void setAnimationTimeOffset(long offset); // set this every frame (before drawing) to a fixed point in time relative to curMusicPos where we become visible
	void setAnimationFrameForce(int frame); // force set a frame, before drawing (e.g. for hitresults in OsuUIRankingScreenRankingPanel)
	void setAnimationFrameClampUp(); // force stop the animation after the last frame, before drawing

	void setDrawClipWidthPercent(float drawClipWidthPercent) {m_fDrawClipWidthPercent = drawClipWidthPercent;}

	Vector2 getSize(); // absolute size scaled to the current resolution (depending on the osuSize as defined when loaded in OsuSkin.cpp)
	Vector2 getSizeBase(); // default assumed size scaled to the current resolution. this is the base resolution which is used for all scaling calculations (to allow skins to overscale or underscale objects)
	Vector2 getSizeBaseRaw(); // default assumed size UNSCALED. that means that e.g. hitcircles will return either 128x128 or 256x256 depending on the @2x flag in the filename
	[[nodiscard]] inline Vector2 getSizeBaseRawForScaling2x() const {return m_vBaseSizeForScaling2x;}

	Vector2 getImageSizeForCurrentFrame(); // width/height of the actual image texture as loaded from disk
	IMAGE getImageForCurrentFrame();

	float getResolutionScale();

	bool isReady();

	[[nodiscard]] inline int getNumImages() const {return m_images.size();}
	[[nodiscard]] inline float getFrameDuration() const {return m_fFrameDuration;}
	[[nodiscard]] inline unsigned int getFrameNumber() const {return m_iFrameCounter;}
	[[nodiscard]] inline bool isMissingTexture() const {return m_bIsMissingTexture;}
	[[nodiscard]] inline bool isFromDefaultSkin() const {return m_bIsFromDefaultSkin;}

	[[nodiscard]] inline std::vector<UString> getFilepathsForExport() const {return m_filepathsForExport;}

private:
	

	bool load(UString skinElementName, UString animationSeparator, bool ignoreDefaultSkin);
	bool loadSingleImage(const UString& elementName, bool ignoreDefaultSkin);

	float getScale();
	float getImageScale();

    [[nodiscard]] UString buildImagePath(const UString& skinPath, const UString& elementName, bool hd = false) const;
    [[nodiscard]] UString buildDefaultImagePath(const UString& elementName, bool hd = false) const;

	OsuSkin *m_skin;
	bool m_bReady;

	// scaling
	Vector2 m_vBaseSizeForScaling2x;
	Vector2 m_vSize;
	float m_fOsuSize;

	// animation
	long m_iCurMusicPos;
	unsigned int m_iFrameCounter;
	unsigned long m_iFrameCounterUnclamped;
	float m_fLastFrameTime;
	float m_fFrameDuration;
	long m_iBeatmapAnimationTimeStartOffset;

	// raw files
	std::vector<IMAGE> m_images;
	bool m_bIsMissingTexture;
	bool m_bIsFromDefaultSkin;

	// custom
	float m_fDrawClipWidthPercent;
	std::vector<UString> m_filepathsForExport;
};

#endif
