///////////////////////////////////////////////////////////////////////////////
// Vectors.h
// =========
// 2D/3D/4D vectors using GLM under the hood
//
// API/ABI compatible with the original McEngine Vectors.h (by Song Ho Ahn)
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef VECTORS_H_DEF
#define VECTORS_H_DEF

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>

#define VECTOR_NORMALIZE_EPSILON 0.000001f

static_assert(sizeof(glm::vec2) == 2 * sizeof(float), "GLM vec2 layout assumption failed");
static_assert(sizeof(glm::vec3) == 3 * sizeof(float), "GLM vec3 layout assumption failed");
static_assert(sizeof(glm::vec4) == 4 * sizeof(float), "GLM vec4 layout assumption failed");

///////////////////////////////////////////////////////////////////////////////
// 2D vector wrapper around glm::vec2
///////////////////////////////////////////////////////////////////////////////
struct Vector2
{
	union {
		glm::vec2 m_vec;
		struct
		{
			float x, y;
		};
	};

	// ctors
	Vector2() : m_vec(0.0f, 0.0f) {}
	Vector2(float x, float y) : m_vec(x, y) {}

	// copy operations (needed due to union)
	Vector2(const Vector2 &other) : m_vec(other.m_vec) {}
	Vector2 &operator=(const Vector2 &other)
	{
		if (this != &other)
		{
			m_vec = other.m_vec;
		}
		return *this;
	}

	// utils functions
	void zero() { m_vec = glm::vec2(0.0f); }
	void set(float x, float y) { m_vec = glm::vec2(x, y); }
	[[nodiscard]] float length() const { return glm::length(m_vec); }
	[[nodiscard]] float distance(const Vector2 &vec) const { return glm::distance(m_vec, vec.m_vec); }
	Vector2 &normalize()
	{
		if (glm::length(m_vec) < VECTOR_NORMALIZE_EPSILON)
			return *this;
		m_vec = glm::normalize(m_vec);
		return *this;
	}
	[[nodiscard]] float dot(const Vector2 &vec) const { return glm::dot(m_vec, vec.m_vec); }
	[[nodiscard]] bool equal(const Vector2 &vec, float e) const { return glm::all(glm::lessThan(glm::abs(m_vec - vec.m_vec), glm::vec2(e))); }
	Vector2 &nudge(const Vector2 &vec, float amount)
	{
		glm::vec2 dir = m_vec - vec.m_vec;
		if (glm::length(dir) > VECTOR_NORMALIZE_EPSILON)
		{
			dir = glm::normalize(dir);
			m_vec += dir * amount;
		}
		return *this;
	}

	// operators
	Vector2 operator-() const { return {-m_vec.x, -m_vec.y}; }
	Vector2 operator+(const Vector2 &rhs) const
	{
		Vector2 result;
		result.m_vec = m_vec + rhs.m_vec;
		return result;
	}
	Vector2 operator-(const Vector2 &rhs) const
	{
		Vector2 result;
		result.m_vec = m_vec - rhs.m_vec;
		return result;
	}
	Vector2 &operator+=(const Vector2 &rhs)
	{
		m_vec += rhs.m_vec;
		return *this;
	}
	Vector2 &operator-=(const Vector2 &rhs)
	{
		m_vec -= rhs.m_vec;
		return *this;
	}
	Vector2 operator*(const float scale) const
	{
		Vector2 result;
		result.m_vec = m_vec * scale;
		return result;
	}
	Vector2 operator*(const Vector2 &rhs) const
	{
		Vector2 result;
		result.m_vec = m_vec * rhs.m_vec;
		return result;
	}
	Vector2 &operator*=(const float scale)
	{
		m_vec *= scale;
		return *this;
	}
	Vector2 &operator*=(const Vector2 &rhs)
	{
		m_vec *= rhs.m_vec;
		return *this;
	}
	Vector2 operator/(const float scale) const
	{
		Vector2 result;
		result.m_vec = m_vec / scale;
		return result;
	}
	Vector2 &operator/=(const float scale)
	{
		m_vec /= scale;
		return *this;
	}
	bool operator==(const Vector2 &rhs) const { return m_vec == rhs.m_vec; }
	bool operator!=(const Vector2 &rhs) const { return m_vec != rhs.m_vec; }
	bool operator<(const Vector2 &rhs) const
	{
		if (m_vec.x < rhs.m_vec.x)
			return true;
		if (m_vec.x > rhs.m_vec.x)
			return false;
		return m_vec.y < rhs.m_vec.y;
	}
	float operator[](int index) const { return (&x)[index]; }
	float &operator[](int index) { return (&x)[index]; }

	friend Vector2 operator*(const float a, const Vector2 vec);
	friend std::ostream &operator<<(std::ostream &os, const Vector2 &vec);
};

// sanity check
static_assert(sizeof(Vector2) == sizeof(glm::vec2), "Vector2 size mismatch");

inline Vector2 operator*(const float a, const Vector2 vec)
{
	Vector2 result;
	result.m_vec = a * vec.m_vec;
	return result;
}

