///////////////////////////////////////////////////////////////////////////////
// Matrices.h
// ==========
// NxN Matrix Math classes using GLM under the hood with optimizations
//
// API compatible with the original McEngine Matrices.h (by Song Ho Ahn)
// (some of the Matrix4 operations are copied to maintain the original semantics)
//
// The elements of the matrix are stored as column major order.
// | 0 2 |    | 0 3 6 |    |  0  4  8 12 |
// | 1 3 |    | 1 4 7 |    |  1  5  9 13 |
//            | 2 5 8 |    |  2  6 10 14 |
//                         |  3  7 11 15 |
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MATH_MATRICES_H
#define MATH_MATRICES_H

#include "Vectors.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <iomanip>
#include <iostream>

constexpr float EPSILON = 0.00001f;

///////////////////////////////////////////////////////////////////////////
// 2x2 matrix wrapper around glm::mat2
///////////////////////////////////////////////////////////////////////////
class Matrix2
{
public:
	// constructors
	Matrix2() : m_mat(1.0f) {} // identity matrix
	Matrix2(const float src[4])
	{
		// column-major
		m_mat[0][0] = src[0];
		m_mat[0][1] = src[1];
		m_mat[1][0] = src[2];
		m_mat[1][1] = src[3];
	}
	Matrix2(float m0, float m1, float m2, float m3)
	{
		m_mat[0][0] = m0;
		m_mat[0][1] = m1;
		m_mat[1][0] = m2;
		m_mat[1][1] = m3;
	}

	// copy operations
	Matrix2(const Matrix2 &other) = default;
	Matrix2 &operator=(const Matrix2 &other)
	{
		if (this != &other)
		{
			m_mat = other.m_mat;
		}
		return *this;
	}

	void set(const float src[4])
	{
		m_mat[0][0] = src[0];
		m_mat[0][1] = src[1];
		m_mat[1][0] = src[2];
		m_mat[1][1] = src[3];
	}

	void set(float m0, float m1, float m2, float m3)
	{
		m_mat[0][0] = m0;
		m_mat[0][1] = m1;
		m_mat[1][0] = m2;
		m_mat[1][1] = m3;
	}

	void setRow(int index, const float row[2])
	{
		m_mat[0][index] = row[0];
		m_mat[1][index] = row[1];
	}

	void setRow(int index, const Vector2 &v)
	{
		m_mat[0][index] = v.x;
		m_mat[1][index] = v.y;
	}

	void setColumn(int index, const float col[2])
	{
		m_mat[index][0] = col[0];
		m_mat[index][1] = col[1];
	}

	void setColumn(int index, const Vector2 &v)
	{
		m_mat[index][0] = v.x;
		m_mat[index][1] = v.y;
	}

	// accessors
	[[nodiscard]] const float *get() const { return &m_mat[0][0]; }
	[[nodiscard]] float getDeterminant() const { return glm::determinant(m_mat); }

	Matrix2 &identity()
	{
		m_mat = glm::mat2(1.0f);
		return *this;
	}

	Matrix2 &transpose()
	{
		m_mat = glm::transpose(m_mat);
		return *this;
	}

	Matrix2 &invert()
	{
		float det = getDeterminant();
		if (fabs(det) <= EPSILON)
		{
			return identity();
		}
		m_mat = glm::inverse(m_mat);
		return *this;
	}

	// operators
	Matrix2 operator+(const Matrix2 &rhs) const
	{
		Matrix2 result;
		result.m_mat = m_mat + rhs.m_mat;
		return result;
	}

	Matrix2 operator-(const Matrix2 &rhs) const
	{
		Matrix2 result;
		result.m_mat = m_mat - rhs.m_mat;
		return result;
	}

	Matrix2 &operator+=(const Matrix2 &rhs)
	{
		m_mat += rhs.m_mat;
		return *this;
	}

