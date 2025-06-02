//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		freetype font wrapper with unicode support
//
// $NoKeywords: $fnt
//===============================================================================//

#include "Font.h"
#include "ConVar.h"
#include "Engine.h"
#include "FontTypeMap.h"
#include "ResourceManager.h"
#include "TextureAtlas.h"

#include <freetype/freetype.h>
#include <freetype/ftbitmap.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

#include <ft2build.h>

#include <algorithm>
#include <utility>

// TODO: use fontconfig on linux?
#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

// constants for atlas generation and rendering
static constexpr float ATLAS_OCCUPANCY_TARGET = 0.75f; // target atlas occupancy before resize
static constexpr size_t MIN_ATLAS_SIZE = 256;
static constexpr size_t MAX_ATLAS_SIZE = 4096;
static constexpr wchar_t UNKNOWN_CHAR = '?'; // ASCII '?'

static constexpr auto VERTS_PER_VAO = Env::cfg(REND::GLES2 | REND::GLES32) ? 6 : 4;

namespace cv
{
ConVar r_drawstring_max_string_length("r_drawstring_max_string_length", 65536, FCVAR_CHEAT, "maximum number of characters per call, sanity/memory buffer limit");
ConVar r_debug_drawstring_unbind("r_debug_drawstring_unbind", false, FCVAR_NONE);
ConVar r_debug_font_atlas_padding("r_debug_font_atlas_padding", 1, FCVAR_NONE, "padding between glyphs in the atlas to prevent bleeding");
ConVar r_debug_font_unicode("r_debug_font_unicode", false, FCVAR_NONE, "debug messages for unicode/fallback font related stuff");
ConVar font_load_system("font_load_system", true, FCVAR_NONE, "try to load a similar system font if a glyph is missing in the bundled fonts");
} // namespace cv

McFont::McFont(const UString &filepath, int fontSize, bool antialiasing, int fontDPI)
    : Resource(filepath), m_vao((Env::cfg(REND::GLES2 | REND::GLES32) ? Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES : Graphics::PRIMITIVE::PRIMITIVE_QUADS),
                                Graphics::USAGE_TYPE::USAGE_DYNAMIC)
{
	std::vector<wchar_t> characters;
	characters.reserve(96); // reserve space for basic ASCII, load the rest as needed
	for (int i = 32; i < 128; i++)
	{
		characters.push_back(static_cast<wchar_t>(i));
	}
	constructor(characters, fontSize, antialiasing, fontDPI);
}

McFont::McFont(const UString &filepath, const std::vector<wchar_t> &characters, int fontSize, bool antialiasing, int fontDPI)
    : Resource(filepath), m_vao((Env::cfg(REND::GLES2 | REND::GLES32) ? Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES : Graphics::PRIMITIVE::PRIMITIVE_QUADS),
                                Graphics::USAGE_TYPE::USAGE_DYNAMIC)
{
	constructor(characters, fontSize, antialiasing, fontDPI);
}

void McFont::constructor(const std::vector<wchar_t> &characters, int fontSize, bool antialiasing, int fontDPI)
{
	m_iFontSize = fontSize;
	m_bAntialiasing = antialiasing;
	m_iFontDPI = fontDPI;
	m_textureAtlas = nullptr;
	m_fHeight = 1.0f;
	m_batchActive = false;
	m_batchQueue.totalVerts = 0;
	m_batchQueue.usedEntries = 0;

	// freetype initialization state
	m_ftLibrary = nullptr;
	m_ftFace = nullptr;
	m_bFreeTypeInitialized = false;
	m_bAtlasNeedsRebuild = false;

	// fallback font system
	m_bFallbacksInitialized = false;

	// setup error glyph
	m_errorGlyph = {.character = UNKNOWN_CHAR,
	                .uvPixelsX = 10,
	                .uvPixelsY = 1,
	                .sizePixelsX = 1,
	                .sizePixelsY = 0,
	                .left = 0,
	                .top = 10,
	                .width = 10,
	                .rows = 1,
	                .advance_x = 0,
	                .fontIndex = 0};

	// pre-allocate space for initial glyphs
	m_vGlyphs.reserve(characters.size());
	for (wchar_t ch : characters)
	{
		addGlyph(ch);
	}
}

