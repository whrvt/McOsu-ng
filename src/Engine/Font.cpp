//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		freetype font wrapper
//
// $NoKeywords: $fnt
//===============================================================================//

#include "Font.h"

#include "ResourceManager.h"
#include "Engine.h"
#include "ConVar.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

#include <algorithm>
#include <utility>

// constants for atlas generation and rendering
static constexpr float ATLAS_OCCUPANCY_TARGET = 0.75f; // target atlas occupancy before resize
static constexpr size_t MIN_ATLAS_SIZE = 256;
static constexpr size_t MAX_ATLAS_SIZE = 4096;

ConVar r_drawstring_max_string_length("r_drawstring_max_string_length", 65536, FCVAR_CHEAT, "maximum number of characters per call, sanity/memory buffer limit");
ConVar r_debug_drawstring_unbind("r_debug_drawstring_unbind", false, FCVAR_NONE);
ConVar r_debug_font_atlas_padding("r_debug_font_atlas_padding", 1, FCVAR_NONE, "padding between glyphs in the atlas to prevent bleeding");

McFont::McFont(UString filepath, int fontSize, bool antialiasing, int fontDPI)
    : Resource(filepath), m_vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS)
{
    std::vector<wchar_t> characters;
    characters.reserve(224); // reserve space for basic ASCII + extended chars
    for (int i = 32; i < 255; i++)
    {
        characters.push_back(static_cast<wchar_t>(i));
    }
    constructor(characters, fontSize, antialiasing, fontDPI);
}

McFont::McFont(UString filepath, std::vector<wchar_t> characters, int fontSize, bool antialiasing, int fontDPI)
    : Resource(filepath), m_vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS)
{
    constructor(characters, fontSize, antialiasing, fontDPI);
}

void McFont::constructor(std::vector<wchar_t> characters, int fontSize, bool antialiasing, int fontDPI)
{
    m_iFontSize = fontSize;
    m_bAntialiasing = antialiasing;
    m_iFontDPI = fontDPI;
    m_textureAtlas = nullptr;
    m_fHeight = 1.0f;

    // setup error glyph
    m_errorGlyph = {
        .character=UNKNOWN_CHAR, // character
        .uvPixelsX=10,           // advance_x
        .uvPixelsY=1,            // sizepixelsx
        .sizePixelsX=1,          // sizepixelsy
        .sizePixelsY=0,          // uvpixelsx
        .left=0,                 // uvpixelsy
        .top=10,                 // top
        .width=10,               // width
        .rows=1,                 // rows
        .advance_x=0             // left
    };

    // pre-allocate space for glyphs
    m_vGlyphs.reserve(characters.size());
    for (wchar_t ch : characters)
    {
        addGlyph(ch);
    }
}

