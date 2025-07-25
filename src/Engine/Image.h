//========== Copyright (c) 2012, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		image wrapper
//
// $NoKeywords: $img
//===============================================================================//

#pragma once
#ifndef IMAGE_H
#define IMAGE_H

#include "Resource.h"

class Image : public Resource
{
public:
	static void saveToImage(unsigned char *data, unsigned int width, unsigned int height, UString filepath);

	enum class TYPE : uint8_t
	{
		TYPE_RGBA,
		TYPE_PNG,
		TYPE_JPG
	};

public:
	Image(UString filepath, bool mipmapped = false, bool keepInSystemMemory = false);
	Image(int width, int height, bool mipmapped = false, bool keepInSystemMemory = false);

	virtual void bind(unsigned int textureUnit = 0) = 0;
	virtual void unbind() = 0;

	virtual void setFilterMode(Graphics::FILTER_MODE filterMode);
	virtual void setWrapMode(Graphics::WRAP_MODE wrapMode);

	void setPixel(int x, int y, Color color);
	void setPixels(const char *data, size_t size, TYPE type);
	void setPixels(const std::vector<unsigned char> &pixels);

	[[nodiscard]] Color getPixel(int x, int y) const;

	[[nodiscard]] inline const Image::TYPE &getType() const { return m_type; }
	[[nodiscard]] inline const int &getNumChannels() const { return m_iNumChannels; }
	[[nodiscard]] inline const int &getWidth() const { return  m_iWidth; }
	[[nodiscard]] inline const int &getHeight() const { return m_iHeight; }
	[[nodiscard]] inline Vector2 getSize() const { return {m_iWidth, m_iHeight}; }

	[[nodiscard]] inline const bool &hasAlphaChannel() const { return m_bHasAlphaChannel; }

	// type inspection
	[[nodiscard]] Type getResType() const final { return IMAGE; }

	Image *asImage() final { return this; }
	[[nodiscard]] const Image *asImage() const final { return this; }

protected:
	void init() override = 0;
	void initAsync() override = 0;
	void destroy() override = 0;

	bool loadRawImage();

	Image::TYPE m_type;
	Graphics::FILTER_MODE m_filterMode;
	Graphics::WRAP_MODE m_wrapMode;

	int m_iNumChannels;
	int m_iWidth;
	int m_iHeight;

	bool m_bHasAlphaChannel;
	bool m_bMipmapped;
	bool m_bCreatedImage;
	bool m_bKeepInSystemMemory;

	std::vector<unsigned char> m_rawImage;

private:
	[[nodiscard]] bool isCompletelyTransparent() const;
	static bool canHaveTransparency(const unsigned char *data, size_t size);	

	static bool decodePNGFromMemory(const unsigned char *data, size_t size, std::vector<unsigned char> &outData, int &outWidth, int &outHeight, int &outChannels);
};

#endif
