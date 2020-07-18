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

#include "image.h"

#include <cstdlib>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "mathf.h"
#include "filesystem.h"

namespace two {

const Color Color::Clear   = Color{0, 0, 0, 0};
const Color Color::Black   = Color{0, 0, 0, 255};
const Color Color::White   = Color{255, 255, 255, 255};
const Color Color::Magenta = Color{255, 0, 255, 255};

Image *load_image(const unsigned char *im_data, int size) {
    int w, h, comp;
    Image::PixelFormat pixelformat;

    auto *decoded = stbi_load_from_memory(
        im_data, size, &w, &h, &comp, STBI_default);

    switch (comp) {
    case 1:
        pixelformat = Image::ALPHA8;
        break;
    case 3:
        pixelformat = Image::RGB24;
        break;
    case 4:
        pixelformat = Image::RGBA32;
        break;
    default:
        PANIC("Invalid pixel format");
        return nullptr;
    }
    return new Image(decoded, w, h, pixelformat);
}

Image *load_image(const std::string &image_asset) {
    File fp(image_asset);

    if (!fp.open(FileMode::Read)) {
        return nullptr;
    }

    char *data = new char[fp.size()];
    if (fp.read(data) < 0) {
        PANIC("Error reading image file %s", image_asset.c_str());
        return nullptr;
    }
    auto *im = load_image((unsigned char *)data, fp.size());
    delete[] data;
    return im;
}

int bytes_per_pixel(Image::PixelFormat pixelformat) {
    switch (pixelformat) {
    case Image::RGBA32: return 4;
    case Image::RGB24:  return 3;
    case Image::MONO8:  return 1;
    case Image::ALPHA8: return 1;
    default:
        PANIC("Invalid pixel format");
        return 4;
    }
}

Image::Image(int width, int height, PixelFormat pixelformat)
    : w{width}, h{height}
    , pixelformat{pixelformat}
{
    int size = w * h * bytes_per_pixel(pixelformat);
    data = new unsigned char[size];
    memset(data, 0, size);
}

Image::~Image() {
    delete[] data;
    data = nullptr;
}

int Image::pitch() const {
    return bytes_per_pixel(pixelformat) * w;
}

Color Image::read(const int2 &xy) const {
    ASSERT(xy.x >= 0 && xy.y >= 0 && xy.x < w && xy.y < h);
    Color c;
    int p;

    switch (get_pixelformat()) {
    case RGBA32:
        p = (xy.x + xy.y * w) * 4;
        c.r = data[p + 3];
        c.g = data[p + 2];
        c.b = data[p + 1];
        c.a = data[p + 0];
        break;

    case RGB24:
        p = (xy.x + xy.y * w) * 3;
        c.r = data[p + 2];
        c.g = data[p + 1];
        c.b = data[p + 0];
        c.a = 255;
        break;

    case ALPHA8:
        p = xy.x + xy.y * w;
        c.r = 255;
        c.g = 255;
        c.b = 255;
        c.a = data[p];
        break;

    case MONO8:
        p = xy.x + xy.y * w;
        c.r = data[p];
        c.g = data[p];
        c.b = data[p];
        c.a = 255;
        break;

    default:
        PANIC("Invalid pixel format");
        return Color{255, 0, 255, 255};
    }
    return c;
};

void Image::write(const int2 &xy, const Color &c) {
    ASSERT(xy.x >= 0 && xy.y >= 0 && xy.x < w && xy.y < h);
    int p = 0;

    ASSERT(data != nullptr);

    switch (pixelformat) {
    case RGBA32:
        p = (xy.x + xy.y * w) * 4;
        data[p + 3] = c.r;
        data[p + 2] = c.g;
        data[p + 1] = c.b;
        data[p + 0] = c.a;
        break;

    case RGB24:
        p = (xy.x + xy.y * w) * 3;
        data[p + 2] = c.r;
        data[p + 1] = c.g;
        data[p + 0] = c.b;
        break;

    case ALPHA8:
        p = xy.x + xy.y * w;
        data[p] = c.a;
        break;

    case MONO8:
        p = xy.x + xy.y * w;
        // Use the alpha channel for grayscale. This function shouldn't
        // be used to convert colors to grayscale.
        data[p] = c.a;
        break;

    default:
        PANIC("Invalid pixel format");
    }
};

Image *Image::clone() const {
    auto *dst = new Image(w, h, pixelformat);
    memcpy(dst->data, data, w * h * bytes_per_pixel(pixelformat));
    return dst;
}

Image *Image::convert(PixelFormat pixelformat) const {
    auto src_format = this->pixelformat;
    auto dst_format = pixelformat;

    if (src_format == dst_format) {
        return clone();
    }

    // Same size images, all that changes is how the bytes are
    // interpreted as color.
    if ((src_format == MONO8 && dst_format == ALPHA8)
        || (src_format == ALPHA8 && dst_format == MONO8)) {
        auto *dst = clone();
        dst->pixelformat = dst_format;
        return dst;
    }

    auto *dst = new Image(w, h, dst_format);
    ASSERT(dst->data != nullptr && this->data != nullptr);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int2 xy{x, y};
            dst->write(xy, read(xy));
        }
    }
    return dst;
}

void Image::paste(Image *im, const int2 &xy) {
    if (im->pixelformat != pixelformat) {
        // We could convert here but that would allocate memory every time
        // which we may not want to do.
        PANIC("Cannot paste image with different formats, use Image::convert");
        return;
    }
    int dst_w = clampi(xy.x + im->w, 0, w);
    int dst_h = clampi(xy.y + im->h, 0, h);

    for (int v = xy.x; v < dst_w; ++v) {
        for (int u = xy.y; u < dst_h; ++u) {
            auto color = im->read(int2{u - xy.x, v - xy.y});
            write(int2{u, v}, color);
        }
    }
}

float3 color_to_hsv(const Color &rgb) {
    double h, s, v;
    auto norm = rgb.normalized();
    double r = norm.x;
    double g = norm.y;
    double b = norm.z;

    double max = fmaxf(r, fmaxf(g, b));
    double min = fminf(r, fminf(g, b));
    double chroma = max - min;

    v = max;
    if (chroma < Epsilon || v <= 0.0) {
        return float3(0.0f, 0.0f, v);
    }
    s = chroma / v;
    if (r >= v)
        h = 0.0 + (g - b) / chroma;
    else if (g >= v)
        h = 2.0 + (b - r) / chroma;
    else
        h = 4.0 + (r - g) / chroma;
    h /= 6.0;
    return float3{float(h), float(s), float(v)};
}

Color hsv_to_color(const float3 &hsv) {
    double h = clamp01(hsv.x);
    double s = clamp01(hsv.y);
    double v = clamp01(hsv.z);

    double h1 = h * 6.0;
    int h2 = int(h1);
    double f = h1 - double(h2);

    double x = v * (1.0 - s);
    double y = v * (1.0 - s * f);
    double z = v * (1.0 - s * (1.0 - f));

    float4 color;
    color.w = 1.0f;

    switch (h2) {
    case 0:
        color.x = v;
        color.y = z;
        color.z = x;
        break;
    case 1:
        color.x = y;
        color.y = v;
        color.z = x;
        break;
    case 2:
        color.x = x;
        color.y = v;
        color.z = z;
        break;
    case 3:
        color.x = x;
        color.y = y;
        color.z = v;
        break;
    case 4:
        color.x = z;
        color.y = x;
        color.z = v;
        break;
    case 5:
    default:
        color.x = v;
        color.y = x;
        color.z = y;
        break;
    }
    return Color(clamp01(color));
}

} // two
