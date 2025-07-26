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

static constexpr auto VECTOR_FLOAT_NORMALIZE_EPSILON = 0.000001f;
static constexpr auto VECTOR_DOUBLE_NORMALIZE_EPSILON = VECTOR_FLOAT_NORMALIZE_EPSILON / 10e6;

///////////////////////////////////////////////////////////////////////////////
// 2D vector
///////////////////////////////////////////////////////////////////////////////
struct Vector2 : public glm::vec2 {
    using glm::vec2::vec2;
    using glm::vec2::operator=;
    using glm::vec2::operator[];

    // ctors
    constexpr Vector2() : glm::vec2(0.0f, 0.0f) {}
    constexpr Vector2(float x, float y) : glm::vec2(x, y) {}
    constexpr Vector2(const glm::vec2 &other) : glm::vec2(other) {}

    // utils functions
    void zero() { *this = glm::vec2(0.0f); }
    void set(float x, float y) { *this = glm::vec2(x, y); }
    [[nodiscard]] float length() const { return glm::length(static_cast<const glm::vec2 &>(*this)); }
    [[nodiscard]] float distance(const Vector2 &vec) const {
        return glm::distance(static_cast<const glm::vec2 &>(*this), static_cast<const glm::vec2 &>(vec));
    }
    Vector2 &normalize() {
        if(glm::length(static_cast<const glm::vec2 &>(*this)) < VECTOR_FLOAT_NORMALIZE_EPSILON) return *this;
        *this = glm::normalize(static_cast<const glm::vec2 &>(*this));
        return *this;
    }
    [[nodiscard]] float dot(const Vector2 &vec) const {
        return glm::dot(static_cast<const glm::vec2 &>(*this), static_cast<const glm::vec2 &>(vec));
    }
    [[nodiscard]] bool equal(const Vector2 &vec, float e) const {
        return glm::all(glm::lessThan(
            glm::abs(static_cast<const glm::vec2 &>(*this) - static_cast<const glm::vec2 &>(vec)), glm::vec2(e)));
    }
    Vector2 &nudge(const Vector2 &vec, float amount) {
        glm::vec2 dir = static_cast<const glm::vec2 &>(*this) - static_cast<const glm::vec2 &>(vec);
        if(glm::length(dir) > VECTOR_FLOAT_NORMALIZE_EPSILON) {
            dir = glm::normalize(dir);
            *this += dir * amount;
        }
        return *this;
    }