	Matrix2 &operator-=(const Matrix2 &rhs)
	{
		m_mat -= rhs.m_mat;
		return *this;
	}

	Vector2 operator*(const Vector2 &rhs) const
	{
		glm::vec2 v(rhs.x, rhs.y);
		glm::vec2 result = m_mat * v;
		return {result.x, result.y};
	}

	Matrix2 operator*(const Matrix2 &rhs) const
	{
		Matrix2 result;
		result.m_mat = m_mat * rhs.m_mat;
		return result;
	}

	Matrix2 &operator*=(const Matrix2 &rhs)
	{
		m_mat *= rhs.m_mat;
		return *this;
	}

	bool operator==(const Matrix2 &rhs) const { return m_mat == rhs.m_mat; }
	bool operator!=(const Matrix2 &rhs) const { return m_mat != rhs.m_mat; }

	float operator[](int index) const { return (&m_mat[0][0])[index]; }
	float &operator[](int index) { return (&m_mat[0][0])[index]; }

	friend Matrix2 operator-(const Matrix2 &m);
	friend Matrix2 operator*(float scalar, const Matrix2 &m);
	friend Vector2 operator*(const Vector2 &vec, const Matrix2 &m);
	friend std::ostream &operator<<(std::ostream &os, const Matrix2 &m);

private:
	glm::mat2 m_mat;
};

inline Matrix2 operator-(const Matrix2 &m)
{
	Matrix2 result;
	result.m_mat = -m.m_mat;
	return result;
}

inline Matrix2 operator*(float s, const Matrix2 &m)
{
	Matrix2 result;
	result.m_mat = s * m.m_mat;
	return result;
}

inline Vector2 operator*(const Vector2 &v, const Matrix2 &m)
{
	glm::vec2 vec(v.x, v.y);
	glm::vec2 result = vec * m.m_mat;
	return {result.x, result.y};
}

inline std::ostream &operator<<(std::ostream &os, const Matrix2 &m)
{
	os << std::fixed << std::setprecision(5);
	os << "[" << std::setw(10) << m[0] << " " << std::setw(10) << m[2] << "]\n"
	   << "[" << std::setw(10) << m[1] << " " << std::setw(10) << m[3] << "]\n";
	os << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
	return os;
}

///////////////////////////////////////////////////////////////////////////
// 3x3 matrix wrapper around glm::mat3
///////////////////////////////////////////////////////////////////////////
class Matrix3
{
public:
	// constructors
	Matrix3() : m_mat(1.0f) {} // identity matrix
	Matrix3(const float src[9])
	{
		for (int col = 0; col < 3; ++col)
			for (int row = 0; row < 3; ++row)
				m_mat[col][row] = src[col * 3 + row];
	}
	Matrix3(float m0, float m1, float m2, float m3, float m4, float m5, float m6, float m7, float m8)
	{
		m_mat[0][0] = m0;
		m_mat[0][1] = m1;
		m_mat[0][2] = m2;
		m_mat[1][0] = m3;
		m_mat[1][1] = m4;
		m_mat[1][2] = m5;
		m_mat[2][0] = m6;
		m_mat[2][1] = m7;
		m_mat[2][2] = m8;
	}

	// copy operations
	Matrix3(const Matrix3 &other) = default;
	Matrix3 &operator=(const Matrix3 &other)
	{
		if (this != &other)
		{
			m_mat = other.m_mat;
		}
		return *this;
	}

	void set(const float src[9])
	{
		for (int col = 0; col < 3; ++col)
			for (int row = 0; row < 3; ++row)
				m_mat[col][row] = src[col * 3 + row];
	}

	void set(float m0, float m1, float m2, float m3, float m4, float m5, float m6, float m7, float m8)
	{
		m_mat[0][0] = m0;
		m_mat[0][1] = m1;
		m_mat[0][2] = m2;
		m_mat[1][0] = m3;
		m_mat[1][1] = m4;
		m_mat[1][2] = m5;
		m_mat[2][0] = m6;
		m_mat[2][1] = m7;
		m_mat[2][2] = m8;
	}