inline std::ostream &operator<<(std::ostream &os, const Vector2 &vec)
{
	os << "(" << vec.x << ", " << vec.y << ")";
	return os;
}

///////////////////////////////////////////////////////////////////////////////
// 3D vector wrapper around glm::vec3
///////////////////////////////////////////////////////////////////////////////
struct Vector3
{
	union {
		glm::vec3 m_vec;
		struct
		{
			float x, y, z;
		};
	};

	// ctors
	Vector3() : m_vec(0.0f, 0.0f, 0.0f) {}
	Vector3(float x, float y, float z) : m_vec(x, y, z) {}

	// copy operations
	Vector3(const Vector3 &other) : m_vec(other.m_vec) {}
	Vector3 &operator=(const Vector3 &other)
	{
		if (this != &other)
		{
			m_vec = other.m_vec;
		}
		return *this;
	}

	// utils functions
	void zero() { m_vec = glm::vec3(0.0f); }
	void set(float x, float y, float z) { m_vec = glm::vec3(x, y, z); }
	void setLength(float length)
	{
		if (glm::length(m_vec) > VECTOR_NORMALIZE_EPSILON)
		{
			m_vec = glm::normalize(m_vec) * length;
		}
	}
	[[nodiscard]] float length() const { return glm::length(m_vec); }
	[[nodiscard]] float distance(const Vector3 &vec) const { return glm::distance(m_vec, vec.m_vec); }
	Vector3 &normalize()
	{
		if (glm::length(m_vec) < VECTOR_NORMALIZE_EPSILON)
			return *this;
		m_vec = glm::normalize(m_vec);
		return *this;
	}
	[[nodiscard]] float dot(const Vector3 &vec) const { return glm::dot(m_vec, vec.m_vec); }
	[[nodiscard]] Vector3 cross(const Vector3 &vec) const
	{
		Vector3 result;
		result.m_vec = glm::cross(m_vec, vec.m_vec);
		return result;
	}
	[[nodiscard]] bool equal(const Vector3 &vec, float e) const { return glm::all(glm::lessThan(glm::abs(m_vec - vec.m_vec), glm::vec3(e))); }
	Vector3 &nudge(const Vector3 &vec, float amount)
	{
		glm::vec3 dir = m_vec - vec.m_vec;
		if (glm::length(dir) > VECTOR_NORMALIZE_EPSILON)
		{
			dir = glm::normalize(dir);
			m_vec += dir * amount;
		}
		return *this;
	}

	// operators
	Vector3 operator-() const { return {-m_vec.x, -m_vec.y, -m_vec.z}; }
	Vector3 operator+(const Vector3 &rhs) const
	{
		Vector3 result;
		result.m_vec = m_vec + rhs.m_vec;
		return result;
	}
	Vector3 operator-(const Vector3 &rhs) const
	{
		Vector3 result;
		result.m_vec = m_vec - rhs.m_vec;
		return result;
	}
	Vector3 &operator+=(const Vector3 &rhs)
	{
		m_vec += rhs.m_vec;
		return *this;
	}
	Vector3 &operator-=(const Vector3 &rhs)
	{
		m_vec -= rhs.m_vec;
		return *this;
	}
	Vector3 operator*(const float scale) const
	{
		Vector3 result;
		result.m_vec = m_vec * scale;
		return result;
	}
	Vector3 operator*(const Vector3 &rhs) const
	{
		Vector3 result;
		result.m_vec = m_vec * rhs.m_vec;
		return result;
	}
	Vector3 &operator*=(const float scale)
	{
		m_vec *= scale;
		return *this;
	}
	Vector3 &operator*=(const Vector3 &rhs)
	{
		m_vec *= rhs.m_vec;
		return *this;
	}
	Vector3 operator/(const float scale) const
	{
		Vector3 result;
		result.m_vec = m_vec / scale;
		return result;
	}
	Vector3 &operator/=(const float scale)
	{
		m_vec /= scale;
		return *this;
	}
	bool operator==(const Vector3 &rhs) const { return m_vec == rhs.m_vec; }
	bool operator!=(const Vector3 &rhs) const { return m_vec != rhs.m_vec; }
	bool operator<(const Vector3 &rhs) const
	{
		if (m_vec.x < rhs.m_vec.x)
			return true;
		if (m_vec.x > rhs.m_vec.x)
			return false;
		if (m_vec.y < rhs.m_vec.y)
			return true;
		if (m_vec.y > rhs.m_vec.y)
			return false;
		return m_vec.z < rhs.m_vec.z;
	}
	float operator[](int index) const { return (&x)[index]; }
	float &operator[](int index) { return (&x)[index]; }

	friend Vector3 operator*(const float a, const Vector3 vec);
	friend std::ostream &operator<<(std::ostream &os, const Vector3 &vec);
};

// sanity
static_assert(sizeof(Vector3) == sizeof(glm::vec3), "Vector3 size mismatch");

inline Vector3 operator*(const float a, const Vector3 vec)
{
	Vector3 result;
	result.m_vec = a * vec.m_vec;
	return result;
}

