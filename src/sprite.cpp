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

Texture make_texture(const Image *im, const Rect &rect) {
    const auto *src = im;

    if (im->get_pixelformat() != Image::RGBA32) {
        log("Creating texture from non RGBA image.");
        src = im->convert(Image::RGBA32);
    }

    auto *tex = SDL_CreateTexture(gfx, SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STATIC,
                                  im->width(), im->height());
    ASSERT(tex != nullptr);

    SDL_Rect q{int(rect.x), int(rect.y), int(rect.w), int(rect.h)};
    SDL_UpdateTexture(tex, &q, (const void *)src->pixels(), src->pitch());
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    if (im->get_pixelformat() != Image::RGBA32) {
        delete src;
    }
    return make_texture(tex);
}

Texture make_texture(const Image *im) {
    Rect rect{0, 0, float(im->width()), float(im->height())};
    return make_texture(im, rect);
}

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
    return Sprite{make_texture(im, rect), rect};
}

Sprite blank_sprite(const Color &color) {
    static Texture blank = nullptr;
    if (UNLIKELY(blank == nullptr)) {
        unsigned char pixels[]{255, 255, 255, 255};
        auto *tex = SDL_CreateTexture(gfx, SDL_PIXELFORMAT_RGBA32,
                                      SDL_TEXTUREACCESS_STATIC, 1, 1);

        SDL_UpdateTexture(tex, nullptr, (const void *)&pixels, sizeof(pixels));
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        blank = make_texture(tex);
    }
    auto sprite = Sprite{blank, Rect{0, 0, 1, 1}};
    sprite.color = color;
    return sprite;
}

std::vector<Sprite> load_atlas(const Image *im,
                               float tile_x, float tile_y,
                               float pad_x, float pad_y)
{
    auto tex = make_texture(im);
    float stride_x = tile_x + pad_x;
    float stride_y = tile_y + pad_y;
    std::vector<Sprite> sprites;
    sprites.reserve((im->width() / stride_x) * (im->height() / stride_y));

    for (int y = 0; y < im->height(); y += stride_y) {
        for (int x = 0; x < im->width(); x += stride_x) {
            Rect rect{float(x), float(y), tile_x, tile_y};
            sprites.push_back(Sprite{tex, rect});
        }
    }
    return sprites;
}

void SpriteRenderer::sort_sprites(World *world,
                                  const std::vector<Entity> &entities,
                                  std::vector<Entity> &sorted) {
    TWO_PROFILE_FUNC();
    sorted.clear();
    sorted.resize(entities.size());
    sort_counts.fill(0);

    for (auto entity : entities) {
        ASSERT(world->has_component<Sprite>(entity));
        auto &sprite = world->unpack<Sprite>(entity);

        ASSERT(sprite.layer < sort_counts.size());
        ++sort_counts[sprite.layer];
    }

    for (size_t i = 1; i < sort_counts.size(); ++i) {
        sort_counts[i] += sort_counts[i - 1];
    }

    for (int i = entities.size() - 1; i >= 0; --i) {
        auto entity = entities[i];
        auto &sprite = world->unpack<Sprite>(entity);
        sorted[--sort_counts[sprite.layer]] = entity;
    }
};