void McFont::init()
{
    debugLog("Resource Manager: Loading %s\n", m_sFilePath.toUtf8());

    FT_Library library;
    if (FT_Init_FreeType(&library))
    {
        engine->showMessageError("Font Error", "FT_Init_FreeType() failed!");
        return;
    }

    struct FTCleanup
    {
        FT_Library &lib;
        FTCleanup(FT_Library &l) : lib(l) {}
        ~FTCleanup() { FT_Done_FreeType(lib); }
    } cleanup(library);

    FT_Face face;
    if (FT_New_Face(library, m_sFilePath.toUtf8(), 0, &face))
    {
        engine->showMessageError("Font Error", "Couldn't load font file!");
        return;
    }

    struct FaceCleanup
    {
        FT_Face &f;
        FaceCleanup(FT_Face &face) : f(face) {}
        ~FaceCleanup() { FT_Done_Face(f); }
    } faceCleanup(face);

    if (FT_Select_Charmap(face, ft_encoding_unicode))
    {
        engine->showMessageError("Font Error", "FT_Select_Charmap() failed!");
        return;
    }

    // set font size (1/64th point units)
    FT_Set_Char_Size(face, m_iFontSize * 64, m_iFontSize * 64, m_iFontDPI, m_iFontDPI);

    // first pass: measure all glyphs
    std::vector<GlyphRect> glyphRects;
    glyphRects.reserve(m_vGlyphs.size());

    for (wchar_t ch : m_vGlyphs)
    {
        if (FT_Load_Glyph(face, FT_Get_Char_Index(face, ch),
                          m_bAntialiasing ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO))
        {
            debugLog("Font Error: Failed to load glyph for character %d\n", (int)ch);
            continue;
        }

        FT_Glyph glyph;
        if (FT_Get_Glyph(face->glyph, &glyph))
        {
            debugLog("Font Error: Failed to get glyph for character %d\n", (int)ch);
            continue;
        }

        FT_Glyph_To_Bitmap(&glyph,
                           m_bAntialiasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO,
                           nullptr, 1);

        auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);

        // even if the glyph has no bitmap (e.g. space), we still need its metrics
        if (bitmapGlyph->bitmap.width > 0 && bitmapGlyph->bitmap.rows > 0)
        {
            glyphRects.push_back({0, 0, // x, y will be set during packing
                                  static_cast<int>(bitmapGlyph->bitmap.width),
                                  static_cast<int>(bitmapGlyph->bitmap.rows),
                                  ch});
        }

        // store initial metrics for every character, including spaces
        GLYPH_METRICS &metrics = m_vGlyphMetrics[ch];
        metrics.character = ch;
        metrics.left = bitmapGlyph->left;
        metrics.top = bitmapGlyph->top;
        metrics.width = bitmapGlyph->bitmap.width;
        metrics.rows = bitmapGlyph->bitmap.rows;
        metrics.advance_x = static_cast<float>(face->glyph->advance.x >> 6);

        // initialize texture coordinates (will be updated for non-empty glyphs)
        metrics.uvPixelsX = 0;
        metrics.uvPixelsY = 0;
        metrics.sizePixelsX = bitmapGlyph->bitmap.width;
        metrics.sizePixelsY = bitmapGlyph->bitmap.rows;

        FT_Done_Glyph(glyph);
    }

    // calculate optimal atlas size and create texture atlas
    const size_t atlasSize = calculateOptimalAtlasSize(glyphRects, ATLAS_OCCUPANCY_TARGET);
    engine->getResourceManager()->requestNextLoadUnmanaged();
    m_textureAtlas = engine->getResourceManager()->createTextureAtlas(atlasSize, atlasSize);

    // pack glyphs into atlas
    if (!glyphRects.empty() && !packGlyphRects(glyphRects, atlasSize, atlasSize))
    {
        engine->showMessageError("Font Error", "Failed to pack glyphs into atlas!");
        return;
    }

    // second pass: render packed glyphs to atlas
    for (const auto &rect : glyphRects)
    {
        if (FT_Load_Glyph(face, FT_Get_Char_Index(face, rect.ch),
                          m_bAntialiasing ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO))
        {
            continue;
        }

        FT_Glyph glyph;
        if (FT_Get_Glyph(face->glyph, &glyph))
        {
            continue;
        }

        FT_Glyph_To_Bitmap(&glyph,
                           m_bAntialiasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO,
                           nullptr, 1);

        auto bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyph);
        auto &bitmap = bitmapGlyph->bitmap;

        // only non-empty glyphs are in glyphRects, so we can safely render
        std::unique_ptr<unsigned char[]> expandedData(
            new unsigned char[4 * bitmap.width * bitmap.rows]);

        std::unique_ptr<unsigned char[]> monoBitmapUnpacked;
        if (!m_bAntialiasing)
        {
            monoBitmapUnpacked.reset(unpackMonoBitmap(bitmap));
        }

        // expand bitmap with proper alpha channel
        for (unsigned int j = 0; j < bitmap.rows; j++)
        {
            for (unsigned int i = 0; i < bitmap.width; i++)
            {
                const size_t expandedIdx = (4 * i + (bitmap.rows - j - 1) * bitmap.width * 4);
                expandedData[expandedIdx] = 0xff;     // R
                expandedData[expandedIdx + 1] = 0xff; // G
                expandedData[expandedIdx + 2] = 0xff; // B

                unsigned char alpha = m_bAntialiasing ? bitmap.buffer[i + bitmap.width * j] : monoBitmapUnpacked[i + bitmap.width * j] > 0 ? 255
                                                                                                                                           : 0;

                expandedData[expandedIdx + 3] = alpha; // A
            }
        }

        // add to atlas and update metrics with actual position
        Vector2 atlasPos = m_textureAtlas->put(bitmap.width, bitmap.rows,
                                               false, true,
                                               reinterpret_cast<Color *>(expandedData.get()));

        GLYPH_METRICS &metrics = m_vGlyphMetrics[rect.ch];
        metrics.uvPixelsX = static_cast<unsigned int>(atlasPos.x);
        metrics.uvPixelsY = static_cast<unsigned int>(atlasPos.y);

        FT_Done_Glyph(glyph);
    }

    // finalize atlas texture
    engine->getResourceManager()->loadResource(m_textureAtlas);
    m_textureAtlas->getAtlasImage()->setFilterMode(
        m_bAntialiasing ? Graphics::FILTER_MODE::FILTER_MODE_LINEAR
                        : Graphics::FILTER_MODE::FILTER_MODE_NONE);

    // precalculate average/max ASCII glyph height
    m_fHeight = 0.0f;
    for (int i = 0; i < 128; i++)
    {
        const int curHeight = getGlyphMetrics(static_cast<wchar_t>(i)).top;
        m_fHeight = std::max(m_fHeight, static_cast<float>(curHeight));
    }

    m_bReady = true;
}

