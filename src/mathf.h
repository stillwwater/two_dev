// Copyright (c) 2020 stillwwater
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef TWO_MATH_H
#define TWO_MATH_H

#include <cmath>
#include <cstdint>

#include "debug.h"
#include "config.h"

#ifdef TWO_SSE
#include <xmmintrin.h>
#endif

#define PI 3.14159265358979
#define E  2.71828182845904

namespace two {

constexpr float Epsilon  = 1.0e-6f;
constexpr float DegToRad = PI / 180.0;
constexpr float RadToDeg = 180.0 / PI;

// Makes certain code easier to read.
constexpr int AxisX = 0;
constexpr int AxisY = 1;
constexpr int AxisZ = 2;
constexpr int AxisW = 3;

struct Vector2;
struct Vector2i;
struct Vector3;
struct Vector4;

struct Rect {
    float x, y, w, h;

    Rect() {}
    Rect(float x, float y, float w, float h)
        : x{x}, y{y}, w{w}, h{h} {}

    Rect(const Vector2 &position, const Vector2 &size);

    static inline Rect zero();

    bool operator==(const Rect &rect) const;
    bool operator!=(const Rect &rect) const;

    bool contains(const Vector2 &v) const;
    bool overlaps(const Rect &rect) const;
};

struct Vector2 {
    float x, y;

    Vector2() {}
    Vector2(float x, float y) : x{x}, y{y} {}
    Vector2(const Vector2i &vec2i);

    explicit Vector2(float s) : x{s}, y{s} {}
    explicit Vector2(const Vector3 &vec3);
    explicit Vector2(const Vector4 &vec4);

    static inline Vector2 zero();
    static inline Vector2 one();

    float operator[](int i) const;
    float &operator[](int i);

    bool operator==(const Vector2 &v) const;
    bool operator!=(const Vector2 &v) const;

    // All operations are component wise
    Vector2 operator-() const;
    Vector2 &operator+=(const Vector2 &v);
    Vector2 &operator-=(const Vector2 &v);
    Vector2 &operator*=(float s);
    Vector2 &operator/=(float s);
    Vector2 &operator*=(const Vector2 &v);
    Vector2 &operator/=(const Vector2 &v);

    // Returns the vector's magnitude
    float length() const;
    float length_sqr() const;

    // Returns a normalized copy of the vector with length 1
    Vector2 normalized() const;

    // Returns the unsigned angle in degrees between this vector and another
    float angle(const Vector2 &v) const;
};

// Transform Component
// Position, scale and rotation of an entity.
struct Transform {
    Vector2 position;
    Vector2 scale;

    // Rotation on the Z axis in degrees
    float rotation;

    // Uninitialized
    Transform() {}

    Transform(const Vector2 &position)
        : position{position}, scale{1.0f, 1.0f}, rotation{0.0f} {}

    Transform(const Vector2 &position, const Vector2 &scale)
        : position{position}, scale{scale}, rotation{0.0f} {}

    Transform(const Vector2 &position, const Vector2 &scale, float rotation)
        : position{position}, scale{scale}, rotation{rotation} {}
};

// Similar to the Transform component but the position is given in screen
// pixels instead of being a world position. This is useful for drawing
// UI overlays.
struct PixelTransform {
    Vector2 position;
    Vector2 scale;

    PixelTransform() {}

    PixelTransform(const Vector2 &position)
        : position{position}, scale{1.0f, 1.0f} {}

    PixelTransform(const Vector2 &position, const Vector2 &scale)
        : position{position}, scale{scale} {}
};

struct Vector2i {
    int x, y;

    Vector2i() {}
    Vector2i(int x, int y) : x{x}, y{y} {}

    explicit Vector2i(const Vector2 &vec2);
    explicit Vector2i(int s) : x{s}, y{s} {}

    static inline Vector2i zero();
    static inline Vector2i one();

    int operator[](int i) const;
    int &operator[](int i);

    bool operator==(const Vector2i &v) const;
    bool operator!=(const Vector2i &v) const;

