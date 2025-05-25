//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		colo(u)r conversion helpers
//
// $NoKeywords: $color
//===============================================================================//

#pragma once
#ifndef COLOR_H
#define COLOR_H

#include <algorithm>
#include <cstdint>

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#define IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) ||                \
    defined(__MIPSEL__) || defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) ||               \
    defined(_M_AMD64)
#define IS_LITTLE_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
#define IS_LITTLE_ENDIAN 0
#else
#error "impossible"
#endif

using Channel = std::uint8_t;

// argb color union
union Color {
	std::uint32_t v;

#if IS_LITTLE_ENDIAN
	struct
	{
		Channel b, g, r, a;
	};
#else
	struct
	{
		Channel a, r, g, b;
	};
#endif

	Color() : v(0) {}
	Color(std::uint32_t val) : v(val) {}

	constexpr Color(Channel alpha, Channel red, Channel green, Channel blue)
	{
		v = (static_cast<std::uint32_t>(alpha) << 24) | (static_cast<std::uint32_t>(red) << 16) | (static_cast<std::uint32_t>(green) << 8) |
		    static_cast<std::uint32_t>(blue);
	}

	template <typename T = float>
	[[nodiscard]] constexpr float Af() const
	{
		return static_cast<T>(static_cast<float>(a) / 255.0f);
	}
	template <typename T = float>
	[[nodiscard]] constexpr float Rf() const
	{
		return static_cast<T>(static_cast<float>(r) / 255.0f);
	}
	template <typename T = float>
	[[nodiscard]] constexpr float Gf() const
	{
		return static_cast<T>(static_cast<float>(g) / 255.0f);
	}
	template <typename T = float>
	[[nodiscard]] constexpr float Bf() const
	{
		return static_cast<T>(static_cast<float>(b) / 255.0f);
	}

	// clang-format off
	constexpr Color& operator&=(std::uint32_t val) { v &= val; return *this; }
	constexpr Color& operator|=(std::uint32_t val) { v |= val; return *this; }
	constexpr Color& operator^=(std::uint32_t val) { v ^= val; return *this; }
	constexpr Color& operator<<=(int shift) { v <<= shift; return *this; }
	constexpr Color& operator>>=(int shift) { v >>= shift; return *this; }

	constexpr Color& operator&=(const Color& other) { v &= other.v; return *this; }
	constexpr Color& operator|=(const Color& other) { v |= other.v; return *this; }
	constexpr Color& operator^=(const Color& other) { v ^= other.v; return *this; }
	//clang-format on

	operator std::uint32_t() const { return v; }
};

template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

constexpr Channel to_byte(Numeric auto value)
{
	if constexpr (std::is_floating_point_v<decltype(value)>)
		return static_cast<Channel>(std::clamp<decltype(value)>(value, 0, 1) * 255);
	else
		return static_cast<Channel>(std::clamp<Channel>(value, 0, 255));
}

// helper to detect if types are "compatible"
template <typename T, typename U>
constexpr bool is_compatible_v = std::is_same_v<T, U> ||
                                 // integer literals
                                 (std::is_integral_v<T> && std::is_integral_v<U> && (std::is_convertible_v<T, U> || std::is_convertible_v<U, T>)) ||
                                 // same "family" of types (all floating or all integral)
                                 (std::is_floating_point_v<T> && std::is_floating_point_v<U>) || (std::is_integral_v<T> && std::is_integral_v<U>);

// check if all four types are compatible with each other
template <typename A, typename R, typename G, typename B>
constexpr bool all_compatible_v =
    is_compatible_v<A, R> && is_compatible_v<A, G> && is_compatible_v<A, B> && is_compatible_v<R, G> && is_compatible_v<R, B> && is_compatible_v<G, B>;

// main conversion func
template <typename A, typename R, typename G, typename B>
constexpr Color argb(A a, R r, G g, B b)
    requires Numeric<A> && Numeric<R> && Numeric<G> && Numeric<B> && all_compatible_v<A, R, G, B>
{
	return Color(to_byte(a), to_byte(r), to_byte(g), to_byte(b));
}

template <typename A, typename R, typename G, typename B>
[[deprecated("parameters should have compatible types")]]
constexpr Color argb(A a, R r, G g, B b)
    requires Numeric<A> && Numeric<R> && Numeric<G> && Numeric<B> && (!all_compatible_v<A, R, G, B>)
{
	return Color(to_byte(a), to_byte(r), to_byte(g), to_byte(b));
}

// convenience
template <typename R, typename G, typename B, typename A>
constexpr Color rgba(R r, G g, B b, A a)
    requires Numeric<R> && Numeric<G> && Numeric<B> && Numeric<A> && all_compatible_v<R, G, B, A>
{
	return argb(a, r, g, b);
}

template <typename R, typename G, typename B, typename A>
[[deprecated("parameters should have compatible types")]]
constexpr Color rgba(R r, G g, B b, A a)
    requires Numeric<R> && Numeric<G> && Numeric<B> && Numeric<A> && (!all_compatible_v<R, G, B, A>)
{
	return argb(a, r, g, b);
}

template <typename R, typename G, typename B>
constexpr Color rgb(R r, G g, B b)
    requires Numeric<R> && Numeric<G> && Numeric<B> && all_compatible_v<R, G, B, R>
{
	return Color(255, to_byte(r), to_byte(g), to_byte(b));
}

template <typename R, typename G, typename B>
[[deprecated("parameters should have compatible types")]]
constexpr Color rgb(R r, G g, B b)
    requires Numeric<R> && Numeric<G> && Numeric<B> && (!all_compatible_v<R, G, B, R>)
{
	return Color(255, to_byte(r), to_byte(g), to_byte(b));
}

namespace Colors
{
constexpr Color invert(Color color)
{
	return {color.a, static_cast<Channel>(255 - color.r), static_cast<Channel>(255 - color.g), static_cast<Channel>(255 - color.b)};
}

constexpr Color multiply(Color color1, Color color2)
{
	return rgb(color1.Rf() * color2.Rf(), color1.Gf() * color2.Gf(), color1.Bf() * color2.Bf());
}

constexpr Color add(Color color1, Color color2)
{
	return rgb(std::clamp(color1.Rf() + color2.Rf(), 0.0f, 1.0f), std::clamp(color1.Gf() + color2.Gf(), 0.0f, 1.0f),
	           std::clamp(color1.Bf() + color2.Bf(), 0.0f, 1.0f));
}

constexpr Color subtract(Color color1, Color color2)
{
	return rgb(std::clamp(color1.Rf() - color2.Rf(), 0.0f, 1.0f), std::clamp(color1.Gf() - color2.Gf(), 0.0f, 1.0f),
	           std::clamp(color1.Bf() - color2.Bf(), 0.0f, 1.0f));
}

constexpr Color scale(Color color, float multiplier)
{
	return {color.a, static_cast<Channel>(static_cast<float>(color.r) * multiplier), static_cast<Channel>(static_cast<float>(color.g) * multiplier),
	        static_cast<Channel>(static_cast<float>(color.b) * multiplier)};
}
} // namespace Colors

#endif
