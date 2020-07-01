#include "two.h"

#include "SDL.h"
#include "physfs/physfs.h"
#include "entity.h"
#include "debug.h"

namespace two {

namespace internal {

EventDispatcher events;

} // internal

static World *world;
static bool running;

SDL_Window *window;
SDL_Renderer *gfx;

void init(int argc, char *argv[]) {
    PHYSFS_init(argc > 0 ? argv[0] : nullptr);
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
}

void create_window(const char *title, int width, int height) {
    window = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              width, height, 0);

    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    gfx = SDL_CreateRenderer(
        window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    set_logical_size(width, height);
}

void set_logical_size(int width, int height) {
    SDL_RenderSetLogicalSize(gfx, width, height);
}

// Rename to load_world?
void make_world(World *w) {
    destroy_world(world);
    world = w;
    world->load();
}

void destroy_world(World *w) {
    if (w == nullptr) {
        return;
    }
    w->unload();
    w->destroy_systems();
    clear_event_listeners();
    delete world; // FIXME
}

World &active_world() {
    return *world;
}

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

void BackgroundRenderer::draw(World &world) {
    auto camera_entity = world.view_one<Camera>();

    ASSERTS(camera_entity.has_value,
            "Missing an entity with a Camera component");

    auto &camera = world.unpack<Camera>(camera_entity.value());
    if (camera.background_is_clear_color) {
        SDL_SetRenderDrawColor(gfx, camera.background.r,
                               camera.background.g, camera.background.b, 255);
        SDL_RenderClear(gfx);
        return;
    }
    SDL_SetRenderDrawColor(gfx, 0, 0, 0, 255);
    SDL_RenderClear(gfx);
    SDL_Rect dst{0, 0, 0, 0};
    SDL_RenderGetLogicalSize(gfx, &dst.w, &dst.h);
    SDL_SetRenderDrawColor(gfx, camera.background.r,
                           camera.background.g, camera.background.b, 255);
    SDL_RenderFillRect(gfx, &dst);
}

void clear_event_listeners() {
    internal::events.clear();
}

static void push_event(const SDL_Event &e) {
    switch (e.type) {
    case SDL_KEYDOWN:
         emit(KeyDown{e.key.keysym.sym,
                      e.key.keysym.scancode,
                      e.key.repeat != 0});
         break;
    case SDL_KEYUP:
         emit(KeyUp{e.key.keysym.sym,
                    e.key.keysym.scancode,
                    e.key.repeat != 0});
         break;
    case SDL_MOUSEBUTTONDOWN:
        {
            // We use button 2 for the secondary click, but SDL uses button 2.
            int index = e.button.button;
            switch (index) {
            case SDL_BUTTON_RIGHT:
                index = 2;
                break;
            case SDL_BUTTON_MIDDLE:
                index = 3;
                break;
            }
            MouseDown res;
            res.button = index;
            res.position = Vector2i{e.button.x, e.button.y};
            emit(res);
        }
        break;
    case SDL_MOUSEBUTTONUP:
        {
            int index = e.button.button;
            switch (index) {
            case SDL_BUTTON_RIGHT:
                index = 2;
                break;
            case SDL_BUTTON_MIDDLE:
                index = 3;
                break;
            }
            MouseUp res;
            res.button = index;
            res.position = Vector2i{e.button.x, e.button.y};
            emit(res);
        }
        break;
    case SDL_MOUSEWHEEL:
        {
            // Mouse scrolling is inverted
            int dir = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
            Vector2 delta{float(e.wheel.x * dir), float(e.wheel.y * dir)};
            emit(MouseScroll{delta});
        }
    case SDL_APP_LOWMEMORY:
        emit(LowMemory{});
        break;
    case SDL_QUIT:
    case SDL_APP_TERMINATING:
        // quit will send out the event
        quit();
        break;
    default:
        break;
    }
}

void pump() {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        push_event(e);
    }
}

Vector2i mouse_position() {
    Vector2i state;
    SDL_GetMouseState(&state.x, &state.y);
    return state;
}

bool get_mouse_button(int button) {
    auto state = SDL_GetMouseState(nullptr, nullptr);
    switch (button) {
    case 2:
        button = SDL_BUTTON_MIDDLE;
        break;
    case 3:
        button = SDL_BUTTON_RIGHT;
        break;
    }
    return SDL_BUTTON(button) & state;
}

int run() {
    ASSERT(window != nullptr);
    running = true;
    SDL_Event e;
    while (running) {
        pump();

        if (!running) {
            // Make sure the application is still running.
            break;
        }

        // World's should handle how update is called on their systems.
        world->update(0);

        // Drawing must be done sequentially so draw is called here for
        // each system.
        for (auto *system : world->systems()) {
            system->draw(*world);
        }

        SDL_RenderPresent(gfx);
    }
    SDL_DestroyRenderer(gfx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void quit() {
    emit(Quit{});
    destroy_world(world);
    running = false;
}

}