    // All operations are component wise
    Vector2i operator-() const;
    Vector2i &operator+=(const Vector2i &v);
    Vector2i &operator-=(const Vector2i &v);
    Vector2i &operator*=(int s);
    Vector2i &operator/=(int s);
    Vector2i &operator*=(const Vector2i &v);
    Vector2i &operator/=(const Vector2i &v);

    // Returns the vector's magnitude
    float length() const;
    int length_sqr() const;
};

struct Vector3 {
    float x, y, z;

    Vector3() {}
    Vector3(float x, float y, float z) : x{x}, y{y}, z{z} {}
    Vector3(const Vector2 &vec2);

    explicit Vector3(const Vector4 &vec4);

    // Broadcast init
    explicit Vector3(float s) : x{s}, y{s}, z{s} {}

    static inline Vector3 zero();
    static inline Vector3 one();

    float operator[](int i) const;
    float &operator[](int i);

    bool operator==(const Vector3 &v) const;
    bool operator!=(const Vector3 &v) const;

    // All operations are component wise
    Vector3 operator-() const;
    Vector3 &operator+=(const Vector3 &v);
    Vector3 &operator-=(const Vector3 &v);
    Vector3 &operator*=(float s);
    Vector3 &operator/=(float s);
    Vector3 &operator*=(const Vector3 &v);
    Vector3 &operator/=(const Vector3 &v);

    // Returns the vector's magnitude
    float length() const;
    float length_sqr() const;

    // Returns a normalized copy of the vector with length 1
    Vector3 normalized() const;
};

struct Vector4 {
#ifdef TWO_SSE
    union {
        struct {
            float x, y, z, w;
        };
        __m128 m128;
    };
    Vector4(const __m128 &m) : m128{m} {}
#else
    float x, y, z, w;
#endif

    Vector4() {}

    Vector4(float x, float y, float z, float w);

    Vector4(const Vector3 &vec3);
    Vector4(const Vector2 &vec2);

    // Broadcast init
    explicit Vector4(float s);

    static inline Vector4 zero();
    static inline Vector4 one();

    float operator[](int i) const;
    float &operator[](int i);

    bool operator==(const Vector4 &v) const;
    bool operator!=(const Vector4 &v) const;

    // All operations are component wise
    Vector4 operator-() const;
    Vector4 &operator+=(const Vector4 &v);
    Vector4 &operator-=(const Vector4 &v);
    Vector4 &operator*=(float s);
    Vector4 &operator/=(float s);
    Vector4 &operator*=(const Vector4 &v);
    Vector4 &operator/=(const Vector4 &v);

    // Returns the vector's magnitude
    float length() const;
    float length_sqr() const;

