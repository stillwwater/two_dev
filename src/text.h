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

#ifndef TWO_UI_H
#define TWO_UI_H

#include <unordered_map>
#include <string>
#include <cstdint>
#include <memory>

#include "SDL_render.h"
#include "mathf.h"
#include "image.h"
#include "entity.h"

namespace two {

struct Font {
    struct Glyph {
        Rect rect;
        int ox, oy;
        int advance;
    };

    Font() = default;

    ~Font();
    Font(const Font &) = delete;
    Font &operator=(const Font &) = delete;

    Font(Font &&) = default;
    Font &operator=(Font &&) = default;

    // Font size in pixel
    int size;

    // The offset in pixels to move to the next line
    int line_height;

    // Texture is owned by font
    SDL_Texture *texture;

    std::unordered_map<uint32_t, Glyph> glyphs;
    std::string name;
};

// A Text component
struct Text {
    enum WrapMode { Overflow, Wrap };
    enum ScreenSpace { Overlay, World };

    // Text components may share the same font.
    std::shared_ptr<Font> font;

    // The actual text to be rendered.
    // TODO: utf8
    std::string text;

    Color color;

    // Line spacing relative to the font line height.
    // `actual_line_height = font->line_height * line_spacing`
    float line_spacing;

    // The maximum width in pixels of this text before wrap is applied.
    // If the wrap mode is set to Overflow (default) then this is allowed
    // to be zero and the text will not wrap to a new line.
    float width;

    // How the text should be wrapped if the text goes over the `width`
    // value when rendering.
    WrapMode wrap;

    Text() = default;

    Text(const std::shared_ptr<Font> &font, const std::string &text)
        : font{font}
        , text{text}
        , color{Color::White}
        , line_spacing{1.0f}
        , width{0.0f}
        , wrap{Overflow} {}

    Text(const std::shared_ptr<Font> &font,
         const std::string &text,
         const Color &color)
        : font{font}
        , text{text}
        , color{color}
        , line_spacing{1.0f}
        , width{0.0f}
        , wrap{Overflow} {}
};

// Loads a font from a .fnt (AngelCode BMFont) binary file.
// Page is which page to use in the font file since a .fnt file can contain
// more than one font variants.
std::shared_ptr<Font> load_font(const std::string &fnt_asset, int page = 0);

// Same as `load_font(fnt_asset, page)` but the given image asset will be
// used to load the font atlas instead of getting the asset name from
// the `fnt_asset` file. Use this function if your .fnt and image file are
// not in the same directory.
std::shared_ptr<Font> load_font(const std::string &fnt_asset,
                                const std::string &image_asset);

// Same as `load_font` but loads a .fnt file from memory
std::shared_ptr<Font> load_font_memory(const char *fnt_data, int page = 0);

// Same as `load_font(fnt_asset, image_asset)` but loads a .fnt and image file
// from memory.
std::shared_ptr<Font> load_font_memory(const char *fnt_data,
                                       const char *image_data);

// Requires Text component and either a PixelTransform or Transform component.
class FontRenderer : public System {
public:
    void draw(World *world) override;

    void wrap_text(const Text &text, const float2 &scale,
                   std::vector<bool> &result) const;

    int2 text_size(const Text &text, const float2 &scale,
                       const std::vector<bool> &wrap_info) const;

private:
    std::vector<bool> wrap_info_cache;
};

} // two

#endif // TWO_UI_H
