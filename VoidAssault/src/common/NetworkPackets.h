#pragma once
#if defined(_WIN32)
#include "fix_win32_compatibility.h"
#endif
#include <cstdint>
#include "raylib.h"

enum class PacketType : uint8_t {
    client_CONNECT,
    client_INPUT,
    server_INIT,
    server_SNAPSHOT,
    server_EVENT
};

struct PacketHeader {
    PacketType type;
};

struct PlayerInputPacket : PacketHeader {
    Vector2 movement;
    Vector2 aimTarget;
    bool isShooting;
};

enum class EntityType : uint8_t { PLAYER, BULLET, ENEMY };

struct EntityState {
    uint32_t id;
    Vector2 position;
    float rotation;
    float health;
    float maxHealth;
    EntityType type;
    float radius;
    Color color;
};

struct WorldSnapshotPacket : PacketHeader {
    uint32_t entityCount;
};


struct InitPacket : PacketHeader {
    uint32_t yourPlayerId;
};


enum class GameEvent : uint8_t {
    BULLET_HIT,
    PLAYER_DEATH
};

struct EventPacket : PacketHeader {
    GameEvent eventId;
    Vector2 position;
};