	void setRow(int index, const float row[3])
	{
		m_mat[0][index] = row[0];
		m_mat[1][index] = row[1];
		m_mat[2][index] = row[2];
	}

	void setRow(int index, const Vector3 &v)
	{
		m_mat[0][index] = v.x;
		m_mat[1][index] = v.y;
		m_mat[2][index] = v.z;
	}

	void setColumn(int index, const float col[3])
	{
		m_mat[index][0] = col[0];
		m_mat[index][1] = col[1];
		m_mat[index][2] = col[2];
	}

	void setColumn(int index, const Vector3 &v)
	{
		m_mat[index][0] = v.x;
		m_mat[index][1] = v.y;
		m_mat[index][2] = v.z;
	}

	const float *get() const { return &m_mat[0][0]; }
	[[nodiscard]] float getDeterminant() const { return glm::determinant(m_mat); }

	Matrix3 &identity()
	{
		m_mat = glm::mat3(1.0f);
		return *this;
	}

	Matrix3 &transpose()
	{
		m_mat = glm::transpose(m_mat);
		return *this;
	}

	Matrix3 &invert()
	{
		float det = getDeterminant();
		if (fabs(det) <= EPSILON)
		{
			return identity();
		}
		m_mat = glm::inverse(m_mat);
		return *this;
	}

	// operators
	Matrix3 operator+(const Matrix3 &rhs) const
	{
		Matrix3 result;
		result.m_mat = m_mat + rhs.m_mat;
		return result;
	}

	Matrix3 operator-(const Matrix3 &rhs) const
	{
		Matrix3 result;
		result.m_mat = m_mat - rhs.m_mat;
		return result;
	}

	Matrix3 &operator+=(const Matrix3 &rhs)
	{
		m_mat += rhs.m_mat;
		return *this;
	}

	Matrix3 &operator-=(const Matrix3 &rhs)
	{
		m_mat -= rhs.m_mat;
		return *this;
	}

	Vector3 operator*(const Vector3 &rhs) const
	{
		glm::vec3 v(rhs.x, rhs.y, rhs.z);
		glm::vec3 result = m_mat * v;
		return {result.x, result.y, result.z};
	}

	Matrix3 operator*(const Matrix3 &rhs) const
	{
		Matrix3 result;
		result.m_mat = m_mat * rhs.m_mat;
		return result;
	}

	Matrix3 &operator*=(const Matrix3 &rhs)
	{
		m_mat *= rhs.m_mat;
		return *this;
	}

	bool operator==(const Matrix3 &rhs) const { return m_mat == rhs.m_mat; }
	bool operator!=(const Matrix3 &rhs) const { return m_mat != rhs.m_mat; }

	float operator[](int index) const { return (&m_mat[0][0])[index]; }
	float &operator[](int index) { return (&m_mat[0][0])[index]; }

	friend Matrix3 operator-(const Matrix3 &m);
	friend Matrix3 operator*(float scalar, const Matrix3 &m);
	friend Vector3 operator*(const Vector3 &vec, const Matrix3 &m);
	friend std::ostream &operator<<(std::ostream &os, const Matrix3 &m);

private:
	glm::mat3 m_mat;
};

inline Matrix3 operator-(const Matrix3 &m)
{
	Matrix3 result;
	result.m_mat = -m.m_mat;
	return result;
}

inline Matrix3 operator*(float s, const Matrix3 &m)
{
	Matrix3 result;
	result.m_mat = s * m.m_mat;
	return result;
}

inline Vector3 operator*(const Vector3 &v, const Matrix3 &m)
{
	glm::vec3 vec(v.x, v.y, v.z);
	glm::vec3 result = vec * m.m_mat;
	return {result.x, result.y, result.z};
}