    // operator overrides to avoid glm templating mess
    Vector2 operator-() const { return {-x, -y}; }
    Vector2 operator+(const Vector2 &rhs) const { return {x + rhs.x, y + rhs.y}; }
    Vector2 operator-(const Vector2 &rhs) const { return {x - rhs.x, y - rhs.y}; }
    Vector2 &operator+=(const Vector2 &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    Vector2 &operator-=(const Vector2 &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    Vector2 operator*(const float scale) const { return {x * scale, y * scale}; }
    Vector2 operator*(const Vector2 &rhs) const { return {x * rhs.x, y * rhs.y}; }
    Vector2 &operator*=(const float scale) {
        x *= scale;
        y *= scale;
        return *this;
    }
    Vector2 &operator*=(const Vector2 &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }
    Vector2 operator/(const float scale) const { return {x / scale, y / scale}; }
    Vector2 &operator/=(const float scale) {
        x /= scale;
        y /= scale;
        return *this;
    }
    bool operator==(const Vector2 &rhs) const { return (x == rhs.x) && (y == rhs.y); }
    bool operator!=(const Vector2 &rhs) const { return (x != rhs.x) || (y != rhs.y); }

    bool operator<(const Vector2 &rhs) const {
        if(x < rhs.x) return true;
        if(x > rhs.x) return false;
        return y < rhs.y;
    }

    friend Vector2 operator*(const float a, const Vector2 vec);
    friend std::ostream &operator<<(std::ostream &os, const Vector2 &vec);
};

inline Vector2 operator*(const float a, const Vector2 vec) { return {a * vec.x, a * vec.y}; }

inline std::ostream &operator<<(std::ostream &os, const Vector2 &vec) {
    os << "(" << vec.x << ", " << vec.y << ")";
    return os;
}

///////////////////////////////////////////////////////////////////////////////
// 2D vector (double)
///////////////////////////////////////////////////////////////////////////////
struct Vector2d : public glm::dvec2 {
    using glm::dvec2::dvec2;
    using glm::dvec2::operator=;
    using glm::dvec2::operator[];

    // ctors
    constexpr Vector2d() : glm::dvec2(0.0, 0.0) {}
    constexpr Vector2d(double x, double y) : glm::dvec2(x, y) {}
    constexpr Vector2d(const glm::dvec2 &other) : glm::dvec2(other) {}

    // utils functions
    void zero() { *this = glm::dvec2(0.0); }
    void set(double x, double y) { *this = glm::dvec2(x, y); }
    [[nodiscard]] double length() const { return glm::length(static_cast<const glm::dvec2 &>(*this)); }
    [[nodiscard]] double distance(const Vector2d &vec) const {
        return glm::distance(static_cast<const glm::dvec2 &>(*this), static_cast<const glm::dvec2 &>(vec));
    }
    Vector2d &normalize() {
        if(glm::length(static_cast<const glm::dvec2 &>(*this)) < VECTOR_DOUBLE_NORMALIZE_EPSILON) return *this;
        *this = glm::normalize(static_cast<const glm::dvec2 &>(*this));
        return *this;
    }
    [[nodiscard]] double dot(const Vector2d &vec) const {
        return glm::dot(static_cast<const glm::dvec2 &>(*this), static_cast<const glm::dvec2 &>(vec));
    }
    [[nodiscard]] bool equal(const Vector2d &vec, double e) const {
        return glm::all(glm::lessThan(
            glm::abs(static_cast<const glm::dvec2 &>(*this) - static_cast<const glm::dvec2 &>(vec)), glm::dvec2(e)));
    }
    Vector2d &nudge(const Vector2d &vec, double amount) {
        glm::dvec2 dir = static_cast<const glm::dvec2 &>(*this) - static_cast<const glm::dvec2 &>(vec);
        if(glm::length(dir) > VECTOR_DOUBLE_NORMALIZE_EPSILON) {
            dir = glm::normalize(dir);
            *this += dir * amount;
        }
        return *this;
    }

    // operator overrides to avoid glm templating mess
    Vector2d operator-() const { return {-x, -y}; }
    Vector2d operator+(const Vector2d &rhs) const { return {x + rhs.x, y + rhs.y}; }
    Vector2d operator-(const Vector2d &rhs) const { return {x - rhs.x, y - rhs.y}; }
    Vector2d &operator+=(const Vector2d &rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    Vector2d &operator-=(const Vector2d &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    Vector2d operator*(const double scale) const { return {x * scale, y * scale}; }
    Vector2d operator*(const Vector2d &rhs) const { return {x * rhs.x, y * rhs.y}; }
    Vector2d &operator*=(const double scale) {
        x *= scale;
        y *= scale;
        return *this;
    }
    Vector2d &operator*=(const Vector2d &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }
    Vector2d operator/(const double scale) const { return {x / scale, y / scale}; }
    Vector2d &operator/=(const double scale) {
        x /= scale;
        y /= scale;
        return *this;
    }
    bool operator==(const Vector2d &rhs) const { return (x == rhs.x) && (y == rhs.y); }
    bool operator!=(const Vector2d &rhs) const { return (x != rhs.x) || (y != rhs.y); }

    bool operator<(const Vector2d &rhs) const {
        if(x < rhs.x) return true;
        if(x > rhs.x) return false;
        return y < rhs.y;
    }

    friend Vector2d operator*(const float a, const Vector2d vec);
    friend std::ostream &operator<<(std::ostream &os, const Vector2d &vec);
};

inline Vector2d operator*(const float a, const Vector2d vec) { return {a * vec.x, a * vec.y}; }

inline std::ostream &operator<<(std::ostream &os, const Vector2d &vec) {
    os << "(" << vec.x << ", " << vec.y << ")";
    return os;
}

///////////////////////////////////////////////////////////////////////////////
// 3D vector
///////////////////////////////////////////////////////////////////////////////
struct Vector3 : public glm::vec3 {
    using glm::vec3::vec3;
    using glm::vec3::operator=;
    using glm::vec3::operator[];

    // ctors
    constexpr Vector3() : glm::vec3(0.0f, 0.0f, 0.0f) {}
    constexpr Vector3(float x, float y, float z) : glm::vec3(x, y, z) {}
    constexpr Vector3(const glm::vec3 &other) : glm::vec3(other) {}

    // utils functions
    void zero() { *this = glm::vec3(0.0f); }
    void set(float x, float y, float z) { *this = glm::vec3(x, y, z); }
    void setLength(float length) {
        if(glm::length(static_cast<const glm::vec3 &>(*this)) > VECTOR_FLOAT_NORMALIZE_EPSILON) {
            *this = glm::normalize(static_cast<const glm::vec3 &>(*this)) * length;
        }
    }
    [[nodiscard]] float length() const { return glm::length(static_cast<const glm::vec3 &>(*this)); }
    [[nodiscard]] float distance(const Vector3 &vec) const {
        return glm::distance(static_cast<const glm::vec3 &>(*this), static_cast<const glm::vec3 &>(vec));
    }
    Vector3 &normalize() {
        if(glm::length(static_cast<const glm::vec3 &>(*this)) < VECTOR_FLOAT_NORMALIZE_EPSILON) return *this;
        *this = glm::normalize(static_cast<const glm::vec3 &>(*this));
        return *this;
    }
    [[nodiscard]] float dot(const Vector3 &vec) const {
        return glm::dot(static_cast<const glm::vec3 &>(*this), static_cast<const glm::vec3 &>(vec));
    }
    [[nodiscard]] Vector3 cross(const Vector3 &vec) const {
        return glm::cross(static_cast<const glm::vec3 &>(*this), static_cast<const glm::vec3 &>(vec));
    }
    [[nodiscard]] bool equal(const Vector3 &vec, float e) const {
        return glm::all(glm::lessThan(
            glm::abs(static_cast<const glm::vec3 &>(*this) - static_cast<const glm::vec3 &>(vec)), glm::vec3(e)));
    }
    Vector3 &nudge(const Vector3 &vec, float amount) {
        glm::vec3 dir = static_cast<const glm::vec3 &>(*this) - static_cast<const glm::vec3 &>(vec);
        if(glm::length(dir) > VECTOR_FLOAT_NORMALIZE_EPSILON) {
            dir = glm::normalize(dir);
            *this += dir * amount;
        }
        return *this;
    }

    // operator overrides to avoid glm templating mess
    Vector3 operator-() const { return {-x, -y, -z}; }
    Vector3 operator+(const Vector3 &rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    Vector3 operator-(const Vector3 &rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    Vector3 &operator+=(const Vector3 &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    Vector3 &operator-=(const Vector3 &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
    Vector3 operator*(const float scale) const { return {x * scale, y * scale, z * scale}; }
    Vector3 operator*(const Vector3 &rhs) const { return {x * rhs.x, y * rhs.y, z * rhs.z}; }
    Vector3 &operator*=(const float scale) {
        x *= scale;
        y *= scale;
        z *= scale;
        return *this;
    }
    Vector3 &operator*=(const Vector3 &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }
    Vector3 operator/(const float scale) const { return {x / scale, y / scale, z / scale}; }
    Vector3 &operator/=(const float scale) {
        x /= scale;
        y /= scale;
        z /= scale;
        return *this;
    }
    bool operator==(const Vector3 &rhs) const { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z); }
    bool operator!=(const Vector3 &rhs) const { return (x != rhs.x) || (y != rhs.y) || (z != rhs.z); }

    bool operator<(const Vector3 &rhs) const {
        if(x < rhs.x) return true;
        if(x > rhs.x) return false;
        if(y < rhs.y) return true;
        if(y > rhs.y) return false;
        return z < rhs.z;
    }

    friend Vector3 operator*(const float a, const Vector3 vec);
    friend std::ostream &operator<<(std::ostream &os, const Vector3 &vec);
};

inline Vector3 operator*(const float a, const Vector3 vec) { return {a * vec.x, a * vec.y, a * vec.z}; }

inline std::ostream &operator<<(std::ostream &os, const Vector3 &vec) {
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return os;
}

///////////////////////////////////////////////////////////////////////////////
// 4D vector
///////////////////////////////////////////////////////////////////////////////
struct Vector4 : public glm::vec4 {
    using glm::vec4::vec4;
    using glm::vec4::operator=;
    using glm::vec4::operator[];

    // ctors
    constexpr Vector4() : glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) {}
    constexpr Vector4(float x, float y, float z, float w) : glm::vec4(x, y, z, w) {}
    constexpr Vector4(const glm::vec4 &other) : glm::vec4(other) {}

    // utils functions - cast to base type when calling GLM functions
    void set(float x, float y, float z, float w) { *this = glm::vec4(x, y, z, w); }
    [[nodiscard]] float length() const { return glm::length(static_cast<const glm::vec4 &>(*this)); }
    [[nodiscard]] float distance(const Vector4 &vec) const {
        return glm::distance(static_cast<const glm::vec4 &>(*this), static_cast<const glm::vec4 &>(vec));
    }
    Vector4 &normalize() {
        if(glm::length(static_cast<const glm::vec4 &>(*this)) < VECTOR_FLOAT_NORMALIZE_EPSILON) return *this;
        *this = glm::normalize(static_cast<const glm::vec4 &>(*this));
        return *this;
    }
    [[nodiscard]] float dot(const Vector4 &vec) const {
        return glm::dot(static_cast<const glm::vec4 &>(*this), static_cast<const glm::vec4 &>(vec));
    }
    [[nodiscard]] bool equal(const Vector4 &vec, float e) const {
        return glm::all(glm::lessThan(
            glm::abs(static_cast<const glm::vec4 &>(*this) - static_cast<const glm::vec4 &>(vec)), glm::vec4(e)));
    }
    Vector4 &nudge(const Vector4 &vec, float amount) {
        glm::vec4 dir = static_cast<const glm::vec4 &>(*this) - static_cast<const glm::vec4 &>(vec);
        if(glm::length(dir) > VECTOR_FLOAT_NORMALIZE_EPSILON) {
            dir = glm::normalize(dir);
            *this += dir * amount;
        }
        return *this;
    }

    // operator overrides to avoid glm templating mess
    Vector4 operator-() const { return {-x, -y, -z, -w}; }
    Vector4 operator+(const Vector4 &rhs) const { return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w}; }
    Vector4 operator-(const Vector4 &rhs) const { return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w}; }
    Vector4 &operator+=(const Vector4 &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }
    Vector4 &operator-=(const Vector4 &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }
    Vector4 operator*(const float scale) const { return {x * scale, y * scale, z * scale, w * scale}; }
    Vector4 operator*(const Vector4 &rhs) const { return {x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w}; }
    Vector4 &operator*=(const float scale) {
        x *= scale;
        y *= scale;
        z *= scale;
        w *= scale;
        return *this;
    }
    Vector4 &operator*=(const Vector4 &rhs) {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }
    Vector4 operator/(const float scale) const { return {x / scale, y / scale, z / scale, w / scale}; }
    Vector4 &operator/=(const float scale) {
        x /= scale;
        y /= scale;
        z /= scale;
        w /= scale;
        return *this;
    }
    bool operator==(const Vector4 &rhs) const { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w); }
    bool operator!=(const Vector4 &rhs) const { return (x != rhs.x) || (y != rhs.y) || (z != rhs.z) || (w != rhs.w); }

    bool operator<(const Vector4 &rhs) const {
        if(x < rhs.x) return true;
        if(x > rhs.x) return false;
        if(y < rhs.y) return true;
        if(y > rhs.y) return false;
        if(z < rhs.z) return true;
        if(z > rhs.z) return false;
        return w < rhs.w;
    }

    friend Vector4 operator*(const float a, const Vector4 vec);
    friend std::ostream &operator<<(std::ostream &os, const Vector4 &vec);
};

inline Vector4 operator*(const float a, const Vector4 vec) { return {a * vec.x, a * vec.y, a * vec.z, a * vec.w}; }

inline std::ostream &operator<<(std::ostream &os, const Vector4 &vec) {
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
    return os;
}

#endif
