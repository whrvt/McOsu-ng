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

class TextureAtlas : public Resource
{
public:
	TextureAtlas(int width = 512, int height = 512);
	~TextureAtlas() override {destroy();}

	Vector2 put(int width, int height, Color *pixels) {return put(width, height, false, false, pixels);}
	Vector2 put(int width, int height, bool flipHorizontal, bool flipVertical, Color *pixels);

	void setPadding(int padding) {m_iPadding = padding;}

	[[nodiscard]] inline int getWidth() const {return m_iWidth;}
	[[nodiscard]] inline int getHeight() const {return m_iHeight;}
	[[nodiscard]] inline Image *getAtlasImage() const {return m_atlasImage;}

	// type inspection
	[[nodiscard]] Type getResType() const override { return TEXTUREATLAS; }

	TextureAtlas *asTextureAtlas() override { return this; }
	[[nodiscard]] const TextureAtlas *asTextureAtlas() const override { return this; }

private:
	void init() override;
	void initAsync() override;
	void destroy() override;

	int m_iPadding;

	int m_iWidth;
	int m_iHeight;

	Image *m_atlasImage;

	int m_iCurX;
	int m_iCurY;
	int m_iMaxHeight;
};

#endif
