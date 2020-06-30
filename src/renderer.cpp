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

#include "renderer.h"

#include <cmath>

#include "SDL.h"
#include "two.h"
#include "image.h"
#include "sprite.h"
#include "mathf.h"
#include "entity.h"

namespace two {

Vector2i world_to_screen(const Vector2 &v, const Camera &camera) {
    int w, h;
    SDL_RenderGetLogicalSize(gfx, &w, &h);
    auto tilesizef = Vector2(camera.tilesize) * camera.scale;

    Vector2i result;
    int ox = int(camera.position.x * tilesizef.x);
    int oy = int(camera.position.y * tilesizef.y);
    result.x = (int(v.x * tilesizef.x) + w / 2) - ox;
    result.y = (int(v.y * tilesizef.y) + h / 2) - oy;
    return result;
}

Vector2 screen_to_world(const Vector2i &v, const Camera &camera) {
    int w, h;
    SDL_RenderGetLogicalSize(gfx, &w, &h);
    auto tilesizef = Vector2(camera.tilesize) * camera.scale;

    Vector2 result;
    float ox = camera.position.x;
    float oy = camera.position.y;
    result.x = float(v.x - w * 0.5f) / tilesizef.x + ox;
    result.y = float(v.y - h * 0.5f) / tilesizef.y + oy;
    return result;
}

void Renderer::sort_sprites(World &world, const std::vector<Entity> &entities,
                            std::vector<Entity> &sorted) {
    sorted.clear();
    sorted.resize(entities.size());
    sort_counts.fill(0);

    for (auto entity : entities) {
        ASSERT(world.has_component<Sprite>(entity));
        auto &sprite = world.unpack<Sprite>(entity);

        ASSERT(sprite.layer < sort_counts.size());
        ++sort_counts[sprite.layer];
    }

    for (size_t i = 1; i < sort_counts.size(); ++i) {
        sort_counts[i] += sort_counts[i - 1];
    }

    for (int i = entities.size() - 1; i >= 0; --i) {
        auto entity = entities[i];
        auto &sprite = world.unpack<Sprite>(entity);
        sorted[--sort_counts[sprite.layer]] = entity;
    }
};

void Renderer::draw(World &world) {
    auto camera_entity = world.view_one<Camera>();

    ASSERTS(camera_entity.has_value,
            "Missing an entity with a Camera component");

    auto &camera = world.unpack<Camera>(camera_entity.value());
    auto cam_scale = camera.scale;
    auto tilesizef = Vector2(camera.tilesize) * cam_scale;
    auto cam_offset = Vector2i(camera.position * tilesizef);

    int screen_w, screen_h;
    SDL_RenderGetLogicalSize(gfx, &screen_w, &screen_h);
    Vector2i screen_wh_2{screen_w / 2, screen_h / 2};

    SDL_SetRenderDrawColor(gfx, camera.background.r,
                           camera.background.g, camera.background.b, 255);
    SDL_RenderClear(gfx);

    const auto &entities = world.view<Transform, Sprite>();
    sort_sprites(world, entities, sprite_buffer);
    Vector2 v[4];

    for (auto entity : sprite_buffer) {
        auto &transform = world.unpack<Transform>(entity);
        auto &sprite = world.unpack<Sprite>(entity);

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

        Vector2 r;
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
        v[0] = (Vector2i(v[0] * tilesizef) + screen_wh_2) - cam_offset;
        v[1] = (Vector2i(v[1] * tilesizef) + screen_wh_2) - cam_offset;
        v[2] = (Vector2i(v[2] * tilesizef) + screen_wh_2) - cam_offset;
        v[3] = (Vector2i(v[3] * tilesizef) + screen_wh_2) - cam_offset;

        // Culling
        auto box_min = vec_min(v[0], v[1]);
        box_min = vec_min(box_min, v[2]);
        box_min = vec_min(box_min, v[3]);

        auto box_max = vec_max(v[0], v[1]);
        box_max = vec_max(box_max, v[2]);
        box_max = vec_max(box_max, v[3]);

        if (box_min.x > screen_w || box_min.y > screen_h
            || box_max.x < 0.0f || box_max.y < 0.0f) {
            continue;
        }

        Vector2 ws = (Vector2i(transform.position * tilesizef)
                   + screen_wh_2) - cam_offset;

        // Scale and offset in pixels

        SDL_Rect src{int(sprite.rect.x), int(sprite.rect.y),
                     int(sprite.rect.w), int(sprite.rect.h)};

        auto scale = transform.scale * Vector2(src.w, src.h) * cam_scale;
        auto offset = Vector2(ws) - origin * scale;

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

} // two
