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

#ifndef TWO_ENTITY_H
#define TWO_ENTITY_H

#include <cstdint>
#include <bitset>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <memory>
#include <algorithm>
#include <climits>

#include "debug.h"
#include "optional.h"
#include "mathf.h"

// Allows size of entity types (identifiers) to be configured
#ifndef TWO_ENTITY_INT_TYPE
#define TWO_ENTITY_INT_TYPE uint16_t
#endif

// Allows size of component types (identifiers) to be configured
#ifndef TWO_COMPONENT_INT_TYPE
#define TWO_COMPONENT_INT_TYPE uint8_t
#endif

// Defines the maximum number of entities that may be alive at the same time
#ifndef TWO_ENTITY_MAX
#define TWO_ENTITY_MAX 4096
#endif

// Defines the maximum number of component types
#ifndef TWO_COMPONENT_MAX
#define TWO_COMPONENT_MAX 64
#endif

// Define only available in this file.
#ifdef TWO_DEBUG_ENTITY
#define E_MSG(...) two::log(__VA_ARGS__)
#else
#define E_MSG(...)
#endif

namespace two {
namespace internal {

template <typename T>
struct TypeIdInfo {
    static const T *const value;
};

template <typename T>
const T *const TypeIdInfo<T>::value = nullptr;

} // internal

// Compile time id for a given type.
using type_id_t = const void *;

// Compile time typeid. Note: T must be a value type.
template <typename T>
constexpr type_id_t type_id() {
  return &internal::TypeIdInfo<typename std::remove_const<
      typename std::remove_volatile<typename std::remove_pointer<
          typename std::remove_reference<T>::type>::type>::type>::type>::value;
}

// A unique identifier representing each entity in the world.
using Entity = TWO_ENTITY_INT_TYPE;
static_assert(std::is_integral<Entity>(), "Entity must be integral");

// A unique identifier represeting each type of component.
using ComponentType = TWO_COMPONENT_INT_TYPE;
static_assert(std::is_integral<ComponentType>(),
              "ComponentType must be integral");

// Holds information on which components are attached to an entity.
// 1 bit is used for each component type.
// Note: Do not serialize an entity mask since which bit represents a
// given component may change. Use the has_component<Component> function
// instead.
using EntityMask = std::bitset<TWO_COMPONENT_MAX>;

// Used to represent an entity that has no value. The NullEntity exists
// in the world but has no components.
constexpr Entity NullEntity = 0;

// An empty component that is used to indicate whther the entity it is
// attached to is currently active.
struct Active {};

class World;

// Base class for all systems. The lifetime of systems is managed by a World.
class System {
public:
    virtual ~System() = default;
    virtual void load(World *world);
    virtual void update(World *world, float dt);
    virtual void draw(World *world);
    virtual void unload(World *world);
};

class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void remove(Entity entity) = 0;
    virtual void copy(Entity dst, Entity src) = 0;
};

// Manages all instances of a component type and keeps track of which
// entity a component is attached to.
template <typename T>
class ComponentArray : public IComponentArray {
public:
    static_assert(std::is_copy_assignable<T>(),
                  "Component type must be copy assignable");

    // Entity int type is used to ensure we can address the maximum
    // number of entities.
    using PackedSizeType = TWO_ENTITY_INT_TYPE;

    // Approximate amount of memory used when the array is initialized,
    // used to reduce the amount of initial allocations.
    static constexpr int MinSize = 2048;

    ComponentArray();

    // Returns a component of type T given an Entity.
    // Note: References returned by this function are only guaranteed to be
    // valid during the frame in which the component was read, after
    // that the component may become invalidated. Don't hold reference.
    inline T &read(Entity entity);

    // Returns the memory contents of a component. May be useful for debugging,
    // prefer using the non virtual `read`. The memory returned is only
    // guaranteed to be valid for the duration of the frame where the function
    // was called. Returns nullptr if entity does not have the component.
    //const uint8_t *read_data(Entity entity, size_t &out_size) override;

