//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		raw opengl es 3.2 graphics interface
//
// $NoKeywords: $gles32i
//===============================================================================//

#include "OpenGLES32Interface.h"

#ifdef MCENGINE_FEATURE_GLES32

#include "Camera.h"
#include "ConVar.h"
#include "Engine.h"

#include "Font.h"
#include "OpenGLES32Shader.h"
#include "OpenGLES32VertexArrayObject.h"
#include "OpenGLImage.h"
#include "OpenGLRenderTarget.h"

#include "OpenGLHeaders.h"

#define GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#define GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049

#define VBO_FREE_MEMORY_ATI 0x87FB
#define TEXTURE_FREE_MEMORY_ATI 0x87FC
#define RENDERBUFFER_FREE_MEMORY_ATI 0x87FD

OpenGLES32Interface::OpenGLES32Interface() : NullGraphicsInterface()
{
	// renderer
	m_bInScene = false;
	m_vResolution = engine->getScreenSize(); // initial viewport size = window size

	m_shaderTexturedGeneric = NULL;
	m_iShaderTexturedGenericPrevType = 0;
	m_iShaderTexturedGenericAttribPosition = 0;
	m_iShaderTexturedGenericAttribUV = 1;
	m_iShaderTexturedGenericAttribCol = 2;
	m_iVBOVertices = 0;
	m_iVBOTexcoords = 0;
	m_iVBOTexcolors = 0;

	// persistent vars
	m_color = 0xffffffff;
	m_bAntiAliasing = true;

	m_syncobj = new OpenGLSync();
}

OpenGLES32Interface::~OpenGLES32Interface()
{
	SAFE_DELETE(m_shaderTexturedGeneric);

	if (m_iVBOVertices != 0)
		glDeleteBuffers(1, &m_iVBOVertices);
	if (m_iVBOTexcoords != 0)
		glDeleteBuffers(1, &m_iVBOTexcoords);
	if (m_iVBOTexcolors != 0)
		glDeleteBuffers(1, &m_iVBOTexcolors);

	SAFE_DELETE(m_syncobj);
}


void OpenGLES32Interface::init()
{
	// check GL version
	const GLubyte *version = glGetString(GL_VERSION);
	debugLog("OpenGLES: OpenGL Version %s\n", version);

	// enable
	glEnable(GL_BLEND);

	// disable
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	// blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	// culling
	glFrontFace(GL_CCW);

	// setWireframe(true);

	// TODO: move these out to a .mcshader (or something) and load like the other OpenGL interface
	constexpr auto texturedGenericV = R"(
#version 320 es

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 vcolor;
layout(location = 3) in vec3 normal;

out vec2 texcoords;
out vec4 texcolor;
out vec3 texnormal;

uniform float type;  // 0=no texture, 1=texture, 2=texture+color, 3=texture+normal
uniform mat4 mvp;
uniform mat3 normalMatrix;

void main() {
    texcoords = uv;
    texcolor = vcolor;
    texnormal = normal;

    // apply normal matrix if we're using normals
    if (type >= 3.0) {
        texnormal = normalMatrix * normal;
    }

    gl_Position = mvp * vec4(position, 1.0);
}
)";

	constexpr auto texturedGenericP = R"(
#version 320 es
precision highp float;

in vec2 texcoords;
in vec4 texcolor;
in vec3 texnormal;

uniform float type;  // 0=no texture, 1=texture, 2=texture+color, 3=texture+normal
uniform vec4 col;
uniform sampler2D tex;
uniform vec3 lightDir;  // normalized direction to light source

out vec4 fragColor;

