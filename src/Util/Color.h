//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		colo(u)r conversion helpers
//
// $NoKeywords: $color
//===============================================================================//

#include <algorithm>
#include <cstdint>

using Color = std::uint32_t;
using Channel = std::uint8_t;

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
	return (to_byte(a) << 24) | (to_byte(r) << 16) | (to_byte(g) << 8) | to_byte(b);
}

template <typename A, typename R, typename G, typename B>
[[deprecated("parameters should have compatible types")]]
constexpr Color argb(A a, R r, G g, B b)
    requires Numeric<A> && Numeric<R> && Numeric<G> && Numeric<B> && (!all_compatible_v<A, R, G, B>)
{
	return (to_byte(a) << 24) | (to_byte(r) << 16) | (to_byte(g) << 8) | to_byte(b);
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
    requires Numeric<R> && Numeric<G> && Numeric<B> && all_compatible_v<R, G, B, R> // use R again as the 4th type for the compatibility check
{
	return argb(static_cast<R>(255), r, g, b);
}

template <typename R, typename G, typename B>
[[deprecated("parameters should have compatible types")]]
constexpr Color rgb(R r, G g, B b)
    requires Numeric<R> && Numeric<G> && Numeric<B> && (!all_compatible_v<R, G, B, R>)
{
	return argb(static_cast<Channel>(255), r, g, b);
}

// components, integer
constexpr Channel Ri(Color color)
{
	return static_cast<Channel>((color >> 16) & 0xFF);
}

constexpr Channel Gi(Color color)
{
	return static_cast<Channel>((color >> 8) & 0xFF);
}

constexpr Channel Bi(Color color)
{
	return static_cast<Channel>(color & 0xFF);
}

constexpr Channel Ai(Color color)
{
	return static_cast<Channel>((color >> 24) & 0xFF);
}

// components, float
constexpr float Rf(Color color)
{
	return static_cast<float>(Ri(color)) / 255.0f;
}

constexpr float Gf(Color color)
{
	return static_cast<float>(Gi(color)) / 255.0f;
}

constexpr float Bf(Color color)
{
	return static_cast<float>(Bi(color)) / 255.0f;
}

constexpr float Af(Color color)
{
	return static_cast<float>(Ai(color)) / 255.0f;
}

namespace Colors
{
constexpr Color invert(Color color)
{
	return rgb(255 - Ri(color), 255 - Gi(color), 255 - Bi(color));
}

constexpr Color multiply(Color color1, Color color2)
{
	return rgb(Rf(color1) * Rf(color2), Gf(color1) * Gf(color2), Bf(color1) * Bf(color2));
}

constexpr Color add(Color color1, Color color2)
{
	return rgb(std::clamp(Rf(color1) + Rf(color2), 0.0f, 1.0f), std::clamp(Gf(color1) + Gf(color2), 0.0f, 1.0f), std::clamp(Bf(color1) + Bf(color2), 0.0f, 1.0f));
}

constexpr Color subtract(Color color1, Color color2)
{
	return rgb(std::clamp(Rf(color1) - Rf(color2), 0.0f, 1.0f), std::clamp(Gf(color1) - Gf(color2), 0.0f, 1.0f), std::clamp(Bf(color1) - Bf(color2), 0.0f, 1.0f));
}

constexpr Color scale(Color color, float multiplier)
{
	return rgb(static_cast<Channel>(static_cast<float>(Ri(color)) * multiplier), static_cast<Channel>(static_cast<float>(Gi(color)) * multiplier),
	           static_cast<Channel>(static_cast<float>(Bi(color)) * multiplier));
}
}; // namespace Colors
