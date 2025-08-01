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
#include "Camera.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuGameRules.h"
#include "OsuBeatmapStandard.h"
#include "OsuModFPoSu.h"

#include "OpenGLHeaders.h"
namespace cv::osu {
ConVar bug_flicker_log("osu_bug_flicker_log", false, FCVAR_NONE);

ConVar circle_color_saturation("osu_circle_color_saturation", 1.0f, FCVAR_NONE);
ConVar circle_rainbow("osu_circle_rainbow", false, FCVAR_NONE);
ConVar circle_number_rainbow("osu_circle_number_rainbow", false, FCVAR_NONE);
ConVar circle_shake_duration("osu_circle_shake_duration", 0.120f, FCVAR_NONE);
ConVar circle_shake_strength("osu_circle_shake_strength", 8.0f, FCVAR_NONE);
ConVar approach_circle_alpha_multiplier("osu_approach_circle_alpha_multiplier", 0.9f, FCVAR_NONE);

ConVar draw_numbers("osu_draw_numbers", true, FCVAR_NONE);
ConVar draw_circles("osu_draw_circles", true, FCVAR_NONE);
ConVar draw_approach_circles("osu_draw_approach_circles", true, FCVAR_NONE);

ConVar slider_draw_endcircle("osu_slider_draw_endcircle", true, FCVAR_NONE);
}

int OsuCircle::rainbowNumber = 0;
int OsuCircle::rainbowColorCounter = 0;

void OsuCircle::drawApproachCircle(OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, bool overrideHDApproachCircle)
{
	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = Colors::scale(beatmap->getSkin()->getComboColorForCounter(colorCounter, colorOffset), colorRGBMultiplier*cv::osu::circle_color_saturation.getFloat());

	drawApproachCircle(beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), comboColor, beatmap->getHitcircleDiameter(), approachScale, alpha, osu->getModHD(), overrideHDApproachCircle);
}

void OsuCircle::draw3DApproachCircle(OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, bool overrideHDApproachCircle)
{
	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = Colors::scale(beatmap->getSkin()->getComboColorForCounter(colorCounter, colorOffset), colorRGBMultiplier*cv::osu::circle_color_saturation.getFloat());

	draw3DApproachCircle(osu->getFPoSu(), baseScale, beatmap->getSkin(), beatmap->osuCoordsTo3D(rawPos, hitObject), comboColor, beatmap->getRawHitcircleDiameter(), approachScale, alpha, osu->getModHD(), overrideHDApproachCircle);
}

