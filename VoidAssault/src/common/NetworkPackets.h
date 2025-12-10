#pragma once
#if defined(_WIN32)
#include "fix_win32_compatibility.h"
#endif
#include <cstdint>
#include "raylib.h"

enum class PacketType : uint8_t {
    client_CONNECT,
    client_INPUT,
    client_PING,
    client_DISCOVERY_REQ,

    server_INIT,
    server_SNAPSHOT,
    server_EVENT,
    server_PONG, // Добавлено для PING/PONG

    master_REGISTER,
    master_HEARTBEAT,
    master_LIST_REQ,
    master_LIST_RES
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

struct PingPacket : PacketHeader {
    double timestamp;
};


struct LobbyInfo {
    uint32_t id;
    char name[32];
    char ip[16];
    uint16_t port;
    uint8_t players;
    uint8_t maxPlayers;
    char mapName[32];
};

struct MasterRegisterPacket : PacketHeader {
    uint16_t gamePort;
    char serverName[32];
    char mapName[32];
    uint8_t maxPlayers;
};

struct MasterHeartbeatPacket : PacketHeader {
    uint8_t currentPlayers;
};


struct MasterListRequestPacket : PacketHeader {

};


struct MasterListResponsePacket : PacketHeader {
    uint16_t count;

};