    // Set a component in the packed array and associate an entity with the
    // component.
    T &write(Entity entity, const T &component);

    // Invalidate this component type for an entity.
    // This function will always succeed, even if the entity does not
    // contain a component of this type.
    //
    // > References returned by `read` may become invalid after remove is
    // called.
    void remove(Entity entity) override;

    void copy(Entity dst, Entity src) override;

    inline bool contains(Entity entity) const;

    // Returns the number of valid components in the packed array.
    size_t count() const { return packed_count; };

private:
    // All instances of component type T are stored in a contiguous vector.
    std::vector<T> packed_array;

    // Maps an Entity id to an index in the packed array.
    std::unordered_map<Entity, PackedSizeType> entity_to_packed;

    // Maps an index in the packed component array to an Entity.
    std::unordered_map<PackedSizeType, Entity> packed_to_entity;

    // Number of valid entries in the packed array, other entries beyond
    // this count may be uninitialized or invalid data.
    size_t packed_count = 0;
};

// A world holds a collection of systems, components and entities.
class World {
public:
    World() = default;

    World(const World &) = delete;
    World &operator=(const World &) = delete;

    World(World &&) = default;
    World &operator=(World &&) = default;

    virtual ~World() = default;

    // Called before the first frame when the world is created.
    virtual void load();

    // Called once per per frame after all events have been handled and
    // before draw.
    //
    // > Systems are not updated on their own, you should call update
    // on systems here. Note that you should not call `draw()` on
    // each system since that is handled in the main loop.
    virtual void update(float dt);

    // Called before a world is unloaded.
    //
    // > Note: When overriding this function you need to call unload
    // on each system and free them.
    virtual void unload();

    // Creates a new entity in the world with an Active component.
    Entity make_entity();

    // Creates a new entity and copies components from another.
    Entity make_entity(Entity prefab);

    // Creates a new inactive entity in the world. The entity will need
    // to have active set before it can be used by systems.
    // Useful to create entties without initializing them.
    // > Note: Inactive entities still exist in the world and can have
    // components added to them.
    Entity make_inactive_entity();

    // Copy components from entity `src` to entity `dst`.
    void copy_entity(Entity dst, Entity src);

    // Destroys an entity and all of its components.
    void destroy_entity(Entity entity);

    // Returns the entity mask
    inline const EntityMask &get_mask(Entity entity) const;

    // Adds or replaces a component and associates an entity with the
    // component.
    //
    // Adding components will invalidate the cache. The number of cached views
    // is *usually* approximately equal to the number of systems, so this
    // operation is not that expensive but you should avoid doing it every
    // frame.
    //
    // > Note: If the entity already has a component of the same type, the old
    // component will be replaced. Replacing a component with a new instance
    // is *much* faster than calling `remove_component` and then `pack` which
    // would result in the cache being rebuilt twice. Replacing a component
    // does not invalidate the cache.
    template <typename Component>
    Component &pack(Entity entity, const Component &component);

    // Returns a component of the given type associated with an entity.
    // This function will only check if the component does not exist for an
    // entity if assertions are enabled, otherwise it is unchecked.
    // Use has_component if you need to verify the existence of a component
    // before removing it. This is a cheap operation.
    //
    // This function returns a reference to a component in the packed array.
    // The reference may become invalid after `remove_component` is called
    // since `remove_component` may move components in memory.
    //
    //     auto &a = world.unpack<A>(entity1);
    //     a.x = 5; // a is a valid reference and x will be updated.
    //
    //     auto b = world.unpack<B>(entity1); // copy B
    //     world.remove_component<B>(entity2);
    //     b.x = 5;
    //     world.pack(entity1, b); // Ensures b will be updated in the array
    //
    //     auto &c = world.unpack<C>(entity1);
    //     world.remove_component<C>(entity2);
    //     c.x = 5; // may not update c.x in the array
    //
    // If you plan on holding the reference it is better to copy the
    // component and then pack it again if you have modified the component.
    // Re-packing a component is a cheap operation and will not invalidate.
    // the cache.
    //
    // > Do not store this reference between frames such as in a member
    // variable, store the entity instead and call unpack each frame. This
    // operation is designed to be called multiple times per frame so it
    // is very fast, there is no need to `cache` a component reference in
    // a member variable.
    template <typename Component>
    inline Component &unpack(Entity entity);

