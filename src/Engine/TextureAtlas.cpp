//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		container for dynamically merging multiple images into one
//
// $NoKeywords: $imgtxat
//===============================================================================//

#include "TextureAtlas.h"

#include "Engine.h"
#include "ResourceManager.h"

#include <algorithm>

TextureAtlas::TextureAtlas(int width, int height) : Resource()
{
	m_iWidth = width;
	m_iHeight = height;

	m_iPadding = 1;

	resourceManager->requestNextLoadUnmanaged();
	m_atlasImage = resourceManager->createImage(m_iWidth, m_iHeight);

	m_iCurX = m_iPadding;
	m_iCurY = m_iPadding;
	m_iMaxHeight = 0;
}

void TextureAtlas::init()
{
	resourceManager->loadResource(m_atlasImage);

	m_bReady = true;
}

void TextureAtlas::initAsync()
{
	m_bAsyncReady = true;
}

void TextureAtlas::destroy()
{
	SAFE_DELETE(m_atlasImage);
}

void TextureAtlas::putAt(int x, int y, int width, int height, bool flipHorizontal, bool flipVertical, Color *pixels)
{
	if (width < 1 || height < 1 || pixels == nullptr || m_atlasImage == nullptr)
		return;

	if (x + width > m_iWidth || y + height > m_iHeight || x < 0 || y < 0)
	{
		debugLog("TextureAtlas::putAt( {}, {}, {}, {} ) WARNING: Out of bounds! Atlas size: {}x{}\n", x, y, width, height, m_iWidth, m_iHeight);
		return;
	}

	// insert pixels at specified coordinates
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			int actualX = (flipHorizontal ? width - i - 1 : i);
			int actualY = (flipVertical ? height - j - 1 : j);

			const int atlasX = x + i;
			const int atlasY = y + j;
			const int sourceIdx = actualY * width + actualX;

			// bounds checking with debug info
			if (atlasX >= m_iWidth || atlasY >= m_iHeight)
			{
				debugLog("WARNING: Pixel placement out of bounds: atlas=({},{}) in {}x{}\n", atlasX, atlasY, m_iWidth, m_iHeight);
				continue;
			}

			m_atlasImage->setPixel(atlasX, atlasY, pixels[sourceIdx]);
		}
	}

	// mirror border pixels for padding > 1
	if (m_iPadding > 1)
	{
		// left border
		for (int j = -1; j < height + 1; j++)
		{
			const int i = 0;
			int actualX = (flipHorizontal ? width - i - 1 : i);
			int actualY = std::clamp<int>((flipVertical ? height - j - 1 : j), 0, height - 1);

			if (x + i - 1 >= 0 && y + j >= 0 && y + j < m_iHeight)
				m_atlasImage->setPixel(x + i - 1, y + j, pixels[actualY * width + actualX]);
		}
		// right border
		for (int j = -1; j < height + 1; j++)
		{
			const int i = width - 1;
			int actualX = (flipHorizontal ? width - i - 1 : i);
			int actualY = std::clamp<int>((flipVertical ? height - j - 1 : j), 0, height - 1);

			if (x + i + 1 < m_iWidth && y + j >= 0 && y + j < m_iHeight)
				m_atlasImage->setPixel(x + i + 1, y + j, pixels[actualY * width + actualX]);
		}
		// top border
		for (int i = -1; i < width + 1; i++)
		{
			const int j = 0;
			int actualX = std::clamp<int>((flipHorizontal ? width - i - 1 : i), 0, width - 1);
			int actualY = (flipVertical ? height - j - 1 : j);

			if (x + i >= 0 && x + i < m_iWidth && y + j - 1 >= 0)
				m_atlasImage->setPixel(x + i, y + j - 1, pixels[actualY * width + actualX]);
		}
		// bottom border
		for (int i = -1; i < width + 1; i++)
		{
			const int j = height - 1;
			int actualX = std::clamp<int>((flipHorizontal ? width - i - 1 : i), 0, width - 1);
			int actualY = (flipVertical ? height - j - 1 : j);

			if (x + i >= 0 && x + i < m_iWidth && y + j + 1 < m_iHeight)
				m_atlasImage->setPixel(x + i, y + j + 1, pixels[actualY * width + actualX]);
		}
	}
}

