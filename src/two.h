#ifndef TWO_H
#define TWO_H

#include "SDL.h"
#include "entity.h"
#include "event.h"
#include "image.h"

namespace two {

namespace internal {

extern EventDispatcher events;

} // internal

// Access to the SDL created window. You may use this to interact with
// the SDL api if you need to. Will always be valid after init is called
// unless you modify it.
extern SDL_Window *window;

// Access to the SDL created rendered. You may use this to interact with
// the SDL api if you need to. Will always be valid after init is called
// unless you modify it.
extern SDL_Renderer *gfx;

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

    // Whether to use the camera color to clear the screen instead of
    // clearing the screen with a black color and then drawing a Rect with
    // the camera color over the entire screen.
    //
    // If this option is enabled and you are rendering at a non native
    // resolution the pillar box or letter box ('black bars') will have the
    // camera background color.
    bool background_is_clear_color;

    Camera() {}

    Camera(int tilesize, Color background)
        : tilesize{Vector2i{tilesize, tilesize}}
        , background{background}
        , position{Vector2{0, 0}}
        , scale{1.0f}
        , background_is_clear_color{false} {}

    Camera(int tilesize, Color background, Vector2 position)
        : tilesize{Vector2i{tilesize, tilesize}}
        , background{background}
        , position{position}
        , scale{1.0f}
        , background_is_clear_color{false} {}

    Camera(Vector2i tilesize, Color background, Vector2 position)
        : tilesize{tilesize}
        , background{background}
        , position{position}
        , scale{1.0f}
        , background_is_clear_color{false} {}
};

// Basic renderer responsible for clearing the screen. This System is
// added by other renderers such as the SpriteRenderer if it has not been
// added.
class BackgroundRenderer : public System {
public:
    void draw(World &world) override;
};

// Initializes SDL and the filesystem. This should be the first function
// you call before calling any other function in the engine.
void init(int argc, char *argv[]);

// Creates a new window. Must be called after init!
void create_window(const char *title, int width, int height);

void set_logical_size(int width, int height);

// Prefer the templated version unless you need to do something different
// when allocating memory for a World.
void make_world(World *world);

template <class T, typename... Args>
void make_world(Args &&...args) {
    make_world(new T(std::forward<Args>(args)...));
}

// Returns the currently loaded world
World &active_world();

// Called when loading a new world. Don't call this unless you have a
// reason to manually unload a scene early.
//
// This function will clean up all resources allocated by the World base
// class after calling Scene::unload.
void destroy_world(World *world);

// Run it! This will fail if you haven't created a window.
int run();


// Convert world to screen (pixel) coordinates.
Vector2i world_to_screen(const Vector2 &v, const Camera &camera);

// Convert screen to world coordinates.
Vector2 screen_to_world(const Vector2i &v, const Camera &camera);

// Registers an event handler with the main `EventDisptacher`.
// Example:
//
//     two::bind<KeyPressed>(&MainWorld::keypressed, this);
//
template <typename Event>
void bind(const std::function<bool(const Event &)> &callback) {
    internal::events.bind(callback);
}

// Same as `bind(callback)` but allows a member function to be used
// as an event handler.
template <typename Event, typename T, class U>
void bind(T callback, U this_ptr) {
    internal::events.bind<Event>(callback, this_ptr);
}

// Emits an event to all listeners using the main `EventDispatcher`.
// If a handler funtion in the chain returns true then the event is
// considered handled and will not propagate to other listeners.
template <typename Event>
void emit(const Event &event) {
    internal::events.emit(event);
}

// Removes all event handlers from the main `EventDispatcher`.
// This function is called when a World is destroyed, so it is not
// necessary to call it when unloading a World.
void clear_event_listeners();

// Emits any SDL events. This function is called in the main loop. Only
// use this if you are using your own main loop and would like SDL events
// handled. This function should be called at the beggining of every frame.
void pump();

// Gets the current mouse position in pixels. To convert to world units
// use `screen_to_world()`.
Vector2i mouse_position();

// Returns true if the specified mouse button is currently pressed.
bool get_mouse_button(int button);

// Stops running the game and cleans up
void quit();

} // two

#endif // TWO_H