inline std::ostream &operator<<(std::ostream &os, const Matrix3 &m)
{
	os << std::fixed << std::setprecision(5);
	os << "[" << std::setw(10) << m[0] << " " << std::setw(10) << m[3] << " " << std::setw(10) << m[6] << "]\n"
	   << "[" << std::setw(10) << m[1] << " " << std::setw(10) << m[4] << " " << std::setw(10) << m[7] << "]\n"
	   << "[" << std::setw(10) << m[2] << " " << std::setw(10) << m[5] << " " << std::setw(10) << m[8] << "]\n";
	os << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
	return os;
}

///////////////////////////////////////////////////////////////////////////
// 4x4 matrix wrapper around glm::mat4
///////////////////////////////////////////////////////////////////////////
class Matrix4
{
public:
	// constructors
	Matrix4() : m_mat(1.0f) {} // identity matrix
	Matrix4(const float src[16])
	{
		for (int col = 0; col < 4; ++col)
			for (int row = 0; row < 4; ++row)
				m_mat[col][row] = src[col * 4 + row];
	}
	Matrix4(float m00, float m01, float m02, float m03, float m04, float m05, float m06, float m07, float m08, float m09, float m10, float m11, float m12, float m13, float m14,
	        float m15)
	{
		m_mat[0][0] = m00;
		m_mat[0][1] = m01;
		m_mat[0][2] = m02;
		m_mat[0][3] = m03;
		m_mat[1][0] = m04;
		m_mat[1][1] = m05;
		m_mat[1][2] = m06;
		m_mat[1][3] = m07;
		m_mat[2][0] = m08;
		m_mat[2][1] = m09;
		m_mat[2][2] = m10;
		m_mat[2][3] = m11;
		m_mat[3][0] = m12;
		m_mat[3][1] = m13;
		m_mat[3][2] = m14;
		m_mat[3][3] = m15;
	}

	// copy operations
	Matrix4(const Matrix4 &other) = default;
	Matrix4 &operator=(const Matrix4 &other)
	{
		if (this != &other)
		{
			m_mat = other.m_mat;
		}
		return *this;
	}

	void set(const float src[16])
	{
		for (int col = 0; col < 4; ++col)
			for (int row = 0; row < 4; ++row)
				m_mat[col][row] = src[col * 4 + row];
	}

	void set(float m00, float m01, float m02, float m03, float m04, float m05, float m06, float m07, float m08, float m09, float m10, float m11, float m12, float m13, float m14,
	         float m15)
	{
		m_mat[0][0] = m00;
		m_mat[0][1] = m01;
		m_mat[0][2] = m02;
		m_mat[0][3] = m03;
		m_mat[1][0] = m04;
		m_mat[1][1] = m05;
		m_mat[1][2] = m06;
		m_mat[1][3] = m07;
		m_mat[2][0] = m08;
		m_mat[2][1] = m09;
		m_mat[2][2] = m10;
		m_mat[2][3] = m11;
		m_mat[3][0] = m12;
		m_mat[3][1] = m13;
		m_mat[3][2] = m14;
		m_mat[3][3] = m15;
	}

	void setRow(int index, const float row[4])
	{
		m_mat[0][index] = row[0];
		m_mat[1][index] = row[1];
		m_mat[2][index] = row[2];
		m_mat[3][index] = row[3];
	}

	void setRow(int index, const Vector4 &v)
	{
		m_mat[0][index] = v.x;
		m_mat[1][index] = v.y;
		m_mat[2][index] = v.z;
		m_mat[3][index] = v.w;
	}

	void setRow(int index, const Vector3 &v)
	{
		m_mat[0][index] = v.x;
		m_mat[1][index] = v.y;
		m_mat[2][index] = v.z;
	}

	void setColumn(int index, const float col[4])
	{
		m_mat[index][0] = col[0];
		m_mat[index][1] = col[1];
		m_mat[index][2] = col[2];
		m_mat[index][3] = col[3];
	}

