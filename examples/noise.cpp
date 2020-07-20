#include <thread>

#include "two.h"
#include "noise.h"
#include "sprite.h"
#include "filesystem.h"
#include "entity.h"
#include "text.h"

namespace two {

namespace examples {

class NoiseWorld : public World {
public:
    Image *im = nullptr;
    void load() override;
    void update(float dt) override;
    void unload() override { delete im; }
};

void NoiseWorld::load() {
    // Render system
    make_system<SpriteRenderer>();
    make_system<FontRenderer>();

    auto &fps = make_fps_display(this, load_font("heartbit.fnt"));
    fps.unit = FpsDisplay::All;

    // Camera
    pack(make_entity(), Camera{1, Color::Black});

    Xorshift64 r{0x62738};

    // Texture
    im = new Image(512, 512, Image::RGBA32);

    // Sprite
    auto entity = make_entity();
    pack(entity, make_sprite(im));
    pack(entity, Transform(float2{0, 0}));
}

static void render_simplex(Image *im, int2 offset, int2 size, float t) {
    for (int y = 0; y < size.y; ++y) {
        for (int x = 0; x < size.x; ++x) {
            float2 p = float2(int2{x, y} + offset);
            p /= 128.0f;
            float c = (snoise_fractal_b(float3(p.x, p.y, t), 4, 2, 0.5) + 1.0f) * 0.5f;
            float3 col = float3(c);
            im->write(int2{x, y} + offset, Color(col));
        }
    }
}

void NoiseWorld::update(float dt) {
    ASSERT(im != nullptr);
    static float t = 0.0f;

    t += dt * 0.06f;
#if 1
    auto t1 = std::thread(render_simplex, im, int2{0  ,   0}, int2{256, 256}, t);
    auto t2 = std::thread(render_simplex, im, int2{0  , 256}, int2{256, 256}, t);
    auto t3 = std::thread(render_simplex, im, int2{256,   0}, int2{256, 256}, t);
    auto t4 = std::thread(render_simplex, im, int2{256, 256}, int2{256, 256}, t);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
#else
    render_simplex(im, int2{0, 0}, int2{512, 512}, t);
#endif

    for (auto entity : view<Sprite>()) {
        auto &sprite = unpack<Sprite>(entity);
        update_texture(sprite.texture, im);
    }

    for (auto *system : systems()) {
        system->update(this, dt);
    }
}

} // examples

} // two

int main(int argc, char *argv[]) {
    two::init(argc, argv);

    // File system
    two::mount("assets.dat");

    two::create_window("Noise", 800, 600);
    two::load_world<two::examples::NoiseWorld>();
    return two::run();
}