void McFont::buildGlyphGeometry(const GLYPH_METRICS &gm, const Vector3 &basePos,
                                float advanceX, size_t &vertexCount)
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

    const size_t idx = vertexCount;
    m_vertices[idx] = Vector3(x, y + sy, z);          // bottom-left
    m_vertices[idx + 1] = Vector3(x, y, z);           // top-left
    m_vertices[idx + 2] = Vector3(x + sx, y, z);      // top-right
    m_vertices[idx + 3] = Vector3(x + sx, y + sy, z); // bottom-right

    m_texcoords[idx] = Vector2(texX, texY);
    m_texcoords[idx + 1] = Vector2(texX, texY + texSizeY);
    m_texcoords[idx + 2] = Vector2(texX + texSizeX, texY + texSizeY);
    m_texcoords[idx + 3] = Vector2(texX + texSizeX, texY);

    vertexCount += 4;
}

void McFont::buildStringGeometry(const UString &text, size_t &vertexCount)
{
    if (!m_bReady || text.length() == 0 ||
        text.length() > r_drawstring_max_string_length.getInt())
    {
        return;
    }

    float advanceX = 0.0f;
    const size_t maxGlyphs = std::min(text.length(),
                                      (int)(m_vertices.size() - vertexCount) / 4);

    for (size_t i = 0; i < maxGlyphs; i++)
    {
        const GLYPH_METRICS &gm = getGlyphMetrics(text[i]);
        buildGlyphGeometry(gm, Vector3(), advanceX, vertexCount);
        advanceX += gm.advance_x;
    }
}

void McFont::drawString(Graphics *g, const UString &text)
{
    if (!m_bReady)
        return;

    const int maxNumGlyphs = r_drawstring_max_string_length.getInt();
    if (text.length() == 0 || text.length() > maxNumGlyphs)
        return;

    m_vao.empty();

    const size_t totalVerts = text.length() * 4;
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

    if (r_debug_drawstring_unbind.getBool())
    {
        m_textureAtlas->getAtlasImage()->unbind();
    }
}

void McFont::beginBatch()
{
    m_batchActive = true;
    m_batchQueue.clear();
}

void McFont::addToBatch(const UString &text, const Vector3 &pos, Color color)
{
    if (!m_batchActive || text.length() == 0)
        return;
    m_batchQueue.push_back({text, pos, color});
}

void McFont::flushBatch(Graphics *g)
{
    if (!m_batchActive || m_batchQueue.empty())
    {
        m_batchActive = false;
        return;
    }

    size_t totalVerts = 0;
    for (const auto &entry : m_batchQueue)
    {
        totalVerts += entry.text.length() * 4; // 4 vertices per glyph
    }

    m_vertices.resize(totalVerts);
    m_texcoords.resize(totalVerts);
    m_vao.empty();

    size_t currentVertex = 0;
    for (const auto &entry : m_batchQueue)
    {
        const size_t stringStart = currentVertex;
        buildStringGeometry(entry.text, currentVertex);

        for (size_t i = stringStart; i < currentVertex; i++)
        {
            m_vertices[i] += entry.pos;
            m_vao.addVertex(m_vertices[i]);
            m_vao.addTexcoord(m_texcoords[i]);
            m_vao.addColor(entry.color);
        }
    }

    m_textureAtlas->getAtlasImage()->setFilterMode(
        m_bAntialiasing ? Graphics::FILTER_MODE::FILTER_MODE_LINEAR
                        : Graphics::FILTER_MODE::FILTER_MODE_NONE);

    m_textureAtlas->getAtlasImage()->bind();

    g->setAntialiasing(true);
    g->drawVAO(&m_vao);
    g->setAntialiasing(false);

    if (r_debug_drawstring_unbind.getBool())
    {
        m_textureAtlas->getAtlasImage()->unbind();
    }

    m_batchActive = false;
}

float McFont::getStringWidth(UString text) const
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

