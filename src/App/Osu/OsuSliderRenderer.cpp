//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		static renderer class, so it can be used outside of OsuSlider
//
// $NoKeywords: $sliderrender
//===============================================================================//

#include "OsuSliderRenderer.h"

#include "Engine.h"
#include "Shader.h"
#include "VertexArrayObject.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "Environment.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"

#include "OpenGLHeaders.h"
#include "DirectX11Interface.h"

Shader *OsuSliderRenderer::BLEND_SHADER{nullptr};

float OsuSliderRenderer::MESH_CENTER_HEIGHT = 0.5f; // Camera::buildMatrixOrtho2D() uses -1 to 1 for zn/zf, so don't make this too high
int OsuSliderRenderer::UNIT_CIRCLE_SUBDIVISIONS = 0; // see osu_slider_body_unit_circle_subdivisions now
std::vector<float> OsuSliderRenderer::UNIT_CIRCLE;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO = NULL;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO_BAKED = NULL;
VertexArrayObject *OsuSliderRenderer::UNIT_CIRCLE_VAO_TRIANGLES = NULL;
float OsuSliderRenderer::UNIT_CIRCLE_VAO_DIAMETER = 0.0f;

float OsuSliderRenderer::m_fBoundingBoxMinX = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxX = 0.0f;
float OsuSliderRenderer::m_fBoundingBoxMinY = std::numeric_limits<float>::max();
float OsuSliderRenderer::m_fBoundingBoxMaxY = 0.0f;

float OsuSliderRenderer::border_feather = 0.0f;

OsuSliderRenderer::UniformCache OsuSliderRenderer::uniformCache{};

namespace cv::osu
{
ConVar slider_debug_draw("osu_slider_debug_draw", false, FCVAR_NONE,
                         "draw hitcircle at every curve point and nothing else (no vao, no rt, no shader, nothing) (requires enabling legacy slider renderer)");
ConVar slider_debug_draw_square_vao("osu_slider_debug_draw_square_vao", false, FCVAR_NONE,
                                    "generate square vaos and nothing else (no rt, no shader) (requires disabling legacy slider renderer)");
ConVar slider_debug_wireframe("osu_slider_debug_wireframe", false, FCVAR_NONE, "unused");

ConVar slider_alpha_multiplier("osu_slider_alpha_multiplier", 1.0f, FCVAR_NONE);
ConVar slider_body_alpha_multiplier("osu_slider_body_alpha_multiplier", 1.0f, FCVAR_NONE, CFUNC(OsuSliderRenderer::onUniformConfigChanged));
ConVar slider_body_color_saturation("osu_slider_body_color_saturation", 1.0f, FCVAR_NONE, CFUNC(OsuSliderRenderer::onUniformConfigChanged));
ConVar slider_border_size_multiplier("osu_slider_border_size_multiplier", 1.0f, FCVAR_NONE, CFUNC(OsuSliderRenderer::onUniformConfigChanged));
ConVar slider_border_tint_combo_color("osu_slider_border_tint_combo_color", false, FCVAR_NONE);
ConVar slider_osu_next_style("osu_slider_osu_next_style", false, FCVAR_NONE, CFUNC(OsuSliderRenderer::onUniformConfigChanged));
ConVar slider_rainbow("osu_slider_rainbow", false, FCVAR_NONE);
ConVar slider_use_gradient_image("osu_slider_use_gradient_image", false, FCVAR_NONE);

ConVar slider_body_unit_circle_subdivisions("osu_slider_body_unit_circle_subdivisions", 42, FCVAR_NONE);
ConVar slider_legacy_use_baked_vao("osu_slider_legacy_use_baked_vao", false, FCVAR_NONE, "use baked cone mesh instead of raw mesh for legacy slider renderer (disabled by default because usually slower on very old gpus even though it should not be)");
}