void main() {
    // base color calculation
    vec4 baseColor;

    if (type < 0.5) { // no texture
        baseColor = col;
    } else if (type < 1.5) { // texture with uniform color
        baseColor = texture(tex, texcoords) * col;
    } else if (type < 2.5) { // vertex color only
        baseColor = texcolor;
    } else if (type < 3.5) { // normal mapping
        baseColor = mix(col, mix(texture(tex, texcoords) * col, texcolor, clamp(type - 1.0, 0.0, 1.0)), clamp(type, 0.0, 1.0));

        // lighting effects for normal mapping
        if (length(texnormal) > 0.01) {
            vec3 N = normalize(texnormal);
            float diffuse = max(dot(N, lightDir), 0.3); // ambient component
            baseColor.rgb *= diffuse;
        }
    } else {
        // texture with vertex color (for text rendering)
        baseColor = texture(tex, texcoords) * texcolor;
    }

    fragColor = baseColor;
}
)";
	m_shaderTexturedGeneric = (OpenGLES32Shader *)createShaderFromSource(texturedGenericV, texturedGenericP);
	m_shaderTexturedGeneric->load();

	glGenBuffers(1, &m_iVBOVertices);
	glGenBuffers(1, &m_iVBOTexcoords);
	glGenBuffers(1, &m_iVBOTexcolors);

	m_iShaderTexturedGenericAttribPosition = m_shaderTexturedGeneric->getAttribLocation("position");
	m_iShaderTexturedGenericAttribUV = m_shaderTexturedGeneric->getAttribLocation("uv");
	m_iShaderTexturedGenericAttribCol = m_shaderTexturedGeneric->getAttribLocation("vcolor");

	// TODO: handle cases where more than 16384 elements are in an unbaked vao
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBOVertices);
	glVertexAttribPointer(m_iShaderTexturedGenericAttribPosition, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glBufferData(GL_ARRAY_BUFFER, 16384 * sizeof(Vector3), NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(m_iShaderTexturedGenericAttribPosition);

	glBindBuffer(GL_ARRAY_BUFFER, m_iVBOTexcoords);
	glVertexAttribPointer(m_iShaderTexturedGenericAttribUV, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);
	glBufferData(GL_ARRAY_BUFFER, 16384 * sizeof(Vector2), NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(m_iShaderTexturedGenericAttribUV);

	glBindBuffer(GL_ARRAY_BUFFER, m_iVBOTexcolors);
	glVertexAttribPointer(m_iShaderTexturedGenericAttribCol, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(Color), (GLvoid *)0);
	glBufferData(GL_ARRAY_BUFFER, 16384 * sizeof(Vector4), NULL, GL_STREAM_DRAW);
	glEnableVertexAttribArray(m_iShaderTexturedGenericAttribCol);
}

void OpenGLES32Interface::beginScene()
{
	m_bInScene = true;

	m_syncobj->begin();

	// enable default shader (must happen before any uniform calls)
	m_shaderTexturedGeneric->enable();

	Matrix4 defaultProjectionMatrix = Camera::buildMatrixOrtho2D(0, m_vResolution.x, m_vResolution.y, 0, -1.0f, 1.0f);

	// push main transforms
	pushTransform();
	setProjectionMatrix(defaultProjectionMatrix);
	translate(r_globaloffset_x->getFloat(), r_globaloffset_y->getFloat());

	// and apply them
	updateTransform();

	// set clear color and clear
	// glClearColor(1, 1, 1, 1);
	// glClearColor(0.9568f, 0.9686f, 0.9882f, 1);
	glClearColor(0, 0, 0, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// display any errors of previous frames
	handleGLErrors();
}

void OpenGLES32Interface::endScene()
{
	popTransform();

	checkStackLeaks();

	if (m_clipRectStack.size() > 0)
	{
		engine->showMessageErrorFatal("ClipRect Stack Leak", "Make sure all push*() have a pop*()!");
		engine->shutdown();
	}

	m_syncobj->end();
	m_bInScene = false;
}

void OpenGLES32Interface::clearDepthBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void OpenGLES32Interface::setColor(Color color)
{
	if (color == m_color)
		return;

	if (m_shaderTexturedGeneric->isActive())
	{
		m_color = color;
		m_shaderTexturedGeneric->setUniform4f("col", ((unsigned char)(m_color >> 16)) / 255.0f, ((unsigned char)(m_color >> 8)) / 255.0f, ((unsigned char)(m_color >> 0)) / 255.0f, ((unsigned char)(m_color >> 24)) / 255.0f);
	}
}

void OpenGLES32Interface::setAlpha(float alpha)
{
	Color tempColor = m_color;

	tempColor &= 0x00ffffff;
	tempColor |= ((int)(255.0f * alpha)) << 24;

	setColor(tempColor);
}

void OpenGLES32Interface::drawLine(int x1, int y1, int x2, int y2)
{
	updateTransform();

	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_LINES);
	vao.addVertex(x1 + 0.5f, y1 + 0.5f);
	vao.addVertex(x2 + 0.5f, y2 + 0.5f);
	drawVAO(&vao);
}

void OpenGLES32Interface::drawLine(Vector2 pos1, Vector2 pos2)
{
	drawLine(pos1.x, pos1.y, pos2.x, pos2.y);
}

void OpenGLES32Interface::drawRect(int x, int y, int width, int height)
{
	drawLine(x, y, x + width, y);
	drawLine(x, y, x, y + height);
	drawLine(x, y + height, x + width + 1, y + height);
	drawLine(x + width, y, x + width, y + height);
}

void OpenGLES32Interface::drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left)
{
	setColor(top);
	drawLine(x, y, x + width, y);
	setColor(left);
	drawLine(x, y, x, y + height);
	setColor(bottom);
	drawLine(x, y + height, x + width + 1, y + height);
	setColor(right);
	drawLine(x + width, y, x + width, y + height);
}

void OpenGLES32Interface::fillRect(int x, int y, int width, int height)
{
	updateTransform();

	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP);
	vao.addVertex(x, y);
	vao.addVertex(x, y + height);
	vao.addVertex(x + width, y);
	vao.addVertex(x + width, y + height);
	drawVAO(&vao);
}

