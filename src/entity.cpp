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

void System::load(World *) {}
void System::update(World *, float) {}
void System::draw(World *) {}
void System::unload(World *) {}

void World::load() {}
void World::update(float) {}
void World::unload() {}

Entity World::make_entity() {
    auto entity = make_inactive_entity();
    pack(entity, Active{});
    return entity;
}

Entity World::make_entity(Entity prefab) {
    auto entity = make_entity();
    copy_entity(entity, prefab);
    return entity;
}

Entity World::make_inactive_entity() {
    Entity entity;
    if (unused_entities.empty()) {
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
        entity = unused_entities.back();
        unused_entities.pop_back();
    }
    entities.push_back(entity);
    ++alive_count;
    return entity;
}

void World::copy_entity(Entity dst, Entity src) {
    auto &dst_mask = entity_masks[dst];
    auto &src_mask = entity_masks[src];
    for (const auto &type_it : component_types) {
        if (!src_mask.test(type_it.second)) {
            continue;
        }
        auto &a = components[type_it.second];
        a->copy(dst, src);
        dst_mask.set(type_it.second);
    }

    for (auto &cached : view_cache) {
        if ((dst_mask & cached.first) != cached.first) {
            continue;
        }
        auto &lookup = cached.second.lookup;
        if (lookup.find(dst) != lookup.end()) {
            continue;
        }

        invalidate_cache(&cached.second,
            EntityCache::Diff{dst, EntityCache::Diff::Add});

        E_MSG("%s now includes entity #%x (copied from #%x)",
              mask.to_string().c_str(), dst, src);
    }
}

void World::destroy_entity(Entity entity) {
    ASSERT(entity != NullEntity);
    for (auto &a : components) {
        if (a != nullptr) {
            a->remove(entity);
        }
    }
    entity_masks[entity] = 0;

    DestroyedEntity destroyed;
    destroyed.entity = entity;

    for (auto &cached : view_cache) {
        auto &lookup = cached.second.lookup;
        if (lookup.find(entity) == lookup.end()) {
            continue;
        }
        cached.second.diffs.emplace_back(
            EntityCache::Diff{entity, EntityCache::Diff::Remove});

        // This cache must be rebuilt before the entity can be reused.
        destroyed.caches.push_back(&cached.second);

        E_MSG("%s no longer includes entity #%x (destroyed)",
              cached.first.to_string().c_str(), entity);
    }
    auto rem = std::find(entities.begin(), entities.end(), entity);
    ASSERT(rem != entities.end());
    std::swap(*rem, entities.back());
    entities.pop_back();

    destroyed_entities.emplace_back(std::move(destroyed));
}

void World::collect_unused_entities() {
    if (destroyed_entities.size() == 0) {
        // Don't want to profile this if it won't be doing anything
        return;
    }
    TWO_PROFILE_FUNC();
    for (const auto &destroyed : destroyed_entities) {
        for (auto *cache : destroyed.caches) {
            // In most cases the cache will have no diffs since if this cache
            // is viewed every frame by some system it would have been rebuilt
            // by this point anyway.
            apply_diffs_to_cache(cache);
        }
        // Make entity id available again
        unused_entities.push_back(destroyed.entity);
    }
    destroyed_entities.clear();
}

void World::destroy_system(System *system) {
    ASSERT(active_systems.size() == active_system_types.size());
    ASSERT(system != nullptr);

    auto pos = std::find(active_systems.begin(), active_systems.end(), system);
    if (pos == active_systems.end()) {
        log_warn("Trying to destroy a system that does not exist");
        return;
    }
    system->unload(this);
    delete system;

    auto index = std::distance(active_systems.begin(), pos);
    active_systems.erase(pos);
    active_system_types.erase(active_system_types.begin() + index);
}

void World::destroy_systems() {
    for (auto *system : active_systems) {
        system->unload(this);
        delete system;
    }
    active_systems.clear();
    active_system_types.clear();
}

void World::apply_diffs_to_cache(EntityCache *cache) {
    ASSERT(cache != nullptr);
    for (const auto &diff : cache->diffs) {
        switch (diff.op) {
        case EntityCache::Diff::Add:
            cache->entities.push_back(diff.entity);
            cache->lookup.insert(diff.entity);
            break;
        case EntityCache::Diff::Remove:
            {
                auto &vec = cache->entities;
                auto rem = std::find(vec.begin(), vec.end(), diff.entity);
                ASSERT(rem != vec.end());

                std::swap(*rem, vec.back());
                vec.pop_back();
                cache->lookup.erase(diff.entity);
                break;
            }
        default:
            PANIC("Invalid cache operation");
            break;
        }
    }
    cache->diffs.clear();
}

void World::invalidate_cache(EntityCache *cache, EntityCache::Diff &&diff) {
    for (const auto &d : cache->diffs) {
        if (d.entity == diff.entity && d.op == diff.op) {
            return;
        }
    }
    cache->diffs.emplace_back(std::move(diff));
}

} // two

#undef E_MSG