VertexArrayObject *OsuSliderRenderer::generateVAO(const std::vector<Vector2> &points, float hitcircleDiameter, Vector3 translation, bool skipOOBPoints)
{
	resourceManager->requestNextLoadUnmanaged();
	VertexArrayObject *vao = resourceManager->createVertexArrayObject();

	checkUpdateVars(hitcircleDiameter);

	const Vector3 xOffset = Vector3(hitcircleDiameter, 0, 0);
	const Vector3 yOffset = Vector3(0, hitcircleDiameter, 0);

	const bool debugSquareVao = cv::osu::slider_debug_draw_square_vao.getBool();

	for (int i=0; i<points.size(); i++)
	{
		// fuck oob sliders
		if (skipOOBPoints)
		{
			if (points[i].x < -hitcircleDiameter-OsuGameRules::OSU_COORD_WIDTH*2 || points[i].x > osu->getVirtScreenWidth()+hitcircleDiameter+OsuGameRules::OSU_COORD_WIDTH*2 || points[i].y < -hitcircleDiameter-OsuGameRules::OSU_COORD_HEIGHT*2 || points[i].y > osu->getVirtScreenHeight()+hitcircleDiameter+OsuGameRules::OSU_COORD_HEIGHT*2)
				continue;
		}

		if (!debugSquareVao)
		{
			const std::vector<Vector3> &meshVertices = UNIT_CIRCLE_VAO_TRIANGLES->getVertices();
			const std::vector<std::vector<Vector2>> &meshTexCoords = UNIT_CIRCLE_VAO_TRIANGLES->getTexcoords();
			for (int v=0; v<meshVertices.size(); v++)
			{
				vao->addVertex(meshVertices[v] + Vector3(points[i].x, points[i].y, 0) + translation);
				vao->addTexcoord(meshTexCoords[0][v]);
			}
		}
		else
		{
			const Vector3 topLeft = Vector3(points[i].x, points[i].y, 0) - xOffset/2.0f - yOffset/2.0f + translation;
			const Vector3 topRight = topLeft + xOffset;
			const Vector3 bottomLeft = topLeft + yOffset;
			const Vector3 bottomRight = bottomLeft + xOffset;

			vao->addVertex(topLeft);
			vao->addTexcoord(0, 0);

			vao->addVertex(bottomLeft);
			vao->addTexcoord(0, 1);

			vao->addVertex(bottomRight);
			vao->addTexcoord(1, 1);

			vao->addVertex(topLeft);
			vao->addTexcoord(0, 0);

			vao->addVertex(bottomRight);
			vao->addTexcoord(1, 1);

			vao->addVertex(topRight);
			vao->addTexcoord(1, 0);
		}
	}

	if (vao->getNumVertices() > 0)
		resourceManager->loadResource(vao);
	else
		debugLog("ERROR: Zero triangles!\n");

	return vao;
}