void OpenGLES32Interface::fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor, Color bottomLeftColor, Color bottomRightColor)
{
	updateTransform();

	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP);
	vao.addVertex(x, y);
	vao.addColor(topLeftColor);
	vao.addVertex(x, y + height);
	vao.addColor(bottomLeftColor);
	vao.addVertex(x + width, y);
	vao.addColor(topRightColor);
	vao.addVertex(x + width, y + height);
	vao.addColor(bottomRightColor);
	drawVAO(&vao);
}

void OpenGLES32Interface::drawQuad(int x, int y, int width, int height)
{
	updateTransform();

	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP);
	vao.addVertex(x, y);
	vao.addTexcoord(0, 0);
	vao.addVertex(x, y + height);
	vao.addTexcoord(0, 1);
	vao.addVertex(x + width, y);
	vao.addTexcoord(1, 0);
	vao.addVertex(x + width, y + height);
	vao.addTexcoord(1, 1);
	drawVAO(&vao);
}

void OpenGLES32Interface::drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft, Color topLeftColor, Color topRightColor, Color bottomRightColor, Color bottomLeftColor)
{
	updateTransform();

	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP);
	vao.addVertex(topLeft.x, topLeft.y);
	vao.addColor(topLeftColor);
	vao.addTexcoord(0, 0);
	vao.addVertex(bottomLeft.x, bottomLeft.y);
	vao.addColor(bottomLeftColor);
	vao.addTexcoord(0, 1);
	vao.addVertex(topRight.x, topRight.y);
	vao.addColor(topRightColor);
	vao.addTexcoord(1, 0);
	vao.addVertex(bottomRight.x, bottomRight.y);
	vao.addColor(bottomRightColor);
	vao.addTexcoord(1, 1);
	drawVAO(&vao);
}

void OpenGLES32Interface::drawImage(Image *image)
{
	if (image == NULL)
	{
		debugLog("WARNING: Tried to draw image with NULL texture!\n");
		return;
	}
	if (!image->isReady())
		return;

	updateTransform();

	float width = image->getWidth();
	float height = image->getHeight();

	float x = -width / 2;
	float y = -height / 2;

	VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP);
	vao.addVertex(x, y);
	vao.addTexcoord(0, 0);
	vao.addVertex(x, y + height);
	vao.addTexcoord(0, 1);
	vao.addVertex(x + width, y);
	vao.addTexcoord(1, 0);
	vao.addVertex(x + width, y + height);
	vao.addTexcoord(1, 1);

	image->bind();
	drawVAO(&vao);
	image->unbind();

	if (r_debug_drawimage->getBool())
	{
		setColor(0xbbff00ff);
		drawRect(x, y, width, height);
	}
}

