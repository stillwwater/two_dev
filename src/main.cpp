#include <string>

#include "SDL.h"

#include "debug.h"
#include "entity.h"
#include "entity_test.cpp"
#include "two.h"
#include "mathf.h"
#include "renderer.h"
#include "image.h"
#include "filesystem.h"
#include "sprite.h"
#include "text.h"
#include "event.h"

namespace two {

class Main2 : public two::World {
    void load() override {

    }
};

class Main : public two::World {
public:
    void load() override {
        make_system<SpriteRenderer>();
        make_system<FontRenderer>();

        bind<KeyDown>(&Main::key_down, this);
        bind<MouseDown>(&Main::mouse_down, this);
        bind<MouseScroll>(&Main::mouse_scroll, this);

        auto font = load_font("heartbit.fnt");

        // All systems invalid beyond this point

        //two::get_key();
        //two::get_scancode();
        //two::get_mouse_button()
        //
        //two::key_down()
        //two::mouse_button_down();
        //two::scancode_down()

        auto camera = make_entity();
        auto &cam = pack(camera, Camera(16, {119, 194, 217}, {0, 0}));
        cam.scale = 8;

        for (float y = 0; y < 6; ++y) {
            for (float x = 0; x < 9; ++x) {
                auto sprite = load_sprite("char16.png").value();

                auto ch = make_entity();
                pack(ch, Transform{{x, y}, {1, 1}, 0});
                pack(ch, sprite);
                break;
            }
            break;
        }
        auto &entity = unpack_one<Sprite>();
        entity.layer = 2;
        auto e = make_entity();
        pack(e, Transform{{0, 0}, {3, 3}});
        pack(e, ShadowEffect{{0, 0, 0, 255}, {0, 4}});
        auto &text = pack(e, Text{font, "Hello World!"});
        text.wrap = Text::Wrap;
        text.width = 800;

    }

    bool mouse_scroll(const MouseScroll &e) {
        auto &camera = unpack_one<Camera>();
        camera.scale += (e.delta.y * 0.2f) ;
        return true;
    }

    bool mouse_down(const MouseDown &e) {
        auto any_entity = view_one<Transform>().value();
        auto &transform = unpack<Transform>(any_entity);
        auto camera_entity = view_one<Camera>().value();
        auto &camera = unpack<Camera>(camera_entity);

        transform.position = screen_to_world(e.position, camera);
        return true;
    }

    void update(float) {
        auto &camera = unpack_one<Camera>();
        for (auto entity : view<Transform, Sprite>()) {
            auto &transform = unpack<Transform>(entity);
            transform.position.x -= 1.0f / 64;
            auto p = transform.position;
            p.x += transform.scale.x * 0.5f;
            auto pos = world_to_screen(p, camera);
            if (pos.x < 0) {
                transform.position.x = screen_to_world({800, 0}, camera).x + 0.5f;
            }
        }
    }

    // bool keydown (probably a better name since it matches unity and SDL)
    bool key_down(const KeyDown &e) {
        // TODO: make this a single operation
        // -  key repeat
        // -  clamp camera tilesize
        auto camera_entity = view_one<Camera>().value();
        auto &camera = unpack<Camera>(camera_entity);
        auto any_entity = view_one<Transform>().value();
        auto &transform = unpack<Transform>(any_entity);
        auto &text = unpack_one<Text>();

        switch (e.key) {
        case SDLK_a: text.text.push_back('a'); return false;
        case SDLK_b: text.text.push_back('b'); return false;
        case SDLK_c: text.text.push_back('c'); return false;
        case SDLK_d: text.text.push_back('d'); return false;
        case SDLK_e: text.text.push_back('e'); return false;
        case SDLK_f: text.text.push_back('f'); return false;
        case SDLK_g: text.text.push_back('g'); return false;
        case SDLK_h: text.text.push_back('h'); return false;
        case SDLK_i: text.text.push_back('i'); return false;
        case SDLK_j: text.text.push_back('j'); return false;
        case SDLK_k: text.text.push_back('k'); return false;
        case SDLK_l: text.text.push_back('l'); return false;
        case SDLK_m: text.text.push_back('m'); return false;
        case SDLK_n: text.text.push_back('n'); return false;
        case SDLK_o: text.text.push_back('o'); return false;
        case SDLK_p: text.text.push_back('p'); return false;
        case SDLK_q: text.text.push_back('q'); return false;
        case SDLK_r: text.text.push_back('r'); return false;
        case SDLK_s: text.text.push_back('s'); return false;
        case SDLK_t: text.text.push_back('t'); return false;
        case SDLK_u: text.text.push_back('u'); return false;
        case SDLK_v: text.text.push_back('v'); return false;
        case SDLK_w: text.text.push_back('w'); return false;
        case SDLK_x: text.text.push_back('x'); return false;
        case SDLK_y: text.text.push_back('y'); return false;
        case SDLK_z: text.text.push_back('z'); return false;
        case SDLK_SPACE: text.text.push_back(' '); break;
        case SDLK_RETURN: text.text.push_back('\n'); break;
        case SDLK_BACKSPACE: text.text.pop_back(); break;
        }


        switch (e.key) {
        case SDLK_w:
            camera.position.y -= .1f;
            break;
        case SDLK_d:
            camera.position.x += 0.1f;
            break;
        case SDLK_s:
            camera.position.y += 0.1f;
            break;
        case SDLK_a:
            camera.position.x -= 0.1f;
            break;
        case SDLK_UP:
            transform.position.y -= 1;
            break;
        case SDLK_RIGHT:
            transform.position.x += 1;
            break;
        case SDLK_DOWN:
            transform.position.y += 1;
            break;
        case SDLK_LEFT:
            transform.position.x -= 1;
            break;
        case SDLK_q:
            camera.tilesize -= Vector2i{4, 4};
            break;
        case SDLK_e:
            camera.tilesize += Vector2i{4, 4};
            break;
        }
        return true;
    }

};

}

int main(int argc, char *argv[]) {
    two::test::run_entity_test();
    two::init(argc, argv);

    bool ok = two::mount("assets.dat");
    ASSERT(ok);

    two::create_window("Two", 896, 504);
    two::set_logical_size(1280, 720);
    two::make_world<two::Main>();
    return two::run();
}
