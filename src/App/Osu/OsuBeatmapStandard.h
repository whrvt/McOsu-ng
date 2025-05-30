//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		osu!standard circle clicking
//
// $NoKeywords: $osustd
//===============================================================================//

#pragma once
#ifndef OSUBEATMAPSTANDARD_H
#define OSUBEATMAPSTANDARD_H

#include "OsuBeatmap.h"

class OsuBackgroundStarCacheLoader;

class OsuBeatmapStandard final : public OsuBeatmap
{
public:
	OsuBeatmapStandard();
	~OsuBeatmapStandard() override;

	void draw() override;
	void drawInt() override;
	void draw3D() override;
	void draw3D2() override;
	void update() override;

	void onModUpdate() override {onModUpdate(true, true);}
	void onModUpdate(bool rebuildSliderVertexBuffers = true, bool recomputeDrainRate = true); // this seems very dangerous compiler-wise, but it works
	[[nodiscard]] bool isLoading() const override;

	[[nodiscard]] Type getType() const override { return STANDARD; }

	OsuBeatmapStandard* asStd() override { return this; }
	[[nodiscard]] const OsuBeatmapStandard* asStd() const override { return this; }

	[[nodiscard]] Vector2 pixels2OsuCoords(Vector2 pixelCoords) const; // only used for positional audio atm
	[[nodiscard]] Vector2 osuCoords2Pixels(Vector2 coords) const; // hitobjects should use this one (includes lots of special behaviour)
	[[nodiscard]] Vector2 osuCoords2RawPixels(Vector2 coords) const; // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
	[[nodiscard]] Vector3 osuCoordsTo3D(Vector2 coords, const OsuHitObject *hitObject) const;
	[[nodiscard]] Vector3 osuCoordsToRaw3D(Vector2 coords) const; // (without any mods whatsoever)
	[[nodiscard]] Vector2 osuCoords2LegacyPixels(Vector2 coords) const; // only applies vanilla osu mods and static mods to the coordinates (used for generating the static slider mesh) centered at (0, 0, 0)

	// cursor
	[[nodiscard]] Vector2 getCursorPos() const;
	[[nodiscard]] Vector2 getFirstPersonCursorDelta() const;
	[[nodiscard]] inline Vector2 getContinueCursorPoint() const {return m_vContinueCursorPoint;}

	// playfield
	[[nodiscard]] inline Vector2 getPlayfieldSize() const {return m_vPlayfieldSize;}
	[[nodiscard]] inline Vector2 getPlayfieldCenter() const {return m_vPlayfieldCenter;}
	[[nodiscard]] inline float getPlayfieldRotation() const {return m_fPlayfieldRotation;}

	// hitobjects
	[[nodiscard]] float getHitcircleDiameter() const; // in actual scaled pixels to the current resolution
	[[nodiscard]] inline float getRawHitcircleDiameter() const {return m_fRawHitcircleDiameter;} // in osu!pixels
	[[nodiscard]] inline float getHitcircleXMultiplier() const {return m_fXMultiplier;} // multiply osu!pixels with this to get screen pixels
	[[nodiscard]] inline float getNumberScale() const {return m_fNumberScale;}
	[[nodiscard]] inline float getHitcircleOverlapScale() const {return m_fHitcircleOverlapScale;}
	[[nodiscard]] inline float getSliderFollowCircleDiameter() const {return m_fSliderFollowCircleDiameter;}
	[[nodiscard]] inline float getRawSliderFollowCircleDiameter() const {return m_fRawSliderFollowCircleDiameter;}
	[[nodiscard]] inline bool isInMafhamRenderChunk() const {return m_bInMafhamRenderChunk;}

	// score
	[[nodiscard]] inline int getNumHitObjects() const {return m_hitobjects.size();}
	[[nodiscard]] inline float getAimStars() const {return m_fAimStars;}
	[[nodiscard]] inline float getAimSliderFactor() const {return m_fAimSliderFactor;}
	[[nodiscard]] inline float getAimDifficultSliders() const {return m_fAimDifficultSliders;}
	[[nodiscard]] inline float getAimDifficultStrains() const {return m_fAimDifficultStrains;}
	[[nodiscard]] inline float getSpeedStars() const {return m_fSpeedStars;}
	[[nodiscard]] inline float getSpeedNotes() const {return m_fSpeedNotes;}
	[[nodiscard]] inline float getSpeedDifficultStrains() const {return m_fSpeedDifficultStrains;}