	void setColumn(int index, const Vector4 &v)
	{
		m_mat[index][0] = v.x;
		m_mat[index][1] = v.y;
		m_mat[index][2] = v.z;
		m_mat[index][3] = v.w;
	}

	void setColumn(int index, const Vector3 &v)
	{
		m_mat[index][0] = v.x;
		m_mat[index][1] = v.y;
		m_mat[index][2] = v.z;
	}

	const float *get() const { return &m_mat[0][0]; }
	const float *getTranspose()
	{
		m_transpose = glm::transpose(m_mat);
		return &m_transpose[0][0];
	}

	[[nodiscard]] float getDeterminant() const { return glm::determinant(m_mat); }

	Matrix4 &identity()
	{
		m_mat = glm::mat4(1.0f);
		return *this;
	}

	Matrix4 &transpose()
	{
		m_mat = glm::transpose(m_mat);
		return *this;
	}

	Matrix4 &invert()
	{
		float det = getDeterminant();
		if (fabs(det) <= EPSILON)
		{
			return identity();
		}
		m_mat = glm::inverse(m_mat);
		return *this;
	}

	Matrix4 &invertEuclidean()
	{
		// R^-1 = R^T (rotation is orthogonal)
		// T' = -R^T * T

		// extract rotation part (3x3 upper-left)
		glm::mat3 rotation = glm::mat3(m_mat);
		glm::mat3 rotationT = glm::transpose(rotation);

		// extract translation (last column)
		glm::vec3 translation = glm::vec3(m_mat[3]);

		// new translation = -R^T * T
		glm::vec3 newTranslation = -rotationT * translation;

		// build the inverse matrix
		m_mat = glm::mat4(rotationT);
		m_mat[3] = glm::vec4(newTranslation, 1.0f);

		return *this;
	}

	Matrix4 &invertAffine() { return invert(); }

	Matrix4 &invertProjective() { return invert(); }

	Matrix4 &invertGeneral() { return invert(); }

	// transform matrix operations
	Matrix4 &translate(float x, float y, float z)
	{
		float *m = &m_mat[0][0];

		m[0] += m[3] * x;
		m[4] += m[7] * x;
		m[8] += m[11] * x;
		m[12] += m[15] * x;
		m[1] += m[3] * y;
		m[5] += m[7] * y;
		m[9] += m[11] * y;
		m[13] += m[15] * y;
		m[2] += m[3] * z;
		m[6] += m[7] * z;
		m[10] += m[11] * z;
		m[14] += m[15] * z;

		return *this;
	}

	Matrix4 &translate(const Vector3 &v) { return translate(v.x, v.y, v.z); }

	Matrix4 &rotate(float angle, const Vector3 &axis) { return rotate(angle, axis.x, axis.y, axis.z); }

	Matrix4 &rotate(float angle, float x, float y, float z)
	{
		float c = cosf(glm::radians(angle)); // cosine
		float s = sinf(glm::radians(angle)); // sine
		float c1 = 1.0f - c;                 // 1 - c

		float *m = &m_mat[0][0];
		float m0 = m[0], m4 = m[4], m8 = m[8], m12 = m[12], m1 = m[1], m5 = m[5], m9 = m[9], m13 = m[13], m2 = m[2], m6 = m[6], m10 = m[10], m14 = m[14];

		// build rotation matrix components
		float r0 = x * x * c1 + c;
		float r1 = x * y * c1 + z * s;
		float r2 = x * z * c1 - y * s;
		float r4 = x * y * c1 - z * s;
		float r5 = y * y * c1 + c;
		float r6 = y * z * c1 + x * s;
		float r8 = x * z * c1 + y * s;
		float r9 = y * z * c1 - x * s;
		float r10 = z * z * c1 + c;

		// multiply rotation matrix
		m[0] = r0 * m0 + r4 * m1 + r8 * m2;
		m[1] = r1 * m0 + r5 * m1 + r9 * m2;
		m[2] = r2 * m0 + r6 * m1 + r10 * m2;
		m[4] = r0 * m4 + r4 * m5 + r8 * m6;
		m[5] = r1 * m4 + r5 * m5 + r9 * m6;
		m[6] = r2 * m4 + r6 * m5 + r10 * m6;
		m[8] = r0 * m8 + r4 * m9 + r8 * m10;
		m[9] = r1 * m8 + r5 * m9 + r9 * m10;
		m[10] = r2 * m8 + r6 * m9 + r10 * m10;
		m[12] = r0 * m12 + r4 * m13 + r8 * m14;
		m[13] = r1 * m12 + r5 * m13 + r9 * m14;
		m[14] = r2 * m12 + r6 * m13 + r10 * m14;

		return *this;
	}