inline std::ostream &operator<<(std::ostream &os, const Vector3 &vec)
{
	os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
	return os;
}

///////////////////////////////////////////////////////////////////////////////
// 4D vector wrapper around glm::vec4
///////////////////////////////////////////////////////////////////////////////
struct Vector4
{
	union {
		glm::vec4 m_vec;
		struct
		{
			float x, y, z, w;
		};
	};

	// ctors
	Vector4() : m_vec(0.0f, 0.0f, 0.0f, 0.0f) {}
	Vector4(float x, float y, float z, float w) : m_vec(x, y, z, w) {}

	// copy operations
	Vector4(const Vector4 &other) : m_vec(other.m_vec) {}
	Vector4 &operator=(const Vector4 &other)
	{
		if (this != &other)
		{
			m_vec = other.m_vec;
		}
		return *this;
	}

	// utils functions
	void set(float x, float y, float z, float w) { m_vec = glm::vec4(x, y, z, w); }
	[[nodiscard]] float length() const { return glm::length(m_vec); }
	[[nodiscard]] float distance(const Vector4 &vec) const { return glm::distance(m_vec, vec.m_vec); }
	Vector4 &normalize()
	{
		// original behavior leaves w-component untouched (bug?)
		auto xyz = glm::vec3(m_vec);
		if (glm::length(xyz) < VECTOR_NORMALIZE_EPSILON)
			return *this;
		xyz = glm::normalize(xyz);
		m_vec.x = xyz.x;
		m_vec.y = xyz.y;
		m_vec.z = xyz.z;
		return *this;
	}
	[[nodiscard]] float dot(const Vector4 &vec) const { return glm::dot(m_vec, vec.m_vec); }
	[[nodiscard]] bool equal(const Vector4 &vec, float e) const { return glm::all(glm::lessThan(glm::abs(m_vec - vec.m_vec), glm::vec4(e))); }
	Vector4 &nudge(const Vector4 &vec, float amount)
	{
		glm::vec4 dir = m_vec - vec.m_vec;
		if (glm::length(dir) > VECTOR_NORMALIZE_EPSILON)
		{
			dir = glm::normalize(dir);
			m_vec += dir * amount;
		}
		return *this;
	}

	// operators
	Vector4 operator-() const { return {-m_vec.x, -m_vec.y, -m_vec.z, -m_vec.w}; }
	Vector4 operator+(const Vector4 &rhs) const
	{
		Vector4 result;
		result.m_vec = m_vec + rhs.m_vec;
		return result;
	}
	Vector4 operator-(const Vector4 &rhs) const
	{
		Vector4 result;
		result.m_vec = m_vec - rhs.m_vec;
		return result;
	}
	Vector4 &operator+=(const Vector4 &rhs)
	{
		m_vec += rhs.m_vec;
		return *this;
	}
	Vector4 &operator-=(const Vector4 &rhs)
	{
		m_vec -= rhs.m_vec;
		return *this;
	}
	Vector4 operator*(const float scale) const
	{
		Vector4 result;
		result.m_vec = m_vec * scale;
		return result;
	}
	Vector4 operator*(const Vector4 &rhs) const
	{
		Vector4 result;
		result.m_vec = m_vec * rhs.m_vec;
		return result;
	}
	Vector4 &operator*=(const float scale)
	{
		m_vec *= scale;
		return *this;
	}
	Vector4 &operator*=(const Vector4 &rhs)
	{
		m_vec *= rhs.m_vec;
		return *this;
	}
	Vector4 operator/(const float scale) const
	{
		Vector4 result;
		result.m_vec = m_vec / scale;
		return result;
	}
	Vector4 &operator/=(const float scale)
	{
		m_vec /= scale;
		return *this;
	}
	bool operator==(const Vector4 &rhs) const { return m_vec == rhs.m_vec; }
	bool operator!=(const Vector4 &rhs) const { return m_vec != rhs.m_vec; }
	bool operator<(const Vector4 &rhs) const
	{
		if (m_vec.x < rhs.m_vec.x)
			return true;
		if (m_vec.x > rhs.m_vec.x)
			return false;
		if (m_vec.y < rhs.m_vec.y)
			return true;
		if (m_vec.y > rhs.m_vec.y)
			return false;
		if (m_vec.z < rhs.m_vec.z)
			return true;
		if (m_vec.z > rhs.m_vec.z)
			return false;
		return m_vec.w < rhs.m_vec.w;
	}
	float operator[](int index) const { return (&x)[index]; }
	float &operator[](int index) { return (&x)[index]; }

	friend Vector4 operator*(const float a, const Vector4 vec);
	friend std::ostream &operator<<(std::ostream &os, const Vector4 &vec);
};

// sanity
static_assert(sizeof(Vector4) == sizeof(glm::vec4), "Vector4 size mismatch");

inline Vector4 operator*(const float a, const Vector4 vec)
{
	Vector4 result;
	result.m_vec = a * vec.m_vec;
	return result;
}

inline std::ostream &operator<<(std::ostream &os, const Vector4 &vec)
{
	os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
	return os;
}

#endif
