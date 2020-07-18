#include <string>

#include "entity.h"
#include "debug.h"

namespace two {
namespace test {

// Some test components

struct float2 {
    float x, y;
};

struct Transform {
    float2 pos;
    float2 scale;
    float rot;
};

struct C1 {
    int x, y, z;
};

struct C2 {};

template <typename... Args>
void pview(World &world, bool include_inactive = false) {
    std::string s = "\tentities: [";
    for (auto entity : world.view<Args...>(include_inactive)) {
        s += std::to_string(entity);
        s += ", ";
    }
    s.pop_back();
    s.pop_back();
    s.push_back(']');
    log("%s\n\n", s.c_str());
}

class Printer : public System {
public:
    void load(World &) override {
        log( "%s", "warming up...");
    }

    void update(World &, float) override {
        log("%s", "brrrr...");
    }

    void unload(World &) override {
        log("%s", "done.");
    }
};

class MWorld : public World {

};

void run_entity_test() {
    MWorld world;
    auto e0 = world.make_entity();
    auto e1 = world.make_entity();
    auto e2 = world.make_entity();
    auto e3 = world.make_entity();
    pview(world);

    world.pack(e1, Transform{});
    world.pack(e1, C1{100, 200, 300});
    world.pack(e1, C2{});
    pview<Transform>(world);

    world.pack(e2, Transform{});
    pview<Transform>(world);
    pview<Transform, C1, C2>(world);

    world.pack(e2, C1{400, 500, 600});
    world.pack(e1, C1{10, 20, 30});
    pview<Transform, C1>(world);

    auto &c1 = world.unpack<C1>(e1);
    log("%d, %d, %d", c1.x, c1.y, c1.z);

    auto c12 = world.unpack<C1>(e2);
    log("%d, %d, %d", c12.x, c12.y, c12.z);

    world.remove_component<C1>(e1);
    c12.x = 10000;
    world.pack(e2, c12);
    c12 = world.unpack<C1>(e2);
    log("%d, %d, %d", c12.x, c12.y, c12.z);
    pview<Transform, C2>(world);

    world.destroy_entity(e2);
    pview<Transform, C1>(world);

    e2 = world.make_entity();
    world.pack(e2, Transform{});
    world.pack(e3, Transform{});
    world.pack(e0, Transform{});

    pview(world);

    world.make_system<Printer>();
    auto *p = world.get_system<Printer>();
    std::vector<Printer *> vec;
    world.get_all_systems<Printer>(vec);

    for (auto *system : world.systems()) {
        system->update(world, 0);
        system->update(world, 0);
        system->update(world, 0);
        system->update(world, 0);
    }
    world.destroy_system(p);
    auto *pp = world.get_system<Printer>();
    ASSERT(pp == nullptr);
}

} // test
} // two
