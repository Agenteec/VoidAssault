// common/NetworkPackets.h
#pragma once
#include "PacketSerialization.h"
#include "raylib.h"
#include <cstdint>
#include <vector>
namespace GamePacket {
    enum Type : uint8_t {
        INPUT = 1,
        INIT,
        SNAPSHOT,
        EVENT,
        STATS,
        ACTION
    };
}

namespace ActionType {
    enum : uint8_t {
        BUILD_WALL = 1,
        BUILD_TURRET,
        BUILD_MINE,
        UPGRADE_BUILDING // NEW: Улучшение постройки
    };
}

// Типы сущностей
enum class EntityType : uint8_t { PLAYER, BULLET, ENEMY, ARTIFACT, WALL, TURRET, MINE };

namespace EnemyType {
    enum : uint8_t { BASIC = 0, FAST, TANK, BOSS };
}

namespace ArtifactType {
    enum : uint8_t { DAMAGE = 0, SPEED, HEALTH, RELOAD, EMPTY = 255 };
}

struct EventPacket {
    uint8_t type;
    Vector2 pos;
    Color color;

    template <typename S>
    void serialize(S& s) {
        s.value1b(type);
        s.object(pos);
        s.object(color);
    }
};

struct ActionPacket {
    uint8_t type;       // ActionType
    Vector2 target;     // Координаты клика (для стройки или улучшения)

    template <typename S>
    void serialize(S& s) {
        s.value1b(type);
        s.object(target);
    }
};

struct PlayerStatsPacket {
    uint32_t level;
    float currentXp;
    float maxXp;
    float maxHealth;
    float damage;
    float speed;
    uint32_t scrap;
    uint32_t kills;

    // Инвентарь
    std::vector<uint8_t> inventory;

    template <typename S>
    void serialize(S& s) {
        s.value4b(level);
        s.value4b(currentXp);
        s.value4b(maxXp);
        s.value4b(maxHealth);
        s.value4b(damage);
        s.value4b(speed);
        s.value4b(scrap);
        s.value4b(kills);
        s.container1b(inventory, 6);
    }
};


// NEW: Типы характеристик
namespace StatType {
    enum : uint8_t {
        HEALTH_REGEN = 0,
        MAX_HEALTH,
        BODY_DAMAGE,
        BULLET_SPEED,
        BULLET_PEN,
        BULLET_DAMAGE,
        RELOAD,
        MOVE_SPEED,
        COUNT // 8 stats
    };
}


struct PlayerInputPacket {
    Vector2 movement;
    Vector2 aimTarget;
    bool isShooting;

    template <typename S>
    void serialize(S& s) {
        s.object(movement);
        s.object(aimTarget);
        s.boolValue(isShooting);
    }
};

struct EntityState {
    uint32_t id;
    Vector2 position;
    float rotation;
    float health;
    float maxHealth;
    EntityType type;
    uint8_t subtype;
    float radius;
    Color color;
    uint32_t level;
    uint32_t kills;

    template <typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.object(position);
        s.value4b(rotation);
        s.value4b(health);
        s.value4b(maxHealth);
        uint8_t typeInt = static_cast<uint8_t>(type);
        s.value1b(typeInt);
        type = static_cast<EntityType>(typeInt);
        s.value1b(subtype);
        s.value4b(radius);
        s.object(color);
        s.value4b(level);
        s.value4b(kills);
    }
};

struct WorldSnapshotPacket {
    double serverTime;
    uint32_t wave;
    std::vector<EntityState> entities;

    template <typename S>
    void serialize(S& s) {
        s.value8b(serverTime);
        s.value4b(wave);
        s.container(entities, 10000);
    }
};

struct InitPacket {
    uint32_t playerId;

    template <typename S>
    void serialize(S& s) {
        s.value4b(playerId);
    }
};