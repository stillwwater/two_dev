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

#ifndef TWO_IMAGE_H
#define TWO_IMAGE_H

#include "SDL.h"
#include "mathf.h"
#include "debug.h"

namespace two {

struct Color {
    static const Color Black;
    static const Color White;
    static const Color Clear;
    static const Color Magenta;

    unsigned char r, g, b, a;

    Color() : r{0}, g{0}, b{0}, a{0} {}

    Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
        : r{r}, g{g}, b{b}, a{a} {}

    // Color will have an alpha of 255 (opaque)
    Color(unsigned char r, unsigned char g, unsigned char b)
        : r{r}, g{g}, b{b}, a{255} {}

    // Creates a color from a Vector4 by multiplying each component in the
    // vector by 255. The vector will be clamped to be between 0.0f and 1.0f.
    Color(const Vector4 &vec4);

    unsigned char operator[](int i) const;
    unsigned char &operator[](int i);

    bool operator==(const Color &c) const;
    bool operator!=(const Color &c) const;

    // Color blending
    Color &operator+=(const Color &c);
    Color &operator-=(const Color &c);
    Color &operator*=(const Color &c);
    Color &operator/=(const Color &c);

    // Returns a vector with rgba components normalized to be between
    // 0.0f and 1.0f (inclusive)
    Vector4 normalized() const;

    // Packs rgba components into an unsigned integer. The resulting int
    // will have the format `0xRRGGBBAA` regardless of endianness.
    uint32_t to_uint32() const;

    // Returns the color with the alpha component set to 0.
    Color clear() const;

    // Returns the color with the alpha component set to 1.
    Color opaque() const;
};

// RGBA32, RGB24, monochrome or monochrome alpha image
class Image {
public:
    enum PixelFormat {
        MONO8,
        ALPHA8,
        RGB24,
        RGBA32,
    };

    Image(unsigned char *data, int width, int height, PixelFormat pixelformat)
        : data{data}, w{width}, h{height}, pixelformat{pixelformat} {}

    Image(int width, int height, PixelFormat pixelformat);

    ~Image();

    inline int width() const { return w; };
    inline int height() const { return h; };
    inline PixelFormat get_pixelformat() const { return pixelformat; };

    SDL_PixelFormat get_texture_format() const;

    // Returns the number of bytes per scanline.
    int pitch() const;

    // Read a pixel at a location
    Color read(const Vector2i &xy) const;

    // Write a pixel to a location
    void write(const Vector2i &xy, const Color &c);

    // Paste an image onto this Image. If the destination
    // image is smaller to source image will be clamped.
    //
    // > Note: for better performance use a target Texture with SDL_RenderCopy.
    void paste(Image *im, const Vector2i &xy);

    // Convert image to a new image with the desired format.
    Image *convert(PixelFormat pixelformat) const;

    // Copy image data.
    Image *clone() const;

    inline const unsigned char *pixels() const { return data; };

private:
    unsigned char *data = nullptr;
    int w, h;
    PixelFormat pixelformat;
};

// Decode an image from memory.
//
// `im_data` format may be PNG, BMP or TGA
Image *load_image(const unsigned char *im_data);

// Load an image from an asset file.
// Returns `nullptr` if file could not be read.
//
// File format may be PNG, BMP or TGA.
//
// > Note: image file must be in an archive added with
// `filesystem.h two::mount()`
Image *load_image(const std::string &image_asset);

// Save an image to a file.
// Returns `false` if file could not be written to.
//
// File format may be PNG, BMP or TGA.
//
// > Note: image will be saved to a writedir mounted with
// `filesystem.h two::mount_rw()`
//bool save_image(const Image *im, const char *filename);

int bytes_per_pixel(Image::PixelFormat pixelformat);

// Convert color to HSV. Returns a vector where the x, y, z component map
// to the h, s, v components respectively.
//
// > Note: This conversion is not precise due to rounding errors so the
// result of `hsv_to_color(color_to_hsv(col))` will not be the same as `col`.
Vector3 color_to_hsv(Color rgb);

// Convert HSV to color.
//
// > Note: This conversion is not precise due to rounding errors so the
// result of `hsv_to_color(color_to_hsv(col))` will not be the same as `col`.
Color hsv_to_color(Vector3 hsv);


inline Color operator+(const Color &a, const Color &b) {
    Color result = a;
    return result += b;
}

inline Color operator-(const Color &a, const Color &b) {
    Color result = a;
    return result -= b;
}

inline Color operator*(const Color &a, const Color &b) {
    Color result = a;
    return result *= b;
}

inline Color operator/(const Color &a, const Color &b) {
    Color result = a;
    return result /= b;
}

inline Color::Color(const Vector4 &rgba) {
    r = (unsigned char)(clamp01(rgba.x) * 255.0f);
    g = (unsigned char)(clamp01(rgba.y) * 255.0f);
    b = (unsigned char)(clamp01(rgba.z) * 255.0f);
    a = (unsigned char)(clamp01(rgba.w) * 255.0f);
}

inline unsigned char Color::operator[](int i) const {
    ASSERT(i >= 0 && i < 4);
    return ((unsigned char *)this)[i];
}

inline unsigned char &Color::operator[](int i) {
    ASSERT(i >= 0 && i < 4);
    return ((unsigned char *)this)[i];
}

inline bool Color::operator==(const Color &c) const {
    return r == c.r && g == c.g && b == c.b && a == c.a;
}

inline bool Color::operator!=(const Color &c) const {
    return !(*this == c);
}

inline Color &Color::operator+=(const Color &c) {
    r = clampi(r + c.r, 0, 255);
    g = clampi(g + c.g, 0, 255);
    b = clampi(b + c.b, 0, 255);
    a = clampi(a + c.a, 0, 255);
    return *this;
}

inline Color &Color::operator-=(const Color &c) {
    r = clampi(r - c.r, 0, 255);
    g = clampi(g - c.g, 0, 255);
    b = clampi(b - c.b, 0, 255);
    a = clampi(a - c.a, 0, 255);
    return *this;
}

inline Color &Color::operator*=(const Color &c) {
    r = clampi(r * c.r, 0, 255);
    g = clampi(g * c.g, 0, 255);
    b = clampi(b * c.b, 0, 255);
    a = clampi(a * c.a, 0, 255);
    return *this;
}

inline Color &Color::operator/=(const Color &c) {
    r /= c.r;
    g /= c.g;
    b /= c.b;
    a /= c.a;
    return *this;
}

inline Vector4 Color::normalized() const {
    constexpr float inv255 = 1.0f / 255.0f;
    return Vector4{float(r) * inv255,
                   float(g) * inv255,
                   float(b) * inv255,
                   float(a) * inv255};
}

inline uint32_t Color::to_uint32() const {
    return SDL_SwapBE32(*(uint32_t *)this);
}

} // two

#endif // TWO_IMAGE_H