void OpenGLES32Interface::drawString(McFont *font, UString text)
{
	if (font == NULL || text.length() < 1 || !font->isReady())
		return;

	updateTransform();

	font->drawString(this, text);
}

void OpenGLES32Interface::drawVAO(VertexArrayObject *vao)
{
	if (vao == NULL)
		return;

	updateTransform();

	// if baked, then we can directly draw the buffer
	if (vao->isReady())
	{
		OpenGLES32VertexArrayObject *glvao = (OpenGLES32VertexArrayObject *)vao;

		// configure shader
		if (m_shaderTexturedGeneric->isActive())
		{
			// both texcoords and colors (e.g., text rendering)
			if (glvao->getNumTexcoords0() > 0 && glvao->getNumColors() > 0)
			{
				if (m_iShaderTexturedGenericPrevType != 4)
				{
					m_shaderTexturedGeneric->setUniform1f("type", 4.0f);
					m_iShaderTexturedGenericPrevType = 4;
				}
			}
			// texcoords
			else if (glvao->getNumTexcoords0() > 0)
			{
				if (m_iShaderTexturedGenericPrevType != 1)
				{
					m_shaderTexturedGeneric->setUniform1f("type", 1.0f);
					m_iShaderTexturedGenericPrevType = 1;
				}
			}
			// colors
			else if (glvao->getNumColors() > 0)
			{
				if (m_iShaderTexturedGenericPrevType != 2)
				{
					m_shaderTexturedGeneric->setUniform1f("type", 2.0f);
					m_iShaderTexturedGenericPrevType = 2;
				}
			}
			// neither
			else if (m_iShaderTexturedGenericPrevType != 0)
			{
				m_shaderTexturedGeneric->setUniform1f("type", 0.0f);
				m_iShaderTexturedGenericPrevType = 0;
			}
		}

		// draw
		glvao->draw();
		return;
	}

	const std::vector<Vector3> &vertices = vao->getVertices();
	const std::vector<Vector3> &normals = vao->getNormals();
	const std::vector<std::vector<Vector2>> &texcoords = vao->getTexcoords();
	const std::vector<Color> &vcolors = vao->getColors();

	if (vertices.size() < 2)
		return;

	// TODO: separate draw for non-quads, update quad draws to triangle draws to avoid rewrite overhead here

	// no support for quads, because fuck you
	// rewrite all quads into triangles
	std::vector<Vector3> finalVertices = vertices;
	std::vector<std::vector<Vector2>> finalTexcoords = texcoords;
	std::vector<Color> colors;
	std::vector<Color> finalColors;

	for (size_t i = 0; i < vcolors.size(); i++)
	{
		Color color = OpenGLES32VertexArrayObject::ARGBtoABGR(vcolors[i]);
		colors.push_back(color);
		finalColors.push_back(color);
	}
	int maxColorIndex = colors.size() - 1;

	Graphics::PRIMITIVE primitive = vao->getPrimitive();
	if (primitive == Graphics::PRIMITIVE::PRIMITIVE_QUADS)
	{
		finalVertices.clear();
		for (size_t t = 0; t < finalTexcoords.size(); t++)
		{
			finalTexcoords[t].clear();
		}
		finalColors.clear();
		primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES;

		if (vertices.size() > 3)
		{
			for (size_t i = 0; i < vertices.size(); i += 4)
			{
				finalVertices.push_back(vertices[i + 0]);
				finalVertices.push_back(vertices[i + 1]);
				finalVertices.push_back(vertices[i + 2]);

				for (size_t t = 0; t < texcoords.size(); t++)
				{
					finalTexcoords[t].push_back(texcoords[t][i + 0]);
					finalTexcoords[t].push_back(texcoords[t][i + 1]);
					finalTexcoords[t].push_back(texcoords[t][i + 2]);
				}

				if (colors.size() > 0)
				{
					finalColors.push_back(colors[clamp<int>(i + 0, 0, maxColorIndex)]);
					finalColors.push_back(colors[clamp<int>(i + 1, 0, maxColorIndex)]);
					finalColors.push_back(colors[clamp<int>(i + 2, 0, maxColorIndex)]);
				}

				finalVertices.push_back(vertices[i + 0]);
				finalVertices.push_back(vertices[i + 2]);
				finalVertices.push_back(vertices[i + 3]);

				for (size_t t = 0; t < texcoords.size(); t++)
				{
					finalTexcoords[t].push_back(texcoords[t][i + 0]);
					finalTexcoords[t].push_back(texcoords[t][i + 2]);
					finalTexcoords[t].push_back(texcoords[t][i + 3]);
				}

				if (colors.size() > 0)
				{
					finalColors.push_back(colors[clamp<int>(i + 0, 0, maxColorIndex)]);
					finalColors.push_back(colors[clamp<int>(i + 2, 0, maxColorIndex)]);
					finalColors.push_back(colors[clamp<int>(i + 3, 0, maxColorIndex)]);
				}
			}
		}
	}

	// upload vertices to gpu
	if (finalVertices.size() > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_iVBOVertices);
		glBufferSubData(GL_ARRAY_BUFFER, 0, finalVertices.size() * sizeof(Vector3), &(finalVertices[0]));
	}

	// upload texcoords to gpu
	if (finalTexcoords.size() > 0 && finalTexcoords[0].size() > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_iVBOTexcoords);
		glBufferSubData(GL_ARRAY_BUFFER, 0, finalTexcoords[0].size() * sizeof(Vector2), &(finalTexcoords[0][0]));
	}

	// upload vertex colors to gpu
	if (finalColors.size() > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_iVBOTexcolors);
		glBufferSubData(GL_ARRAY_BUFFER, 0, finalColors.size() * sizeof(Color), &(finalColors[0]));
	}

	// configure shader
	if (m_shaderTexturedGeneric->isActive())
	{
		// texcoords and colors (e.g., text rendering)
		if (finalTexcoords.size() > 0 && finalTexcoords[0].size() > 0 && finalColors.size() > 0)
		{
			if (m_iShaderTexturedGenericPrevType != 4)
			{
				m_shaderTexturedGeneric->setUniform1f("type", 4.0f);
				m_iShaderTexturedGenericPrevType = 4;
			}
		}
		// texcoords
		else if (finalTexcoords.size() > 0 && finalTexcoords[0].size() > 0)
		{
			if (m_iShaderTexturedGenericPrevType != 1)
			{
				m_shaderTexturedGeneric->setUniform1f("type", 1.0f);
				m_iShaderTexturedGenericPrevType = 1;
			}
		}
		// colors
		else if (finalColors.size() > 0)
		{
			if (m_iShaderTexturedGenericPrevType != 2)
			{
				m_shaderTexturedGeneric->setUniform1f("type", 2.0f);
				m_iShaderTexturedGenericPrevType = 2;
			}
		}
		// neither
		else if (m_iShaderTexturedGenericPrevType != 0)
		{
			m_shaderTexturedGeneric->setUniform1f("type", 0.0f);
			m_iShaderTexturedGenericPrevType = 0;
		}
	}

	// draw it
	glDrawArrays(primitiveToOpenGL(primitive), 0, finalVertices.size());
}