void McFont::init()
{
	debugLog("Resource Manager: Loading {:s}\n", m_sFilePath.toUtf8());

	if (!initializeFreeType())
		return;

	initializeFallbackFonts();

	// load metrics for all initial glyphs
	for (wchar_t ch : m_vGlyphs)
	{
		loadGlyphMetrics(ch);
	}

	// create atlas and render all glyphs
	if (!createAndPackAtlas(m_vGlyphs))
		return;

	// precalculate average/max ASCII glyph height
	m_fHeight = 0.0f;
	for (int i = 32; i < 128; i++)
	{
		const int curHeight = getGlyphMetrics(static_cast<wchar_t>(i)).top;
		m_fHeight = std::max(m_fHeight, static_cast<float>(curHeight));
	}

	m_bReady = true;
}

bool McFont::loadGlyphDynamic(wchar_t ch)
{
	if (!m_bFreeTypeInitialized || hasGlyph(ch))
		return hasGlyph(ch);

	int fontIndex = 0;
	FT_Face targetFace = getFontFaceForGlyph(ch, fontIndex);

	if (!targetFace)
	{
		if (cv::r_debug_font_unicode.getBool())
		{
			// character not supported by any available font
			const char *charRange = FontTypeMap::getCharacterRangeName(ch);
			if (charRange)
				debugLog("Font Warning: Character U+{:04X} ({:s}) not supported by any font\n", (unsigned int)ch, charRange);
			else
				debugLog("Font Warning: Character U+{:04X} not supported by any font\n", (unsigned int)ch);
		}
		return false;
	}

	if (cv::r_debug_font_unicode.getBool() && fontIndex > 0)
		debugLog("Font Info: Using fallback font #{:d} for character U+{:04X}\n", fontIndex, (unsigned int)ch);

	// load glyph from the selected font face
	if (!loadGlyphFromFace(ch, targetFace, fontIndex))
		return false;

	const auto &metrics = m_vGlyphMetrics[ch];

	// check if we need atlas space for non-empty glyphs
	if (metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0)
	{
		const int padding = cv::r_debug_font_atlas_padding.getInt();
		const int requiredWidth = metrics.sizePixelsX + padding;
		const int requiredHeight = metrics.sizePixelsY + padding;

		if (!ensureAtlasSpace(requiredWidth, requiredHeight))
		{
			// atlas is full, queue for rebuild
			m_vPendingGlyphs.push_back(ch);
			m_bAtlasNeedsRebuild = true;
			addGlyph(ch);
			return true; // glyph metrics are stored, will be packed later
		}

		// try to pack into current atlas
		std::vector<TextureAtlas::PackRect> singleRect = {
		    {0, 0, static_cast<int>(metrics.sizePixelsX), static_cast<int>(metrics.sizePixelsY), 0}
        };

		if (m_textureAtlas->packRects(singleRect))
		{
			// successfully packed, render to atlas using the correct font face
			renderGlyphToAtlas(ch, singleRect[0].x, singleRect[0].y, targetFace);
		}
		else
		{
			// couldn't pack into current atlas, queue for rebuild
			m_vPendingGlyphs.push_back(ch);
			m_bAtlasNeedsRebuild = true;
		}
	}

	addGlyph(ch);
	return true;
}