bool TextureAtlas::packRects(std::vector<PackRect> &rects)
{
	if (rects.empty())
		return true;

	// sort rectangles by height (tallest first) for better packing efficiency
	std::ranges::sort(rects, [](const PackRect &a, const PackRect &b) { return a.height > b.height; });

	// initialize skyline - start with single segment covering entire width
	std::vector<Skyline> skylines = {
	    {0, m_iPadding, m_iWidth}
    };

	for (auto &rect : rects)
	{
		const int rectWidth = rect.width + m_iPadding;
		const int rectHeight = rect.height + m_iPadding;

		int bestHeight = m_iHeight;
		int bestIndex = -1;
		int bestX = m_iWidth; // initialize to rightmost position for leftmost preference

		// find best position along skyline
		for (size_t i = 0; i < skylines.size(); ++i)
		{
			// check if rectangle fits horizontally at this skyline segment
			if (skylines[i].x + rectWidth > m_iWidth)
				continue;

			// find maximum height across all skyline segments this rect would span
			int maxY = skylines[i].y;
			int currentX = skylines[i].x;

			for (size_t j = i; j < skylines.size() && currentX < skylines[i].x + rectWidth; ++j)
			{
				maxY = std::max(maxY, skylines[j].y);
				currentX += skylines[j].width;
			}

			// select this position if it gives us the minimum height
			if (maxY + rectHeight < bestHeight)
			{
				bestHeight = maxY + rectHeight;
				bestIndex = static_cast<int>(i);
				bestX = skylines[i].x;
			}
		}

		if (bestIndex == -1 || bestHeight > m_iHeight)
		{
			debugLog("ERROR: Packing failed for rect id={}: bestIndex={}, bestHeight={}, atlasHeight={}\n", rect.id, bestIndex, bestHeight, m_iHeight);
			return false;
		}

		// place the rectangle
		rect.x = bestX + m_iPadding;
		rect.y = bestHeight - rectHeight;

		// update skyline - remove segments covered by this rectangle and add new segment
		std::vector<Skyline> newSkylines;

		// copy segments before the placed rectangle
		for (auto & skyline : skylines)
		{
			if (skyline.x + skyline.width <= bestX)
				newSkylines.push_back(skyline);
			else if (skyline.x < bestX)	// partial segment before rectangle
				newSkylines.push_back({skyline.x, skyline.y, bestX - skyline.x});
			else
				break;
		}

		// add new segment for placed rectangle
		newSkylines.push_back({bestX, bestHeight, rectWidth});

		// add remaining segments after the placed rectangle
		for (auto & skyline : skylines)
		{
			if (skyline.x >= bestX + rectWidth)
			{
				newSkylines.push_back(skyline);
			}
			else if (skyline.x + skyline.width > bestX + rectWidth)
			{
				// partial segment after rectangle
				int newX = bestX + rectWidth;
				int newWidth = skyline.x + skyline.width - newX;
				newSkylines.push_back({newX, skyline.y, newWidth});
			}
		}

		skylines = std::move(newSkylines);

		// merge adjacent skylines with same height
		for (size_t i = 0; i < skylines.size() - 1; ++i)
		{
			if (skylines[i].y == skylines[i + 1].y && skylines[i].x + skylines[i].width == skylines[i + 1].x)
			{
				skylines[i].width += skylines[i + 1].width;
				skylines.erase(skylines.begin() + i + 1);
				--i;
			}
		}
	}

	return true;
}

size_t TextureAtlas::calculateOptimalSize(const std::vector<PackRect> &rects, float targetOccupancy, int padding, size_t minSize, size_t maxSize)
{
	if (rects.empty())
		return minSize;

	// calculate total area including padding
	size_t totalArea = 0;
	for (const auto &rect : rects)
	{
		totalArea += static_cast<size_t>((rect.width + padding) * (rect.height + padding));
	}

	// add 20% for packing inefficiency
	totalArea = static_cast<size_t>(totalArea * 1.2f);

	// find smallest power of 2 that can fit the rectangles with desired occupancy
	size_t size = minSize;
	while (size * size * targetOccupancy < totalArea && size < maxSize)
	{
		size *= 2;
	}

	return size;
}