float McFont::getStringHeight(UString text) const
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
    {
        return it->second;
    }

    it = m_vGlyphMetrics.find(UNKNOWN_CHAR);
    if (it != m_vGlyphMetrics.end())
    {
        return it->second;
    }

    debugLog("Font Error: Missing default backup glyph (UNKNOWN_CHAR)?\n");
    return m_errorGlyph;
}

void McFont::initAsync()
{
    m_bAsyncReady = true;
}

void McFont::destroy()
{
    SAFE_DELETE(m_textureAtlas);
    m_vGlyphMetrics.clear();
    m_fHeight = 1.0f;
}

// debug
void McFont::drawTextureAtlas(Graphics *g)
{
    if (!m_bReady || !m_textureAtlas)
        return;

    g->pushTransform();
    g->translate(m_textureAtlas->getWidth() / 2 + 50, m_textureAtlas->getHeight() / 2 + 50);
    g->drawImage(m_textureAtlas->getAtlasImage());
    g->popTransform();
}

bool McFont::addGlyph(wchar_t ch)
{
    if (m_vGlyphExistence.find(ch) != m_vGlyphExistence.end() || ch < 32)
    {
        return false;
    }

    m_vGlyphs.push_back(ch);
    m_vGlyphExistence[ch] = true;
    return true;
}

// helper implementation for glyph packing (skyline algorithm)
bool McFont::packGlyphRects(std::vector<GlyphRect> &rects, int atlasWidth, int atlasHeight)
{
    const int padding = r_debug_font_atlas_padding.getInt();

    // sort rectangles by height
    std::ranges::sort(rects,
              [](const GlyphRect &a, const GlyphRect &b)
              {
                  return a.height > b.height;
              });

    struct Skyline
    {
        int x, y, width;
    };
    std::vector<Skyline> skylines = {{0, 0, atlasWidth}};

    for (auto &rect : rects)
    {
        const int rectWidth = rect.width + padding;
        const int rectHeight = rect.height + padding;

        int bestHeight = atlasHeight;
        int bestWidth = atlasWidth;
        int bestIndex = -1;

        // find best position along skyline
        for (size_t i = 0; i < skylines.size(); ++i)
        {
            int y = 0;
            int width = rectWidth;

            // check if rectangle fits at this position
            if (skylines[i].x + rectWidth > atlasWidth)
            {
                continue;
            }

            // find maximum height at this position
            for (size_t j = i; j < skylines.size(); ++j)
            {
                if (skylines[j].x >= skylines[i].x + rectWidth)
                {
                    break;
                }
                y = std::max(y, skylines[j].y);
                width = std::min(width, skylines[j].width);
            }

            // select this position if it gives us the minimum height
            if (y + rectHeight < bestHeight ||
                (y + rectHeight == bestHeight && width < bestWidth))
            {
                bestHeight = y + rectHeight;
                bestIndex = i;
            }
        }

        if (bestIndex == -1 || bestHeight > atlasHeight)
        {
            return false; // packing failed
        }

        // place the rectangle
        rect.x = skylines[bestIndex].x;
        rect.y = bestHeight - rectHeight;

        // update skyline
        Skyline newSkyline{.x=rect.x, .y=rect.y + rectHeight, .width=rectWidth};
        skylines.insert(skylines.begin() + bestIndex, newSkyline);

        // merge skylines if possible
        for (size_t i = 0; i < skylines.size() - 1; ++i)
        {
            if (skylines[i].y == skylines[i + 1].y)
            {
                skylines[i].width += skylines[i + 1].width;
                skylines.erase(skylines.begin() + i + 1);
                --i;
            }
        }
    }

    return true;
}

size_t McFont::calculateOptimalAtlasSize(const std::vector<GlyphRect> &glyphs, float targetOccupancy) const
{
    // calculate total glyph area
    size_t totalArea = 0;
    for (const auto &glyph : glyphs)
    {
        totalArea += (glyph.width + r_debug_font_atlas_padding.getInt()) *
                     (glyph.height + r_debug_font_atlas_padding.getInt());
    }

    // add 20% for packing inefficiency
    totalArea = static_cast<size_t>(totalArea * 1.2f);

    // find smallest power of 2 that can fit the glyphs with desired occupancy
    size_t size = MIN_ATLAS_SIZE;
    while (size * size * targetOccupancy < totalArea && size < MAX_ATLAS_SIZE)
    {
        size *= 2;
    }

    return size;
}

unsigned char *McFont::unpackMonoBitmap(const FT_Bitmap &bitmap)
{
    auto result = new unsigned char[bitmap.rows * bitmap.width];

    for (int y = 0; std::cmp_less(y , bitmap.rows); y++)
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