FT_Face McFont::getFontFaceForGlyph(wchar_t ch, int &fontIndex)
{
	// check primary font first
	fontIndex = 0;
	FT_UInt glyphIndex = FT_Get_Char_Index(m_ftFace, ch);
	if (glyphIndex != 0)
		return m_ftFace;

	// search through fallback fonts if init
	if (m_bFallbacksInitialized)
	{
		for (size_t i = 0; i < m_fallbackFonts.size(); ++i)
		{
			glyphIndex = FT_Get_Char_Index(m_fallbackFonts[i].face, ch);
			if (glyphIndex != 0)
			{
				fontIndex = static_cast<int>(i + 1); // offset by 1 since 0 is primary font
				return m_fallbackFonts[i].face;
			}
		}
	}

	return nullptr; // character not found in any font
}

bool McFont::loadGlyphFromFace(wchar_t ch, FT_Face face, int fontIndex)
{
	if (FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), m_bAntialiasing ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO))
	{
		debugLog("Font Error: Failed to load glyph for character {:d} from font index {:d}\n", (int)ch, fontIndex);
		return false;
	}

	FT_Glyph glyph{};
	if (FT_Get_Glyph(face->glyph, &glyph))
	{
		debugLog("Font Error: Failed to get glyph for character {:d} from font index {:d}\n", (int)ch, fontIndex);
		return false;
	}

	FT_Glyph_To_Bitmap(&glyph, m_bAntialiasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO, nullptr, 1);

	auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

	// store metrics for the character
	GLYPH_METRICS &metrics = m_vGlyphMetrics[ch];
	metrics.character = ch;
	metrics.left = bitmapGlyph->left;
	metrics.top = bitmapGlyph->top;
	metrics.width = bitmapGlyph->bitmap.width;
	metrics.rows = bitmapGlyph->bitmap.rows;
	metrics.advance_x = static_cast<float>(face->glyph->advance.x >> 6);
	metrics.fontIndex = fontIndex;

	// initialize texture coordinates (will be updated when rendered to atlas)
	metrics.uvPixelsX = 0;
	metrics.uvPixelsY = 0;
	metrics.sizePixelsX = bitmapGlyph->bitmap.width;
	metrics.sizePixelsY = bitmapGlyph->bitmap.rows;

	FT_Done_Glyph(glyph);
	return true;
}

bool McFont::initializeFallbackFonts()
{
	if (m_bFallbacksInitialized)
		return true;

	// check all bundled fonts first
	std::vector<UString> bundledFallbacks = env->getFilesInFolder(ResourceManager::PATH_DEFAULT_FONTS);

	for (const auto &fontName : bundledFallbacks)
	{
		if (loadFallbackFont(fontName, false))
		{
			if (cv::r_debug_font_unicode.getBool())
				debugLog("Font Info: Loaded bundled fallback font: {:s}\n", fontName.toUtf8());
		}
	}

	// then find likely system fonts
	if (cv::font_load_system.getBool())
		discoverSystemFallbacks();

	m_bFallbacksInitialized = true;
	return true;
}

void McFont::discoverSystemFallbacks()
{
#ifdef _WIN32
	wchar_t windir[MAX_PATH];
	if (GetWindowsDirectoryW(windir, MAX_PATH) <= 0)
		return;

	std::vector<UString> systemFonts = {
	    UString(windir) + "\\Fonts\\arial.ttf",
	    UString(windir) + "\\Fonts\\msyh.ttc",     // Microsoft YaHei (Chinese)
	    UString(windir) + "\\Fonts\\malgun.ttf",   // Malgun Gothic (Korean)
	    UString(windir) + "\\Fonts\\meiryo.ttc",   // Meiryo (Japanese)
	    UString(windir) + "\\Fonts\\seguiemj.ttf", // Segoe UI Emoji
	    UString(windir) + "\\Fonts\\seguisym.ttf"  // Segoe UI Symbol
	};
#elif defined(__linux__)
	// linux system fonts (common locations)
	std::vector<UString> systemFonts = {"/usr/share/fonts/TTF/dejavu/DejaVuSans.ttf",
	                                    "/usr/share/fonts/TTF/liberation/LiberationSans-Regular.ttf",
	                                    "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
	                                    "/usr/share/fonts/noto/NotoSans-Regular.ttf",
	                                    "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
	                                    "/usr/share/fonts/TTF/noto/NotoColorEmoji.ttf"};
#endif

	for (const auto &fontPath : systemFonts)
	{
		if (env->fileExists(fontPath))
		{
			loadFallbackFont(fontPath, true);
		}
	}
}

