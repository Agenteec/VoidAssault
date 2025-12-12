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
        EVENT
    };
}

enum class EntityType : uint8_t { PLAYER, BULLET, ENEMY };

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
    float radius;
    Color color;

    template <typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.object(position);
        s.value4b(rotation);
        s.value4b(health);
        s.value4b(maxHealth);

        // БЕЗОПАСНАЯ СЕРИАЛИЗАЦИЯ ENUM
        uint8_t typeInt = static_cast<uint8_t>(type);
        s.value1b(typeInt);
        type = static_cast<EntityType>(typeInt);

        s.value4b(radius);
        s.object(color);
    }
};

struct WorldSnapshotPacket {
    double serverTime;
    std::vector<EntityState> entities;

    template <typename S>
    void serialize(S& s) {
        s.value8b(serverTime);
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