void OpenGLES32Interface::setClipRect(McRect clipRect)
{
	if (r_debug_disable_cliprect->getBool())
		return;
	// if (m_bIs3DScene) return; // HACKHACK:TODO:

	// HACKHACK: compensate for viewport changes caused by RenderTargets!
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// debugLog("viewport = %i, %i, %i, %i\n", viewport[0], viewport[1], viewport[2], viewport[3]);

	glEnable(GL_SCISSOR_TEST);
	glScissor((int)clipRect.getX() + viewport[0], viewport[3] - ((int)clipRect.getY() - viewport[1] - 1 + (int)clipRect.getHeight()), (int)clipRect.getWidth(), (int)clipRect.getHeight());

	// debugLog("scissor = %i, %i, %i, %i\n", (int)clipRect.getX()+viewport[0], viewport[3]-((int)clipRect.getY()-viewport[1]-1+(int)clipRect.getHeight()), (int)clipRect.getWidth(), (int)clipRect.getHeight());
}

void OpenGLES32Interface::pushClipRect(McRect clipRect)
{
	if (m_clipRectStack.size() > 0)
		m_clipRectStack.push(m_clipRectStack.top().intersect(clipRect));
	else
		m_clipRectStack.push(clipRect);

	setClipRect(m_clipRectStack.top());
}

