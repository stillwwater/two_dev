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

#ifndef TWO_EVENT_H
#define TWO_EVENT_H

#include <functional>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <memory>

#include "SDL.h"
#include "entity.h"
#include "mathf.h"

namespace two {

// Emitted when a key is pressed.
struct KeyDown {
    // The virtual key that was pressed. This key will always be the same
    // regardless of keyboard layout, but the key might be in a different
    // physical location on the keyboard.
    SDL_Keycode key;

    // The physical key that was pressed in the equivalent ANSI layou key.
    // The scancode will always represent the same physical location on the
    // keyboard but the actual key might differ depending on the layout.
    // The scancode is ideal for checking if WASD is pressed for example.
    SDL_Scancode scancode;

    // Whether this is event is a key repeat event. The delay between
    // repeat events will depend on the system settings.
    bool is_repeat;
};

// Emitted when a key is released.
struct KeyUp {
    // The virtual key that was released. This key will always be the same
    // regardless of keyboard layout, but the key might be in a different
    // physical location on the keyboard.
    SDL_Keycode key;

    // The physical key that was released in the equivalent ANSI layou key.
    // The scancode will always represent the same physical location on the
    // keyboard but the actual key might differ depending on the layout.
    // The scancode is ideal for checking if WASD is released for example.
    SDL_Scancode scancode;

    // Whether this is event is a key repeat event. The delay between
    // repeat events will depend on the system settings.
    bool is_repeat;
};

// Emitted when a mouse button is pressed
struct MouseDown {
    // Mouse position in pixels. Use `two::screen_to_world()` to convert
    // to world coordinates.
    Vector2i position;

    // The button index that was pressed. 1 is the primary button, 2 is
    // the secondary and 3 is the middle mouse button. Other buttons
    // depend on the device.
    int button;
};

// Emitted when a mouse button is released.
struct MouseUp {
    // Mouse position in pixels. Use `two::screen_to_world()` to convert
    // to world coordinates.
    Vector2i position;

    // The button index that was released. 1 is the primary button, 2 is
    // the secondary and 3 is the middle mouse button. Other buttons
    // depend on the device.
    int button;
};

// Emitted when the mouse or is scrolled on the X or Y axis.
struct MouseScroll {
    // The amount the scroll wheel has moved.
    Vector2 delta;
};

// Emitted when the game is closing. The event is emitted before the loaded
// World is destroyed.
struct Quit {};

// Emitted when the aplication is low on memory.
struct LowMemory {};

class IEventBus {
public:
    virtual ~IEventBus() = default;
};

// An EventBus handles events for a single event type.
template <typename T>
class EventBus : public IEventBus {
public:
    using EventHandler = std::function<bool(const T &)>;

    // Adds a function as an event handler
    void bind(const EventHandler &callback);

    // Removes event handler
    void unbind(const EventHandler &callback);

    // Emits an event to all event handlers
    void emit(const T &event) const;

private:
    std::vector<EventHandler> handlers;
};

// The event manager handles many event types. You can bind event handlers
// with `two::bind()` and emit events using `two::emit()` which will use the
// default EventDispatcher. You may also create other EventDispatchers if you need
// to.
class EventDispatcher {
public:
    // Adds a function to receive events of type T
    template <typename T>
    void bind(std::function<bool(const T &)> callback);

    // Same as `bind(callback)` but allows a member function to be used
    // as an event handler.
    template <typename Event, typename T, class U>
    void bind(T callback, U this_ptr);

    // Emit an event to all event handlers. If a handler funtion in the
    // chain returns true then the event is considered handled and will
    // not propagate to other listeners.
    template <typename T>
    void emit(const T &event);

    // Removes all event handlers
    void clear();

private:
    std::unordered_map<type_id_t, std::unique_ptr<IEventBus>> event_buses;
};

template <typename T>
void EventDispatcher::bind(std::function<bool (const T &)> callback) {
    auto type = type_id<T>();
    if (event_buses.find(type) == event_buses.end()) {
        auto event_bus = std::unique_ptr<EventBus<T>>(new EventBus<T>);
        event_bus->bind(callback);
        event_buses.emplace(std::make_pair(type, std::move(event_bus)));
        return;
    }
    static_cast<EventBus<T> *>(event_buses[type].get())->bind(callback);
}

template <typename Event, typename T, class U>
void EventDispatcher::bind(T callback, U this_ptr) {
    bind<Event>(std::bind(callback, this_ptr, std::placeholders::_1));
}

template <typename T>
void EventDispatcher::emit(const T &event) {
    if (event_buses.find(type_id<T>()) == event_buses.end()) {
        return;
    }
    static_cast<EventBus<T> *>(event_buses[type_id<T>()].get())->emit(event);
}

inline void EventDispatcher::clear() {
    event_buses.clear();
}

template <typename T>
void EventBus<T>::bind(const EventHandler &callback) {
    handlers.push_back(callback);
}

template <typename T>
void EventBus<T>::unbind(const EventHandler &callback) {
    auto pos = std::find(handlers.begin(), handlers.end(), callback);
    handlers.erase(pos);
}

template <typename T>
void EventBus<T>::emit(const T &event) const {
    for (auto &callback : handlers) {
        if (callback(event)) {
            break;
        }
    }
}

} // two

#endif // TWO_EVENT_H