    // Returns true if a component of the given type is associated with an
    // entity. This is a cheap operation.
    template <typename Component>
    inline bool has_component(Entity entity);

    // Removes a component from an entity. Removing components invalidates
    // the cache.
    //
    // This function will only check if the component does not exist for an
    // entity if assertions are enabled, otherwise it is unchecked. Use
    // has_component if you need to verify the existence of a component before
    // removing it.
    template <typename Component>
    void remove_component(Entity entity);

    // Components will be registered on their own if a new type of component is
    // added to an entity. There is no need to call this function unless you
    // are doing something specific that requires it.
    template <typename Component>
    void register_component();

    // Adds or removes an Active component
    inline void set_active(Entity entity, bool active);

    template <typename... Components>
    const std::vector<Entity> &view(bool include_inactive = false);

    // Returns the **first** entity that contains all components requested.
    // Views always keep entities in the order that the entity was
    // added to the view, so `view_one()` will reliabily return the same
    // entity that matches the constraints unless the entity was destroyed
    // or has had one of the required components removed.
    //
    // The returned optional will have no value if no entities exist with
    // all requested components.
    template <typename... Components>
    Optional<Entity> view_one(bool include_inactive = false);

    // Finds the first entity with the requested component and unpacks
    // the component requested. This is convenience function for getting at
    // a single component in a single entity.
    //
    //     auto camera = world.unpack_any<Camera>();
    //
    //     // is equivalent to
    //     auto entity = world.view_any<Camera>().value();
    //     auto camera = world.unpack<Camera>(entity);
    //
    // Unlike `view_one()` this function will panic if no entities with
    // the requested component were matched. Only use this function if not
    // matching any entity should be an error, if not use `view_one()`
    // instead.
    template <typename Component>
    Component &unpack_one(bool include_inactive = false);

    // Returns all entities in the world. Entities returned may be inactive.
    // > Note: Calling `destroy_entity()` will invalidate the iterator.
    inline const std::vector<Entity> &unsafe_view_all() { return entities; }

    // Creates and adds a system to the world. This function calls
    // `System::load` to initialize the system.
    //
    // > Systems are not unique. 'Duplicate' systems, that is different
    // instances of the same system type, are allowed in the world.
    template <class T, typename... Args>
    T *make_system(Args &&...args);

    // Adds a system to the world. This function calls `System::load` to
    // initialiaze the system.
    //
    // > You may add multiple different instances of the same system type,
    // but passing a system pointer that points to an instance that is
    // already in the world is not allowed and will not be checked by default.
    // Enable `TWO_PARANOIA` to check for duplicate pointers.
    template <class T>
    T* make_system(T *system);

    // Adds a system to the world before another system if it exists.
    // `Before` is an existing system.
    // `T` is the system to be added before an existing system.
    template <class Before, class T, typename... Args>
    T *make_system_before(Args &&...args);

    // Returns the first system that matches the given type.
    // System returned will be `nullptr` if it is not found.
    template <class T>
    T *get_system();

    // Returns all system that matches the given type.
    // Systems returned will not be null.
    template <class T>
    void get_all_systems(std::vector<T *> &systems);

    // System must not be null. Do not destroy a system while the main loop is
    // running as it could invalidate the system iterator, consider checking
    // if the system should run or not in the system update loop instead.
    void destroy_system(System *system);

    // Destroy all systems in the world.
    void destroy_systems();

    // Systems returned will not be null.
    inline const std::vector<System *> &systems() { return active_systems; }

    template <typename Component>
    inline ComponentType find_or_register_component();