	Matrix4 &rotateX(float angle)
	{
		float c = cosf(glm::radians(angle));
		float s = sinf(glm::radians(angle));
		float *m = &m_mat[0][0];

		float m1 = m[1], m2 = m[2], m5 = m[5], m6 = m[6], m9 = m[9], m10 = m[10], m13 = m[13], m14 = m[14];

		m[1] = m1 * c + m2 * -s;
		m[2] = m1 * s + m2 * c;
		m[5] = m5 * c + m6 * -s;
		m[6] = m5 * s + m6 * c;
		m[9] = m9 * c + m10 * -s;
		m[10] = m9 * s + m10 * c;
		m[13] = m13 * c + m14 * -s;
		m[14] = m13 * s + m14 * c;

		return *this;
	}

	Matrix4 &rotateY(float angle)
	{
		float c = cosf(glm::radians(angle));
		float s = sinf(glm::radians(angle));
		float *m = &m_mat[0][0];

		float m0 = m[0], m2 = m[2], m4 = m[4], m6 = m[6], m8 = m[8], m10 = m[10], m12 = m[12], m14 = m[14];

		m[0] = m0 * c + m2 * s;
		m[2] = m0 * -s + m2 * c;
		m[4] = m4 * c + m6 * s;
		m[6] = m4 * -s + m6 * c;
		m[8] = m8 * c + m10 * s;
		m[10] = m8 * -s + m10 * c;
		m[12] = m12 * c + m14 * s;
		m[14] = m12 * -s + m14 * c;

		return *this;
	}

	Matrix4 &rotateZ(float angle)
	{
		float c = cosf(glm::radians(angle));
		float s = sinf(glm::radians(angle));
		float *m = &m_mat[0][0];

		float m0 = m[0], m1 = m[1], m4 = m[4], m5 = m[5], m8 = m[8], m9 = m[9], m12 = m[12], m13 = m[13];

		m[0] = m0 * c + m1 * -s;
		m[1] = m0 * s + m1 * c;
		m[4] = m4 * c + m5 * -s;
		m[5] = m4 * s + m5 * c;
		m[8] = m8 * c + m9 * -s;
		m[9] = m8 * s + m9 * c;
		m[12] = m12 * c + m13 * -s;
		m[13] = m12 * s + m13 * c;

		return *this;
	}

	Matrix4 &scale(float s) { return scale(s, s, s); }

	Matrix4 &scale(float sx, float sy, float sz)
	{
		float *m = &m_mat[0][0];

		m[0] *= sx;
		m[4] *= sx;
		m[8] *= sx;
		m[12] *= sx;
		m[1] *= sy;
		m[5] *= sy;
		m[9] *= sy;
		m[13] *= sy;
		m[2] *= sz;
		m[6] *= sz;
		m[10] *= sz;
		m[14] *= sz;

		return *this;
	}

	// operators
	Matrix4 operator+(const Matrix4 &rhs) const
	{
		Matrix4 result;
		result.m_mat = m_mat + rhs.m_mat;
		return result;
	}

