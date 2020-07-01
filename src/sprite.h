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

#ifndef TWO_SPRITE_H
#define TWO_SPRITE_H

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

#include "SDL.h"
#include "mathf.h"
#include "entity.h"
#include "image.h"
#include "optional.h"
#include "two.h"

namespace two {

using Texture = std::shared_ptr<SDL_Texture>;

using SpriteLayer = uint8_t;

static const int SpriteLayerMax = 256;

// Sprite component
struct Sprite {
    enum Flip {
        FlipNone = SDL_FLIP_NONE,
        FlipX    = SDL_FLIP_VERTICAL,
        FlipY    = SDL_FLIP_HORIZONTAL,
        FlipXY   = SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL
    };

    // Texture memory for this sprite. Multiple sprites can share
    // the same texture.
    Texture texture;

    // Section of texture (in pixels) that corresponds to this sprite.
    Rect rect;

    // Transformations are applied relative to this origin, where (0, 0) is
    // the top left corner of the sprite and (1, 1) is the bottom right.
    // By the default this is value (0.5, 0.5) which is the center of
    // the sprite.
    Vector2 origin;

    Flip flip;

    // Apply a color modulation when rendering the sprite. A white color
    // means the sprite will keep its the original color.
    Color color;

    // Sorting layer (0 to 255). Sprites with a greater value for the layer
    // will be rendered on top of sprites with smaller layer values.
    // Sprites with the same layer can be rendered in any order.
    SpriteLayer layer;

    Sprite() {}

    Sprite(const Texture &texture, const Rect &rect)
        : texture{texture}
        , rect{rect}
        , origin{Vector2{0.5f, 0.5f}}
        , flip{FlipNone}
        , color{Color{255, 255, 255, 255}}
        , layer{0} {}
};

inline Texture make_texture(SDL_Texture *texture) {
    return std::shared_ptr<SDL_Texture>(texture, [](SDL_Texture *tex) {
        // Make sure we still have a graphics device. This is likely to
        // be false if the texture is a static variable since static
        // shared pointers go out of scope after the graphics device is
        // released. Freeing the graphics device will free all textures.
        if (gfx != nullptr) {
            SDL_DestroyTexture(tex);
        }
    });
}

// Load a sprite asset from from an asset file.
Optional<Sprite> load_sprite(const std::string &image_asset);

// Creates a sprite from an image. A new texture will be allocated
// for this sprite with the image contents. It is safe to free the
// image once the sprite is created.
Sprite make_sprite(const Image *im);

// Creates a sprite from a section of an image. A new texture will be
// allocated for this sprite with the image contents. It is safe to
// free the image once the sprite is created.
Sprite make_sprite(const Image *im, const Rect &rect);

// Returns a sprite which uses a single color. All blank sprites share the
// same texture (a 1x1 px white image).
Sprite blank_sprite(const Color &color);

// Load an atlas from a asset file. The asset file must exist in an archive
// added with `filesystem.h mount()`.
Optional<std::vector<Sprite>> load_atlas(const std::string &atlas_asset);

// Creates multiples sprites from the same texture. The position and
// dimentions of each sprites is given using the list of `rects`.
// A single texture will be allocated and shared between all sprite. It is
// safe to free the image after the atlas is created.
Optional<std::vector<Sprite>> load_atlas(const Image *im,
                                         const std::vector<Rect> &rects);

// Creates multiple sprites from the same texture with uniform tile sizes
// for each sprite. A single texture will be allocated and shared between
// all sprites. It is safe to free the image after the atlas is created.
//
// * `tile_x, tile_y` The width and height in pixels of each sprite in the
//    atlas.
//
// * `pad_x, pad_y`   How many pixels are used for padding between sprites
//    in the atlas.
Optional<std::vector<Sprite>> load_atlas(const Image *im,
                                         float tile_x, float tile_y,
                                         float pad_x = 0, float pad_y = 0);

// Requires a Transform component and a Sprite component
class SpriteRenderer : public System {
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

// Requires a PixelTransform component and a Sprite component
// > Note: Does not handle text, use `FontRenderer` for text instead.
// > This system does not sort sprites by layer like the SpriteRenderer,
// sprites are drawn in the order they were creted.
class OverlayRenderer : public System {
public:
    void draw(World &world) override;
};

} // two

#endif // TWO_SPRITE_H