    // Recycles entity ids so that they can be safely reused. This function
    // exists to ensure we don't reuse entity ids that are still present in
    // some cache even though the entity has been destroyed. This can happen
    // since cache operations are done in a 'lazy' manner.
    // Called at the end of each frame.
    void collect_unused_entities();

private:
    // Used to speed up entity lookups
    struct EntityCache {
        struct Diff {
            enum Operation { Add, Remove };
            Entity entity;
            Operation op;
        };
        std::vector<Entity> entities;
        std::vector<Diff> diffs;
        std::unordered_set<Entity> lookup;
    };

    struct DestroyedEntity {
        Entity entity;
        // Caches that needs to be rebuilt before the entity can be reused
        std::vector<EntityCache *> caches;
    };

    size_t component_type_index = 0;
    size_t alive_count = 0;

    // Systems cannot outlive World.
    std::vector<System *> active_systems;

    // Kept separate since most of the time we just want to iterate through
    // all the systems and do not need to know their types.
    std::vector<type_id_t> active_system_types;

    // Contains available entity ids. When creating entities check if this
    // is not empty, otherwise use alive_count + 1 as the new id.
    std::vector<Entity> unused_entities;

    // Contains available entity ids that may still be present in
    // some cache. Calling `collect_unused_entities()` will remove the
    // entity from the caches so that the entity can be reused.
    std::vector<DestroyedEntity> destroyed_entities;

    // All alive (but not necessarily active) entities.
    std::vector<Entity> entities;

    std::unordered_map<EntityMask, EntityCache> view_cache;

    // Index with component index from component_types[type]
    std::array<std::unique_ptr<IComponentArray>, TWO_COMPONENT_MAX> components;

    // Masks for all entities.
    std::array<EntityMask, TWO_ENTITY_MAX> entity_masks = {0};

    std::unordered_map<type_id_t, ComponentType> component_types;

    void apply_diffs_to_cache(EntityCache *cache);
    void invalidate_cache(EntityCache *cache, EntityCache::Diff &&diff);
};

inline const EntityMask &World::get_mask(Entity entity) const {
    return entity_masks[entity];
}

template <typename Component>
Component &World::pack(Entity entity, const Component &component) {
    auto current_mask = entity_masks[entity];
    auto &mask = entity_masks[entity];

    // Component may not have been regisered yet
    auto type = find_or_register_component<Component>();
    mask.set(type);

    auto *a = static_cast<ComponentArray<Component> *>(components[type].get());
    auto &new_component = a->write(entity, component);

    if (current_mask[type] == mask[type]) {
        // entity already has a component of this type, the component was
        // replaced, but since the mask is unchanged there is no need to
        // rebuild the cache.
        E_MSG("%s is unchanged since entity #%x is unchanged",
              mask.to_string().c_str(), entity);
        return new_component;
    }

    // Invalidate caches
    for (auto &cached : view_cache) {
        if ((mask & cached.first) != cached.first) {
            continue;
        }
        auto &lookup = cached.second.lookup;
        if (lookup.find(entity) != lookup.end()) {
            // Entity is already in the cache
            continue;
        }

        invalidate_cache(&cached.second,
            EntityCache::Diff{entity, EntityCache::Diff::Add});

        E_MSG("%s now includes entity #%x",
              mask.to_string().c_str(), entity);
    }
    return new_component;
}

template <typename Component>
inline Component &World::unpack(Entity entity) {
    // Assume component was registered when it was packed
    ASSERT(component_types.find(type_id<Component>())
           != component_types.end());

    auto type = component_types[type_id<Component>()];
    auto *a = static_cast<ComponentArray<Component> *>(components[type].get());
    return a->read(entity);
}

template <typename Component>
inline bool World::has_component(Entity entity) {
    // This function must work if a component has never been registered,
    // since it's reasonable to check if an entity has a component when
    // a component type has never been added to any entity.
    auto type_it = component_types.find(type_id<Component>());
    if (type_it == component_types.end()) {
        return false;
    }
    auto type = type_it->second;
    auto *a = static_cast<ComponentArray<Component> *>(components[type].get());
    return a->contains(entity);
}

