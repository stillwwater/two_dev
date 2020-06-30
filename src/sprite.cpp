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

#include "sprite.h"

#include <string>
#include <vector>
#include <memory>

#include "SDL_render.h"
#include "mathf.h"
#include "debug.h"
#include "image.h"
#include "two.h"

namespace two {

Optional<Sprite> load_sprite(const std::string &image_asset) {
    auto *im = load_image(image_asset);
    if (im == nullptr) {
        return {};
    }
    auto sprite = make_sprite(im);
    delete im;
    return sprite;
}

Sprite make_sprite(const Image *im) {
    return make_sprite(im, Rect{0.0f, 0.0f,
                                float(im->width()),
                                float(im->height())});
}

Sprite make_sprite(const Image *im, const Rect &rect) {
    const auto *src = im;

    if (im->get_pixelformat() != Image::RGBA32) {
        log("Creating texture from non RGBA image.");
        src = im->convert(Image::RGBA32);
    }

    auto *tex = SDL_CreateTexture(gfx, SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STATIC,
                                  im->width(), im->height());

    SDL_Rect q{int(rect.x), int(rect.y), int(rect.w), int(rect.h)};
    SDL_UpdateTexture(tex, &q, (const void *)src->pixels(), src->pitch());
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    if (im->get_pixelformat() != Image::RGBA32) {
        delete src;
    }
    return Sprite{make_texture(tex), rect};
}

} // two
