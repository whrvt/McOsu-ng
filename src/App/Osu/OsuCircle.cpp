//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		circle
//
// $NoKeywords: $circle
//===============================================================================//

#include "OsuCircle.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "SoundEngine.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "Camera.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuGameRules.h"
#include "OsuBeatmapStandard.h"
#include "OsuModFPoSu.h"

#include "OpenGLHeaders.h"

ConVar osu_bug_flicker_log("osu_bug_flicker_log", false, FCVAR_NONE);

ConVar osu_circle_color_saturation("osu_circle_color_saturation", 1.0f, FCVAR_NONE);
ConVar osu_circle_rainbow("osu_circle_rainbow", false, FCVAR_NONE);
ConVar osu_circle_number_rainbow("osu_circle_number_rainbow", false, FCVAR_NONE);
ConVar osu_circle_shake_duration("osu_circle_shake_duration", 0.120f, FCVAR_NONE);
ConVar osu_circle_shake_strength("osu_circle_shake_strength", 8.0f, FCVAR_NONE);
ConVar osu_approach_circle_alpha_multiplier("osu_approach_circle_alpha_multiplier", 0.9f, FCVAR_NONE);

ConVar osu_draw_numbers("osu_draw_numbers", true, FCVAR_NONE);
ConVar osu_draw_circles("osu_draw_circles", true, FCVAR_NONE);
ConVar osu_draw_approach_circles("osu_draw_approach_circles", true, FCVAR_NONE);

ConVar osu_slider_draw_endcircle("osu_slider_draw_endcircle", true, FCVAR_NONE);

int OsuCircle::rainbowNumber = 0;
int OsuCircle::rainbowColorCounter = 0;

void OsuCircle::drawApproachCircle(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, bool overrideHDApproachCircle)
{
	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = beatmap->getSkin()->getComboColorForCounter(colorCounter, colorOffset);
	comboColor = rgb((int)(Ri(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Gi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Bi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()));

	drawApproachCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), comboColor, beatmap->getHitcircleDiameter(), approachScale, alpha, beatmap->getOsu()->getModHD(), overrideHDApproachCircle);
}