void OpenGLES32Interface::popClipRect()
{
	m_clipRectStack.pop();

	if (m_clipRectStack.size() > 0)
		setClipRect(m_clipRectStack.top());
	else
		setClipping(false);
}

void OpenGLES32Interface::pushStencil()
{
	// init and clear
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);

	// set mask
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
}

void OpenGLES32Interface::fillStencil(bool inside)
{
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilFunc(GL_NOTEQUAL, inside ? 0 : 1, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void OpenGLES32Interface::popStencil()
{
	glDisable(GL_STENCIL_TEST);
}

void OpenGLES32Interface::setClipping(bool enabled)
{
	if (enabled)
	{
		if (m_clipRectStack.size() > 0)
			glEnable(GL_SCISSOR_TEST);
	}
	else
		glDisable(GL_SCISSOR_TEST);
}

void OpenGLES32Interface::setAlphaTesting(bool enabled)
{
	if (enabled)
		glEnable(GL_ALPHA_TEST);
	else
		glDisable(GL_ALPHA_TEST);
}

void OpenGLES32Interface::setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref)
{
	glAlphaFunc(compareFuncToOpenGL(alphaFunc), ref);
}

void OpenGLES32Interface::setBlending(bool enabled)
{
	if (enabled)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
}

void OpenGLES32Interface::setBlendMode(BLEND_MODE blendMode)
{
	switch (blendMode)
	{
	case BLEND_MODE::BLEND_MODE_ALPHA:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_MODE::BLEND_MODE_ADDITIVE:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BLEND_MODE::BLEND_MODE_PREMUL_ALPHA:
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_MODE::BLEND_MODE_PREMUL_COLOR:
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}
}

void OpenGLES32Interface::setDepthBuffer(bool enabled)
{
	if (enabled)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

void OpenGLES32Interface::setCulling(bool culling)
{
	if (culling)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
}

void OpenGLES32Interface::setAntialiasing(bool aa)
{
	m_bAntiAliasing = aa;
	if (aa)
		glEnable(GL_MULTISAMPLE);
	else
		glDisable(GL_MULTISAMPLE);
}


void OpenGLES32Interface::setWireframe(bool enabled)
{
	// TODO
}

void OpenGLES32Interface::flush()
{
	glFlush();
}

std::vector<unsigned char> OpenGLES32Interface::getScreenshot()
{
	unsigned int width = m_vResolution.x;
	unsigned int height = m_vResolution.y;

	// sanity check
	if (width > 65535 || height > 65535 || width < 1 || height < 1)
	{
		engine->showMessageError("Renderer Error", "getScreenshot() called with invalid arguments!");
		return {};
	}

	// returned buffer, no alpha component
	std::vector<unsigned char> result(width * height * 3);

	// buffer to read into
	std::vector<unsigned char> tempBuffer(width * height * 4);

	// prep framebuffer for reading
	GLint currentFramebuffer;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer);
	GLint previousAlignment;
	glGetIntegerv(GL_PACK_ALIGNMENT, &previousAlignment);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glFinish();

	// read
	GLenum error = GL_NO_ERROR;
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tempBuffer.data());

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		engine->showMessageError("Screenshot Error", UString::format("glReadPixels failed with error code: %d", error));
		result.clear();

		glPixelStorei(GL_PACK_ALIGNMENT, previousAlignment);
		glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer);
		return result;
	}

	// restore state
	glPixelStorei(GL_PACK_ALIGNMENT, previousAlignment);
	glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer);

	// discard alpha component and flip it (Engine coordinates vs GL coordinates)
	for (unsigned int y = 0; y < height; ++y)
	{
		const unsigned int srcRow = height - y - 1;
		const unsigned int dstRow = y;

		for (unsigned int x = 0; x < width; ++x)
		{
			result[(dstRow * width + x) * 3 + 0] = tempBuffer[(srcRow * width + x) * 4 + 0]; // R
			result[(dstRow * width + x) * 3 + 1] = tempBuffer[(srcRow * width + x) * 4 + 1]; // G
			result[(dstRow * width + x) * 3 + 2] = tempBuffer[(srcRow * width + x) * 4 + 2]; // B
			                                                                                 // no alpha
		}
	}

	return result;
}

