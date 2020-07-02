#include <string>
#include <vector>
#include <unordered_map>

#include "two.h"
#include "mathf.h"
#include "sprite.h"
#include "entity.h"
#include "filesystem.h"
#include "text.h"

namespace two {

namespace examples {

enum Type {
    Entity_Ground,
    Entity_Wall,
    Entity_Crate,
    Entity_Player,
    Enitty_Target,
};

struct Tag {
    Type type;
    int collision_layer;
};

static const std::unordered_map<char, Type> token_to_type{
    {'.', Entity_Ground},
    {'#', Entity_Wall},
    {'*', Entity_Crate},
    {'P', Entity_Player},
    {'O', Enitty_Target}
};

struct Player {};

struct Target {};

struct Animation {
    static constexpr float MaxLerpTime = 0.12f;
    Vector2 target;
    float lerp_time;
};

struct Move {
    Vector2 direction;
};

struct Room {
    int width, height;
    int layers;
    std::vector<Entity> entities;
    bool win = false;

    Room() = default;
    Room(int width, int height)
        : width{width}
        , height{height}
        , layers{2} { entities.resize(width * height * layers); }

    Entity at(const Vector2 &position, int layer) const;
    Entity top(const Vector2 &position) const;
    void set(const Vector2 &position, int layer, Entity e);
};

Entity Room::at(const Vector2 &position, int layer) const {
    int x = int(position.x);
    int y = int(position.y);
    return entities[layer + layers * (x + y * width)];
}

Entity Room::top(const Vector2 &position) const {
    auto entity = at(position, 1);
    if (entity != NullEntity) {
        return entity;
    }
    return at(position, 0);
}

void Room::set(const Vector2 &position, int layer, Entity e) {
    int x = int(position.x);
    int y = int(position.y);
    entities[layer + layers * (x + y * width)] = e;
}

struct WinEvent {};

class Animator : public System {
public:
    // This state can be in a component in an entity instead,
    // this is to show later how to find a system in the world.
    bool active = false;
    void update(World &world, float dt) override;
};

void Animator::update(World &world, float dt) {
    for (auto e : world.view<Animation, Transform>()) {
        auto &transform = world.unpack<Transform>(e);
        auto &animation = world.unpack<Animation>(e);

        if (animation.lerp_time >= animation.MaxLerpTime) {
            animation.lerp_time = animation.MaxLerpTime;
            transform.position = animation.target;
            world.remove_component<Animation>(e);

            if (world.unpack_one<Room>().win) {
                // This is probably not the best place to do this...
                emit(WinEvent{});
            }
            active = false;
            continue;
        }
        active = true;
        animation.lerp_time += dt;
        float t = animation.lerp_time / animation.MaxLerpTime;
        t = t*t * (3.0f - 2.0f*t);
        transform.position = lerp(transform.position, animation.target, t);
    }
}

class Collision : public System {
public:
    void update(World &world, float dt) override;