template <typename Component>
void World::remove_component(Entity entity) {
    // Assume component was registered when it was packed
    ASSERT(component_types.find(type_id<Component>())
           != component_types.end());

    auto type = component_types[type_id<Component>()];
    auto *a = static_cast<ComponentArray<Component> *>(components[type].get());
    a->remove(entity);

    // Invalidate caches
    for (auto &cached : view_cache) {
        if (!cached.first[type]) {
            continue;
        }
        auto &lookup = cached.second.lookup;
        if (lookup.find(entity) == lookup.end()) {
            // Entity has already been removed from cache
            continue;
        }
        invalidate_cache(&cached.second,
            EntityCache::Diff{entity, EntityCache::Diff::Remove});

        E_MSG("%s no longer includes entity #%x",
              cached.first.to_string().c_str(), entity);
    }
    entity_masks[entity].reset(type);
}

inline void World::set_active(Entity entity, bool active) {
    // If active is constant this conditional should be removed when
    // the function is inlined
    if (active)
        pack(entity, Active{});
    else
        remove_component<Active>(entity);
}

template <typename... Components>
const std::vector<Entity> &World::view(bool include_inactive) {
    static const ComponentType active_component_t =
        find_or_register_component<Active>();

    EntityMask mask;

    // Need the alias for msvc
    using Expand = int[];
    (void)Expand{
        ((void)(mask.set(
            // Component may not have been registered
            find_or_register_component<Components>())), 0)
        ...
    };

    if (!include_inactive) {
        mask.set(active_component_t);
    }

    auto cache_it = view_cache.find(mask);
    if (LIKELY(cache_it != view_cache.end())) {
        auto &cache = cache_it->second;

        E_MSG("%s view (%lu) [ops: %lu]",
              mask.to_string().c_str(),
              cache.entities.size(),
              cache.diffs.size());

        if (LIKELY(cache.diffs.empty())) {
            return cache.entities;
        }
        apply_diffs_to_cache(&cache);
        return cache.entities;
    }
    E_MSG("%s view (initial cache build)", mask.to_string().c_str());

    std::vector<Entity> cache;
    std::unordered_set<Entity> lookup;

    for (auto entity : entities) {
        if ((mask & entity_masks[entity]) == mask) {
            cache.push_back(entity);
            lookup.insert(entity);
        }
    }
    view_cache.emplace(std::make_pair(mask,
        EntityCache{std::move(cache), {}, std::move(lookup)}));

    return view_cache[mask].entities;
}

template <typename... Components>
Optional<Entity> World::view_one(bool include_inactive) {
    auto &v = view<Components...>(include_inactive);
    if (v.size() > 0) {
        return v[0];
    }
    return {};
}

template <typename Component>
Component &World::unpack_one(bool include_inactive) {
    auto &v = view<Component>(include_inactive);
    ASSERTS(v.size() > 0, "No entities were matched");
    return unpack<Component>(v[0]);
}

template <class T, typename... Args>
T *World::make_system(Args &&...args) {
    return make_system(new T(std::forward<Args>(args)...));
}

template <class T>
T *World::make_system(T *system) {
    static_assert(std::is_convertible<T *, System *>(),
                  "Type cannot be converted to a System");
    ASSERTS_PARANOIA(
        std::find(active_systems.begin(),
                  active_systems.end(), system) == active_systems.end(),
        "%p points to a system already in the world", (void *)system);

    active_systems.push_back(system);
    active_system_types.push_back(type_id<T>());
    system->load(this);
    return system;
}

template <class Before, class T, typename... Args>
T *World::make_system_before(Args &&...args) {
    static_assert(std::is_convertible<T *, System *>(),
                  "Type cannot be converted to a System");
    ASSERT(active_systems.size() == active_system_types.size());

    auto pos = std::find(active_system_types.begin(),
                         active_system_types.end(),
                         type_id<Before>());

    auto *system = new T(std::forward<Args>(args)...);
    if (pos == active_system_types.end()) {
        log("not found");
        return system;
    }
    size_t index = std::distance(pos, active_system_types.end()) - 1;
    active_systems.insert(active_systems.begin() + index, system);
    active_system_types.insert(pos, type_id<T>());
    return system;
}