bool McFont::loadFallbackFont(const UString &fontPath, bool isSystemFont)
{
	FT_Face face{};
	if (FT_New_Face(m_ftLibrary, fontPath.toUtf8(), 0, &face))
	{
		if (cv::r_debug_font_unicode.getBool())
			debugLog("Font Warning: Failed to load fallback font: {:s}\n", fontPath.toUtf8());
		return false;
	}

	if (FT_Select_Charmap(face, ft_encoding_unicode))
	{
		if (cv::r_debug_font_unicode.getBool())
			debugLog("Font Warning: Failed to select unicode charmap for fallback font: {:s}\n", fontPath.toUtf8());
		FT_Done_Face(face);
		return false;
	}

	// set font size to match primary font
	FT_Set_Char_Size(face, m_iFontSize * 64, m_iFontSize * 64, m_iFontDPI, m_iFontDPI);

	m_fallbackFonts.push_back({fontPath, face, isSystemFont});
	return true;
}

void McFont::addFallbackFont(const UString &fontPath)
{
	if (!m_bFreeTypeInitialized)
	{
		debugLog("Font Error: Cannot add fallback font before FreeType initialization\n");
		return;
	}

	loadFallbackFont(fontPath, false);
}

void McFont::loadSystemFallbacks()
{
	if (!m_bFallbacksInitialized)
		initializeFallbackFonts();
	else
		discoverSystemFallbacks();
}

bool McFont::ensureAtlasSpace(int requiredWidth, int requiredHeight)
{
	if (!m_textureAtlas)
		return false;

	// simple heuristic: check if there's roughly enough space left
	// this is not perfect but avoids complex packing simulation
	const int atlasWidth = m_textureAtlas->getWidth();
	const int atlasHeight = m_textureAtlas->getHeight();
	const float occupiedRatio = static_cast<float>(m_vGlyphMetrics.size()) / 100.0f; // rough estimation

	return (requiredWidth < atlasWidth / 4 && requiredHeight < atlasHeight / 4 && occupiedRatio < 0.8f);
}

void McFont::rebuildAtlas()
{
	if (!m_bAtlasNeedsRebuild || m_vPendingGlyphs.empty())
		return;

	// collect all glyphs that need to be in the atlas
	std::vector<wchar_t> allGlyphs;
	allGlyphs.reserve(m_vGlyphMetrics.size());

	// add existing glyphs with non-empty size
	for (const auto &[ch, metrics] : m_vGlyphMetrics)
	{
		if (metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0)
		{
			allGlyphs.push_back(ch);
		}
	}

	// destroy old atlas
	SAFE_DELETE(m_textureAtlas);

	// create new atlas and render all glyphs
	if (!createAndPackAtlas(allGlyphs))
	{
		debugLog("Font Error: Failed to rebuild atlas!\n");
		return;
	}

	m_vPendingGlyphs.clear();
	m_bAtlasNeedsRebuild = false;
}

bool McFont::initializeFreeType()
{
	if (FT_Init_FreeType(&m_ftLibrary))
	{
		engine->showMessageError("Font Error", "FT_Init_FreeType() failed!");
		return false;
	}

	if (FT_New_Face(m_ftLibrary, m_sFilePath.toUtf8(), 0, &m_ftFace))
	{
		engine->showMessageError("Font Error", "Couldn't load font file!");
		FT_Done_FreeType(m_ftLibrary);
		return false;
	}

	if (FT_Select_Charmap(m_ftFace, ft_encoding_unicode))
	{
		engine->showMessageError("Font Error", "FT_Select_Charmap() failed!");
		FT_Done_Face(m_ftFace);
		FT_Done_FreeType(m_ftLibrary);
		return false;
	}

	// set font size (1/64th point units)
	FT_Set_Char_Size(m_ftFace, m_iFontSize * 64, m_iFontSize * 64, m_iFontDPI, m_iFontDPI);

	m_bFreeTypeInitialized = true;
	return true;
}

