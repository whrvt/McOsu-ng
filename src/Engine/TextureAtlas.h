//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		container for dynamically merging multiple images into one
//
// $NoKeywords: $imgtxat
//===============================================================================//

#pragma once
#ifndef TEXTUREATLAS_H
#define TEXTUREATLAS_H

#include "Resource.h"

class Image;

class TextureAtlas final : public Resource
{
public:
	struct PackRect
	{
		int x, y, width, height;
		int id; // user-defined identifier for tracking
	};

	TextureAtlas(int width = 512, int height = 512);
	~TextureAtlas() override { destroy(); }

	// place pixels at specific coordinates (for use after packing)
	void putAt(int x, int y, int width, int height, bool flipHorizontal, bool flipVertical, Color *pixels);

	// advanced skyline packing for efficient atlas utilization
	bool packRects(std::vector<PackRect> &rects);

	// calculate optimal atlas size for given rectangles
	static size_t calculateOptimalSize(
	    const std::vector<PackRect> &rects, float targetOccupancy = 0.75f, int padding = 1, size_t minSize = 256, size_t maxSize = 4096);

	void setPadding(int padding) { m_iPadding = padding; }

	[[nodiscard]] inline const int &getWidth() const { return m_iWidth; }
	[[nodiscard]] inline const int &getHeight() const { return m_iHeight; }
	[[nodiscard]] inline const int &getPadding() const { return m_iPadding; }
	[[nodiscard]] inline Image *getAtlasImage() const { return m_atlasImage; }

	// type inspection
	[[nodiscard]] Type getResType() const final { return TEXTUREATLAS; }

	TextureAtlas *asTextureAtlas() final { return this; }
	[[nodiscard]] const TextureAtlas *asTextureAtlas() const final { return this; }

private:
	struct Skyline
	{
		int x, y, width;
	};

	void init() override;
	void initAsync() override;
	void destroy() override;

	int m_iPadding;

	int m_iWidth;
	int m_iHeight;

	Image *m_atlasImage;

	// legacy packing state
	int m_iCurX;
	int m_iCurY;
	int m_iMaxHeight;
};

#endif