UString OpenGLES32Interface::getVendor()
{
	const GLubyte *vendor = glGetString(GL_VENDOR);
	return reinterpret_cast<const char *>(vendor);
}

UString OpenGLES32Interface::getModel()
{
	const GLubyte *model = glGetString(GL_RENDERER);
	return reinterpret_cast<const char *>(model);
}

UString OpenGLES32Interface::getVersion()
{
	const GLubyte *version = glGetString(GL_VERSION);
	return reinterpret_cast<const char *>(version);
}

int OpenGLES32Interface::getVRAMTotal()
{
	int nvidiaMemory[4];
	int atiMemory[4];

	for (int i = 0; i < 4; i++)
	{
		nvidiaMemory[i] = -1;
		atiMemory[i] = -1;
	}

	glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, nvidiaMemory);
	glGetIntegerv(TEXTURE_FREE_MEMORY_ATI, atiMemory);

	glGetError(); // clear error state

	if (nvidiaMemory[0] < 1)
		return atiMemory[0];
	else
		return nvidiaMemory[0];
}

int OpenGLES32Interface::getVRAMRemaining()
{
	int nvidiaMemory[4];
	int atiMemory[4];

	for (int i = 0; i < 4; i++)
	{
		nvidiaMemory[i] = -1;
		atiMemory[i] = -1;
	}

	glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, nvidiaMemory);
	glGetIntegerv(TEXTURE_FREE_MEMORY_ATI, atiMemory);

	glGetError(); // clear error state

	if (nvidiaMemory[0] < 1)
		return atiMemory[0];
	else
		return nvidiaMemory[0];
}

void OpenGLES32Interface::onResolutionChange(Vector2 newResolution)
{
	// rebuild viewport
	m_vResolution = newResolution;
	glViewport(0, 0, m_vResolution.x, m_vResolution.y);

	// special case: custom rendertarget resolution rendering, update active projection matrix immediately
	if (m_bInScene)
	{
		m_projectionTransformStack.top() = Camera::buildMatrixOrtho2D(0, m_vResolution.x, m_vResolution.y, 0, -1.0f, 1.0f);
		m_bTransformUpToDate = false;
	}
}

Image *OpenGLES32Interface::createImage(UString filePath, bool mipmapped, bool keepInSystemMemory)
{
	return new OpenGLImage(filePath, mipmapped, keepInSystemMemory);
}

Image *OpenGLES32Interface::createImage(int width, int height, bool mipmapped, bool keepInSystemMemory)
{
	return new OpenGLImage(width, height, mipmapped, keepInSystemMemory);
}

RenderTarget *OpenGLES32Interface::createRenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType)
{
	return new OpenGLRenderTarget(x, y, width, height, multiSampleType);
}

Shader *OpenGLES32Interface::createShaderFromFile(UString vertexShaderFilePath, UString fragmentShaderFilePath)
{
	OpenGLES32Shader *shader = new OpenGLES32Shader(vertexShaderFilePath, fragmentShaderFilePath, false);
	registerShader(shader);
	return shader;
}

Shader *OpenGLES32Interface::createShaderFromSource(UString vertexShader, UString fragmentShader)
{
	OpenGLES32Shader *shader = new OpenGLES32Shader(vertexShader, fragmentShader, true);
	registerShader(shader);
	return shader;
}