bool McFont::loadGlyphMetrics(wchar_t ch)
{
	if (!m_bFreeTypeInitialized)
		return false;

	int fontIndex = 0;
	FT_Face face = getFontFaceForGlyph(ch, fontIndex);

	if (!face)
		return false;

	return loadGlyphFromFace(ch, face, fontIndex);
}

std::unique_ptr<Channel[]> McFont::createExpandedBitmapData(const FT_Bitmap &bitmap)
{
	std::unique_ptr<Channel[]> expandedData(new Channel[sizeof(Color) * bitmap.width * bitmap.rows]);

	std::unique_ptr<Channel[]> monoBitmapUnpacked;
	if (!m_bAntialiasing)
		monoBitmapUnpacked.reset(unpackMonoBitmap(bitmap));

	for (unsigned int j = 0; j < bitmap.rows; j++)
	{
		for (unsigned int k = 0; k < bitmap.width; k++)
		{
			const size_t expandedIdx = (4 * k + (bitmap.rows - j - 1) * bitmap.width * 4);
			expandedData[expandedIdx] = 0xff;     // R
			expandedData[expandedIdx + 1] = 0xff; // G
			expandedData[expandedIdx + 2] = 0xff; // B

			Channel alpha = m_bAntialiasing ? bitmap.buffer[k + bitmap.width * j] : monoBitmapUnpacked[k + bitmap.width * j] > 0 ? 255 : 0;

			expandedData[expandedIdx + 3] = alpha; // A
		}
	}

	return expandedData;
}

void McFont::renderGlyphToAtlas(wchar_t ch, int x, int y, FT_Face face)
{
	if (!face)
	{
		// fall back to getting the face again if not provided
		int fontIndex = 0;
		face = getFontFaceForGlyph(ch, fontIndex);
		if (!face)
			return;
	}

	if (FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), m_bAntialiasing ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO))
		return;

	FT_Glyph glyph{};
	if (FT_Get_Glyph(face->glyph, &glyph))
		return;

	FT_Glyph_To_Bitmap(&glyph, m_bAntialiasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO, nullptr, 1);

	auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
	auto &bitmap = bitmapGlyph->bitmap;

	if (bitmap.width > 0 && bitmap.rows > 0)
	{
		auto expandedData = createExpandedBitmapData(bitmap);
		m_textureAtlas->putAt(x, y, bitmap.width, bitmap.rows, false, true, reinterpret_cast<Color *>(expandedData.get()));

		// update metrics with atlas coordinates
		GLYPH_METRICS &metrics = m_vGlyphMetrics[ch];
		metrics.uvPixelsX = static_cast<unsigned int>(x);
		metrics.uvPixelsY = static_cast<unsigned int>(y);
	}

	FT_Done_Glyph(glyph);
}