template <class T>
T *World::get_system() {
    static_assert(std::is_convertible<T *, System *>(),
                  "Type cannot be converted to a System");
    ASSERT(active_systems.size() == active_system_types.size());

    auto pos = std::find(active_system_types.begin(),
                         active_system_types.end(),
                         type_id<T>());

    if (pos == active_system_types.end()) {
        return nullptr;
    }
    auto index = std::distance(active_system_types.begin(), pos);
    return static_cast<T *>(active_systems[index]);
}

template <class T>
void World::get_all_systems(std::vector<T *> &systems) {
    static_assert(std::is_convertible<T *, System *>(),
                  "Type cannot be converted to a System");
    ASSERT(active_systems.size() == active_system_types.size());

    for (size_t i = 0; i < active_systems.size(); ++i) {
        if (active_system_types[i] != type_id<T>()) {
            continue;
        }
        systems.push_back(static_cast<T *>(active_systems[i]));
    }
}

template <typename Component>
void World::register_component() {
    // Component must not already exist
    ASSERT(component_types.find(
        type_id<Component>()) == component_types.end());

    int i = component_type_index++;
    component_types.emplace(std::make_pair(type_id<Component>(), i));
    components[i] = std::unique_ptr<ComponentArray<Component>>(
        new ComponentArray<Component>);
}

template <typename Component>
inline ComponentType World::find_or_register_component() {
    auto match = component_types.find(type_id<Component>());
    if (LIKELY(match != component_types.end())) {
        return match->second;
    }
    register_component<Component>();
    return component_types[type_id<Component>()];
}

template <typename T>
inline ComponentArray<T>::ComponentArray() {
    packed_array.reserve(next_pow2(MinSize / sizeof(T)));
}

template <typename T>
inline T &ComponentArray<T>::read(Entity entity) {
    ASSERTS(entity_to_packed.find(entity) != entity_to_packed.end(),
            "Missing component for Entity #%x", entity);
    return packed_array[entity_to_packed[entity]];
}

template <typename T>
T &ComponentArray<T>::write(Entity entity, const T &component) {
    if (entity_to_packed.find(entity) != entity_to_packed.end()) {
        // Replace component
        auto pos = entity_to_packed[entity];
        packed_array[pos] = component;
        return packed_array[pos];
    }

    ASSERT(packed_count < TWO_ENTITY_MAX);

    auto pos = packed_count++;
    entity_to_packed[entity] = pos;
    packed_to_entity[pos] = entity;

    if (pos < packed_array.size())
        packed_array[pos] = component;
    else
        packed_array.push_back(component);

    return packed_array[pos];
}

template <typename T>
void ComponentArray<T>::remove(Entity entity) {
    if (entity_to_packed.find(entity) == entity_to_packed.end()) {
        // This is a no-op since calling this as a virtual member function
        // means there is no way for the caller to check if the entity
        // contains a component. `contains` is not virtual as it needs to
        // be fast.
        return;
    }
    // Move the last component into the empty slot to keep the array packed
    auto last = packed_count - 1;
    auto removed = entity_to_packed[entity];
    packed_array[removed] = packed_array[last];

    // Need to know which entity "owns" the component we just moved
    auto moved_entity = packed_to_entity[last];
    packed_to_entity[removed] = moved_entity;

    // Update the entity that has its component moved to reference
    // the new location in the packed array
    entity_to_packed[moved_entity] = removed;

    entity_to_packed.erase(entity);
    packed_to_entity.erase(last);
    --packed_count;
}

template <typename T>
void ComponentArray<T>::copy(Entity dst, Entity src) {
    write(dst, read(src));
}

template <typename T>
inline bool ComponentArray<T>::contains(Entity entity) const {
    return entity_to_packed.find(entity) != entity_to_packed.end();
}

} // two

#undef E_MSG
#endif // TWO_ENTITY_H