Shader *OpenGLES32Interface::createShaderFromFile(UString shaderFilePath)
{
	OpenGLES32Shader *shader = new OpenGLES32Shader(shaderFilePath, false);
	registerShader(shader);
	return shader;
}

Shader *OpenGLES32Interface::createShaderFromSource(UString shaderSource)
{
	OpenGLES32Shader *shader = new OpenGLES32Shader(shaderSource, true);
	registerShader(shader);
	return shader;
}

VertexArrayObject *OpenGLES32Interface::createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage, bool keepInSystemMemory)
{
	return new OpenGLES32VertexArrayObject(primitive, usage, keepInSystemMemory);
}

void OpenGLES32Interface::forceUpdateTransform()
{
	updateTransform();
}

void OpenGLES32Interface::onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix)
{
	m_projectionMatrix = projectionMatrix;
	m_worldMatrix = worldMatrix;

	m_MP = m_projectionMatrix * m_worldMatrix;

	// update all registered shaders, including the default one
	updateAllShaderTransforms();
}

void OpenGLES32Interface::handleGLErrors()
{
	int error = glGetError();
	if (error != 0)
		debugLog("OpenGL Error: %i on frame %i\n", error, engine->getFrameCount());
}

int OpenGLES32Interface::primitiveToOpenGL(Graphics::PRIMITIVE primitive)
{
	switch (primitive)
	{
	case Graphics::PRIMITIVE::PRIMITIVE_LINES:
		return GL_LINES;
	case Graphics::PRIMITIVE::PRIMITIVE_LINE_STRIP:
		return GL_LINE_STRIP;
	case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES:
		return GL_TRIANGLES;
	case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN:
		return GL_TRIANGLE_FAN;
	case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP:
		return GL_TRIANGLE_STRIP;
	case Graphics::PRIMITIVE::PRIMITIVE_QUADS:
		return 0; // not supported
	}

	return GL_TRIANGLES;
}

int OpenGLES32Interface::compareFuncToOpenGL(Graphics::COMPARE_FUNC compareFunc)
{
	switch (compareFunc)
	{
	case Graphics::COMPARE_FUNC::COMPARE_FUNC_NEVER:
		return GL_NEVER;
	case Graphics::COMPARE_FUNC::COMPARE_FUNC_LESS:
		return GL_LESS;
	case Graphics::COMPARE_FUNC::COMPARE_FUNC_EQUAL:
		return GL_EQUAL;
	case Graphics::COMPARE_FUNC::COMPARE_FUNC_LESSEQUAL:
		return GL_LEQUAL;
	case Graphics::COMPARE_FUNC::COMPARE_FUNC_GREATER:
		return GL_GREATER;
	case Graphics::COMPARE_FUNC::COMPARE_FUNC_NOTEQUAL:
		return GL_NOTEQUAL;
	case Graphics::COMPARE_FUNC::COMPARE_FUNC_GREATEREQUAL:
		return GL_GEQUAL;
	case Graphics::COMPARE_FUNC::COMPARE_FUNC_ALWAYS:
		return GL_ALWAYS;
	}

	return GL_ALWAYS;
}

void OpenGLES32Interface::registerShader(OpenGLES32Shader *shader)
{
	// check if already registered
	for (size_t i = 0; i < m_registeredShaders.size(); i++)
	{
		if (m_registeredShaders[i] == shader)
			return;
	}

	m_registeredShaders.push_back(shader);
}

void OpenGLES32Interface::unregisterShader(OpenGLES32Shader *shader)
{
	// remove from registry if found
	for (size_t i = 0; i < m_registeredShaders.size(); i++)
	{
		if (m_registeredShaders[i] == shader)
		{
			m_registeredShaders.erase(m_registeredShaders.begin() + i);
			return;
		}
	}
}

void OpenGLES32Interface::updateAllShaderTransforms()
{
	for (size_t i = 0; i < m_registeredShaders.size(); i++)
	{
		if (m_registeredShaders[i]->isActive())
		{
			m_registeredShaders[i]->setUniformMatrix4fv("mvp", m_MP);
		}
	}
}

#endif
