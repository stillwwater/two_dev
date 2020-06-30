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

#include "entity.h"

#include <tuple>
#include <algorithm>

#include "SDL_render.h"
#include "debug.h"

#ifdef TWO_DEBUG_ENTITY
#define E_MSG(...) two::log(__VA_ARGS__)
#else
#define E_MSG(...)
#endif

namespace two {

void System::load(World &) {}
void System::update(World &, float) {}
void System::draw(World &) {}
void System::unload(World &) {}

void World::load() {}
void World::update(float) {}
void World::unload() {}

Entity World::make_entity() {
    auto entity = make_inactive_entity();
    pack(entity, Active{});
    return entity;
}

Entity World::make_inactive_entity() {
    Entity entity;
    if (destroyed_entities.empty()) {
        ASSERTS(alive_count < TWO_ENTITY_MAX, "Too many entities");
        entity = alive_count;

        // Create a nullentity that will never have any components
        // This is useful when we need to store entities in an array and
        // need a way to define entities that are not valid.
        if (entity == NullEntity) {
            entities.push_back(NullEntity);
            ++entity;
            ++alive_count;
        }
    } else {
        entity = destroyed_entities.back();
        destroyed_entities.pop_back();
    }
    entities.push_back(entity);
    ++alive_count;
    return entity;
}

void World::destroy_entity(Entity entity) {
    ASSERT(entity != NullEntity);
    for (auto &a : components) {
        if (a != nullptr) {
            a->remove(entity);
        }
    }
    entity_masks[entity] = 0;
    for (auto &cached : view_cache) {
        auto &lookup = cached.second.lookup;
        if (lookup.find(entity) == lookup.end()) {
            continue;
        }
        cached.second.diffs.emplace_back(
            EntityCache::Diff{entity, EntityCache::Diff::Remove});

        E_MSG("%s no longer includes entity #%x (destroyed)",
              cached.first.to_string().c_str(), entity);
    }
    auto rem = std::find(entities.begin(), entities.end(), entity);
    ASSERT(rem != entities.end());
    std::swap(*rem, entities.back());
    entities.pop_back();

    // Make this entity id available again
    destroyed_entities.push_back(entity);
}

void World::destroy_system(System *system) {
    ASSERT(active_systems.size() == active_system_types.size());
    ASSERT(system != nullptr);

    auto pos = std::find(active_systems.begin(), active_systems.end(), system);
    if (pos == active_systems.end()) {
        log_warn("Trying to destroy a system that does not exist");
        return;
    }
    system->unload(*this);
    delete system;

    auto index = std::distance(active_systems.begin(), pos);
    active_systems.erase(pos);
    active_system_types.erase(active_system_types.begin() + index);
}

void World::apply_diffs_to_cache(EntityCache &cache) {
    for (const auto &diff : cache.diffs) {
        switch (diff.op) {
        case EntityCache::Diff::Add:
            cache.entities.push_back(diff.entity);
            cache.lookup.insert(diff.entity);
            break;
        case EntityCache::Diff::Remove:
            {
                auto &vec = cache.entities;
                auto rem = std::find(vec.begin(), vec.end(), diff.entity);
                ASSERT(rem != vec.end());

                std::swap(*rem, vec.back());
                vec.pop_back();
                cache.lookup.erase(diff.entity);
                break;
            }
        default:
            PANIC("Invalid cache operation");
            break;
        }
    }
    cache.diffs.clear();
}

void World::invalidate_cache(EntityCache &cache, EntityCache::Diff &&diff) {
    for (const auto &d : cache.diffs) {
        if (d.entity == diff.entity && d.op == diff.op) {
            return;
        }
    }
    cache.diffs.emplace_back(std::move(diff));
}

} // two

#undef E_MSG
