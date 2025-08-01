//================ Copyright (c) 2015, PG & Jeffrey Han (opsu!), All rights reserved. =================//
//
// Purpose:		curve classes have been taken from opsu!, albeit heavily modified.
//				contains all classes and functions for calculating slider curves
//
// $NoKeywords: $slidercurves
//=====================================================================================================//

#pragma once
#ifndef OSUSLIDERCURVES_H
#define OSUSLIDERCURVES_H

#include "cbase.h"



//**********************//
//	 Curve Base Class	//
//**********************//

class OsuSliderCurve
{
public:
	enum OSUSLIDERCURVETYPE
	{
		OSUSLIDERCURVETYPE_CATMULL = 'C',
		OSUSLIDERCURVETYPE_BEZIER = 'B',
		OSUSLIDERCURVETYPE_LINEAR = 'L',
		OSUSLIDERCURVETYPE_PASSTHROUGH = 'P'
	};

	static OsuSliderCurve *createCurve(char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength);
	static OsuSliderCurve *createCurve(char osuSliderCurveType, std::vector<Vector2> controlPoints, float pixelLength, float curvePointsSeparation);

public:
	OsuSliderCurve(std::vector<Vector2> controlPoints, float pixelLength);
	virtual ~OsuSliderCurve() {;}

	virtual void updateStackPosition(float stackMulStackOffset, bool HR);

	virtual Vector2 pointAt(float t) = 0;			// with stacking
	virtual Vector2 originalPointAt(float t) = 0;	// without stacking

	[[nodiscard]] inline float getStartAngle() const {return m_fStartAngle;}
	[[nodiscard]] inline float getEndAngle() const {return m_fEndAngle;}

	[[nodiscard]] inline const std::vector<Vector2> &getPoints() const {return m_curvePoints;}
	[[nodiscard]] inline const std::vector<std::vector<Vector2>> &getPointSegments() const {return m_curvePointSegments;}

	[[nodiscard]] inline float getPixelLength() const {return m_fPixelLength;}

protected:
	// original input values
	float m_fPixelLength;
	std::vector<Vector2> m_controlPoints;

	// these must be explicitly calculated/set in one of the subclasses
	std::vector<std::vector<Vector2>> m_curvePointSegments;
	std::vector<std::vector<Vector2>> m_originalCurvePointSegments;
	std::vector<Vector2> m_curvePoints;
	std::vector<Vector2> m_originalCurvePoints;
	float m_fStartAngle;
	float m_fEndAngle;
};



//******************************************//
//	 Curve Type Base Class & Curve Types	//
//******************************************//

class OsuSliderCurveType
{
public:
	OsuSliderCurveType();
	virtual ~OsuSliderCurveType() {;}

	virtual Vector2 pointAt(float t) = 0;

	[[nodiscard]] inline const int getNumPoints() const {return m_points.size();}

	[[nodiscard]] inline const std::vector<Vector2> &getCurvePoints() const {return m_points;}
	[[nodiscard]] inline const std::vector<float> &getCurveDistances() const {return m_curveDistances;}

protected:
	// either one must be called from one of the subclasses
	void init(float approxLength);					// subdivide the curve by calling virtual pointAt() to create all intermediary points
	void initCustom(std::vector<Vector2> points);	// assume that the input vector = all intermediary points (already precalculated somewhere else)

private:
	void calculateIntermediaryPoints(float approxLength);
	void calculateCurveDistances();

	std::vector<Vector2> m_points;

	float m_fTotalDistance;
	std::vector<float> m_curveDistances;
};



class OsuSliderCurveTypeBezier2 final : public OsuSliderCurveType
{
public:
	OsuSliderCurveTypeBezier2(const std::vector<Vector2> &points);
	~OsuSliderCurveTypeBezier2() override {;}

	Vector2 pointAt(float  /*t*/) override {return Vector2();} // unused
};



class OsuSliderCurveTypeCentripetalCatmullRom final : public OsuSliderCurveType
{
public:
	OsuSliderCurveTypeCentripetalCatmullRom(const std::vector<Vector2> &points);
	~OsuSliderCurveTypeCentripetalCatmullRom() override {;}

	Vector2 pointAt(float t) override;

private:
	float m_time[4];
	std::vector<Vector2> m_points;
};



//*******************//
//	 Curve Classes	 //
//*******************//

class OsuSliderCurveEqualDistanceMulti : public OsuSliderCurve
{
public:
	OsuSliderCurveEqualDistanceMulti(std::vector<Vector2> controlPoints, float pixelLength, float curvePointsSeparation);
	~OsuSliderCurveEqualDistanceMulti() override {;}

	// must be called from one of the subclasses
	void init(const std::vector<OsuSliderCurveType*> &curvesList);

	Vector2 pointAt(float t) override;
	Vector2 originalPointAt(float t) override;

private:
	int m_iNCurve;
};



class OsuSliderCurveLinearBezier final : public OsuSliderCurveEqualDistanceMulti
{
public:
	OsuSliderCurveLinearBezier(std::vector<Vector2> controlPoints, float pixelLength, bool line, float curvePointsSeparation);
	~OsuSliderCurveLinearBezier() override {;}
};



class OsuSliderCurveCatmull final : public OsuSliderCurveEqualDistanceMulti
{
public:
	OsuSliderCurveCatmull(std::vector<Vector2> controlPoints, float pixelLength, float curvePointsSeparation);
	~OsuSliderCurveCatmull() override {;}
};



class OsuSliderCurveCircumscribedCircle final : public OsuSliderCurve
{
public:
	OsuSliderCurveCircumscribedCircle(const std::vector<Vector2>& controlPoints, float pixelLength, float curvePointsSeparation);
	~OsuSliderCurveCircumscribedCircle() override {;}

	Vector2 pointAt(float t) override;
	Vector2 originalPointAt(float t) override;

	void updateStackPosition(float stackMulStackOffset, bool HR) override; // must also override this, due to the custom pointAt() function!

private:
	Vector2 intersect(Vector2 a, Vector2 ta, Vector2 b, Vector2 tb);

	inline bool const isIn(float a, float b, float c) {return ((b > a && b < c) || (b < a && b > c));}

	Vector2 m_vCircleCenter;
	Vector2 m_vOriginalCircleCenter;
	float m_fRadius;
	float m_fCalculationStartAngle;
	float m_fCalculationEndAngle;
};



//********************//
//	 Helper Classes	  //
//********************//

// https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Objects/BezierApproximator.cs
// https://github.com/ppy/osu-framework/blob/master/osu.Framework/MathUtils/PathApproximator.cs
// Copyright (c) ppy Pty Ltd <contact@ppy.sh>. Licensed under the MIT Licence.

class OsuSliderBezierApproximator
{
public:
	OsuSliderBezierApproximator();

	std::vector<Vector2> createBezier(const std::vector<Vector2> &controlPoints);

private:
	static double TOLERANCE_SQ;

	bool isFlatEnough(const std::vector<Vector2> &controlPoints);
	void subdivide(std::vector<Vector2> &controlPoints, std::vector<Vector2> &l, std::vector<Vector2> &r);
	void approximate(std::vector<Vector2> &controlPoints, std::vector<Vector2> &output);

	int m_iCount;
	std::vector<Vector2> m_subdivisionBuffer1;
	std::vector<Vector2> m_subdivisionBuffer2;
};

#endif