void OsuCircle::draw3DApproachCircle(Graphics *g, OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, bool overrideHDApproachCircle)
{
	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = beatmap->getSkin()->getComboColorForCounter(colorCounter, colorOffset);
	comboColor = rgb((int)(Ri(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Gi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Bi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()));

	draw3DApproachCircle(g, beatmap->getOsu()->getFPoSu(), baseScale, beatmap->getSkin(), beatmap->osuCoordsTo3D(rawPos, hitObject), comboColor, beatmap->getRawHitcircleDiameter(), approachScale, alpha, beatmap->getOsu()->getModHD(), overrideHDApproachCircle);
}

void OsuCircle::drawCircle(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::draw3DCircle(Graphics *g, OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	draw3DCircle(g, beatmap->getOsu()->getFPoSu(), baseScale, beatmap->getSkin(), beatmap->osuCoordsTo3D(rawPos, hitObject), beatmap->getRawHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !osu_draw_circles.getBool()) return;

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
	comboColor = rgb((int)(Ri(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Gi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Bi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()));

	// approach circle
	///drawApproachCircle(g, skin, pos, comboColor, hitcircleDiameter, approachScale, alpha, modHD, overrideHDApproachCircle); // they are now drawn separately in draw2()

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(g, skin->getHitCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getHitCircleOverlay2()->getSizeBaseRaw().x;
	if (!skin->getHitCircleOverlayAboveNumber())
		drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);

	// number
	if (drawNumber)
		drawHitCircleNumber(g, skin, numberScale, overlapScale, pos, number, numberAlpha, colorRGBMultiplier);

	// overlay
	if (skin->getHitCircleOverlayAboveNumber())
		drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
}

void OsuCircle::draw3DCircle(Graphics *g, OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float rawHitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !osu_draw_circles.getBool()) return;

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
	comboColor = rgb((int)(Ri(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Gi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Bi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()));

	// approach circle
	///draw3DApproachCircle(g, fposu, baseScale, skin, pos, comboColor, rawHitcircleDiameter, approachScale, alpha, modHD, overrideHDApproachCircle); // they are now drawn separately in draw3D2()

	// circle
	draw3DHitCircle(g, fposu, skin, baseScale, skin->getHitCircle(), pos, comboColor, alpha);

	// overlay
	if (!skin->getHitCircleOverlayAboveNumber())
		draw3DHitCircleOverlay(g, fposu, baseScale, skin->getHitCircleOverlay2(), pos, alpha, colorRGBMultiplier);

	// number
	if (drawNumber)
		draw3DHitCircleNumber(g, skin, numberScale, overlapScale, pos, number, numberAlpha, colorRGBMultiplier);

	// overlay
	if (skin->getHitCircleOverlayAboveNumber())
		draw3DHitCircleOverlay(g, fposu, baseScale, skin->getHitCircleOverlay2(), pos, alpha, colorRGBMultiplier);
}

void OsuCircle::drawCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, Color color, float alpha)
{
	// this function is only used by the target practice heatmap

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(g, skin->getHitCircle(), pos, color, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getHitCircleOverlay2()->getSizeBaseRaw().x;
	drawHitCircleOverlay(g, skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, 1.0f);
}

void OsuCircle::drawSliderStartCircle(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawSliderStartCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::draw3DSliderStartCircle(Graphics *g, OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	draw3DSliderStartCircle(g, beatmap->getOsu()->getFPoSu(), baseScale, beatmap->getSkin(), beatmap->osuCoordsTo3D(rawPos, hitObject), beatmap->getRawHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawSliderStartCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float hitcircleOverlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !osu_draw_circles.getBool()) return;

	// if no sliderstartcircle image is preset, fallback to default circle
	if (skin->getSliderStartCircle() == skin->getMissingTexture())
	{
		drawCircle(g, skin, pos, hitcircleDiameter, numberScale, hitcircleOverlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle); // normal
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
	comboColor = rgb((int)(Ri(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Gi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Bi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()));

	// approach circle
	///drawApproachCircle(g, skin, pos, comboColor, beatmap->getHitcircleDiameter(), approachScale, alpha, beatmap->getOsu()->getModHD(), overrideHDApproachCircle); // they are now drawn separately in draw2()

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isSliderStartCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(g, skin->getSliderStartCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getSliderStartCircleOverlay2()->getSizeBaseRaw().x;
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (!skin->getHitCircleOverlayAboveNumber())
			drawHitCircleOverlay(g, skin->getSliderStartCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
	}

	// number
	if (drawNumber)
		drawHitCircleNumber(g, skin, numberScale, hitcircleOverlapScale, pos, number, numberAlpha, colorRGBMultiplier);

	// overlay
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (skin->getHitCircleOverlayAboveNumber())
			drawHitCircleOverlay(g, skin->getSliderStartCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
	}
}

void OsuCircle::draw3DSliderStartCircle(Graphics *g, OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float rawHitcircleDiameter, float numberScale, float hitcircleOverlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !osu_draw_circles.getBool()) return;

	// if no sliderstartcircle image is preset, fallback to default circle
	if (skin->getSliderStartCircle() == skin->getMissingTexture())
	{
		draw3DCircle(g, fposu, baseScale, skin, pos, rawHitcircleDiameter, numberScale, hitcircleOverlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle); // normal
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
	comboColor = rgb((int)(Ri(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Gi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Bi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()));

	// approach circle
	///drawApproachCircle(g, skin, pos, comboColor, beatmap->getHitcircleDiameter(), approachScale, alpha, beatmap->getOsu()->getModHD(), overrideHDApproachCircle); // they are now drawn separately in draw3D2()

	// circle
	draw3DHitCircle(g, fposu, skin, baseScale, skin->getHitCircle(), pos, comboColor, alpha);

	// overlay
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (!skin->getHitCircleOverlayAboveNumber())
			draw3DHitCircleOverlay(g, fposu, baseScale, skin->getHitCircleOverlay2(), pos, alpha, colorRGBMultiplier);
	}

	// number
	if (drawNumber)
		draw3DHitCircleNumber(g, skin, numberScale, hitcircleOverlapScale, pos, number, numberAlpha, colorRGBMultiplier);

	// overlay
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (skin->getHitCircleOverlayAboveNumber())
			draw3DHitCircleOverlay(g, fposu, baseScale, skin->getHitCircleOverlay2(), pos, alpha, colorRGBMultiplier);
	}
}

void OsuCircle::drawSliderEndCircle(Graphics *g, OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawSliderEndCircle(g, beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::draw3DSliderEndCircle(Graphics *g, OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	draw3DSliderEndCircle(g, beatmap->getOsu()->getFPoSu(), baseScale, beatmap->getSkin(), beatmap->osuCoordsTo3D(rawPos, hitObject), beatmap->getRawHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawSliderEndCircle(Graphics *g, OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !osu_slider_draw_endcircle.getBool() || !osu_draw_circles.getBool()) return;

	// if no sliderendcircle image is preset, fallback to default circle
	if (skin->getSliderEndCircle() == skin->getMissingTexture())
	{
		drawCircle(g, skin, pos, hitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
	comboColor = rgb((int)(Ri(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Gi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Bi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()));

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isSliderEndCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(g, skin->getSliderEndCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	if (skin->getSliderEndCircleOverlay() != skin->getMissingTexture())
	{
		const float circleOverlayImageScale = hitcircleDiameter / skin->getSliderEndCircleOverlay2()->getSizeBaseRaw().x;
		drawHitCircleOverlay(g, skin->getSliderEndCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
	}
}

void OsuCircle::draw3DSliderEndCircle(Graphics *g, OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float rawHitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (m_fposu_3d_spheres_ref->getBool()) return;
	if (alpha <= 0.0f || !osu_slider_draw_endcircle.getBool() || !osu_draw_circles.getBool()) return;

	// if no sliderendcircle image is preset, fallback to default circle
	if (skin->getSliderEndCircle() == skin->getMissingTexture())
	{
		draw3DCircle(g, fposu, baseScale, skin, pos, rawHitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = skin->getComboColorForCounter(colorCounter, colorOffset);
	comboColor = rgb((int)(Ri(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Gi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()), (int)(Bi(comboColor)*colorRGBMultiplier*osu_circle_color_saturation.getFloat()));

	// circle
	draw3DHitCircle(g, fposu, skin, baseScale, skin->getSliderEndCircle(), pos, comboColor, alpha);

	// overlay
	if (skin->getSliderEndCircleOverlay() != skin->getMissingTexture())
		draw3DHitCircleOverlay(g, fposu, baseScale, skin->getSliderEndCircleOverlay2(), pos, alpha, colorRGBMultiplier);
}

void OsuCircle::drawApproachCircle(Graphics *g, OsuSkin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle)
{
	if ((!modHD || overrideHDApproachCircle) && osu_draw_approach_circles.getBool() && !OsuGameRules::osu_mod_mafham.getBool())
	{
		if (approachScale > 1.0f)
		{
			const float approachCircleImageScale = hitcircleDiameter / (128.0f * (skin->isApproachCircle2x() ? 2.0f : 1.0f));

			g->setColor(comboColor);

			if (osu_circle_rainbow.getBool())
			{
				float frequency = 0.3f;
				float time = engine->getTime()*20;

				const Channel red1	 = std::sin(frequency*time + 0 + rainbowNumber*rainbowColorCounter) * 127 + 128;
				const Channel green1 = std::sin(frequency*time + 2 + rainbowNumber*rainbowColorCounter) * 127 + 128;
				const Channel blue1	 = std::sin(frequency*time + 4 + rainbowNumber*rainbowColorCounter) * 127 + 128;

				g->setColor(rgb(red1, green1, blue1));
			}

			g->setAlpha(alpha*osu_approach_circle_alpha_multiplier.getFloat());

			g->pushTransform();
			{
				g->scale(approachCircleImageScale*approachScale, approachCircleImageScale*approachScale);
				g->translate(pos.x, pos.y);
				g->drawImage(skin->getApproachCircle());
			}
			g->popTransform();
		}
	}
}

void OsuCircle::draw3DApproachCircle(Graphics *g, OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, Color comboColor, float rawHitcircleDiameter, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle)
{
	if ((!modHD || overrideHDApproachCircle) && osu_draw_approach_circles.getBool() && !OsuGameRules::osu_mod_mafham.getBool())
	{
		if (approachScale > 1.0f)
		{
			const float approachCircleImageScale = skin->getApproachCircle()->getSize().x / (128.0f * (skin->isApproachCircle2x() ? 2.0f : 1.0f));

			g->setColor(comboColor);

			if (osu_circle_rainbow.getBool())
			{
				float frequency = 0.3f;
				float time = engine->getTime()*20;

				const Channel red1	 = std::sin(frequency*time + 0 + rainbowNumber*rainbowColorCounter) * 127 + 128;
				const Channel green1 = std::sin(frequency*time + 2 + rainbowNumber*rainbowColorCounter) * 127 + 128;
				const Channel blue1	 = std::sin(frequency*time + 4 + rainbowNumber*rainbowColorCounter) * 127 + 128;

				g->setColor(rgb(red1, green1, blue1));
			}

			g->setAlpha(alpha*osu_approach_circle_alpha_multiplier.getFloat());

			g->pushTransform();
			{
				Matrix4 modelMatrix;
				{
					Matrix4 scale = baseScale;
					scale.scale(approachCircleImageScale * approachScale);

					Matrix4 translation;
					translation.translate(pos.x, pos.y, pos.z);

					if (m_fposu_3d_hitobjects_look_at_player_ref->getBool() && m_fposu_3d_approachcircles_look_at_player_ref->getBool())
						modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - fposu->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale;
					else
						modelMatrix = translation * scale;
				}
				g->setWorldMatrixMul(modelMatrix);

				skin->getApproachCircle()->bind();
				{
					fposu->getUVPlaneModel()->draw3D(g);
				}
				skin->getApproachCircle()->unbind();
			}
			g->popTransform();
		}
	}
}

void OsuCircle::drawHitCircleOverlay(Graphics *g, OsuSkinImage *hitCircleOverlayImage, Vector2 pos, float circleOverlayImageScale, float alpha, float colorRGBMultiplier)
{
	g->setColor(argb(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier));
	g->setAlpha(alpha);
	hitCircleOverlayImage->drawRaw(g, pos, circleOverlayImageScale);
}

void OsuCircle::draw3DHitCircleOverlay(Graphics *g, OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkinImage *hitCircleOverlayImage, Vector3 pos, float alpha, float colorRGBMultiplier)
{
	if (m_fposu_3d_spheres_ref->getBool()) return;

	g->setColor(argb(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier));
	g->setAlpha(alpha);
	g->pushTransform();
	{
		Matrix4 modelMatrix;
		{
			Matrix4 scale = baseScale;
			scale.scale((hitCircleOverlayImage->getImageSizeForCurrentFrame().x / hitCircleOverlayImage->getSizeBaseRaw().x));

			Matrix4 translation;
			translation.translate(pos.x, pos.y, pos.z);

			if (m_fposu_3d_hitobjects_look_at_player_ref->getBool())
				modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - fposu->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale;
			else
				modelMatrix = translation * scale;
		}
		g->setWorldMatrixMul(modelMatrix);

		hitCircleOverlayImage->getImageForCurrentFrame().img->bind();
		{
			fposu->getUVPlaneModel()->draw3D(g);
		}
		hitCircleOverlayImage->getImageForCurrentFrame().img->unbind();
	}
	g->popTransform();
}

void OsuCircle::drawHitCircle(Graphics *g, Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale, float alpha)
{
	g->setColor(comboColor);

	if (osu_circle_rainbow.getBool())
	{
		float frequency = 0.3f;
		float time = engine->getTime()*20;

		const Channel red1	 = std::sin(frequency*time + 0 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		const Channel green1 = std::sin(frequency*time + 2 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		const Channel blue1	 = std::sin(frequency*time + 4 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;

		g->setColor(rgb(red1, green1, blue1));
	}

	g->setAlpha(alpha);

	g->pushTransform();
	{
		g->scale(circleImageScale, circleImageScale);
		g->translate(pos.x, pos.y);
		g->drawImage(hitCircleImage);
	}
	g->popTransform();
}

void OsuCircle::draw3DHitCircle(Graphics *g, OsuModFPoSu *fposu, OsuSkin *skin, const Matrix4 &baseScale, Image *hitCircleImage, Vector3 pos, Color comboColor, float alpha)
{
	g->setColor(comboColor);

	if (osu_circle_rainbow.getBool())
	{
		float frequency = 0.3f;
		float time = engine->getTime()*20;

		const Channel red1	 = std::sin(frequency*time + 0 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		const Channel green1 = std::sin(frequency*time + 2 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		const Channel blue1	 = std::sin(frequency*time + 4 + rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;

		g->setColor(rgb(red1, green1, blue1));
	}

	g->setAlpha(alpha);

	g->pushTransform();
	{
		if (m_fposu_3d_spheres_ref->getBool())
		{
			Matrix4 modelMatrix;
			{
				Matrix4 translation;
				translation.translate(pos.x, pos.y, pos.z);

				if (m_fposu_3d_hitobjects_look_at_player_ref->getBool())
					modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - fposu->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * baseScale;
				else
					modelMatrix = translation * baseScale;
			}
			g->setWorldMatrixMul(modelMatrix);

			Matrix4 modelMatrixInverseTransposed = modelMatrix;
			modelMatrixInverseTransposed.invert();
			modelMatrixInverseTransposed.transpose();

#if defined(MCENGINE_FEATURE_OPENGL) || defined (MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3)
			if constexpr (Env::cfg(REND::GL | REND::GLES2 | REND::GLES32 | REND::GL3))
				glBlendEquation(GL_MAX); // HACKHACK: OpenGL hardcoded
#endif

			fposu->getHitcircleShader()->enable();
			{
				fposu->getHitcircleShader()->setUniformMatrix4fv("modelMatrix", modelMatrix);
				fposu->getHitcircleShader()->setUniformMatrix4fv("modelMatrixInverseTransposed", modelMatrixInverseTransposed);

				fposu->getHitcircleModel()->draw3D(g);
			}
			fposu->getHitcircleShader()->disable();

#if defined(MCENGINE_FEATURE_OPENGL) || defined (MCENGINE_FEATURE_GLES2) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3)
			if constexpr (Env::cfg(REND::GL | REND::GLES2 | REND::GLES32 | REND::GL3))
				glBlendEquation(GL_FUNC_ADD); // HACKHACK: OpenGL hardcoded
#endif
		}
		else
		{
			Matrix4 modelMatrix;
			{
				Matrix4 scale = baseScale;
				scale.scale(hitCircleImage->getSize().x / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f)));

				Matrix4 translation;
				translation.translate(pos.x, pos.y, pos.z);

				if (m_fposu_3d_hitobjects_look_at_player_ref->getBool())
					modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - fposu->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale;
				else
					modelMatrix = translation * scale;
			}
			g->setWorldMatrixMul(modelMatrix);

			hitCircleImage->bind();
			{
				fposu->getUVPlaneModel()->draw3D(g);
			}
			hitCircleImage->unbind();
		}
	}
	g->popTransform();
}

void OsuCircle::drawHitCircleNumber(Graphics *g, OsuSkin *skin, float numberScale, float overlapScale, Vector2 pos, int number, float numberAlpha, float colorRGBMultiplier)
{
	if (!osu_draw_numbers.getBool()) return;

	class DigitWidth
	{
	public:
		static float getWidth(OsuSkin *skin, int digit)
		{
			switch (digit)
			{
			case 0:
				return skin->getDefault0()->getWidth();
			case 1:
				return skin->getDefault1()->getWidth();
			case 2:
				return skin->getDefault2()->getWidth();
			case 3:
				return skin->getDefault3()->getWidth();
			case 4:
				return skin->getDefault4()->getWidth();
			case 5:
				return skin->getDefault5()->getWidth();
			case 6:
				return skin->getDefault6()->getWidth();
			case 7:
				return skin->getDefault7()->getWidth();
			case 8:
				return skin->getDefault8()->getWidth();
			case 9:
				return skin->getDefault9()->getWidth();
			}

			return skin->getDefault0()->getWidth();
		}
	};

	// generate digits
	std::vector<int> digits;
	while (number >= 10)
	{
		digits.push_back(number % 10);
		number = number / 10;
	}
	digits.push_back(number);

	// set color
	//g->setColor(argb(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier)); // see https://github.com/ppy/osu/issues/24506
	g->setColor(0xffffffff);
	if (osu_circle_number_rainbow.getBool())
	{
		float frequency = 0.3f;
		float time = engine->getTime()*20;

		const Channel red1	 = std::sin(frequency*time + 0 + rainbowNumber*rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		const Channel green1 = std::sin(frequency*time + 2 + rainbowNumber*rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;
		const Channel blue1	 = std::sin(frequency*time + 4 + rainbowNumber*rainbowNumber*rainbowNumber*rainbowColorCounter) * 127 + 128;

		g->setColor(rgb(red1, green1, blue1));
	}
	g->setAlpha(numberAlpha);

	// draw digits, start at correct offset
	g->pushTransform();
	{
		g->scale(numberScale, numberScale);
		g->translate(pos.x, pos.y);
		{
			float digitWidthCombined = 0.0f;
			for (size_t i=0; i<digits.size(); i++)
			{
				digitWidthCombined += DigitWidth::getWidth(skin, digits[i]);
			}

			const int digitOverlapCount = digits.size() - 1;
			g->translate(-(digitWidthCombined*numberScale - skin->getHitCircleOverlap()*digitOverlapCount*overlapScale)*0.5f + DigitWidth::getWidth(skin, (digits.size() > 0 ? digits[digits.size()-1] : 0))*numberScale*0.5f, 0);
		}

		for (int i=digits.size()-1; i>=0; i--)
		{
			switch (digits[i])
			{
			case 0:
				g->drawImage(skin->getDefault0());
				break;
			case 1:
				g->drawImage(skin->getDefault1());
				break;
			case 2:
				g->drawImage(skin->getDefault2());
				break;
			case 3:
				g->drawImage(skin->getDefault3());
				break;
			case 4:
				g->drawImage(skin->getDefault4());
				break;
			case 5:
				g->drawImage(skin->getDefault5());
				break;
			case 6:
				g->drawImage(skin->getDefault6());
				break;
			case 7:
				g->drawImage(skin->getDefault7());
				break;
			case 8:
				g->drawImage(skin->getDefault8());
				break;
			case 9:
				g->drawImage(skin->getDefault9());
				break;
			}

			if (i > 0)
				g->translate((DigitWidth::getWidth(skin, digits[i])*numberScale + DigitWidth::getWidth(skin, digits[i-1])*numberScale)*0.5f - skin->getHitCircleOverlap()*overlapScale, 0);			
		}
	}
	g->popTransform();
}

void OsuCircle::draw3DHitCircleNumber(Graphics *g, OsuSkin *skin, float numberScale, float overlapScale, Vector3 pos, int number, float numberAlpha, float colorRGBMultiplier)
{
	if (m_fposu_3d_spheres_ref->getBool()) return;

	// TODO: implement above
}



OsuCircle::OsuCircle(int x, int y, long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, OsuBeatmapStandard *beatmap) : OsuHitObject(time, sampleType, comboNumber, isEndOfCombo, colorCounter, colorOffset, beatmap)
{
	m_vOriginalRawPos = Vector2(x,y);
	m_vRawPos = m_vOriginalRawPos;

	m_beatmap = beatmap;

	m_bWaiting = false;
	m_fHitAnimation = 0.0f;
	m_fShakeAnimation = 0.0f;

	m_bOnHitVRLeftControllerHapticFeedback = false;
}

OsuCircle::~OsuCircle()
{
	onReset(0);
}

void OsuCircle::draw(Graphics *g)
{
	OsuHitObject::draw(g);

	// draw hit animation
	if (m_fHitAnimation > 0.0f && m_fHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
	{
		float alpha = 1.0f - m_fHitAnimation;

		float scale = m_fHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		const bool drawNumber = m_beatmap->getSkin()->getVersion() > 1.0f ? false : true;

		g->pushTransform();
		{
			g->scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()), (1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));
			m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
			drawCircle(g, m_beatmap, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
		}
		g->popTransform();
	}

	if (m_bFinished || (!m_bVisible && !m_bWaiting)) // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object
		return;

	// draw circle
	const bool hd = m_beatmap->getOsu()->getModHD();
	Vector2 shakeCorrectedPos = m_vRawPos;
	if (engine->getTime() < m_fShakeAnimation && !m_beatmap->isInMafhamRenderChunk()) // handle note blocking shaking
	{
		float smooth = 1.0f - ((m_fShakeAnimation - engine->getTime()) / osu_circle_shake_duration.getFloat()); // goes from 0 to 1
		if (smooth < 0.5f)
			smooth = smooth / 0.5f;
		else
			smooth = (1.0f - smooth) / 0.5f;
		// (now smooth goes from 0 to 1 to 0 linearly)
		smooth = -smooth*(smooth-2); // quad out
		smooth = -smooth*(smooth-2); // quad out twice
		shakeCorrectedPos.x += std::sin(engine->getTime()*120) * smooth * osu_circle_shake_strength.getFloat();
	}
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
	drawCircle(g, m_beatmap, shakeCorrectedPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlpha, m_bWaiting && !hd ? 1.0f : m_fAlpha, true, m_bOverrideHDApproachCircle);

	// debug
	/*
	if (std::abs(m_iDelta) <= OsuGameRules::getHitWindowMiss(m_beatmap) && m_iDelta > 0)
	{
		const Vector2 pos = m_beatmap->osuCoords2Pixels(m_vRawPos);

		g->setColor(0xbbff0000);
		g->fillRect(pos.x - 20, pos.y - 20, 40, 40);
	}
	{
		const Vector2 pos = m_beatmap->osuCoords2Pixels(m_vRawPos);

		g->pushTransform();
		{
			g->translate(pos.x + 50, pos.y + 50);
			g->drawString(m_beatmap->getOsu()->getSongBrowserFont(), UString::format("%f", m_fHittableDimRGBColorMultiplierPercent));
		}
		g->popTransform();
	}
	*/
}

void OsuCircle::draw2(Graphics *g)
{
	OsuHitObject::draw2(g);
	if (m_bFinished || (!m_bVisible && !m_bWaiting)) return; // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object

	// draw approach circle
	const bool hd = m_beatmap->getOsu()->getModHD();

	// HACKHACK: don't fucking change this piece of code here, it fixes a heisenbug (https://github.com/McKay42/McOsu/issues/165)
	if (osu_bug_flicker_log.getBool())
	{
		const float approachCircleImageScale = m_beatmap->getHitcircleDiameter() / (128.0f * (m_beatmap->getSkin()->isApproachCircle2x() ? 2.0f : 1.0f));
		debugLog("m_iTime = %ld, aScale = %f, iScale = %f\n", m_iTime, m_fApproachScale, approachCircleImageScale);
	}

	drawApproachCircle(g, m_beatmap, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
}

void OsuCircle::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	// TODO: performance! if nothing of the circle is visible, then we don't have to calculate anything
	///if (m_bVisible)
	{
		float clampedApproachScalePercent = m_fApproachScale - 1.0f; // goes from <m_osu_approach_scale_multiplier_ref> to 0
		clampedApproachScalePercent = std::clamp<float>(clampedApproachScalePercent / m_osu_approach_scale_multiplier_ref->getFloat(), 0.0f, 1.0f); // goes from 1 to 0

		Matrix4 translation;
		translation.translate(0, 0, -clampedApproachScalePercent*vr->getApproachDistance());
		Matrix4 finalMVP = mvp * translation;

		vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", finalMVP);
		draw(g);

		if (m_osu_vr_draw_approach_circles->getBool() && !m_osu_vr_approach_circles_on_top->getBool())
		{
			if (m_osu_vr_approach_circles_on_playfield->getBool())
				vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);

			draw2(g);
		}
	}
}

void OsuCircle::drawVR2(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	// TODO: performance! if nothing of the circle is visible, then we don't have to calculate anything
	///if (m_bVisible)
	{
		float clampedApproachScalePercent = m_fApproachScale - 1.0f; // goes from <m_osu_approach_scale_multiplier_ref> to 0
		clampedApproachScalePercent = std::clamp<float>(clampedApproachScalePercent / m_osu_approach_scale_multiplier_ref->getFloat(), 0.0f, 1.0f); // goes from 1 to 0

		if (m_osu_vr_approach_circles_on_playfield->getBool())
			clampedApproachScalePercent = 0.0f;

		Matrix4 translation;
		translation.translate(0, 0, -clampedApproachScalePercent*vr->getApproachDistance());
		Matrix4 finalMVP = mvp * translation;

		vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", finalMVP);
		draw2(g);
	}
}

void OsuCircle::draw3D(Graphics *g)
{
	OsuHitObject::draw3D(g);

	// draw hit animation
	if (!m_fposu_3d_spheres_ref->getBool())
	{
		if (m_fHitAnimation > 0.0f && m_fHitAnimation != 1.0f && !m_beatmap->getOsu()->getModHD())
		{
			float alpha = 1.0f - m_fHitAnimation;

			float scale = m_fHitAnimation;
			scale = -scale*(scale-2.0f); // quad out scale

			const bool drawNumber = m_beatmap->getSkin()->getVersion() > 1.0f ? false : true;

			Matrix4 baseScale;
			baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
			baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());
			baseScale.scale((1.0f+scale*OsuGameRules::osu_circle_fade_out_scale.getFloat()));

			m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
			draw3DCircle(g, m_beatmap, this, baseScale, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
		}
	}

	if (m_bFinished || (!m_bVisible && !m_bWaiting)) // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object
		return;

	Matrix4 baseScale;
	baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
	baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());

	// draw circle
	const bool hd = m_beatmap->getOsu()->getModHD();
	Vector2 shakeCorrectedRawPos = m_vRawPos;
	if (engine->getTime() < m_fShakeAnimation && !m_beatmap->isInMafhamRenderChunk()) // handle note blocking shaking
	{
		float smooth = 1.0f - ((m_fShakeAnimation - engine->getTime()) / osu_circle_shake_duration.getFloat()); // goes from 0 to 1
		if (smooth < 0.5f)
			smooth = smooth / 0.5f;
		else
			smooth = (1.0f - smooth) / 0.5f;
		// (now smooth goes from 0 to 1 to 0 linearly)
		smooth = -smooth*(smooth-2); // quad out
		smooth = -smooth*(smooth-2); // quad out twice
		shakeCorrectedRawPos.x += std::sin(engine->getTime()*120) * smooth * osu_circle_shake_strength.getFloat();
	}
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
	draw3DCircle(g, m_beatmap, this, baseScale, shakeCorrectedRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, (m_fposu_3d_spheres_ref->getBool() ? 1.0f : m_fHittableDimRGBColorMultiplierPercent), m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlpha, m_bWaiting && !hd ? 1.0f : m_fAlpha, true, m_bOverrideHDApproachCircle);
}

void OsuCircle::draw3D2(Graphics *g)
{
	OsuHitObject::draw3D2(g);
	if (m_bFinished || (!m_bVisible && !m_bWaiting)) return; // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object

	Matrix4 baseScale;
	baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
	baseScale.scale(m_beatmap->getOsu()->getFPoSu()->get3DPlayfieldScale());

	// draw approach circle
	const bool hd = m_beatmap->getOsu()->getModHD();

	draw3DApproachCircle(g, m_beatmap, this, baseScale, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
}

void OsuCircle::update(long curPos)
{
	OsuHitObject::update(curPos);

	// if we have not been clicked yet, check if we are in the timeframe of a miss, also handle auto and relax
	if (!m_bFinished)
	{
		if (m_beatmap->getOsu()->getModAuto())
		{
			if (curPos >= m_iTime)
				onHit(OsuScore::HIT::HIT_300, 0);
		}
		else
		{
			const long delta = curPos - m_iTime;

			if (m_beatmap->getOsu()->getModRelax() || m_beatmap->getOsu()->isInVRMode())
			{
				if (curPos >= m_iTime + (long)m_osu_relax_offset_ref->getInt() && !m_beatmap->isPaused() && !m_beatmap->isContinueScheduled())
				{
					const Vector2 pos = m_beatmap->osuCoords2Pixels(m_vRawPos);
					const float cursorDelta = (m_beatmap->getCursorPos() - pos).length();

					float vrCursor1Delta = 0.0f;
					float vrCursor2Delta = 0.0f;
					bool vrCursor1Inside = false;
					bool vrCursor2Inside = false;
					if (m_beatmap->getOsu()->isInVRMode())
					{
						vrCursor1Delta = (m_beatmap->getOsu()->getVR()->getCursorPos1() - m_beatmap->osuCoords2VRPixels(m_vRawPos)).length();
						vrCursor2Delta = (m_beatmap->getOsu()->getVR()->getCursorPos2() - m_beatmap->osuCoords2VRPixels(m_vRawPos)).length();
						vrCursor1Inside = vrCursor1Delta < ((m_beatmap->getRawHitcircleDiameter()/2.0f) * m_beatmap->getOsu()->getVR()->getCircleHitboxScale());
						vrCursor2Inside = vrCursor2Delta < ((m_beatmap->getRawHitcircleDiameter()/2.0f) * m_beatmap->getOsu()->getVR()->getCircleHitboxScale());
					}

					if ((cursorDelta < m_beatmap->getHitcircleDiameter()/2.0f && m_beatmap->getOsu()->getModRelax()) || (vrCursor1Inside || vrCursor2Inside))
					{
						OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);

						if (result != OsuScore::HIT::HIT_NULL)
						{
							const float targetDelta = cursorDelta / (m_beatmap->getHitcircleDiameter()/2.0f);
							const float targetAngle = glm::degrees(atan2(m_beatmap->getCursorPos().y - pos.y, m_beatmap->getCursorPos().x - pos.x));

							if (m_beatmap->getOsu()->isInVRMode())
							{
								// distance to circle
								if (vrCursor1Delta < vrCursor2Delta)
									m_bOnHitVRLeftControllerHapticFeedback = true;
								else
									m_bOnHitVRLeftControllerHapticFeedback = false;

								// distance to playfield, if both cursors were valid (overrides distance to circle for haptic feedback)
								if (vrCursor1Inside && vrCursor2Inside)
								{
									if (m_beatmap->getOsu()->getVR()->getCursorDist1() < m_beatmap->getOsu()->getVR()->getCursorDist2())
										m_bOnHitVRLeftControllerHapticFeedback = true;
									else
										m_bOnHitVRLeftControllerHapticFeedback = false;
								}
							}

							onHit(result, delta, targetDelta, targetAngle);
						}
					}
				}
			}

			if (delta >= 0)
			{
				m_bWaiting = true;

				// if this is a miss after waiting
				if (delta > (long)OsuGameRules::getHitWindow50(m_beatmap))
					onHit(OsuScore::HIT::HIT_MISS, delta);
			}
			else
				m_bWaiting = false;
		}
	}
}

void OsuCircle::updateStackPosition(float stackOffset)
{
	m_vRawPos = m_vOriginalRawPos - Vector2(m_iStack * stackOffset, m_iStack * stackOffset * (m_beatmap->getOsu()->getModHR() ? -1.0f : 1.0f));
}

void OsuCircle::miss(long curPos)
{
	if (m_bFinished) return;

	const long delta = curPos - m_iTime;

	onHit(OsuScore::HIT::HIT_MISS, delta);
}

void OsuCircle::onClickEvent(std::vector<OsuBeatmap::CLICK> &clicks)
{
	if (m_bFinished) return;

	const Vector2 cursorPos = m_beatmap->getCursorPos();

	const Vector2 pos = m_beatmap->osuCoords2Pixels(m_vRawPos);
	const float cursorDelta = (cursorPos - pos).length();

	if (cursorDelta < m_beatmap->getHitcircleDiameter()/2.0f)
	{
		// note blocking & shake
		if (m_bBlocked)
		{
			m_fShakeAnimation = engine->getTime() + osu_circle_shake_duration.getFloat();
			return; // ignore click event completely
		}

		const long delta = (long)clicks[0].musicPos - (long)m_iTime;

		OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);
		if (result != OsuScore::HIT::HIT_NULL)
		{
			const float targetDelta = cursorDelta / (m_beatmap->getHitcircleDiameter()/2.0f);
			const float targetAngle = glm::degrees(atan2(cursorPos.y - pos.y, cursorPos.x - pos.x));

			clicks.erase(clicks.begin());
			onHit(result, delta, targetDelta, targetAngle);
		}
	}
}

void OsuCircle::onHit(OsuScore::HIT result, long delta, float targetDelta, float targetAngle)
{
	// sound and hit animation
	if (result != OsuScore::HIT::HIT_MISS)
	{
		if (m_osu_timingpoints_force->getBool())
			m_beatmap->updateTimingPoints(m_iTime);

		const Vector2 osuCoords = m_beatmap->pixels2OsuCoords(m_beatmap->osuCoords2Pixels(m_vRawPos));

		m_beatmap->getSkin()->playHitCircleSound(m_iSampleType, OsuGameRules::osuCoords2Pan(osuCoords.x));

		m_fHitAnimation = 0.001f; // quickfix for 1 frame missing images
		anim->moveQuadOut(&m_fHitAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);

		if (m_beatmap->getOsu()->isInVRMode())
		{
			if (m_bOnHitVRLeftControllerHapticFeedback)
				openvr->getLeftController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getHapticPulseStrength());
			else
				openvr->getRightController()->triggerHapticPulse(m_beatmap->getOsu()->getVR()->getHapticPulseStrength());
		}
	}

	// add it, and we are finished
	addHitResult(result, delta, m_bIsEndOfCombo, m_vRawPos, targetDelta, targetAngle);
	m_bFinished = true;
}

void OsuCircle::onReset(long curPos)
{
	OsuHitObject::onReset(curPos);

	m_bWaiting = false;
	m_fShakeAnimation = 0.0f;

	anim->deleteExistingAnimation(&m_fHitAnimation);

	if (m_iTime > curPos)
	{
		m_bFinished = false;
		m_fHitAnimation = 0.0f;
	}
	else
	{
		m_bFinished = true;
		m_fHitAnimation = 1.0f;
	}
}

Vector2 OsuCircle::getAutoCursorPos(long curPos) const
{
	return m_beatmap->osuCoords2Pixels(m_vRawPos);
}
