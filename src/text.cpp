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

int2 FontRenderer::text_size(const Text &text, const float2 &scale,
                                 const std::vector<bool> &wrap_info) const {
    // line_height is probably negative as it indicates how much to
    // move on the screen but here we only care about the total size
    int line_height = int(abs(text.font->line_height) * text.line_spacing);
    int2 size{0, line_height};
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

void FontRenderer::wrap_text(const Text &text, const float2 &scale,
                             std::vector<bool> &result) const {
    TWO_PROFILE_FUNC();
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

void FontRenderer::draw(World *world) {
    TWO_PROFILE_FUNC();
    ShadowEffect shadow;
    auto &camera = world->unpack_one<Camera>();
    for (auto entity : world->view<Text>()) {
        auto &text = world->unpack<Text>(entity);

        int2 offset;
        float2 scale;
        if (world->has_component<PixelTransform>(entity)) {
            // Use absolute screen position
            auto &transform = world->unpack<PixelTransform>(entity);
            offset = int2(transform.position);
            scale = transform.scale;
        } else if (world->has_component<Transform>(entity)) {
            // Use relative world position
            auto &transform = world->unpack<Transform>(entity);
            offset = world_to_screen(transform.position, camera);
            scale = transform.scale * camera.scale;
        } else {
            continue;
        }
        bool has_shadow = world->has_component<ShadowEffect>(entity);
        if (has_shadow)
            shadow = world->unpack<ShadowEffect>(entity);

        wrap_text(text, scale, wrap_info_cache);
        int x = 0;
        for (size_t i = 0; i < text.text.size(); ++i) {
            uint32_t codepoint = text.text[i];
            if (codepoint == '\r') {
                continue;
            }
            if (codepoint == '\n' || wrap_info_cache[i]) {
                offset.y += text.font->line_height * scale.y;
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
            SDL_Rect dst{int(offset.x + (x + glyph.ox) * scale.x),
                         int(offset.y + glyph.oy * scale.y),
                         int(glyph.rect.w * scale.x),
                         int(glyph.rect.h * scale.y)};

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

FpsDisplay &make_fps_display(World *world,
                            const std::shared_ptr<Font> &font, // Remove
                            const Color &color) {
    auto entity = world->make_entity();
    world->make_system<FrameTimer>();
    world->pack(entity, Text{font, "---", color});
    world->pack(entity, ShadowEffect{Color::Black, {0.0f, 2.0f}});
    world->pack(entity, PixelTransform{{10.0f, 10.0f}});
    return world->pack(entity, FpsDisplay{FpsDisplay::FPS});
}

void FrameTimer::update(World *world, float dt) {
    char buffer[32];
    for (auto entity : world->view<Text, FpsDisplay>()) {
        auto &fps = world->unpack<FpsDisplay>(entity);
        fps.time += dt;

        if (fps.time < fps.interval) {
            continue;
        }
        fps.time = 0.0f;

        auto &text = world->unpack<Text>(entity);
        int value_fps = int(roundf(1.0f / dt));
        float value_ms = dt * 1000.0f;

        switch (fps.unit) {
        case FpsDisplay::FPS:
            SDL_snprintf(buffer, sizeof(buffer), "%d", value_fps);
            break;
        case FpsDisplay::Miliseconds:
            SDL_snprintf(buffer, sizeof(buffer), "%.3fms", value_ms);
            break;
        case FpsDisplay::All:
        default:
            SDL_snprintf(buffer, sizeof(buffer),
                         "%d (%.3fms)", value_fps, value_ms);
            break;
        }
        text.text = std::string{buffer};
    }
}

} // two