    // Returns a normalized copy of the vector with length 1
    Vector4 normalized() const;
};

//
// Misc
//

inline float madd(float a, float b, float c) {
    return a * b + c;
}

inline float clamp(float value, float minv, float maxv) {
    if (value < minv) return minv;
    if (value > maxv) return maxv;
    return value;
}

inline int clampi(int value, int minv, int maxv) {
    if (value < minv) return minv;
    if (value > maxv) return maxv;
    return value;
}

inline float clamp01(float value) {
    return clamp(value, 0.0f, 1.0f);
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * clamp01(t);
}

inline float signf(float value) {
    if (value > 0.0f)
        return 1.0f;
    if (value < 0.0f)
        return -1.0f;
    return 0.0f;
}

inline uint64_t next_pow2(uint64_t value) {
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return value + 1;
}

//
// Rect
//

inline Rect::Rect(const Vector2 &position, const Vector2 &size) {
    x = position.x;
    y = position.y;
    w = size.x;
    h = size.y;
}

inline Rect Rect::zero() {
    return Rect{0.0f, 0.0f, 0.0f, 0.0f};
}

inline bool Rect::operator==(const Rect &rect) const {
    return x == rect.x && y == rect.y && w == rect.w && h == rect.h;
}

inline bool Rect::operator!=(const Rect &rect) const {
    return !(*this == rect);
}

inline bool Rect::contains(const Vector2 &v) const {
    return v.x >= x && v.y >= v.y && v.x < (w + x) && v.y < (h + y);
}

inline bool Rect::overlaps(const Rect &rect) const {
    return ((rect.x + rect.w) > x && rect.x < (x + w)
            && (rect.y + rect.h) > y && rect.y < (x + h));
}

//
// Vector2
//

inline Vector2 operator+(const Vector2 &a, const Vector2 &b) {
    return Vector2{a.x + b.x, a.y + b.y};
}

inline Vector2 operator-(const Vector2 &a, const Vector2 &b) {
    return Vector2{a.x - b.x, a.y - b.y};
}

inline Vector2 operator*(const Vector2 &v, float s) {
    return Vector2{v.x * s, v.y * s};
}

inline Vector2 operator*(float s, const Vector2 &v) {
    return Vector2{s * v.x, s * v.y};
}

inline Vector2 operator/(const Vector2 &v, float s) {
    float invs = 1.0f / s;
    return Vector2{v.x * invs, v.y * invs};
}

inline Vector2 operator*(const Vector2 &a, const Vector2 &b) {
    return Vector2{a.x * b.x, a.y * b.y};
}

inline Vector2 operator/(const Vector2 &a, const Vector2 &b) {
    return Vector2{a.x / b.x, a.y / b.y};
}

inline float dot(const Vector2 &a, const Vector2 &b) {
    return a.x * b.x + a.y * b.y;
}

inline Vector2 lerp(const Vector2 &a, const Vector2 &b, float t) {
    t = clamp01(t);
    return Vector2{a.x + (b.x - a.x) * t,
                   a.y + (b.y - a.y) * t};
}

inline Vector2 clamp(const Vector2 &v, float minv, float maxv) {
    return Vector2{clamp(v.x, minv, maxv),
                   clamp(v.y, minv, maxv)};
}

inline Vector2 clamp01(const Vector2 &v) {
    return clamp(v, 0.0f, 1.0f);
}

inline Vector2 vec_min(const Vector2 &a, const Vector2 &b) {
    return Vector2{fminf(a.x, b.x), fminf(a.y, b.y)};
}

inline Vector2 vec_max(const Vector2 &a, const Vector2 &b) {
    return Vector2{fmaxf(a.x, b.x), fmaxf(a.y, b.y)};
}

inline Vector2 vec_abs(const Vector2 &v) {
    return Vector2{fabsf(v.x), fabsf(v.y)};
}

inline Vector2 reflect(const Vector2 &v, const Vector2 &normal) {
    float s = 2.0f * dot(v, normal);
    return Vector2{v.x - s * normal.x,
                   v.y - s * normal.y};
}

inline Vector2::Vector2(const Vector2i &vec2)
    : x{float(vec2.x)}, y{float(vec2.y)} {}

inline Vector2::Vector2(const Vector3 &vec3)
    : x{vec3.x}, y{vec3.y} {}

inline Vector2::Vector2(const Vector4 &vec4)
    : x{vec4.x}, y{vec4.y} {}

inline Vector2 Vector2::zero() {
    return Vector2{0.0f, 0.0f};
}

inline Vector2 Vector2::one() {
    return Vector2{1.0f, 1.0f};
}

inline float Vector2::operator[](int i) const {
    ASSERT(i >= 0 && i < 2);
    return ((float *)this)[i];
}

inline float &Vector2::operator[](int i) {
    ASSERT(i >= 0 && i < 2);
    return ((float *)this)[i];
}

inline bool Vector2::operator==(const Vector2 &v) const {
    return x == v.x && y == v.y;
}

inline bool Vector2::operator!=(const Vector2 &v) const {
    return !(*this == v);
}

inline Vector2 Vector2::operator-() const {
    return Vector2{-x, -y};
}

inline Vector2 &Vector2::operator+=(const Vector2 &v) {
    x += v.x;
    y += v.y;
    return *this;
}

inline Vector2 &Vector2::operator-=(const Vector2 &v) {
    x -= v.x;
    y -= v.y;
    return *this;
}

inline Vector2 &Vector2::operator*=(float s) {
    x *= s;
    y *= s;
    return *this;
}

inline Vector2 &Vector2::operator/=(float s) {
    float invs = 1.0f / s;
    x *= invs;
    y *= invs;
    return *this;
}

inline Vector2 &Vector2::operator*=(const Vector2 &v) {
    x *= v.x;
    y *= v.y;
    return *this;
}

inline Vector2 &Vector2::operator/=(const Vector2 &v) {
    x /= v.x;
    y /= v.y;
    return *this;
}

inline float Vector2::length() const {
    return sqrtf(x * x + y * y);
}

inline float Vector2::length_sqr() const {
    return x * x + y * y;
}

inline Vector2 Vector2::normalized() const {
    float len = length();
    if (len != 0.0f) {
        float invl = 1.0f / len;
        return Vector2{x * invl, y * invl};
    }
    return Vector2{0.0f, 0.0f};
}

inline float Vector2::angle(const Vector2 &v) const {
    float denom = sqrtf(length_sqr() * v.length_sqr());
    if (denom < Epsilon * Epsilon) {
        return 0.0f;
    }
    return acosf(clamp(dot(*this, v) / denom, -1.0f, 1.0f)) * RadToDeg;
}

//
// Vector2i
//

inline Vector2i operator+(const Vector2i &a, const Vector2i &b) {
    return Vector2i{a.x + b.x, a.y + b.y};
}

inline Vector2i operator-(const Vector2i &a, const Vector2i &b) {
    return Vector2i{a.x - b.x, a.y - b.y};
}

inline Vector2i operator*(const Vector2i &v, int s) {
    return Vector2i{v.x * s, v.y * s};
}

inline Vector2i operator*(int s, const Vector2i &v) {
    return Vector2i{s * v.x, s * v.y};
}

inline Vector2i operator/(const Vector2i &v, int s) {
    return Vector2i{v.x / s, v.y / s};
}

inline Vector2i operator*(const Vector2i &a, const Vector2i &b) {
    return Vector2i{a.x * b.x, a.y * b.y};
}

inline Vector2i operator/(const Vector2i &a, const Vector2i &b) {
    return Vector2i{a.x / b.x, a.y / b.y};
}

inline int dot(const Vector2i &a, const Vector2i &b) {
    return a.x * b.x + a.y * b.y;
}

inline Vector2i clamp(const Vector2i &v, float minv, float maxv) {
    return Vector2i{int(clamp(v.x, minv, maxv)),
                    int(clamp(v.y, minv, maxv))};
}

inline Vector2i reflect(const Vector2i &v, const Vector2i &normal) {
    int s = 2 * dot(v, normal);
    return Vector2i{v.x - s * normal.x,
                    v.y - s * normal.y};
}

inline Vector2i vec_abs(const Vector2i &v) {
    return Vector2i{abs(v.x), abs(v.y)};
}

inline Vector2i::Vector2i(const Vector2 &vec2)
    : x{int(vec2.x)}, y{int(vec2.y)} {}

inline Vector2i Vector2i::zero() {
    return Vector2i{0, 0};
}

inline Vector2i Vector2i::one() {
    return Vector2i{1, 1};
}

inline int Vector2i::operator[](int i) const {
    ASSERT(i >= 0 && i < 2);
    return ((int  *)this)[i];
}

inline int &Vector2i::operator[](int i) {
    ASSERT(i >= 0 && i < 2);
    return ((int *)this)[i];
}

inline bool Vector2i::operator==(const Vector2i &v) const {
    return x == v.x && y == v.y;
}

inline bool Vector2i::operator!=(const Vector2i &v) const {
    return !(*this == v);
}

inline Vector2i Vector2i::operator-() const {
    return Vector2i{-x, -y};
}

inline Vector2i &Vector2i::operator+=(const Vector2i &v) {
    x += v.x;
    y += v.y;
    return *this;
}

inline Vector2i &Vector2i::operator-=(const Vector2i &v) {
    x -= v.x;
    y -= v.y;
    return *this;
}

inline Vector2i &Vector2i::operator*=(int s) {
    x *= s;
    y *= s;
    return *this;
}

inline Vector2i &Vector2i::operator/=(int s) {
    x /= s;
    y /= s;
    return *this;
}

inline Vector2i &Vector2i::operator*=(const Vector2i &v) {
    x *= v.x;
    y *= v.y;
    return *this;
}

inline Vector2i &Vector2i::operator/=(const Vector2i &v) {
    x /= v.x;
    y /= v.y;
    return *this;
}

inline float Vector2i::length() const {
    return sqrtf(x * x + y * y);
}

inline int Vector2i::length_sqr() const {
    return x * x + y * y;
}

//
// Vector3
//

inline Vector3 operator+(const Vector3 &a, const Vector3 &b) {
    return Vector3{a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Vector3 operator-(const Vector3 &a, const Vector3 &b) {
    return Vector3{a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Vector3 operator*(const Vector3 &v, float s) {
    return Vector3{v.x * s, v.y * s, v.z * s};
}

inline Vector3 operator*(float s, const Vector3 &v) {
    return Vector3{s * v.x, s * v.y, s * v.z};
}

inline Vector3 operator/(const Vector3 &v, float s) {
    float invs = 1.0f / s;
    return Vector3{v.x * invs, v.y * invs, v.z * invs};
}

inline Vector3 operator*(const Vector3 &a, const Vector3 &b) {
    return Vector3{a.x * b.x, a.y * b.y, a.z * b.z};
}

inline Vector3 operator/(const Vector3 &a, const Vector3 &b) {
    return Vector3{a.x / b.x, a.y / b.y, a.z / b.z};
}

inline float dot(const Vector3 &a, const Vector3 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3 cross(const Vector3 &a, const Vector3 &b) {
    return Vector3{a.y * b.z - a.z * b.y,
                   a.z * b.x - a.x * b.z,
                   a.x * b.y - a.y * b.x};
}

inline Vector3 lerp(const Vector3 &a, const Vector3 &b, float t) {
    t = clamp01(t);
    return Vector3{a.x + (b.x - a.x) * t,
                   a.y + (b.y - a.y) * t,
                   a.z + (b.z - a.z) * t};
}

inline Vector3 clamp(const Vector3 &v, float minv, float maxv) {
    return Vector3{clamp(v.x, minv, maxv),
                   clamp(v.y, minv, maxv),
                   clamp(v.z, minv, maxv)};
}

inline Vector3 clamp01(const Vector3 &v) {
    return clamp(v, 0.0f, 1.0f);
}

inline Vector3 vec_min(const Vector3 &a, const Vector3 &b) {
    return Vector3{fminf(a.x, b.x),
                   fminf(a.y, b.y),
                   fminf(a.z, b.z)};
}

inline Vector3 vec_max(const Vector3 &a, const Vector3 &b) {
    return Vector3{fmaxf(a.x, b.x),
                   fmaxf(a.y, b.y),
                   fmaxf(a.z, b.z)};
}

inline Vector3 vec_abs(const Vector3 &v) {
    return Vector3{fabsf(v.x),
                   fabsf(v.y),
                   fabsf(v.z)};
}

inline Vector3 reflect(const Vector3 &v, const Vector3 &normal) {
    float s = 2.0f * dot(v, normal);
    return Vector3{v.x - s * normal.x,
                   v.y - s * normal.y,
                   v.z - s * normal.z};
}

inline Vector3::Vector3(const Vector2 &vec2)
    : x{vec2.x}, y{vec2.y}, z{0.0f} {}

inline Vector3::Vector3(const Vector4 &vec4)
    : x{vec4.x}, y{vec4.y}, z{vec4.z} {}

inline Vector3 Vector3::zero() {
    return Vector3{0.0f, 0.0f, 0.0f};
}

inline Vector3 Vector3::one() {
    return Vector3{1.0f, 1.0f, 1.0f};
}

inline float Vector3::operator[](int i) const {
    ASSERT(i >= 0 && i < 3);
    return ((float *)this)[i];
}

inline float &Vector3::operator[](int i) {
    ASSERT(i >= 0 && i < 3);
    return ((float *)this)[i];
}

inline bool Vector3::operator==(const Vector3 &v) const {
    return x == v.x && y == v.y && z == v.z;
}

inline bool Vector3::operator!=(const Vector3 &v) const {
    return !(*this == v);
}

inline Vector3 Vector3::operator-() const {
    return Vector3{-x, -y, -z};
}

inline Vector3 &Vector3::operator+=(const Vector3 &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

inline Vector3 &Vector3::operator-=(const Vector3 &v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

inline Vector3 &Vector3::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

inline Vector3 &Vector3::operator/=(float s) {
    float invs = 1.0f / s;
    x *= invs;
    y *= invs;
    z *= invs;
    return *this;
}

inline Vector3 &Vector3::operator*=(const Vector3 &v) {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    return *this;
}

inline Vector3 &Vector3::operator/=(const Vector3 &v) {
    x /= v.x;
    y /= v.y;
    z /= v.z;
    return *this;
}

inline float Vector3::length() const {
    return sqrtf(x * x + y * y + z * z);
}

inline float Vector3::length_sqr() const {
    return x * x + y * y + z * z;
}

inline Vector3 Vector3::normalized() const {
    float len = length();
    if (len != 0.0f) {
        float invl = 1.0f / len;
        return Vector3{x * invl, y * invl, z * invl};
    }
    return Vector3{0.0f, 0.0f, 0.0f};
}

//
// Vector4
//

inline Vector4 operator+(const Vector4 &a, const Vector4 &b) {
#ifdef TWO_SSE
    return Vector4{_mm_add_ps(a.m128, b.m128)};
#else
    return Vector4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
#endif
}

inline Vector4 operator-(const Vector4 &a, const Vector4 &b) {
#ifdef TWO_SSE
    return Vector4{_mm_sub_ps(a.m128, b.m128)};
#else
    return Vector4{a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
#endif
}

inline Vector4 operator*(const Vector4 &v, float s) {
    return Vector4{v.x * s, v.y * s, v.z * s, v.w * s};
}

inline Vector4 operator*(float s, const Vector4 &v) {
    return Vector4{s * v.x, s * v.y, s * v.z, s * v.w};
}

inline Vector4 operator/(const Vector4 &v, float s) {
    float invs = 1.0f / s;
    return Vector4{v.x * invs, v.y * invs, v.z * invs, v.w * invs};
}

inline Vector4 operator*(const Vector4 &a, const Vector4 &b) {
#ifdef TWO_SSE
    return Vector4{_mm_mul_ps(a.m128, b.m128)};
#else
    return Vector4{a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
#endif
}

inline Vector4 operator/(const Vector4 &a, const Vector4 &b) {
#ifdef TWO_SSE
    return Vector4{_mm_div_ps(a.m128, b.m128)};
#else
    return Vector4{a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
#endif
}

inline float dot(const Vector4 &a, const Vector4 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline Vector4 lerp(const Vector4 &a, const Vector4 &b, float t) {
    t = clamp01(t);
    return Vector4{a.x + (b.x - a.x) * t,
                   a.y + (b.y - a.y) * t,
                   a.z + (b.z - a.z) * t,
                   a.w + (b.w - a.w) * t};
}

inline Vector4 clamp(const Vector4 &v, float minv, float maxv) {
    return Vector4{clamp(v.x, minv, maxv),
                   clamp(v.y, minv, maxv),
                   clamp(v.z, minv, maxv),
                   clamp(v.w, minv, maxv)};
}

inline Vector4 clamp01(const Vector4 &v) {
    return clamp(v, 0.0f, 1.0f);
}

inline Vector4 vec_min(const Vector4 &a, const Vector4 &b) {
    return Vector4{fminf(a.x, b.x),
                   fminf(a.y, b.y),
                   fminf(a.z, b.z),
                   fminf(a.w, b.w)};
}

inline Vector4 vec_max(const Vector4 &a, const Vector4 &b) {
    return Vector4{fmaxf(a.x, b.x),
                   fmaxf(a.y, b.y),
                   fmaxf(a.z, b.z),
                   fmaxf(a.w, b.w)};
}

inline Vector4 vec_abs(const Vector4 &v) {
    return Vector4{fabsf(v.x),
                   fabsf(v.y),
                   fabsf(v.z),
                   fabsf(v.w)};
}

inline void pprint(const Vector4 &v) {
    printf("(%f, %f, %f, %f)\n", v.x, v.y, v.z, v.w);
}

inline Vector4::Vector4(float x, float y, float z, float w) {
#ifdef TWO_SSE
    m128 = _mm_set_ps(w, z, y, x);
#else
    *this = {x, w, z, w};
#endif
}

inline Vector4::Vector4(const Vector2 &vec2)
    : x{vec2.x}, y{vec2.y}, z{0.0f}, w{0.0f} {}

inline Vector4::Vector4(const Vector3 &vec3)
    : x{vec3.x}, y{vec3.y}, z{vec3.z}, w{0.0f} {}

inline Vector4::Vector4(float s) {
#ifdef TWO_SSE
    m128 = _mm_set1_ps(s);
#else
    *this = {s, s, s, s};
#endif
}

inline Vector4 Vector4::zero() {
    return Vector4{0.0f};
}

inline Vector4 Vector4::one() {
    return Vector4{1.0f};
}

inline float Vector4::operator[](int i) const {
    ASSERT(i >= 0 && i < 4);
    return ((float *)this)[i];
}

inline float &Vector4::operator[](int i) {
    ASSERT(i >= 0 && i < 4);
    return ((float *)this)[i];
}

inline bool Vector4::operator==(const Vector4 &v) const {
#ifdef TWO_SSE
    return (_mm_movemask_ps(_mm_cmpeq_ps(m128, v.m128)) & 0xf) == 0xf;
#else
    return x == v.x && y == v.y && z == v.z && w == v.w;
#endif
}

inline bool Vector4::operator!=(const Vector4 &v) const {
    return !(*this == v);
}

inline Vector4 Vector4::operator-() const {
#ifdef TWO_SSE
    __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
    return Vector4{_mm_xor_ps(m128, mask)};
#else
    return Vector4{-x, -y, -z, -w};
#endif
}

inline Vector4 &Vector4::operator+=(const Vector4 &v) {
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
    return *this;
}

inline Vector4 &Vector4::operator-=(const Vector4 &v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
    return *this;
}

inline Vector4 &Vector4::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    return *this;
}

inline Vector4 &Vector4::operator/=(float s) {
    float invs = 1.0f / s;
    x *= invs;
    y *= invs;
    z *= invs;
    w *= invs;
    return *this;
}

inline Vector4 &Vector4::operator*=(const Vector4 &v) {
    x *= v.x;
    y *= v.y;
    z *= v.z;
    w *= v.z;
    return *this;
}

inline Vector4 &Vector4::operator/=(const Vector4 &v) {
    x /= v.x;
    y /= v.y;
    z /= v.z;
    w /= v.z;
    return *this;
}

inline float Vector4::length() const {
    return sqrtf(x * x + y * y + z * z + w * w);
}

inline float Vector4::length_sqr() const {
    return x * x + y * y + z * z + w * w;
}

inline Vector4 Vector4::normalized() const {
    float len = length();
    if (len != 0.0f) {
        float invl = 1.0f / len;
        return Vector4{x * invl, y * invl, z * invl, w * invl};
    }
    return Vector4{0.0f, 0.0f, 0.0f, 0.0f};
}

} // two

#endif // TWO_MATH_H