bool McFont::createAndPackAtlas(const std::vector<wchar_t> &glyphs)
{
	if (glyphs.empty())
		return true;

	// prepare packing rectangles
	std::vector<TextureAtlas::PackRect> packRects;
	std::vector<wchar_t> rectsToChars;
	packRects.reserve(glyphs.size());
	rectsToChars.reserve(glyphs.size());

	size_t rectIndex = 0;
	for (wchar_t ch : glyphs)
	{
		const auto &metrics = m_vGlyphMetrics[ch];
		if (metrics.sizePixelsX > 0 && metrics.sizePixelsY > 0)
		{
			packRects.push_back({0, 0, static_cast<int>(metrics.sizePixelsX), static_cast<int>(metrics.sizePixelsY), static_cast<int>(rectIndex)});
			rectsToChars.push_back(ch);
			rectIndex++;
		}
	}

	if (packRects.empty())
		return true;

	// calculate optimal atlas size and create texture atlas
	const int padding = cv::r_debug_font_atlas_padding.getInt();
	const size_t atlasSize = TextureAtlas::calculateOptimalSize(packRects, ATLAS_OCCUPANCY_TARGET, padding, MIN_ATLAS_SIZE, MAX_ATLAS_SIZE);

	resourceManager->requestNextLoadUnmanaged();
	m_textureAtlas = resourceManager->createTextureAtlas(atlasSize, atlasSize);
	m_textureAtlas->setPadding(padding);

	// pack glyphs into atlas
	if (!m_textureAtlas->packRects(packRects))
	{
		engine->showMessageError("Font Error", "Failed to pack glyphs into atlas!");
		return false;
	}

	// render all packed glyphs to atlas
	for (const auto &rect : packRects)
	{
		const wchar_t ch = rectsToChars[rect.id];

		// get the correct font face for this glyph
		int fontIndex = 0;
		FT_Face face = getFontFaceForGlyph(ch, fontIndex);

		renderGlyphToAtlas(ch, rect.x, rect.y, face);
	}

	// finalize atlas texture
	resourceManager->loadResource(m_textureAtlas);
	m_textureAtlas->getAtlasImage()->setFilterMode(m_bAntialiasing ? Graphics::FILTER_MODE::FILTER_MODE_LINEAR : Graphics::FILTER_MODE::FILTER_MODE_NONE);

	return true;
}

void McFont::buildGlyphGeometry(const GLYPH_METRICS &gm, const Vector3 &basePos, float advanceX, size_t &vertexCount)
{
	const float atlasWidth = static_cast<float>(m_textureAtlas->getAtlasImage()->getWidth());
	const float atlasHeight = static_cast<float>(m_textureAtlas->getAtlasImage()->getHeight());

	const float x = basePos.x + gm.left + advanceX;
	const float y = basePos.y - (gm.top - gm.rows);
	const float z = basePos.z;
	const float sx = gm.width;
	const float sy = -gm.rows;

	const float texX = static_cast<float>(gm.uvPixelsX) / atlasWidth;
	const float texY = static_cast<float>(gm.uvPixelsY) / atlasHeight;
	const float texSizeX = static_cast<float>(gm.sizePixelsX) / atlasWidth;
	const float texSizeY = static_cast<float>(gm.sizePixelsY) / atlasHeight;

	// corners of the "quad"
	Vector3 bottomLeft = Vector3(x, y + sy, z);
	Vector3 topLeft = Vector3(x, y, z);
	Vector3 topRight = Vector3(x + sx, y, z);
	Vector3 bottomRight = Vector3(x + sx, y + sy, z);

	// texcoords
	Vector2 texBottomLeft = Vector2(texX, texY);
	Vector2 texTopLeft = Vector2(texX, texY + texSizeY);
	Vector2 texTopRight = Vector2(texX + texSizeX, texY + texSizeY);
	Vector2 texBottomRight = Vector2(texX + texSizeX, texY);

	const size_t idx = vertexCount;

	if constexpr (Env::cfg(REND::GLES2 | REND::GLES32))
	{
		// first triangle (bottom-left, top-left, top-right)
		m_vertices[idx] = bottomLeft;
		m_vertices[idx + 1] = topLeft;
		m_vertices[idx + 2] = topRight;

		m_texcoords[idx] = texBottomLeft;
		m_texcoords[idx + 1] = texTopLeft;
		m_texcoords[idx + 2] = texTopRight;

		// second triangle (bottom-left, top-right, bottom-right)
		m_vertices[idx + 3] = bottomLeft;
		m_vertices[idx + 4] = topRight;
		m_vertices[idx + 5] = bottomRight;

		m_texcoords[idx + 3] = texBottomLeft;
		m_texcoords[idx + 4] = texTopRight;
		m_texcoords[idx + 5] = texBottomRight;
	}
	else
	{
		m_vertices[idx] = bottomLeft;      // bottom-left
		m_vertices[idx + 1] = topLeft;     // top-left
		m_vertices[idx + 2] = topRight;    // top-right
		m_vertices[idx + 3] = bottomRight; // bottom-right

		m_texcoords[idx] = texBottomLeft;
		m_texcoords[idx + 1] = texTopLeft;
		m_texcoords[idx + 2] = texTopRight;
		m_texcoords[idx + 3] = texBottomRight;
	}
	vertexCount += VERTS_PER_VAO;
}