void OsuSliderRenderer::draw(Osu *osu, const std::vector<Vector2> &points, const std::vector<Vector2> &alwaysPoints, float hitcircleDiameter, float from, float to, Color undimmedColor, float colorRGBMultiplier, float alpha, long sliderTimeForRainbow)
{
	if (cv::osu::slider_alpha_multiplier.getFloat() <= 0.0f || alpha <= 0.0f) return;

	checkUpdateVars(hitcircleDiameter);

	const int drawFromIndex = std::clamp<int>((int)std::round(points.size() * from), 0, points.size());
	const int drawUpToIndex = std::clamp<int>((int)std::round(points.size() * to), 0, points.size());

	// debug sliders
	if (cv::osu::slider_debug_draw.getBool())
	{
		const float circleImageScale = hitcircleDiameter / (float)osu->getSkin()->getHitCircle()->getWidth();
		const float circleImageScaleInv = (1.0f / circleImageScale);

		const float width = (float)osu->getSkin()->getHitCircle()->getWidth();
		const float height = (float)osu->getSkin()->getHitCircle()->getHeight();

		const float x = (-width / 2.0f);
		const float y = (-height / 2.0f);
		const float z = -1.0f;

		g->pushTransform();
		{
			g->scale(circleImageScale, circleImageScale);

			const Color dimmedColor = Colors::scale(undimmedColor, colorRGBMultiplier);

			g->setColor(dimmedColor);
			g->setAlpha(alpha*cv::osu::slider_alpha_multiplier.getFloat());
			osu->getSkin()->getHitCircle()->bind();
			{
				for (int i=drawFromIndex; i<drawUpToIndex; i++)
				{
					const Vector2 point = points[i] * circleImageScaleInv;

					static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
					vao.empty();
					{
						vao.addTexcoord(0, 0);
						vao.addVertex(point.x + x, point.y + y, z);

						vao.addTexcoord(0, 1);
						vao.addVertex(point.x + x, point.y + y + height, z);

						vao.addTexcoord(1, 1);
						vao.addVertex(point.x + x + width, point.y + y + height, z);

						vao.addTexcoord(1, 0);
						vao.addVertex(point.x + x + width, point.y + y, z);
					}
					g->drawVAO(&vao);
				}
			}
			osu->getSkin()->getHitCircle()->unbind();
		}
		g->popTransform();

		return; // nothing more to draw here
	}

	// reset
	resetRenderTargetBoundingBox();

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		osu->getSliderFrameBuffer()->enable();
		{
			const Color undimmedBorderColor = cv::osu::slider_border_tint_combo_color.getBool() ? undimmedColor : osu->getSkin()->getSliderBorderColor();
			const Color undimmedBodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : undimmedColor;

			Color dimmedBorderColor;
			Color dimmedBodyColor;

			if (cv::osu::slider_rainbow.getBool())
			{
				float frequency = 0.3f;
				float time = engine->getTime()*20;

				const Channel red1	 = std::sin(frequency*time + 0 + sliderTimeForRainbow) * 127 + 128;
				const Channel green1 = std::sin(frequency*time + 2 + sliderTimeForRainbow) * 127 + 128;
				const Channel blue1	 = std::sin(frequency*time + 4 + sliderTimeForRainbow) * 127 + 128;

				const Channel red2	 = std::sin(frequency*time*1.5f + 0 + sliderTimeForRainbow) * 127 + 128;
				const Channel green2 = std::sin(frequency*time*1.5f + 2 + sliderTimeForRainbow) * 127 + 128;
				const Channel blue2	 = std::sin(frequency*time*1.5f + 4 + sliderTimeForRainbow) * 127 + 128;

				dimmedBorderColor = rgb(red1, green1, blue1);
				dimmedBodyColor = rgb(red2, green2, blue2);
			}
			else
			{
				dimmedBorderColor = Colors::scale(undimmedBorderColor, colorRGBMultiplier);
				dimmedBodyColor = Colors::scale(undimmedBodyColor, colorRGBMultiplier);
			}

			if (!cv::osu::slider_use_gradient_image.getBool())
			{
				BLEND_SHADER->enable();
				updateConfigUniforms();
				updateColorUniforms(dimmedBorderColor, dimmedBodyColor);
			}

			g->setColor(argb(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier)); // this only affects the gradient image if used (meaning shaders either don't work or are disabled on purpose)
			osu->getSkin()->getSliderGradient()->bind();
			{
				// draw curve mesh
				{
					drawFillSliderBodyPeppy(osu, points, (cv::osu::slider_legacy_use_baked_vao.getBool() ? UNIT_CIRCLE_VAO_BAKED : UNIT_CIRCLE_VAO), hitcircleDiameter/2.0f, drawFromIndex, drawUpToIndex, BLEND_SHADER);

					if (alwaysPoints.size() > 0)
						drawFillSliderBodyPeppy(osu, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size(), BLEND_SHADER);
				}
			}

			if (!cv::osu::slider_use_gradient_image.getBool())
				BLEND_SHADER->disable();
		}
		osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	// now draw the slider to the screen (with alpha blending enabled again)
	const int pixelFudge = 2;
	m_fBoundingBoxMinX -= pixelFudge;
	m_fBoundingBoxMaxX += pixelFudge;
	m_fBoundingBoxMinY -= pixelFudge;
	m_fBoundingBoxMaxY += pixelFudge;

	osu->getSliderFrameBuffer()->setColor(argb(alpha*cv::osu::slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
	osu->getSliderFrameBuffer()->drawRect(m_fBoundingBoxMinX, m_fBoundingBoxMinY, m_fBoundingBoxMaxX - m_fBoundingBoxMinX, m_fBoundingBoxMaxY - m_fBoundingBoxMinY);
}

void OsuSliderRenderer::draw(Osu *osu, VertexArrayObject *vao, const std::vector<Vector2> &alwaysPoints, Vector2 translation, float scale, float hitcircleDiameter, float from, float to, Color undimmedColor, float colorRGBMultiplier, float alpha, long sliderTimeForRainbow, bool doEnableRenderTarget, bool doDisableRenderTarget, bool doDrawSliderFrameBufferToScreen)
{
	if ((cv::osu::slider_alpha_multiplier.getFloat() <= 0.0f && doDrawSliderFrameBufferToScreen) || (alpha <= 0.0f && doDrawSliderFrameBufferToScreen) || vao == NULL) return;

	checkUpdateVars(hitcircleDiameter);

	if (cv::osu::slider_debug_draw_square_vao.getBool())
	{
		const Color dimmedColor = Colors::scale(undimmedColor, colorRGBMultiplier);

		g->setColor(dimmedColor);
		g->setAlpha(alpha*cv::osu::slider_alpha_multiplier.getFloat());
		osu->getSkin()->getHitCircle()->bind();

		vao->setDrawPercent(from, to, 6); // HACKHACK: hardcoded magic number
		{
			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate(translation.x, translation.y);

				g->drawVAO(vao);
			}
			g->popTransform();
		}

		return; // nothing more to draw here
	}

	// NOTE: this would add support for aspire slider distortions, but calculating the bounding box live is a waste of performance, not worth it
	/*
	{
		m_fBoundingBoxMinX = std::numeric_limits<float>::max();
		m_fBoundingBoxMaxX = std::numeric_limits<float>::min();
		m_fBoundingBoxMinY = std::numeric_limits<float>::max();
		m_fBoundingBoxMaxY = std::numeric_limits<float>::min();

		// NOTE: to get the animated effect, would have to use from -> to (not implemented atm)
		for (int i=0; i<points.size(); i++)
		{
			const float &x = points[i].x;
			const float &y = points[i].y;
			const float radius = hitcircleDiameter/2.0f;

			if (x-radius < m_fBoundingBoxMinX)
				m_fBoundingBoxMinX = x-radius;
			if (x+radius > m_fBoundingBoxMaxX)
				m_fBoundingBoxMaxX = x+radius;
			if (y-radius < m_fBoundingBoxMinY)
				m_fBoundingBoxMinY = y-radius;
			if (y+radius > m_fBoundingBoxMaxY)
				m_fBoundingBoxMaxY = y+radius;
		}

		const Vector4 tLS = (g->getProjectionMatrix() * Vector4(m_fBoundingBoxMinX, m_fBoundingBoxMinY, 0, 0) + Vector4(1, 1, 0, 0)) * 0.5f;
		const Vector4 bRS = (g->getProjectionMatrix() * Vector4(m_fBoundingBoxMaxX, m_fBoundingBoxMaxY, 0, 0) + Vector4(1, 1, 0, 0)) * 0.5f;

		float scaleToApplyAfterTranslationX = 1.0f;
		float scaleToApplyAfterTranslationY = 1.0f;

		const float sclX = (32768.0f / (float)osu->getVirtScreenWidth());
		const float sclY = (32768.0f / (float)osu->getVirtScreenHeight());

		if (-tLS.x + bRS.x > sclX)
			scaleToApplyAfterTranslationX = sclX / (-tLS.x + bRS.x);
		if (-tLS.y + bRS.y > sclY)
			scaleToApplyAfterTranslationY = sclY / (-tLS.y + bRS.y);
	}
	*/

	// draw entire slider into framebuffer
	g->setDepthBuffer(true);
	g->setBlending(false);
	{
		if (doEnableRenderTarget)
			osu->getSliderFrameBuffer()->enable();

		// render
		{
			const Color undimmedBorderColor = cv::osu::slider_border_tint_combo_color.getBool() ? undimmedColor : osu->getSkin()->getSliderBorderColor();
			const Color undimmedBodyColor = osu->getSkin()->isSliderTrackOverridden() ? osu->getSkin()->getSliderTrackOverride() : undimmedColor;

			Color dimmedBorderColor;
			Color dimmedBodyColor;

			if (cv::osu::slider_rainbow.getBool())
			{
				float frequency = 0.3f;
				float time = engine->getTime()*20;

				const Channel red1	 = std::sin(frequency*time + 0 + sliderTimeForRainbow) * 127 + 128;
				const Channel green1 = std::sin(frequency*time + 2 + sliderTimeForRainbow) * 127 + 128;
				const Channel blue1	 = std::sin(frequency*time + 4 + sliderTimeForRainbow) * 127 + 128;

				const Channel red2	 = std::sin(frequency*time*1.5f + 0 + sliderTimeForRainbow) * 127 + 128;
				const Channel green2 = std::sin(frequency*time*1.5f + 2 + sliderTimeForRainbow) * 127 + 128;
				const Channel blue2	 = std::sin(frequency*time*1.5f + 4 + sliderTimeForRainbow) * 127 + 128;

				dimmedBorderColor = rgb(red1, green1, blue1);
				dimmedBodyColor = rgb(red2, green2, blue2);
			}
			else
			{
				dimmedBorderColor = Colors::scale(undimmedBorderColor, colorRGBMultiplier);
				dimmedBodyColor = Colors::scale(undimmedBodyColor, colorRGBMultiplier);
			}

			if (!cv::osu::slider_use_gradient_image.getBool())
			{
				BLEND_SHADER->enable();
				updateConfigUniforms();
				updateColorUniforms(dimmedBorderColor, dimmedBodyColor);
			}

			g->setColor(argb(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier)); // this only affects the gradient image if used (meaning shaders either don't work or are disabled on purpose)
			osu->getSkin()->getSliderGradient()->bind();
			{
				// draw curve mesh
				{
					vao->setDrawPercent(from, to, UNIT_CIRCLE_VAO_TRIANGLES->getVertices().size());
					g->pushTransform();
					{
						g->scale(scale, scale);
						g->translate(translation.x, translation.y);
						///g->scale(scaleToApplyAfterTranslationX, scaleToApplyAfterTranslationY); // aspire slider distortions

						if constexpr (Env::cfg(REND::DX11)) {
						if (!cv::osu::slider_use_gradient_image.getBool())
						{
							g->forceUpdateTransform();
							Matrix4 mvp = g->getMVP();
							BLEND_SHADER->setUniformMatrix4fv("mvp", mvp);
						}
						}

						g->drawVAO(vao);
					}
					g->popTransform();

					if (alwaysPoints.size() > 0)
						drawFillSliderBodyPeppy(osu, alwaysPoints, UNIT_CIRCLE_VAO_BAKED, hitcircleDiameter/2.0f, 0, alwaysPoints.size(), BLEND_SHADER);
				}
			}

			if (!cv::osu::slider_use_gradient_image.getBool())
				BLEND_SHADER->disable();
		}

		if (doDisableRenderTarget)
			osu->getSliderFrameBuffer()->disable();
	}
	g->setBlending(true);
	g->setDepthBuffer(false);

	if (doDrawSliderFrameBufferToScreen)
	{
		osu->getSliderFrameBuffer()->setColor(argb(alpha*cv::osu::slider_alpha_multiplier.getFloat(), 1.0f, 1.0f, 1.0f));
		osu->getSliderFrameBuffer()->draw(0, 0);
	}
}

void OsuSliderRenderer::drawFillSliderBodyPeppy(Osu *osu, const std::vector<Vector2> &points, VertexArrayObject *circleMesh, float radius, int drawFromIndex, int drawUpToIndex, Shader *shader)
{
	if (drawFromIndex < 0)
		drawFromIndex = 0;
	if (drawUpToIndex < 0)
		drawUpToIndex = points.size();

	g->pushTransform();
	{
		// now, translate and draw the master vao for every curve point
		float startX = 0.0f;
		float startY = 0.0f;
		for (int i=drawFromIndex; i<drawUpToIndex; ++i)
		{
			const float x = points[i].x;
			const float y = points[i].y;

			// fuck oob sliders
			if (x < -radius*2 || x > osu->getVirtScreenWidth()+radius*2 || y < -radius*2 || y > osu->getVirtScreenHeight()+radius*2)
				continue;

			g->translate(x-startX, y-startY, 0);

			if constexpr (Env::cfg(REND::DX11)) {
			if (shader)
			{
				g->forceUpdateTransform();
				Matrix4 mvp = g->getMVP();
				shader->setUniformMatrix4fv("mvp", mvp);
			}
			}

			g->drawVAO(circleMesh);

			startX = x;
			startY = y;

			if (x-radius < m_fBoundingBoxMinX)
				m_fBoundingBoxMinX = x-radius;
			if (x+radius > m_fBoundingBoxMaxX)
				m_fBoundingBoxMaxX = x+radius;
			if (y-radius < m_fBoundingBoxMinY)
				m_fBoundingBoxMinY = y-radius;
			if (y+radius > m_fBoundingBoxMaxY)
				m_fBoundingBoxMaxY = y+radius;
		}
	}
	g->popTransform();
}

void OsuSliderRenderer::checkUpdateVars(float hitcircleDiameter)
{
	// static globals

	if constexpr (Env::cfg(REND::DX11))
	{
		// NOTE: compensate for zn/zf Camera::buildMatrixOrtho2DDXLH() differences compared to OpenGL
		if (MESH_CENTER_HEIGHT > 0.0f)
			MESH_CENTER_HEIGHT = -MESH_CENTER_HEIGHT;
	}

	// build shaders and circle mesh
	if (BLEND_SHADER == NULL) // only do this once
	{
		// build shaders
		BLEND_SHADER = resourceManager->loadShader("slider.mcshader", "slider");
	}

	const int subdivisions = cv::osu::slider_body_unit_circle_subdivisions.getInt();
	if (subdivisions != UNIT_CIRCLE_SUBDIVISIONS)
	{
		UNIT_CIRCLE_SUBDIVISIONS = subdivisions;

		// build unit cone
		{
			UNIT_CIRCLE.clear();

			// tip of the cone
			// texture coordinates
			UNIT_CIRCLE.push_back(1.0f);
			UNIT_CIRCLE.push_back(0.0f);

			// position
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(MESH_CENTER_HEIGHT);

			for (int j=0; j<subdivisions; ++j)
			{
				float phase = j * (float) PI * 2.0f / subdivisions;

				// texture coordinates
				UNIT_CIRCLE.push_back(0.0f);
				UNIT_CIRCLE.push_back(0.0f);

				// positon
				UNIT_CIRCLE.push_back((float)std::sin(phase));
				UNIT_CIRCLE.push_back((float)std::cos(phase));
				UNIT_CIRCLE.push_back(0.0f);
			}

			// texture coordinates
			UNIT_CIRCLE.push_back(0.0f);
			UNIT_CIRCLE.push_back(0.0f);

			// positon
			UNIT_CIRCLE.push_back((float)std::sin(0.0f));
			UNIT_CIRCLE.push_back((float)std::cos(0.0f));
			UNIT_CIRCLE.push_back(0.0f);
		}
	}

	// build vaos
	if (UNIT_CIRCLE_VAO == NULL)
		UNIT_CIRCLE_VAO = new VertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN);
	if (UNIT_CIRCLE_VAO_BAKED == NULL)
		UNIT_CIRCLE_VAO_BAKED = resourceManager->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN);
	if (UNIT_CIRCLE_VAO_TRIANGLES == NULL)
		UNIT_CIRCLE_VAO_TRIANGLES = new VertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES);

	// (re-)generate master circle mesh (centered) if the size changed
	// dynamic mods like minimize or wobble have to use the legacy renderer anyway, since the slider shape may change every frame
	if (hitcircleDiameter != UNIT_CIRCLE_VAO_DIAMETER)
	{
		const float radius = hitcircleDiameter/2.0f;

		UNIT_CIRCLE_VAO_BAKED->release();

		// triangle fan
		UNIT_CIRCLE_VAO_DIAMETER = hitcircleDiameter;
		UNIT_CIRCLE_VAO->clear();
		for (int i=0; i<UNIT_CIRCLE.size()/5; i++)
		{
			Vector3 vertexPos = Vector3((radius * UNIT_CIRCLE[i * 5 + 2]), (radius * UNIT_CIRCLE[i * 5 + 3]), UNIT_CIRCLE[i * 5 + 4]);
			Vector2 vertexTexcoord = Vector2(UNIT_CIRCLE[i * 5 + 0], UNIT_CIRCLE[i * 5 + 1]);

			UNIT_CIRCLE_VAO->addVertex(vertexPos);
			UNIT_CIRCLE_VAO->addTexcoord(vertexTexcoord);

			UNIT_CIRCLE_VAO_BAKED->addVertex(vertexPos);
			UNIT_CIRCLE_VAO_BAKED->addTexcoord(vertexTexcoord);
		}

		resourceManager->loadResource(UNIT_CIRCLE_VAO_BAKED);

		// pure triangles (needed for VertexArrayObject, because we can't merge multiple triangle fan meshes into one VertexArrayObject)
		UNIT_CIRCLE_VAO_TRIANGLES->clear();
		Vector3 startVertex = Vector3((radius * UNIT_CIRCLE[0 * 5 + 2]), (radius * UNIT_CIRCLE[0 * 5 + 3]), UNIT_CIRCLE[0 * 5 + 4]);
		Vector2 startUV = Vector2(UNIT_CIRCLE[0 * 5 + 0], UNIT_CIRCLE[0 * 5 + 1]);
		for (int i=1; i<UNIT_CIRCLE.size()/5 - 1; i++)
		{
			// center
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(startVertex);
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(startUV);

			// pizza slice edge 1
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(Vector3((radius * UNIT_CIRCLE[i * 5 + 2]), (radius * UNIT_CIRCLE[i * 5 + 3]), UNIT_CIRCLE[i * 5 + 4]));
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(Vector2(UNIT_CIRCLE[i * 5 + 0], UNIT_CIRCLE[i * 5 + 1]));

			// pizza slice edge 2
			UNIT_CIRCLE_VAO_TRIANGLES->addVertex(Vector3((radius * UNIT_CIRCLE[(i+1) * 5 + 2]), (radius * UNIT_CIRCLE[(i+1) * 5 + 3]), UNIT_CIRCLE[(i+1) * 5 + 4]));
			UNIT_CIRCLE_VAO_TRIANGLES->addTexcoord(Vector2(UNIT_CIRCLE[(i+1) * 5 + 0], UNIT_CIRCLE[(i+1) * 5 + 1]));
		}
	}
}