    bool move(World &world,
              const Vector2 &position,
              const Vector2 &direction) const;
};

void Collision::update(World &world, float) {
    for (auto e : world.view<Move, Tag, Transform, Sprite>()) {
        auto position = world.unpack<Transform>(e).position;
        auto direction = world.unpack<Move>(e).direction;
        auto &sprite = world.unpack<Sprite>(e);
        world.remove_component<Move>(e);
        move(world, position, direction);

        if (direction.x < 0 && sprite.flip == Sprite::FlipNone) {
            sprite.flip = Sprite::FlipY;
            continue;
        }
        if (direction.x > 0 && sprite.flip == Sprite::FlipY) {
            sprite.flip = Sprite::FlipNone;
        }
    }

    // Check for the win condition
    auto &room = world.unpack_one<Room>();
    room.win = true;
    for (auto e : world.view<Transform, Target>()) {
        auto position = world.unpack<Transform>(e).position;
        auto crate = room.at(position, 1);
        if (crate == NullEntity) {
            // Nothing on the target
            room.win = false;
            break;
        }
        // Something is on the target, make sure it's a crate
        auto &tag = world.unpack<Tag>(crate);
        if (tag.type != Entity_Crate) {
            room.win = false;
            break;
        }
    }
}

bool Collision::move(World &world,
                     const Vector2 &position,
                     const Vector2 &direction) const
{
    auto target = position + direction;
    auto &room = world.unpack_one<Room>();
    auto entity = room.top(position);
    auto neighbor = room.top(target);

    auto entity_tag = world.unpack<Tag>(entity);
    auto neighbor_tag = world.unpack<Tag>(neighbor);

    if (entity_tag.collision_layer == 0) {
        // Background
        return false;
    }

    switch (neighbor_tag.type) {
    case Entity_Crate:
        // push crate
        if (!move(world, target, direction)) {
            return false;
        }
        break;
    case Entity_Ground:
    case Enitty_Target:
        break;
    default:
        return false;
    }

    room.set(position, 1, NullEntity);
    room.set(target, 1, entity);
    world.pack(entity, Animation{target, 0.0f});
    return true;
}

class Sokoban : public World {
public:
    void load_room(const std::string &level, int width, int height);
    void load_sprites(const std::string &filename);
    void load() override;
    void update(float dt) override;
    bool key_down(const KeyDown &e);
    bool win(const WinEvent &e);

private:
    std::vector<Sprite> atlas;
};

void Sokoban::load() {
    make_system<SpriteRenderer>();
    make_system<Collision>();
    make_system<Animator>();

    auto &camera = pack(make_entity(), Camera{8, {29, 43, 83, 255}});
    camera.scale = 8;

    load_sprites("sokoban.png");
    load_room("sokoban_l0.txt", 8, 7);

    bind<KeyDown>(&Sokoban::key_down, this);
    bind<WinEvent>(&Sokoban::win, this);
}

void Sokoban::load_sprites(const std::string &filename) {
    auto *im = load_image(filename);
    ASSERT(im != nullptr);
    atlas = load_atlas(im, 8, 8);
    delete im;
}

void Sokoban::load_room(const std::string &level, int width, int height) {
    File file(level);
    bool ok = file.open(FileMode::Read);
    ASSERT(ok);

    auto *data = file.read_all();
    auto len = file.size();
    ASSERT(data != nullptr);

    auto &room = pack(make_entity(), Room{width, height});
    auto &cam = unpack_one<Camera>();
    cam.position = Vector2{width / 2.0f - 0.5f, height / 2.0f - 0.5f};

    auto throwe = make_entity();
    pack(throwe, Move{});
    pack(throwe, Animation{});
    destroy_entity(throwe);

    std::string lvl;
    for (int i = 0; i < len; ++i) {
        if (data[i] != '\n' && data[i] != '\r')
            lvl.push_back(data[i]);
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Vector2 position(x, y);
            auto type = token_to_type.at(lvl[x + y * width]);
            int layer = 0;
            if (type == Entity_Player || type == Entity_Crate) {
                // Add a floor tile under player and crates
                auto bg = make_entity();
                pack(bg, Transform{Vector2(x, y)});
                pack(bg, Tag{Entity_Ground, 0});
                pack(bg, atlas[Entity_Ground]);
                room.set(position, 0, bg);
                layer = 1;
            }
            auto e = make_entity();
            pack(e, Transform{position});
            pack(e, Tag{type, layer});
            auto &sprite = pack(e, atlas[type]);
            sprite.layer = layer;
            room.set(position, layer, e);

            if (type == Entity_Player) {
                pack(e, Player{});
            } else if (type == Enitty_Target) {
                pack(e, Target{});
            }
        }
    }
}

bool Sokoban::key_down(const KeyDown &e) {
    if (get_system<Animator>()->active) {
        return false;
    }
    auto player = view_one<Player>().value();
    switch (e.key) {
    case SDLK_UP:
        pack(player, Move{{0.0f, -1.0f}});
        break;
    case SDLK_RIGHT:
        pack(player, Move{{1.0f, 0.0f}});
        break;
    case SDLK_DOWN:
        pack(player, Move{{0.0f, 1.0f}});
        break;
    case SDLK_LEFT:
        pack(player, Move{{-1.0f, 0.0f}});
        break;
    default:
        return false;
    }
    return true;
}

bool Sokoban::win(const WinEvent &) {
    load_world<Sokoban>();
    return true;
}

void Sokoban::update(float dt) {
    for (auto *system : systems()) {
        system->update(*this, dt);
    }
}

class TitleScreen : public World {
public:
    void load();
    bool key_down(const KeyDown &e);
};

void TitleScreen::load() {
    auto font_renderer = make_system<FontRenderer>();
    auto &camera = pack(make_entity(), Camera{1, Color::Black});
    auto font = load_font("heartbit.fnt");

    auto title = make_entity();
    auto &tf = pack(title, PixelTransform{{0, 0}, {3, 3}});
    auto &text = pack(title, Text{font, "Sokoban!\npress a key to start"});

    std::vector<bool> wrap_info;
    font_renderer->wrap_text(text, tf.scale, wrap_info);
    Vector2i size = font_renderer->text_size(text, tf.scale, wrap_info);
    tf.position.x = 640 - size.x / 2;
    tf.position.y = 360 - size.y / 2;

    bind<KeyDown>(&TitleScreen::key_down, this);
}

bool TitleScreen::key_down(const KeyDown &e) {
    load_world<Sokoban>();
    return true;
}

} // examples
} // two

int main(int argc, char *argv[]) {
    two::init(argc, argv);
    two::mount("assets.dat");
    two::create_window("sokoban", 1280, 720);
    two::set_logical_size(1280, 720);
    two::load_world<two::examples::TitleScreen>();
    return two::run();
}
