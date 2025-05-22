//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		rect
//
// $NoKeywords: $rect
//===============================================================================//

#pragma once
#ifndef RECT_H
#define RECT_H

#include "Vectors.h"

class McRect
{
public:
	McRect(float x = 0, float y = 0, float width = 0, float height = 0, bool isCentered = false);
	McRect(Vector2 pos, Vector2 size, bool isCentered = false);
	McRect(const McRect& other) = default;

	void set(float x, float y, float width, float height, bool isCentered = false);

	[[nodiscard]] inline bool contains(const Vector2 &point) const {return (point.x >= m_fMinX && point.x <= m_fMaxX && point.y >= m_fMinY && point.y <= m_fMaxY);}
	[[nodiscard]] McRect intersect(const McRect &rect) const;
	[[nodiscard]] bool intersects(const McRect &rect) const;
	[[nodiscard]] McRect Union(const McRect &rect) const;

	[[nodiscard]] inline Vector2 getPos() const {return {m_fMinX, m_fMinY};}
	[[nodiscard]] inline Vector2 getSize() const {return {m_fMaxX - m_fMinX, m_fMaxY - m_fMinY};}
	[[nodiscard]] inline Vector2 getCenter() const {return {((m_fMaxX - m_fMinX)/2)+m_fMinX, ((m_fMaxY - m_fMinY)/2)+m_fMinY};}

	[[nodiscard]] inline float getX() const {return m_fMinX;}
	[[nodiscard]] inline float getY() const {return m_fMinY;}
	[[nodiscard]] inline float getWidth() const {return (m_fMaxX - m_fMinX);}
	[[nodiscard]] inline float getHeight() const {return (m_fMaxY - m_fMinY);}

	[[nodiscard]] inline float getMinX() const {return m_fMinX;}
	[[nodiscard]] inline float getMinY() const {return m_fMinY;}
	[[nodiscard]] inline float getMaxX() const {return m_fMaxX;}
	[[nodiscard]] inline float getMaxY() const {return m_fMaxY;}

	inline void setMaxX(float maxx) {m_fMaxX = maxx;}
	inline void setMaxY(float maxy) {m_fMaxY = maxy;}
	inline void setMinX(float minx) {m_fMinX = minx;}
	inline void setMinY(float miny) {m_fMinY = miny;}

	// operators
	McRect &operator = (const McRect &rect);

private:
	float m_fMinX;
	float m_fMinY;
	float m_fMaxX;
	float m_fMaxY;
};

#endif

