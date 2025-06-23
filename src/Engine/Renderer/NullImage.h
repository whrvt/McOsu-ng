//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty implementation of Image
//
// $NoKeywords: $nimg
//===============================================================================//

#pragma once
#ifndef NULLIMAGE_H
#define NULLIMAGE_H

#include "Image.h"

class NullImage final : public Image
{
public:
	NullImage(UString filePath, bool mipmapped = false, bool keepInSystemMemory = false) : Image(filePath, mipmapped, keepInSystemMemory) {;}
	NullImage(int width, int height, bool mipmapped = false, bool keepInSystemMemory = false) : Image(width, height, mipmapped, keepInSystemMemory) {;}
	~NullImage() override {destroy();}

	void bind(unsigned int textureUnit = 0) override {;}
	void unbind() override {;}

	void setFilterMode(Graphics::FILTER_MODE filterMode) override;
	void setWrapMode(Graphics::WRAP_MODE wrapMode) override;

private:
	void init() override {m_bReady = true;}
	void initAsync() override {m_bAsyncReady = true;}
	void destroy() override {;}
};

#endif