void OsuSliderRenderer::resetRenderTargetBoundingBox()
{
	OsuSliderRenderer::m_fBoundingBoxMinX = std::numeric_limits<float>::max();
	OsuSliderRenderer::m_fBoundingBoxMaxX = 0.0f;
	OsuSliderRenderer::m_fBoundingBoxMinY = std::numeric_limits<float>::max();
	OsuSliderRenderer::m_fBoundingBoxMaxY = 0.0f;
}

// helper function to update color uniforms
void OsuSliderRenderer::updateColorUniforms(const Color &borderColor, const Color &bodyColor)
{
	if (uniformCache.lastBorderColor != borderColor)
	{
		BLEND_SHADER->setUniform3f("colBorder", borderColor.Rf(), borderColor.Gf(), borderColor.Bf());
		uniformCache.lastBorderColor = borderColor;
	}

	if (uniformCache.lastBodyColor != bodyColor)
	{
		BLEND_SHADER->setUniform3f("colBody", bodyColor.Rf(), bodyColor.Gf(), bodyColor.Bf());
		uniformCache.lastBodyColor = bodyColor;
	}

	if (uniformCache.borderFeather != border_feather)
	{
		BLEND_SHADER->setUniform1f("borderFeather", border_feather);
		uniformCache.borderFeather = border_feather;
	}
}