void McFont::buildStringGeometry(const UString &text, size_t &vertexCount)
{
	if (!m_bReady || text.length() == 0 || text.length() > cv::r_drawstring_max_string_length.getInt())
	{
		return;
	}

	// check if atlas needs rebuilding before geometry generation
	if (m_bAtlasNeedsRebuild)
		rebuildAtlas();

	float advanceX = 0.0f;
	const size_t maxGlyphs = std::min(text.length(), (int)(m_vertices.size() - vertexCount) / VERTS_PER_VAO);

	for (size_t i = 0; i < maxGlyphs; i++)
	{
		const GLYPH_METRICS &gm = getGlyphMetrics(text[i]);
		buildGlyphGeometry(gm, Vector3(), advanceX, vertexCount);
		advanceX += gm.advance_x;
	}
}

void McFont::drawString(const UString &text)
{
	if (!m_bReady)
		return;

	const int maxNumGlyphs = cv::r_drawstring_max_string_length.getInt();
	if (text.length() == 0 || text.length() > maxNumGlyphs)
		return;

	m_vao.empty();

	const size_t totalVerts = text.length() * VERTS_PER_VAO;
	m_vertices.resize(totalVerts);
	m_texcoords.resize(totalVerts);

	size_t vertexCount = 0;
	buildStringGeometry(text, vertexCount);

	for (size_t i = 0; i < vertexCount; i++)
	{
		m_vao.addVertex(m_vertices[i]);
		m_vao.addTexcoord(m_texcoords[i]);
	}

	m_textureAtlas->getAtlasImage()->bind();
	g->drawVAO(&m_vao);

	if (cv::r_debug_drawstring_unbind.getBool())
	{
		m_textureAtlas->getAtlasImage()->unbind();
	}
}

void McFont::beginBatch()
{
	m_batchActive = true;
	m_batchQueue.totalVerts = 0;
	m_batchQueue.usedEntries = 0; // don't clear/reallocate, reuse the entries instead
}

void McFont::addToBatch(const UString &text, const Vector3 &pos, Color color)
{
	size_t verts{};
	if (!m_batchActive || (verts = text.length() * VERTS_PER_VAO) == 0)
		return;
	m_batchQueue.totalVerts += verts;

	if (m_batchQueue.usedEntries < m_batchQueue.entryList.size())
	{
		// reuse existing entry
		BatchEntry &entry = m_batchQueue.entryList[m_batchQueue.usedEntries];
		entry.text = text;
		entry.pos = pos;
		entry.color = color;
	}
	else
	{
		// need to add new entry
		m_batchQueue.entryList.push_back({text, pos, color});
	}
	m_batchQueue.usedEntries++;
}

void McFont::flushBatch()
{
	if (!m_batchActive || !m_batchQueue.totalVerts)
	{
		m_batchActive = false;
		return;
	}

	m_vertices.resize(m_batchQueue.totalVerts);
	m_texcoords.resize(m_batchQueue.totalVerts);
	m_vao.empty();

	size_t currentVertex = 0;
	for (size_t i = 0; i < m_batchQueue.usedEntries; i++)
	{
		const auto &entry = m_batchQueue.entryList[i];
		const size_t stringStart = currentVertex;
		buildStringGeometry(entry.text, currentVertex);

		for (size_t j = stringStart; j < currentVertex; j++)
		{
			m_vertices[j] += entry.pos;
			m_vao.addVertex(m_vertices[j]);
			m_vao.addTexcoord(m_texcoords[j]);
			m_vao.addColor(entry.color);
		}
	}

	m_textureAtlas->getAtlasImage()->bind();

	g->drawVAO(&m_vao);

	if (cv::r_debug_drawstring_unbind.getBool())
		m_textureAtlas->getAtlasImage()->unbind();

	m_batchActive = false;
}

