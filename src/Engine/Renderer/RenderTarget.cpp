//================ Copyright (c) 2013, PG, All rights reserved. =================//
//
// Purpose:		offscreen rendering
//
// $NoKeywords: $rt
//===============================================================================//

#include "RenderTarget.h"
#include "Engine.h"
#include "ConVar.h"

#include "VertexArrayObject.h"
#include "ResourceManager.h"
namespace cv
{
ConVar debug_rt("debug_rt", false, FCVAR_CHEAT, "draws all rendertargets with a translucent green background");
}

RenderTarget::RenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
    : Resource(),
      m_vao1(g->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES, Graphics::USAGE_TYPE::USAGE_DYNAMIC, true)),
      m_vao2(g->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES, Graphics::USAGE_TYPE::USAGE_DYNAMIC, true)),
      m_vao3(g->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES, Graphics::USAGE_TYPE::USAGE_DYNAMIC, true)),
      m_vPos(Vector2{x, y}),
      m_vSize(width, height),
      m_multiSampleType(multiSampleType)
{
}

RenderTarget::~RenderTarget() = default;

void RenderTarget::draw(int x, int y) {
    if(!m_bReady) {
        if(cv::debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
        return;
    }

    g->setColor(m_color);

    bind();
    {
        // all draw*() functions of the RenderTarget class guarantee correctly flipped images
        // if bind() is used, no guarantee can be made about the texture orientation (assuming an anonymous
        // Renderer)
        static std::vector<Vector3> vertices(6, Vector3{0.f, 0.f, 0.f});

        // clang-format off
        std::vector<Vector3> newVertices = {
            {x, y, 0.f},
            {x, y + m_vSize.y, 0.f},
            {x + m_vSize.x, y + m_vSize.y, 0.f},
            {x + m_vSize.x, y + m_vSize.y, 0.f},
            {x + m_vSize.x, y, 0.f},
            {x, y, 0.f}
        };
        // clang-format on

        if(!m_vao1->isReady() || vertices != newVertices) {
            m_vao1->release();

            vertices = newVertices;

            m_vao1->setVertices(vertices);

            static std::vector<Vector2> texcoords(
                {Vector2{0.f, 1.f}, Vector2{0.f, 0.f}, Vector2{1.f, 0.f}, Vector2{1.f, 0.f}, Vector2{1.f, 1.f}, Vector2{0.f, 1.f}});

            m_vao1->setTexcoords(texcoords);

            m_vao1->loadAsync();
            m_vao1->load();
        }

        g->drawVAO(m_vao1.get());
    }
    unbind();
}

void RenderTarget::draw(int x, int y, int width, int height) {
    if(!m_bReady) {
        if(cv::debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
        return;
    }

    g->setColor(m_color);

    bind();
    {
        static std::vector<Vector3> vertices(6, Vector3{0.f, 0.f, 0.f});

        // clang-format off
        std::vector<Vector3> newVertices = {
            {x, y, 0.f},
            {x, y + height, 0.f},
            {x + width, y + height, 0.f},
            {x + width, y + height, 0.f},
            {x + width, y, 0.f},
            {x, y, 0.f}
        };
        // clang-format on

        if(!m_vao2->isReady() || vertices != newVertices) {
            m_vao2->release();

            vertices = newVertices;

            m_vao2->setVertices(vertices);

            static std::vector<Vector2> texcoords(
                {Vector2{0.f, 1.f}, Vector2{0.f, 0.f}, Vector2{1.f, 0.f}, Vector2{1.f, 0.f}, Vector2{1.f, 1.f}, Vector2{0.f, 1.f}});

            m_vao2->setTexcoords(texcoords);

            m_vao2->loadAsync();
            m_vao2->load();
        }

        g->drawVAO(m_vao2.get());
    }
    unbind();
}

void RenderTarget::drawRect(int x, int y, int width, int height) {
    if(!m_bReady) {
        if(cv::debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
        return;
    }

    const float texCoordWidth0 = x / m_vSize.x;
    const float texCoordWidth1 = (x + width) / m_vSize.x;
    const float texCoordHeight1 = 1.0f - y / m_vSize.y;
    const float texCoordHeight0 = 1.0f - (y + height) / m_vSize.y;

    g->setColor(m_color);

    bind();
    {
        static std::vector<Vector3> vertices(6, Vector3{0.f, 0.f, 0.f});
        static std::vector<Vector2> texcoords(6, Vector2{0.f, 0.f});

        // clang-format off
        std::vector<Vector3> newVertices = {
            {x, y, 0.f},
            {x, y + height, 0.f},
            {x + width, y + height, 0.f},
            {x + width, y + height, 0.f},
            {x + width, y, 0.f},
            {x, y, 0.f}
        };
        // clang-format on

        std::vector<Vector2> newTexcoords = {{texCoordWidth0, texCoordHeight1}, {texCoordWidth0, texCoordHeight0},
                                          {texCoordWidth1, texCoordHeight0}, {texCoordWidth1, texCoordHeight0},
                                          {texCoordWidth1, texCoordHeight1}, {texCoordWidth0, texCoordHeight1}};

        if(!m_vao3->isReady() || vertices != newVertices || texcoords != newTexcoords) {
            m_vao3->release();

            texcoords = newTexcoords;
            vertices = newVertices;

            m_vao3->setVertices(vertices);
            m_vao3->setTexcoords(texcoords);

            m_vao3->loadAsync();
            m_vao3->load();
        }

        g->drawVAO(m_vao3.get());
    }
    unbind();
}

void RenderTarget::rebuild(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
{
	m_vPos.x = x;
	m_vPos.y = y;
	m_vSize.x = width;
	m_vSize.y = height;
	m_multiSampleType = multiSampleType;

	reload();
	m_vao1->release();
	m_vao2->release();
	m_vao3->release();
}

void RenderTarget::rebuild(int x, int y, int width, int height)
{
	rebuild(x, y, width, height, m_multiSampleType);
}

void RenderTarget::rebuild(int width, int height)
{
	rebuild(m_vPos.x, m_vPos.y, width, height, m_multiSampleType);
}

void RenderTarget::rebuild(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
{
	rebuild(m_vPos.x, m_vPos.y, width, height, multiSampleType);
}