void OsuSliderRenderer::updateConfigUniforms()
{
	if (!BLEND_SHADER || !uniformCache.needsConfigUpdate)
		return;

	const int newStyle = cv::osu::slider_osu_next_style.getBool() ? 1 : 0;
	const float newBodyAlpha = cv::osu::slider_body_alpha_multiplier.getFloat();
	const float newBodySat = cv::osu::slider_body_color_saturation.getFloat();
	const float newBorderSize = cv::osu::slider_border_size_multiplier.getFloat();

	if (uniformCache.style != newStyle)
	{
		BLEND_SHADER->setUniform1i("style", newStyle);
		uniformCache.style = newStyle;
	}

	if (uniformCache.bodyAlphaMultiplier != newBodyAlpha)
	{
		BLEND_SHADER->setUniform1f("bodyAlphaMultiplier", newBodyAlpha);
		uniformCache.bodyAlphaMultiplier = newBodyAlpha;
	}

	if (uniformCache.bodyColorSaturation != newBodySat)
	{
		BLEND_SHADER->setUniform1f("bodyColorSaturation", newBodySat);
		uniformCache.bodyColorSaturation = newBodySat;
	}

	if (uniformCache.borderSizeMultiplier != newBorderSize)
	{
		BLEND_SHADER->setUniform1f("borderSizeMultiplier", newBorderSize);
		uniformCache.borderSizeMultiplier = newBorderSize;
	}

	uniformCache.needsConfigUpdate = false;
}