void OsuCircle::drawCircle(OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawCircle(beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::draw3DCircle(OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	draw3DCircle(osu->getFPoSu(), baseScale, beatmap->getSkin(), beatmap->osuCoordsTo3D(rawPos, hitObject), beatmap->getRawHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawCircle(OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float  /*approachScale*/, float alpha, float numberAlpha, bool drawNumber, bool  /*overrideHDApproachCircle*/)
{
	if (alpha <= 0.0f || !cv::osu::draw_circles.getBool()) return;

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = Colors::scale(skin->getComboColorForCounter(colorCounter, colorOffset), colorRGBMultiplier*cv::osu::circle_color_saturation.getFloat());

	// approach circle
	///drawApproachCircle(skin, pos, comboColor, hitcircleDiameter, approachScale, alpha, modHD, overrideHDApproachCircle); // they are now drawn separately in draw2()

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(skin->getHitCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getHitCircleOverlay2()->getSizeBaseRaw().x;
	if (!skin->getHitCircleOverlayAboveNumber())
		drawHitCircleOverlay(skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);

	// number
	if (drawNumber)
		drawHitCircleNumber(skin, numberScale, overlapScale, pos, number, numberAlpha, colorRGBMultiplier);

	// overlay
	if (skin->getHitCircleOverlayAboveNumber())
		drawHitCircleOverlay(skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
}

void OsuCircle::draw3DCircle(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float  /*rawHitcircleDiameter*/, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float  /*approachScale*/, float alpha, float numberAlpha, bool drawNumber, bool  /*overrideHDApproachCircle*/)
{
	if (alpha <= 0.0f || !cv::osu::draw_circles.getBool()) return;

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = Colors::scale(skin->getComboColorForCounter(colorCounter, colorOffset), colorRGBMultiplier*cv::osu::circle_color_saturation.getFloat());

	// approach circle
	///draw3DApproachCircle(fposu, baseScale, skin, pos, comboColor, rawHitcircleDiameter, approachScale, alpha, modHD, overrideHDApproachCircle); // they are now drawn separately in draw3D2()

	// circle
	draw3DHitCircle(fposu, skin, baseScale, skin->getHitCircle(), pos, comboColor, alpha);

	// overlay
	if (!skin->getHitCircleOverlayAboveNumber())
		draw3DHitCircleOverlay(fposu, baseScale, skin->getHitCircleOverlay2(), pos, alpha, colorRGBMultiplier);

	// number
	if (drawNumber)
		draw3DHitCircleNumber(skin, numberScale, overlapScale, pos, number, numberAlpha, colorRGBMultiplier);

	// overlay
	if (skin->getHitCircleOverlayAboveNumber())
		draw3DHitCircleOverlay(fposu, baseScale, skin->getHitCircleOverlay2(), pos, alpha, colorRGBMultiplier);
}

void OsuCircle::drawCircle(OsuSkin *skin, Vector2 pos, float hitcircleDiameter, Color color, float alpha)
{
	// this function is only used by the target practice heatmap

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isHitCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(skin->getHitCircle(), pos, color, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getHitCircleOverlay2()->getSizeBaseRaw().x;
	drawHitCircleOverlay(skin->getHitCircleOverlay2(), pos, circleOverlayImageScale, alpha, 1.0f);
}

void OsuCircle::drawSliderStartCircle(OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawSliderStartCircle(beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::draw3DSliderStartCircle(OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	draw3DSliderStartCircle(osu->getFPoSu(), baseScale, beatmap->getSkin(), beatmap->osuCoordsTo3D(rawPos, hitObject), beatmap->getRawHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawSliderStartCircle(OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float hitcircleOverlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !cv::osu::draw_circles.getBool()) return;

	// if no sliderstartcircle image is preset, fallback to default circle
	if (skin->getSliderStartCircle() == skin->getMissingTexture())
	{
		drawCircle(skin, pos, hitcircleDiameter, numberScale, hitcircleOverlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle); // normal
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = Colors::scale(skin->getComboColorForCounter(colorCounter, colorOffset), colorRGBMultiplier*cv::osu::circle_color_saturation.getFloat());

	// approach circle
	///drawApproachCircle(skin, pos, comboColor, beatmap->getHitcircleDiameter(), approachScale, alpha, osu->getModHD(), overrideHDApproachCircle); // they are now drawn separately in draw2()

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isSliderStartCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(skin->getSliderStartCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	const float circleOverlayImageScale = hitcircleDiameter / skin->getSliderStartCircleOverlay2()->getSizeBaseRaw().x;
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (!skin->getHitCircleOverlayAboveNumber())
			drawHitCircleOverlay(skin->getSliderStartCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
	}

	// number
	if (drawNumber)
		drawHitCircleNumber(skin, numberScale, hitcircleOverlapScale, pos, number, numberAlpha, colorRGBMultiplier);

	// overlay
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (skin->getHitCircleOverlayAboveNumber())
			drawHitCircleOverlay(skin->getSliderStartCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
	}
}

void OsuCircle::draw3DSliderStartCircle(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float rawHitcircleDiameter, float numberScale, float hitcircleOverlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !cv::osu::draw_circles.getBool()) return;

	// if no sliderstartcircle image is preset, fallback to default circle
	if (skin->getSliderStartCircle() == skin->getMissingTexture())
	{
		draw3DCircle(fposu, baseScale, skin, pos, rawHitcircleDiameter, numberScale, hitcircleOverlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle); // normal
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = Colors::scale(skin->getComboColorForCounter(colorCounter, colorOffset), colorRGBMultiplier*cv::osu::circle_color_saturation.getFloat());

	// approach circle
	///drawApproachCircle(skin, pos, comboColor, beatmap->getHitcircleDiameter(), approachScale, alpha, osu->getModHD(), overrideHDApproachCircle); // they are now drawn separately in draw3D2()

	// circle
	draw3DHitCircle(fposu, skin, baseScale, skin->getHitCircle(), pos, comboColor, alpha);

	// overlay
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (!skin->getHitCircleOverlayAboveNumber())
			draw3DHitCircleOverlay(fposu, baseScale, skin->getHitCircleOverlay2(), pos, alpha, colorRGBMultiplier);
	}

	// number
	if (drawNumber)
		draw3DHitCircleNumber(skin, numberScale, hitcircleOverlapScale, pos, number, numberAlpha, colorRGBMultiplier);

	// overlay
	if (skin->getSliderStartCircleOverlay() != skin->getMissingTexture())
	{
		if (skin->getHitCircleOverlayAboveNumber())
			draw3DHitCircleOverlay(fposu, baseScale, skin->getHitCircleOverlay2(), pos, alpha, colorRGBMultiplier);
	}
}

void OsuCircle::drawSliderEndCircle(OsuBeatmapStandard *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	drawSliderEndCircle(beatmap->getSkin(), beatmap->osuCoords2Pixels(rawPos), beatmap->getHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::draw3DSliderEndCircle(OsuBeatmapStandard *beatmap, const OsuHitObject *hitObject, const Matrix4 &baseScale, Vector2 rawPos, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	draw3DSliderEndCircle(osu->getFPoSu(), baseScale, beatmap->getSkin(), beatmap->osuCoordsTo3D(rawPos, hitObject), beatmap->getRawHitcircleDiameter(), beatmap->getNumberScale(), beatmap->getHitcircleOverlapScale(), number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
}

void OsuCircle::drawSliderEndCircle(OsuSkin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (alpha <= 0.0f || !cv::osu::slider_draw_endcircle.getBool() || !cv::osu::draw_circles.getBool()) return;

	// if no sliderendcircle image is preset, fallback to default circle
	if (skin->getSliderEndCircle() == skin->getMissingTexture())
	{
		drawCircle(skin, pos, hitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = Colors::scale(skin->getComboColorForCounter(colorCounter, colorOffset), colorRGBMultiplier*cv::osu::circle_color_saturation.getFloat());

	// circle
	const float circleImageScale = hitcircleDiameter / (128.0f * (skin->isSliderEndCircle2x() ? 2.0f : 1.0f));
	drawHitCircle(skin->getSliderEndCircle(), pos, comboColor, circleImageScale, alpha);

	// overlay
	if (skin->getSliderEndCircleOverlay() != skin->getMissingTexture())
	{
		const float circleOverlayImageScale = hitcircleDiameter / skin->getSliderEndCircleOverlay2()->getSizeBaseRaw().x;
		drawHitCircleOverlay(skin->getSliderEndCircleOverlay2(), pos, circleOverlayImageScale, alpha, colorRGBMultiplier);
	}
}

void OsuCircle::draw3DSliderEndCircle(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, float rawHitcircleDiameter, float numberScale, float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha, bool drawNumber, bool overrideHDApproachCircle)
{
	if (cv::osu::fposu::threeD_spheres.getBool()) return;
	if (alpha <= 0.0f || !cv::osu::slider_draw_endcircle.getBool() || !cv::osu::draw_circles.getBool()) return;

	// if no sliderendcircle image is preset, fallback to default circle
	if (skin->getSliderEndCircle() == skin->getMissingTexture())
	{
		draw3DCircle(fposu, baseScale, skin, pos, rawHitcircleDiameter, numberScale, overlapScale, number, colorCounter, colorOffset, colorRGBMultiplier, approachScale, alpha, numberAlpha, drawNumber, overrideHDApproachCircle);
		return;
	}

	rainbowNumber = number;
	rainbowColorCounter = colorCounter;

	Color comboColor = Colors::scale(skin->getComboColorForCounter(colorCounter, colorOffset), colorRGBMultiplier*cv::osu::circle_color_saturation.getFloat());

	// circle
	draw3DHitCircle(fposu, skin, baseScale, skin->getSliderEndCircle(), pos, comboColor, alpha);

	// overlay
	if (skin->getSliderEndCircleOverlay() != skin->getMissingTexture())
		draw3DHitCircleOverlay(fposu, baseScale, skin->getSliderEndCircleOverlay2(), pos, alpha, colorRGBMultiplier);
}

void OsuCircle::drawApproachCircle(OsuSkin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle)
{
	if ((!modHD || overrideHDApproachCircle) && cv::osu::draw_approach_circles.getBool() && !cv::osu::stdrules::mod_mafham.getBool())
	{
		if (approachScale > 1.0f)
		{
			const float approachCircleImageScale = hitcircleDiameter / (128.0f * (skin->isApproachCircle2x() ? 2.0f : 1.0f));

			g->setColor(comboColor);

			if (cv::osu::circle_rainbow.getBool())
			{
				float frequency = 0.3f;
				float time = engine->getTime()*20;

				const Channel red1	 = std::sin(frequency*time + 0 + rainbowNumber*rainbowColorCounter) * 127 + 128;
				const Channel green1 = std::sin(frequency*time + 2 + rainbowNumber*rainbowColorCounter) * 127 + 128;
				const Channel blue1	 = std::sin(frequency*time + 4 + rainbowNumber*rainbowColorCounter) * 127 + 128;

				g->setColor(rgb(red1, green1, blue1));
			}

			g->setAlpha(alpha*cv::osu::approach_circle_alpha_multiplier.getFloat());

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

void OsuCircle::draw3DApproachCircle(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkin *skin, Vector3 pos, Color comboColor, float  /*rawHitcircleDiameter*/, float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle)
{
	if ((!modHD || overrideHDApproachCircle) && cv::osu::draw_approach_circles.getBool() && !cv::osu::stdrules::mod_mafham.getBool())
	{
		if (approachScale > 1.0f)
		{
			const float approachCircleImageScale = skin->getApproachCircle()->getSize().x / (128.0f * (skin->isApproachCircle2x() ? 2.0f : 1.0f));

			g->setColor(comboColor);

			if (cv::osu::circle_rainbow.getBool())
			{
				float frequency = 0.3f;
				float time = engine->getTime()*20;

				const Channel red1	 = std::sin(frequency*time + 0 + rainbowNumber*rainbowColorCounter) * 127 + 128;
				const Channel green1 = std::sin(frequency*time + 2 + rainbowNumber*rainbowColorCounter) * 127 + 128;
				const Channel blue1	 = std::sin(frequency*time + 4 + rainbowNumber*rainbowColorCounter) * 127 + 128;

				g->setColor(rgb(red1, green1, blue1));
			}

			g->setAlpha(alpha*cv::osu::approach_circle_alpha_multiplier.getFloat());

			g->pushTransform();
			{
				Matrix4 modelMatrix;
				{
					Matrix4 scale = baseScale;
					scale.scale(approachCircleImageScale * approachScale);

					Matrix4 translation;
					translation.translate(pos.x, pos.y, pos.z);

					if (cv::osu::fposu::threeD_hitobjects_look_at_player.getBool() && cv::osu::fposu::threeD_approachcircles_look_at_player.getBool())
						modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - fposu->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale;
					else
						modelMatrix = translation * scale;
				}
				g->setWorldMatrixMul(modelMatrix);

				skin->getApproachCircle()->bind();
				{
					fposu->getUVPlaneModel()->draw3D();
				}
				skin->getApproachCircle()->unbind();
			}
			g->popTransform();
		}
	}
}

void OsuCircle::drawHitCircleOverlay(OsuSkinImage *hitCircleOverlayImage, Vector2 pos, float circleOverlayImageScale, float alpha, float colorRGBMultiplier)
{
	g->setColor(argb(1.0f, colorRGBMultiplier, colorRGBMultiplier, colorRGBMultiplier));
	g->setAlpha(alpha);
	hitCircleOverlayImage->drawRaw(pos, circleOverlayImageScale);
}

void OsuCircle::draw3DHitCircleOverlay(const OsuModFPoSu *fposu, const Matrix4 &baseScale, OsuSkinImage *hitCircleOverlayImage, Vector3 pos, float alpha, float colorRGBMultiplier)
{
	if (cv::osu::fposu::threeD_spheres.getBool()) return;

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

			if (cv::osu::fposu::threeD_hitobjects_look_at_player.getBool())
				modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - fposu->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale;
			else
				modelMatrix = translation * scale;
		}
		g->setWorldMatrixMul(modelMatrix);

		hitCircleOverlayImage->getImageForCurrentFrame().img->bind();
		{
			fposu->getUVPlaneModel()->draw3D();
		}
		hitCircleOverlayImage->getImageForCurrentFrame().img->unbind();
	}
	g->popTransform();
}

void OsuCircle::drawHitCircle(Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale, float alpha)
{
	g->setColor(comboColor);

	if (cv::osu::circle_rainbow.getBool())
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

void OsuCircle::draw3DHitCircle(const OsuModFPoSu *fposu, OsuSkin *skin, const Matrix4 &baseScale, Image *hitCircleImage, Vector3 pos, Color comboColor, float alpha)
{
	g->setColor(comboColor);

	if (cv::osu::circle_rainbow.getBool())
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
		if (cv::osu::fposu::threeD_spheres.getBool())
		{
			Matrix4 modelMatrix;
			{
				Matrix4 translation;
				translation.translate(pos.x, pos.y, pos.z);

				if (cv::osu::fposu::threeD_hitobjects_look_at_player.getBool())
					modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - fposu->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * baseScale;
				else
					modelMatrix = translation * baseScale;
			}
			g->setWorldMatrixMul(modelMatrix);

			Matrix4 modelMatrixInverseTransposed = modelMatrix;
			modelMatrixInverseTransposed.invert();
			modelMatrixInverseTransposed.transpose();

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3)
			if constexpr (Env::cfg(REND::GL | REND::GLES32 | REND::GL3))
				glBlendEquation(GL_MAX); // HACKHACK: OpenGL hardcoded
#endif

			fposu->getHitcircleShader()->enable();
			{
				fposu->getHitcircleShader()->setUniformMatrix4fv("modelMatrix", modelMatrix);
				fposu->getHitcircleShader()->setUniformMatrix4fv("modelMatrixInverseTransposed", modelMatrixInverseTransposed);

				fposu->getHitcircleModel()->draw3D();
			}
			fposu->getHitcircleShader()->disable();

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32) || defined(MCENGINE_FEATURE_GL3)
			if constexpr (Env::cfg(REND::GL | REND::GLES32 | REND::GL3))
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

				if (cv::osu::fposu::threeD_hitobjects_look_at_player.getBool())
					modelMatrix = translation * Camera::buildMatrixLookAt(Vector3(0, 0, 0), pos - fposu->getCamera()->getPos(), Vector3(0, 1, 0)).invert() * scale;
				else
					modelMatrix = translation * scale;
			}
			g->setWorldMatrixMul(modelMatrix);

			hitCircleImage->bind();
			{
				fposu->getUVPlaneModel()->draw3D();
			}
			hitCircleImage->unbind();
		}
	}
	g->popTransform();
}

void OsuCircle::drawHitCircleNumber(OsuSkin *skin, float numberScale, float overlapScale, Vector2 pos, int number, float numberAlpha, float  /*colorRGBMultiplier*/)
{
	if (!cv::osu::draw_numbers.getBool()) return;

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
	if (cv::osu::circle_number_rainbow.getBool())
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

void OsuCircle::draw3DHitCircleNumber(OsuSkin * /*skin*/, float  /*numberScale*/, float  /*overlapScale*/, Vector3  /*pos*/, int  /*number*/, float  /*numberAlpha*/, float  /*colorRGBMultiplier*/)
{
	if (cv::osu::fposu::threeD_spheres.getBool()) return;

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
}

OsuCircle::~OsuCircle()
{
	onReset(0);
}

void OsuCircle::draw()
{
	OsuHitObject::draw();

	// draw hit animation
	if (m_fHitAnimation > 0.0f && m_fHitAnimation != 1.0f && !osu->getModHD())
	{
		float alpha = 1.0f - m_fHitAnimation;

		float scale = m_fHitAnimation;
		scale = -scale*(scale-2.0f); // quad out scale

		const bool drawNumber = m_beatmap->getSkin()->getVersion() > 1.0f ? false : true;

		g->pushTransform();
		{
			g->scale((1.0f+scale*cv::osu::stdrules::circle_fade_out_scale.getFloat()), (1.0f+scale*cv::osu::stdrules::circle_fade_out_scale.getFloat()));
			m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
			drawCircle(m_beatmap, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
		}
		g->popTransform();
	}

	if (m_bFinished || (!m_bVisible && !m_bWaiting)) // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object
		return;

	// draw circle
	const bool hd = osu->getModHD();
	Vector2 shakeCorrectedPos = m_vRawPos;
	if (engine->getTime() < m_fShakeAnimation && !m_beatmap->isInMafhamRenderChunk()) // handle note blocking shaking
	{
		float smooth = 1.0f - ((m_fShakeAnimation - engine->getTime()) / cv::osu::circle_shake_duration.getFloat()); // goes from 0 to 1
		if (smooth < 0.5f)
			smooth = smooth / 0.5f;
		else
			smooth = (1.0f - smooth) / 0.5f;
		// (now smooth goes from 0 to 1 to 0 linearly)
		smooth = -smooth*(smooth-2); // quad out
		smooth = -smooth*(smooth-2); // quad out twice
		shakeCorrectedPos.x += std::sin(engine->getTime()*120) * smooth * cv::osu::circle_shake_strength.getFloat();
	}
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
	drawCircle(m_beatmap, shakeCorrectedPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlpha, m_bWaiting && !hd ? 1.0f : m_fAlpha, true, m_bOverrideHDApproachCircle);

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
			g->drawString(osu->getSongBrowserFont(), UString::format("%f", m_fHittableDimRGBColorMultiplierPercent));
		}
		g->popTransform();
	}
	*/
}

void OsuCircle::draw2()
{
	OsuHitObject::draw2();
	if (m_bFinished || (!m_bVisible && !m_bWaiting)) return; // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object

	// draw approach circle
	const bool hd = osu->getModHD();

	// HACKHACK: don't fucking change this piece of code here, it fixes a heisenbug (https://github.com/McKay42/McOsu/issues/165)
	if (cv::osu::bug_flicker_log.getBool())
	{
		const float approachCircleImageScale = m_beatmap->getHitcircleDiameter() / (128.0f * (m_beatmap->getSkin()->isApproachCircle2x() ? 2.0f : 1.0f));
		debugLog("m_iTime = {}, aScale = {}, iScale = {}\n", m_iTime, m_fApproachScale, approachCircleImageScale);
	}

	drawApproachCircle(m_beatmap, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
}

void OsuCircle::draw3D()
{
	OsuHitObject::draw3D();

	// draw hit animation
	if (!cv::osu::fposu::threeD_spheres.getBool())
	{
		if (m_fHitAnimation > 0.0f && m_fHitAnimation != 1.0f && !osu->getModHD())
		{
			float alpha = 1.0f - m_fHitAnimation;

			float scale = m_fHitAnimation;
			scale = -scale*(scale-2.0f); // quad out scale

			const bool drawNumber = m_beatmap->getSkin()->getVersion() > 1.0f ? false : true;

			Matrix4 baseScale;
			baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
			baseScale.scale(osu->getFPoSu()->get3DPlayfieldScale());
			baseScale.scale((1.0f+scale*cv::osu::stdrules::circle_fade_out_scale.getFloat()));

			m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
			draw3DCircle(m_beatmap, this, baseScale, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, 1.0f, 1.0f, alpha, alpha, drawNumber);
		}
	}

	if (m_bFinished || (!m_bVisible && !m_bWaiting)) // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object
		return;

	Matrix4 baseScale;
	baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
	baseScale.scale(osu->getFPoSu()->get3DPlayfieldScale());

	// draw circle
	const bool hd = osu->getModHD();
	Vector2 shakeCorrectedRawPos = m_vRawPos;
	if (engine->getTime() < m_fShakeAnimation && !m_beatmap->isInMafhamRenderChunk()) // handle note blocking shaking
	{
		float smooth = 1.0f - ((m_fShakeAnimation - engine->getTime()) / cv::osu::circle_shake_duration.getFloat()); // goes from 0 to 1
		if (smooth < 0.5f)
			smooth = smooth / 0.5f;
		else
			smooth = (1.0f - smooth) / 0.5f;
		// (now smooth goes from 0 to 1 to 0 linearly)
		smooth = -smooth*(smooth-2); // quad out
		smooth = -smooth*(smooth-2); // quad out twice
		shakeCorrectedRawPos.x += std::sin(engine->getTime()*120) * smooth * cv::osu::circle_shake_strength.getFloat();
	}
	m_beatmap->getSkin()->getHitCircleOverlay2()->setAnimationTimeOffset(!m_beatmap->isInMafhamRenderChunk() ? m_iTime - m_iApproachTime : m_beatmap->getCurMusicPosWithOffsets());
	draw3DCircle(m_beatmap, this, baseScale, shakeCorrectedRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, (cv::osu::fposu::threeD_spheres.getBool() ? 1.0f : m_fHittableDimRGBColorMultiplierPercent), m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlpha, m_bWaiting && !hd ? 1.0f : m_fAlpha, true, m_bOverrideHDApproachCircle);
}

void OsuCircle::draw3D2()
{
	OsuHitObject::draw3D2();
	if (m_bFinished || (!m_bVisible && !m_bWaiting)) return; // special case needed for when we are past this objects time, but still within not-miss range, because we still need to draw the object

	Matrix4 baseScale;
	baseScale.scale(m_beatmap->getRawHitcircleDiameter() * OsuModFPoSu::SIZEDIV3D);
	baseScale.scale(osu->getFPoSu()->get3DPlayfieldScale());

	// draw approach circle
	const bool hd = osu->getModHD();

	draw3DApproachCircle(m_beatmap, this, baseScale, m_vRawPos, m_iComboNumber, m_iColorCounter, m_iColorOffset, m_fHittableDimRGBColorMultiplierPercent, m_bWaiting && !hd ? 1.0f : m_fApproachScale, m_bWaiting && !hd ? 1.0f : m_fAlphaForApproachCircle, m_bOverrideHDApproachCircle);
}

void OsuCircle::update(long curPos)
{
	OsuHitObject::update(curPos);

	// if we have not been clicked yet, check if we are in the timeframe of a miss, also handle auto and relax
	if (!m_bFinished)
	{
		if (osu->getModAuto())
		{
			if (curPos >= m_iTime)
				onHit(OsuScore::HIT::HIT_300, 0);
		}
		else
		{
			const long delta = curPos - m_iTime;

			if (osu->getModRelax())
			{
				if (curPos >= m_iTime + (long)cv::osu::relax_offset.getInt() && !m_beatmap->isPaused() && !m_beatmap->isContinueScheduled())
				{
					const Vector2 pos = m_beatmap->osuCoords2Pixels(m_vRawPos);
					const float cursorDelta = (m_beatmap->getCursorPos() - pos).length();

					if ((cursorDelta < m_beatmap->getHitcircleDiameter()/2.0f && osu->getModRelax()))
					{
						OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);

						if (result != OsuScore::HIT::HIT_NULL)
						{
							const float targetDelta = cursorDelta / (m_beatmap->getHitcircleDiameter()/2.0f);
							const float targetAngle = glm::degrees(glm::atan2(m_beatmap->getCursorPos().y - pos.y, m_beatmap->getCursorPos().x - pos.x));

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
	m_vRawPos = m_vOriginalRawPos - Vector2(m_iStack * stackOffset, m_iStack * stackOffset * (osu->getModHR() ? -1.0f : 1.0f));
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
			m_fShakeAnimation = engine->getTime() + cv::osu::circle_shake_duration.getFloat();
			return; // ignore click event completely
		}

		const long delta = (long)clicks[0].musicPos - (long)m_iTime;

		OsuScore::HIT result = OsuGameRules::getHitResult(delta, m_beatmap);
		if (result != OsuScore::HIT::HIT_NULL)
		{
			const float targetDelta = cursorDelta / (m_beatmap->getHitcircleDiameter()/2.0f);
			const float targetAngle = glm::degrees(glm::atan2(cursorPos.y - pos.y, cursorPos.x - pos.x));

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
		if (cv::osu::timingpoints_force.getBool())
			m_beatmap->updateTimingPoints(m_iTime);

		const Vector2 osuCoords = m_beatmap->pixels2OsuCoords(m_beatmap->osuCoords2Pixels(m_vRawPos));

		m_beatmap->getSkin()->playHitCircleSound(m_iSampleType, OsuGameRules::osuCoords2Pan(osuCoords.x));

		m_fHitAnimation = 0.001f; // quickfix for 1 frame missing images
		anim->moveQuadOut(&m_fHitAnimation, 1.0f, OsuGameRules::getFadeOutTime(m_beatmap), true);
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

Vector2 OsuCircle::getAutoCursorPos(long  /*curPos*/) const
{
	return m_beatmap->osuCoords2Pixels(m_vRawPos);
}