	// hud
	[[nodiscard]] inline bool isSpinnerActive() const {return m_bIsSpinnerActive;}

private:
	static ConVar *m_osu_draw_statistics_pp_ref;
	static ConVar *m_osu_draw_statistics_livestars_ref;
	static ConVar *m_osu_mod_fullalternate_ref;
	static ConVar *m_fposu_distance_ref;
	static ConVar *m_fposu_curved_ref;
	static ConVar *m_fposu_3d_curve_multiplier_ref;
	static ConVar *m_fposu_mod_strafing_ref;
	static ConVar *m_fposu_mod_strafing_frequency_x_ref;
	static ConVar *m_fposu_mod_strafing_frequency_y_ref;
	static ConVar *m_fposu_mod_strafing_frequency_z_ref;
	static ConVar *m_fposu_mod_strafing_strength_x_ref;
	static ConVar *m_fposu_mod_strafing_strength_y_ref;
	static ConVar *m_fposu_mod_strafing_strength_z_ref;
	static ConVar *m_fposu_mod_3d_depthwobble_ref;
	static ConVar *m_osu_slider_scorev2_ref;

	static inline Vector2 mapNormalizedCoordsOntoUnitCircle(const Vector2 &in)
	{
		return {in.x * std::sqrt(1.0f - in.y * in.y / 2.0f), in.y * std::sqrt(1.0f - in.x * in.x / 2.0f)};
	}

	static float quadLerp3f(float left, float center, float right, float percent)
	{
		if (percent >= 0.5f)
		{
			percent = (percent - 0.5f) / 0.5f;
			percent *= percent;
			return std::lerp(center, right, percent);
		}
		else
		{
			percent = percent / 0.5f;
			percent = 1.0f - (1.0f - percent)*(1.0f - percent);
			return std::lerp(left, center, percent);
		}
	}

	void onBeforeLoad() override;
	void onLoad() override;
	void onPlayStart() override;
	void onBeforeStop(bool quit) override;
	void onStop(bool quit) override;
	void onPaused(bool first) override;
	void onUnpaused() override;
	void onRestart(bool quick) override;

	void drawFollowPoints();
	void drawHitObjects();

	void updateAutoCursorPos();
	void updatePlayfieldMetrics();
	void updateHitobjectMetrics();
	void updateSliderVertexBuffers();

	void calculateStacks();
	void computeDrainRate();

	void updateStarCache();
	void stopStarCacheLoader();
	[[nodiscard]] bool isLoadingStarCache() const;
	[[nodiscard]] bool isLoadingInt() const;

	// beatmap
	bool m_bIsSpinnerActive;
	Vector2 m_vContinueCursorPoint;

	// playfield
	float m_fPlayfieldRotation;
	float m_fScaleFactor;
	Vector2 m_vPlayfieldCenter;
	Vector2 m_vPlayfieldOffset;
	Vector2 m_vPlayfieldSize;

	// hitobject scaling
	float m_fXMultiplier;
	float m_fRawHitcircleDiameter;
	float m_fHitcircleDiameter;
	float m_fNumberScale;
	float m_fHitcircleOverlapScale;
	float m_fSliderFollowCircleDiameter;
	float m_fRawSliderFollowCircleDiameter;

	// auto
	Vector2 m_vAutoCursorPos;
	int m_iAutoCursorDanceIndex;

	// pp calculation buffer (only needs to be recalculated in onModUpdate(), instead of on every hit)
	float m_fAimStars;
	float m_fAimSliderFactor;
	float m_fAimDifficultSliders;
	float m_fAimDifficultStrains;
	float m_fSpeedStars;
	float m_fSpeedNotes;
	float m_fSpeedDifficultStrains;
	OsuBackgroundStarCacheLoader *m_starCacheLoader;
	float m_fStarCacheTime;

	// dynamic slider vertex buffer and other recalculation checks (for live mod switching)
	float m_fPrevHitCircleDiameter;
	bool m_bWasHorizontalMirrorEnabled;
	bool m_bWasVerticalMirrorEnabled;
	bool m_bWasEZEnabled;
	bool m_bWasMafhamEnabled;
	float m_fPrevPlayfieldRotationFromConVar;
	float m_fPrevPlayfieldStretchX;
	float m_fPrevPlayfieldStretchY;
	float m_fPrevHitCircleDiameterForStarCache;
	float m_fPrevSpeedForStarCache;

	// custom
	bool m_bIsPreLoading;
	int m_iPreLoadingIndex;
	bool m_bWasHREnabled; // dynamic stack recalculation

	RenderTarget *m_mafhamActiveRenderTarget;
	RenderTarget *m_mafhamFinishedRenderTarget;
	bool m_bMafhamRenderScheduled;
	int m_iMafhamHitObjectRenderIndex; // scene buffering for rendering entire beatmaps at once with an acceptable framerate
	int m_iMafhamPrevHitObjectIndex;
	int m_iMafhamActiveRenderHitObjectIndex;
	int m_iMafhamFinishedRenderHitObjectIndex;
	bool m_bInMafhamRenderChunk; // used by OsuSlider to not animate the reverse arrow, and by OsuCircle to not animate note blocking shaking, while being rendered into the scene buffer

	int m_iMandalaIndex;
};

#endif
