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

#include "text.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <sstream>

#include "SDL.h"
#include "image.h"
#include "filesystem.h"
#include "two.h"
#include "renderer.h"

namespace two {

struct BM_Block {
    std::string id;
    std::unordered_map<std::string, std::string> values;

    // Give more useful error messages during parsing
    const std::string *filename;
    int line_pos;

    const std::string &attr(const std::string &id) const;
};

const std::string &BM_Block::attr(const std::string &key) const {
    auto kv = values.find(key);

    ASSERTS(kv != values.end(),
            "BM Font: expected '%s' in %s:%d",
            key.c_str(), filename->c_str(), line_pos);

    return kv->second;
}

// This function assumes a correct BM font text file
static BM_Block parse_block(const std::string &line) {
    BM_Block block;

    // Each line starts with the block id followed by a space
    block.id = line.substr(0, line.find(' '));

    for (size_t pos = 0; pos < line.size();) {
        // Key value pairs seprated by '='
        auto eq_pos = line.find('=', pos);

        if (eq_pos == std::string::npos) {
            // Nothing left to parse
            break;
        }
        // <space>key=value ...
        auto key_pos = line.rfind(' ', eq_pos);
        ASSERT(key_pos != std::string::npos);
        auto key = line.substr(key_pos + 1, eq_pos - (key_pos + 1));

        auto val_pos = eq_pos + 1;
        size_t val_end;

        if (line[val_pos] == '"')
            // Quoted value
            val_end = line.find('"', ++val_pos) - 1;
        else
            val_end = line.find(' ', val_pos + 1) - 1;

        val_end = std::min(val_end, line.size() - 1);

        auto value = line.substr(val_pos, val_end - val_pos + 1);
        block.values.emplace(std::make_pair(std::move(key), std::move(value)));
        pos = val_end;
    }
    return block;
}

static SDL_Texture *load_font_texture(const std::string &image_asset) {
    auto *im = load_image(image_asset);
    if (im == nullptr) {
        // TODO: fallback to default font?
        PANIC("Font: failed to load font image data for %s",
              image_asset.c_str());
    }
    if (im->get_pixelformat() != Image::RGBA32) {
        auto *rgba32 = im->convert(Image::RGBA32);
        delete im;
        im = rgba32;
    }
    auto *tex = SDL_CreateTexture(gfx, SDL_PIXELFORMAT_RGBA32,
                                  SDL_TEXTUREACCESS_STATIC,
                                  im->width(), im->height());
    ASSERT(tex != nullptr);
    SDL_UpdateTexture(tex, nullptr, (const void *)im->pixels(), im->pitch());
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    delete im;
    return tex;
}

Font::~Font() {
    SDL_DestroyTexture(texture);
}

std::shared_ptr<Font> load_font(const std::string &fnt_asset, int page) {
    auto font = std::make_shared<Font>();

    File file(fnt_asset);
    file.open(FileMode::Read);
    std::string data(file.read_all(), file.size());

    std::stringstream lines(data);
    std::string line;
    BM_Block block;
    int line_pos = 1;

    while (std::getline(lines, line)) {
        block = parse_block(line);
        block.filename = &fnt_asset;
        block.line_pos = line_pos++;

        // Blocks are in reverse order than they appear in the file since "char"
        // is the most common block.
        if (block.id == "char") {
            if (std::stoi(block.attr("page")) != page) {
                continue;
            }
            uint32_t glyph_id = std::stoll(block.attr("id"));

            Rect rect;
            rect.x = float(std::stoi(block.attr("x")));
            rect.y = float(std::stoi(block.attr("y")));
            rect.w = float(std::stoi(block.attr("width")));
            rect.h = float(std::stoi(block.attr("height")));

            Font::Glyph glyph;
            glyph.rect = rect;
            glyph.ox = std::stoi(block.attr("xoffset"));
            glyph.oy = std::stoi(block.attr("yoffset"));
            glyph.advance = std::stoi(block.attr("xadvance"));

            font->glyphs.emplace(std::make_pair(glyph_id, std::move(glyph)));
            continue;
        }
        if (block.id == "page") {
            if (std::stoi(block.attr("id")) != page) {
                continue;
            }
            font->texture = load_font_texture(block.attr("file"));
            continue;
        }
        if (block.id == "common") {
            font->line_height = std::stoi(block.attr("lineHeight"));
            continue;
        }
        if (block.id == "info") {
            font->name = block.attr("face");
            font->size = std::stoi(block.attr("size"));
            continue;
        }

        PANIC("BM Font: invalid block id '%s' in %s:%d",
              block.id.c_str(), fnt_asset.c_str(), line_pos - 1);
    }
    return font;
}

static inline void missing_glyph(const std::shared_ptr<Font> &font,
                                 uint32_t codepoint) {
    log_warn("Font: '%s' missing character 0x%X",
             font->name.c_str(), codepoint);
}

void FontRenderer::load(World &world) {
    auto *bg = world.get_system<BackgroundRenderer>();
    if (bg == nullptr) {
        world.make_system_before<FontRenderer, BackgroundRenderer>();
    }
}

Vector2i FontRenderer::text_size(const Text &text, const Vector2 &scale,
                                 const std::vector<bool> &wrap_info) const {
    // line_height is probably negative as it indicates how much to
    // move on the screen but here we only care about the total size
    int line_height = int(abs(text.font->line_height) * text.line_spacing);
    Vector2i size{0, line_height};
    int x = 0;

    for (size_t i = 0; i < text.text.size(); ++i) {
        uint32_t codepoint = text.text[i];
        if (codepoint == '\r') {
            continue;
        }
        if (codepoint == '\n' || wrap_info[i]) {
            size.x = std::max(size.x, x);
            size.y += line_height * scale.y;
            x = 0;
            if (codepoint == '\n') continue;
        }
        auto glyph_it = text.font->glyphs.find(codepoint);
        if (glyph_it == text.font->glyphs.end()) {
            missing_glyph(text.font, codepoint);
            glyph_it = text.font->glyphs.find('?');
        }
        auto &glyph = glyph_it->second;
        x += glyph.advance * scale.x;
    }
    size.x = std::max(size.x, x);
    return size;
}

void FontRenderer::wrap_text(const Text &text, const Vector2 &scale,
                             std::vector<bool> &result) const {
    result.resize(text.text.size(), false);
    if (text.wrap != Text::Wrap || text.width <= 0) {
        return;
    }
    size_t word_start = 0;
    int word_width = 0;
    ASSERT(text.font->glyphs.find(' ') != text.font->glyphs.end());
    int space_width = text.font->glyphs[' '].advance;
    int space_left = text.width + space_width;

    for (size_t i = 0; i < text.text.size(); ++i) {
        uint32_t codepoint = text.text[i];
        if (codepoint == '\r') {
            continue;
        }
        if (codepoint == '\n') {
            word_start = i + 1;
            word_width = 0;
            space_left = text.width;
            continue;
        }
        auto glyph_it = text.font->glyphs.find(codepoint);
        if (glyph_it == text.font->glyphs.end()) {
            missing_glyph(text.font, codepoint);
            glyph_it = text.font->glyphs.find('?');
        }

        auto &glyph = glyph_it->second;
        int advance = glyph.advance * scale.x;
        space_left -= advance;
        word_width += advance;

        if (space_left < 0) {
            // Word wraps to the next line
            result[word_start] = true;
            space_left = (text.width - word_width) + space_width;
        }
        if (codepoint == ' ') {
            word_start = i + 1;
            word_width = 0;
        }
    }
}

void FontRenderer::draw(World &world) {
    ShadowEffect shadow;
    auto &camera = world.unpack_one<Camera>();
    for (auto entity : world.view<Transform, Text>()) {
        auto &transform = world.unpack<Transform>(entity);
        auto &text = world.unpack<Text>(entity);
        bool has_shadow = world.has_component<ShadowEffect>(entity);

        if (has_shadow)
            shadow = world.unpack<ShadowEffect>(entity);

        Vector2i offset;
        switch (text.screen_space) {
        case Text::Overlay:
            offset = Vector2i(transform.position);
            break;
        case Text::World:
            offset = world_to_screen(transform.position, camera);
            break;
        default:
            PANIC("Invalid screen_space for entity #%x", entity);
        }
        wrap_text(text, transform.scale, wrap_info_cache);

        int x = 0;
        SDL_SetRenderDrawColor(gfx, 0, 0, 0, 255);
        SDL_Rect rect{int(offset.x), int(offset.y), 800, 256};
        SDL_RenderDrawRect(gfx, &rect);

        for (size_t i = 0; i < text.text.size(); ++i) {
            uint32_t codepoint = text.text[i];
            if (codepoint == '\r') {
                continue;
            }
            if (codepoint == '\n' || wrap_info_cache[i]) {
                offset.y += text.font->line_height * transform.scale.y;
                x = 0;
                if (codepoint == '\n') continue;
            }
            auto glyph_it = text.font->glyphs.find(codepoint);
            if (glyph_it == text.font->glyphs.end()) {
                missing_glyph(text.font, codepoint);
                glyph_it = text.font->glyphs.find('?');
            }
            auto &glyph = glyph_it->second;

            SDL_Rect src{int(glyph.rect.x), int(glyph.rect.y),
                         int(glyph.rect.w), int(glyph.rect.h)};

            // Text size does not depend on tile size. This is to allow text to
            // be used in overlays and UI elements. If text should act as a
            // sprite it should probably be a sprite and not dynamic text.
            SDL_Rect dst{int(offset.x + (x + glyph.ox) * transform.scale.x),
                         int(offset.y + glyph.oy * transform.scale.y),
                         int(glyph.rect.w * transform.scale.x),
                         int(glyph.rect.h * transform.scale.y)};

            if (has_shadow) {
                SDL_Rect shadow_dst = dst;
                shadow_dst.x += shadow.offset.x;
                shadow_dst.y += shadow.offset.y;

                SDL_SetTextureColorMod(text.font->texture, shadow.color.r,
                                       shadow.color.g, shadow.color.b);

                SDL_SetTextureAlphaMod(text.font->texture, shadow.color.a);
                SDL_RenderCopy(gfx, text.font->texture, &src, &shadow_dst);
            }

            SDL_SetTextureColorMod(text.font->texture, text.color.r,
                                   text.color.g, text.color.b);

            SDL_SetTextureAlphaMod(text.font->texture, text.color.a);
            // Copy glyph texture
            SDL_RenderCopy(gfx, text.font->texture, &src, &dst);
            // Advance to next character
            x += glyph.advance;
        }
    }
}

} // two
