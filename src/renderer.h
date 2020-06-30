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

#ifndef TWO_RENDERER_H
#define TWO_RENDERER_H

#include "SDL.h"
#include "mathf.h"
#include "entity.h"
#include "image.h"
#include "sprite.h"
#include "two.h"

namespace two {

// Camera component
struct Camera {
    // How many pixels per world unit.
    Vector2i tilesize;

    // The color to use when clearing the screen.
    Color background;

    // Position in world units.
    // The camera will be centered on this position.
    Vector2 position;

    // How much to scale the world by when rendering. This is useful for
    // pixel art where sprites are low resolution but you want to render
    // at a higher resolution for smoother animations.
    float scale;

    Camera() {}

    Camera(int tilesize, Color background)
        : tilesize{Vector2i{tilesize, tilesize}}
        , background{background}
        , position{Vector2{0, 0}}
        , scale{1.0f} {}

    Camera(int tilesize, Color background, Vector2 position)
        : tilesize{Vector2i{tilesize, tilesize}}
        , background{background}
        , position{position}
        , scale{1.0f} {}

    Camera(Vector2i tilesize, Color background, Vector2 position)
        : tilesize{tilesize}
        , background{background}
        , position{position}
        , scale{1.0f} {}
};


Vector2i world_to_screen(const Vector2 &v, const Camera &camera);

Vector2 screen_to_world(const Vector2i &v, const Camera &camera);

class Renderer : public System {
public:
    void draw(World &world) override;

    // Sorts entities with sprite components by the sprite sorting layer.
    // This function assumes `entities` all have a sprite component.
    void sort_sprites(World &world, const std::vector<Entity> &entities,
                      std::vector<Entity> &sorted);

private:
    std::vector<Entity> sprite_buffer;
    std::array<TWO_ENTITY_INT_TYPE, SpriteLayerMax> sort_counts;
};

} // two

#endif // TWO_RENDERER_H