	Matrix4 operator-(const Matrix4 &rhs) const
	{
		Matrix4 result;
		result.m_mat = m_mat - rhs.m_mat;
		return result;
	}

	Matrix4 &operator+=(const Matrix4 &rhs)
	{
		m_mat += rhs.m_mat;
		return *this;
	}

	Matrix4 &operator-=(const Matrix4 &rhs)
	{
		m_mat -= rhs.m_mat;
		return *this;
	}

	Vector4 operator*(const Vector4 &rhs) const
	{
		glm::vec4 v(rhs.x, rhs.y, rhs.z, rhs.w);
		glm::vec4 result = m_mat * v;
		return {result.x, result.y, result.z, result.w};
	}

	Vector3 operator*(const Vector3 &rhs) const
	{
		// treat Vector3 as homogeneous coordinate with w=1, but only return x,y,z
		glm::vec4 v(rhs.x, rhs.y, rhs.z, 1.0f);
		glm::vec4 result = m_mat * v;
		return {result.x, result.y, result.z};
	}

	Matrix4 operator*(const Matrix4 &rhs) const
	{
		Matrix4 result;
		result.m_mat = m_mat * rhs.m_mat;
		return result;
	}

	Matrix4 &operator*=(const Matrix4 &rhs)
	{
		m_mat *= rhs.m_mat;
		return *this;
	}

	bool operator==(const Matrix4 &rhs) const { return m_mat == rhs.m_mat; }
	bool operator!=(const Matrix4 &rhs) const { return m_mat != rhs.m_mat; }

	float operator[](int index) const { return (&m_mat[0][0])[index]; }
	float &operator[](int index) { return (&m_mat[0][0])[index]; }

	friend Matrix4 operator-(const Matrix4 &m);
	friend Matrix4 operator*(float scalar, const Matrix4 &m);
	friend Vector3 operator*(const Vector3 &vec, const Matrix4 &m);
	friend Vector4 operator*(const Vector4 &vec, const Matrix4 &m);
	friend std::ostream &operator<<(std::ostream &os, const Matrix4 &m);

private:
	glm::mat4 m_mat;
	mutable glm::mat4 m_transpose; // cache for getTranspose()
};

inline Matrix4 operator-(const Matrix4 &m)
{
	Matrix4 result;
	result.m_mat = -m.m_mat;
	return result;
}

inline Matrix4 operator*(float s, const Matrix4 &m)
{
	Matrix4 result;
	result.m_mat = s * m.m_mat;
	return result;
}

inline Vector4 operator*(const Vector4 &v, const Matrix4 &m)
{
	glm::vec4 vec(v.x, v.y, v.z, v.w);
	glm::vec4 result = vec * m.m_mat; // row-major mult
	return {result.x, result.y, result.z, result.w};
}

inline Vector3 operator*(const Vector3 &v, const Matrix4 &m)
{
	glm::vec4 vec(v.x, v.y, v.z, 1.0f);
	glm::vec4 result = vec * m.m_mat; // row-major mult
	return {result.x, result.y, result.z};
}

inline std::ostream &operator<<(std::ostream &os, const Matrix4 &m)
{
	os << std::fixed << std::setprecision(5);
	os << "[" << std::setw(10) << m[0] << " " << std::setw(10) << m[4] << " " << std::setw(10) << m[8] << " " << std::setw(10) << m[12] << "]\n"
	   << "[" << std::setw(10) << m[1] << " " << std::setw(10) << m[5] << " " << std::setw(10) << m[9] << " " << std::setw(10) << m[13] << "]\n"
	   << "[" << std::setw(10) << m[2] << " " << std::setw(10) << m[6] << " " << std::setw(10) << m[10] << " " << std::setw(10) << m[14] << "]\n"
	   << "[" << std::setw(10) << m[3] << " " << std::setw(10) << m[7] << " " << std::setw(10) << m[11] << " " << std::setw(10) << m[15] << "]\n";
	os << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
	return os;
}

#endif