float McFont::getStringWidth(const UString &text) const
{
	if (!m_bReady)
		return 1.0f;

	float width = 0.0f;
	for (int i = 0; i < text.length(); i++)
	{
		width += getGlyphMetrics(text[i]).advance_x;
	}
	return width;
}

float McFont::getStringHeight(const UString &text) const
{
	if (!m_bReady)
		return 1.0f;

	float height = 0.0f;
	for (int i = 0; i < text.length(); i++)
	{
		height = std::max(height, static_cast<float>(getGlyphMetrics(text[i]).top));
	}
	return height;
}

const McFont::GLYPH_METRICS &McFont::getGlyphMetrics(wchar_t ch) const
{
	auto it = m_vGlyphMetrics.find(ch);
	if (it != m_vGlyphMetrics.end())
		return it->second;

	// attempt dynamic loading for unicode characters
	if (const_cast<McFont *>(this)->loadGlyphDynamic(ch))
	{
		it = m_vGlyphMetrics.find(ch);
		if (it != m_vGlyphMetrics.end())
			return it->second;
	}

	// fallback to unknown character glyph
	it = m_vGlyphMetrics.find(UNKNOWN_CHAR);
	if (it != m_vGlyphMetrics.end())
		return it->second;

	debugLog("Font Error: Missing default backup glyph (UNKNOWN_CHAR)?\n");
	return m_errorGlyph;
}

void McFont::initAsync()
{
	m_bAsyncReady = true;
}

void McFont::destroy()
{
	// clean up fallback fonts
	for (auto &fallbackFont : m_fallbackFonts)
	{
		if (fallbackFont.face)
			FT_Done_Face(fallbackFont.face);
	}
	m_fallbackFonts.clear();
	m_bFallbacksInitialized = false;

	if (m_bFreeTypeInitialized)
	{
		if (m_ftFace)
		{
			FT_Done_Face(m_ftFace);
			m_ftFace = nullptr;
		}
		if (m_ftLibrary)
		{
			FT_Done_FreeType(m_ftLibrary);
			m_ftLibrary = nullptr;
		}
		m_bFreeTypeInitialized = false;
	}

	SAFE_DELETE(m_textureAtlas);
	m_vGlyphMetrics.clear();
	m_vPendingGlyphs.clear();
	m_fHeight = 1.0f;
	m_bAtlasNeedsRebuild = false;
}

bool McFont::addGlyph(wchar_t ch)
{
	if (m_vGlyphExistence.find(ch) != m_vGlyphExistence.end() || ch < 32)
		return false;

	m_vGlyphs.push_back(ch);
	m_vGlyphExistence[ch] = true;
	return true;
}

Channel *McFont::unpackMonoBitmap(const FT_Bitmap &bitmap)
{
	auto result = new Channel[bitmap.rows * bitmap.width];

	for (int y = 0; std::cmp_less(y, bitmap.rows); y++)
	{
		for (int byteIdx = 0; byteIdx < bitmap.pitch; byteIdx++)
		{
			const unsigned char byteValue = bitmap.buffer[y * bitmap.pitch + byteIdx];
			const int numBitsDone = byteIdx * 8;
			const int rowstart = y * bitmap.width + byteIdx * 8;

			const int bits = std::min(8, static_cast<int>(bitmap.width - numBitsDone));

			for (int bitIdx = 0; bitIdx < bits; bitIdx++)
			{
				result[rowstart + bitIdx] = (byteValue & (1 << (7 - bitIdx))) ? 1 : 0;
			}
		}
	}

	return result;
}