void SpriteRenderer::draw(World *world) {
    TWO_PROFILE_FUNC();
    // Existence of a camera is checked by the Background Renderer
    auto &camera = world->unpack_one<Camera>();
    auto cam_scale = camera.scale;
    auto tilesizef = float2(camera.tilesize) * cam_scale;
    auto cam_offset = int2(camera.position * tilesizef);

    int screen_w, screen_h;
    SDL_RenderGetLogicalSize(gfx, &screen_w, &screen_h);
    int2 screen_wh_2{screen_w / 2, screen_h / 2};

    const auto &entities = world->view<Transform, Sprite>();
    sort_sprites(world, entities, sprite_buffer);
    float2 v[4];

    for (auto entity : sprite_buffer) {
        auto &transform = world->unpack<Transform>(entity);
        auto &sprite = world->unpack<Sprite>(entity);

        v[0] = transform.position;
        v[1] = {v[0].x + transform.scale.x, v[0].y};
        v[2] = {v[0].x + transform.scale.x, v[0].y + transform.scale.y};
        v[3] = {v[0].x, v[0].y + transform.scale.y};

        auto origin = clamp01(sprite.origin);
        auto woffset = (origin * transform.scale) + transform.position;
        v[0] -= woffset;
        v[1] -= woffset;
        v[2] -= woffset;
        v[3] -= woffset;

        float theta = transform.rotation * DegToRad;
        float st = sinf(theta);
        float ct = cosf(theta);

        float2 r;
        r.x = ct*v[0].x - st*v[0].y + transform.position.x;
        r.y = st*v[0].x + ct*v[0].y + transform.position.y;
        v[0] = r;

        r.x = ct*v[1].x - st*v[1].y + transform.position.x;
        r.y = st*v[1].x + ct*v[1].y + transform.position.y;
        v[1] = r;

        r.x = ct*v[2].x - st*v[2].y + transform.position.x;
        r.y = st*v[2].x + ct*v[2].y + transform.position.y;
        v[2] = r;

        r.x = ct*v[3].x - st*v[3].y + transform.position.x;
        r.y = st*v[3].x + ct*v[3].y + transform.position.y;
        v[3] = r;

        // World to screen projection
        v[0] = float2((int2(v[0] * tilesizef) + screen_wh_2) - cam_offset);
        v[1] = float2((int2(v[1] * tilesizef) + screen_wh_2) - cam_offset);
        v[2] = float2((int2(v[2] * tilesizef) + screen_wh_2) - cam_offset);
        v[3] = float2((int2(v[3] * tilesizef) + screen_wh_2) - cam_offset);

        // Culling
        auto box_min = vmin(v[0], v[1]);
        box_min = vmin(box_min, v[2]);
        box_min = vmin(box_min, v[3]);

        auto box_max = vmax(v[0], v[1]);
        box_max = vmax(box_max, v[2]);
        box_max = vmax(box_max, v[3]);

        if (box_min.x > screen_w || box_min.y > screen_h
            || box_max.x < 0.0f || box_max.y < 0.0f) {
            continue;
        }

        // Offset in pixels
        float2 ws = float2((int2(transform.position * tilesizef)
                  + screen_wh_2) - cam_offset);

        SDL_Rect src{int(sprite.rect.x), int(sprite.rect.y),
                     int(sprite.rect.w), int(sprite.rect.h)};

        auto scale = transform.scale * float2(src.w, src.h) * cam_scale;
        auto offset = float2(ws) - origin * scale;

        SDL_Rect dst{int(offset.x), int(offset.y),
                     int(scale.x), int(scale.y)};

        SDL_Point center{int(origin.x * dst.w),
                         int(origin.y * dst.h)};

        SDL_SetTextureColorMod(sprite.texture.get(),
                               sprite.color.r, sprite.color.g, sprite.color.b);

        SDL_SetTextureAlphaMod(sprite.texture.get(), sprite.color.a);

        SDL_RenderCopyEx(gfx, sprite.texture.get(), &src, &dst,
                         transform.rotation, &center,
                         (SDL_RendererFlip)sprite.flip);
    }
}

void OverlayRenderer::draw(World *world) {
    for (auto entity : world->view<PixelTransform, Sprite>()) {
        auto &transform = world->unpack<PixelTransform>(entity);
        auto &sprite = world->unpack<Sprite>(entity);

        SDL_Rect src{int(sprite.rect.x), int(sprite.rect.y),
                     int(sprite.rect.w), int(sprite.rect.h)};

        SDL_Rect dst{int(transform.position.x),
                     int(transform.position.y),
                     int(sprite.rect.w * transform.scale.x),
                     int(sprite.rect.h * transform.scale.y)};

        if (world->has_component<ShadowEffect>(entity)) {
            auto &shadow = world->unpack<ShadowEffect>(entity);
            SDL_SetTextureColorMod(sprite.texture.get(), shadow.color.r,
                                   shadow.color.g, shadow.color.b);

            SDL_SetTextureAlphaMod(sprite.texture.get(), shadow.color.a);
            SDL_Rect shadow_dst{int(dst.x + shadow.offset.x),
                                int(dst.y + shadow.offset.y),
                                dst.w, dst.h};
            SDL_RenderCopy(gfx, sprite.texture.get(), &src, &shadow_dst);
        }

        SDL_SetTextureColorMod(sprite.texture.get(), sprite.color.r,
                               sprite.color.g, sprite.color.b);

        SDL_SetTextureAlphaMod(sprite.texture.get(), sprite.color.a);
        SDL_RenderCopy(gfx, sprite.texture.get(), &src, &dst);
    }
}

} // two
