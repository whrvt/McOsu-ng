//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		freetype font wrapper
//
// $NoKeywords: $fnt
//===============================================================================//

#pragma once
#ifndef FONT_H
#define FONT_H

#include "Resource.h"
#include "VertexArrayObject.h"

typedef struct FT_Bitmap_ FT_Bitmap;

class TextureAtlas;
class VertexArrayObject;

class McFont final : public Resource
{
public:
	McFont(UString filepath, int fontSize = 16, bool antialiasing = true, int fontDPI = 96);
	McFont(UString filepath, std::vector<wchar_t> characters, int fontSize = 16, bool antialiasing = true, int fontDPI = 96);
	~McFont() override { destroy(); }

	void drawString(const UString &text);
	void beginBatch();
	void addToBatch(const UString &text, const Vector3 &pos, Color color = 0xffffffff);
	void flushBatch();

	void setSize(int fontSize) { m_iFontSize = fontSize; }
	void setDPI(int dpi) { m_iFontDPI = dpi; }
	void setHeight(float height) { m_fHeight = height; }

	inline int getSize() const { return m_iFontSize; }
	inline int getDPI() const { return m_iFontDPI; }
	inline float getHeight() const { return m_fHeight; } // precomputed average height (fast)

	float getStringWidth(UString text) const;
	float getStringHeight(UString text) const;

	// debug
	void drawTextureAtlas();

	// type inspection
	[[nodiscard]] Type getResType() const override { return FONT; }

	McFont *asFont() override { return this; }
	[[nodiscard]] const McFont *asFont() const override { return this; }

protected:
	void constructor(std::vector<wchar_t> characters, int fontSize, bool antialiasing, int fontDPI);
	void init() override;
	void initAsync() override;
	void destroy() override;

private:
	struct GlyphRect
	{
		int x, y, width, height;
		wchar_t ch;
	};

	struct GLYPH_METRICS
	{
		wchar_t character;
		unsigned int uvPixelsX, uvPixelsY;
		unsigned int sizePixelsX, sizePixelsY;
		int left, top, width, rows;
		float advance_x;
	};

	struct BatchEntry
	{
		UString text;
		Vector3 pos;
		Color color;
	};

	struct TextBatch
	{
		size_t totalVerts;
		size_t usedEntries;
		std::vector<BatchEntry> entryList;
	};

	forceinline bool hasGlyph(wchar_t ch) const { return m_vGlyphMetrics.find(ch) != m_vGlyphMetrics.end(); };
	bool addGlyph(wchar_t ch);

	void buildGlyphGeometry(const GLYPH_METRICS &gm, const Vector3 &basePos, float advanceX, size_t &vertexCount);
	void buildStringGeometry(const UString &text, size_t &vertexCount);

	const GLYPH_METRICS &getGlyphMetrics(wchar_t ch) const;
	bool packGlyphRects(std::vector<GlyphRect> &rects, int atlasWidth, int atlasHeight);
	size_t calculateOptimalAtlasSize(const std::vector<GlyphRect> &glyphs, float targetOccupancy) const;
	unsigned char *unpackMonoBitmap(const FT_Bitmap &bitmap);

	int m_iFontSize;
	bool m_bAntialiasing;
	int m_iFontDPI;
	float m_fHeight;
	TextureAtlas *m_textureAtlas;
	GLYPH_METRICS m_errorGlyph;

	std::vector<wchar_t> m_vGlyphs;
	std::unordered_map<wchar_t, bool> m_vGlyphExistence;
	std::unordered_map<wchar_t, GLYPH_METRICS> m_vGlyphMetrics;

	VertexArrayObject m_vao;
	TextBatch m_batchQueue;
	std::vector<Vector3> m_vertices;
	std::vector<Vector2> m_texcoords;
	bool m_batchActive;
};

#endif
