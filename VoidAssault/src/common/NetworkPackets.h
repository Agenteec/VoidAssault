// common\NetworkPackets.h
#pragma once
#include "PacketSerialization.h"
#include "raylib.h"
#include <cstdint>
#include <vector>
#include <string>

namespace GamePacket {
    enum Type : uint8_t {
        JOIN = 0,
        INPUT,
        INIT,
        SNAPSHOT,
        EVENT,
        STATS,
        ACTION,
        ADMIN_CMD,
        // Master Server Packets
        MASTER_REGISTER,
        MASTER_HEARTBEAT,
        MASTER_LIST_REQ,
        MASTER_LIST_RES
    };
}

namespace ActionType {
    enum : uint8_t {
        BUILD_WALL = 1,
        BUILD_TURRET,
        BUILD_MINE,
        UPGRADE_BUILDING
    };
}

namespace AdminCmdType {
    enum : uint8_t {
        LOGIN = 0,
        GIVE_SCRAP,
        GIVE_XP,
        KILL_ALL_ENEMIES,
        SPAWN_BOSS,
        CLEAR_BUILDINGS,
        RESET_SERVER
    };
}

enum class EntityType : uint8_t {
    PLAYER,
    BULLET,
    ENEMY,
    ARTIFACT,
    WALL,
    TURRET,
    MINE
};

namespace EnemyType {
    enum : uint8_t {
        BASIC = 0,
        FAST,
        TANK,
        BOSS,
        HOMING,
        LASER
    };
}

namespace ArtifactType {
    enum : uint8_t {
        DAMAGE = 0,
        SPEED,
        HEALTH,
        RELOAD,
        EMPTY = 255
    };
}


struct LobbyInfo {
    uint32_t id;
    std::string name;
    std::string ip;
    uint16_t port;
    uint8_t currentPlayers;
    uint8_t maxPlayers;
    uint8_t wave;

    template <typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.text1b(name, 32);
        s.text1b(ip, 64);
        s.value2b(port);
        s.value1b(currentPlayers);
        s.value1b(maxPlayers);
        s.value1b(wave);
    }
};

struct MasterRegisterPacket {
    uint16_t gamePort;
    std::string serverName;
    uint8_t maxPlayers;

    template <typename S>
    void serialize(S& s) {
        s.value2b(gamePort);
        s.text1b(serverName, 32);
        s.value1b(maxPlayers);
    }
};

struct MasterHeartbeatPacket {
    uint8_t currentPlayers;
    uint8_t wave;

    template <typename S>
    void serialize(S& s) {
        s.value1b(currentPlayers);
        s.value1b(wave);
    }
};

struct MasterListResPacket {
    std::vector<LobbyInfo> lobbies;

    template <typename S>
    void serialize(S& s) {
        s.container(lobbies, 100);
    }
};


struct JoinPacket {
    std::string name;

    template <typename S>
    void serialize(S& s) {
        s.text1b(name, 32);
    }
};

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
    uint8_t type;
    Vector2 target;
    template <typename S>
    void serialize(S& s) {
        s.value1b(type);
        s.object(target);
    }
};

struct AdminCommandPacket {
    uint8_t cmdType;
    uint32_t value;
    template <typename S>
    void serialize(S& s) {
        s.value1b(cmdType);
        s.value4b(value);
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
    std::vector<uint8_t> inventory;
    bool isAdmin;

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
        s.boolValue(isAdmin);
    }
};

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
    uint32_t id = 0;
    Vector2 position = { 0,0 };
    float rotation = 0;
    float health = 100.0f;
    float maxHealth = 100.0f;
    EntityType type = EntityType::PLAYER;
    uint8_t subtype = 0;
    float radius = 10.0f;
    Color color = WHITE;
    uint32_t level = 1;
    uint32_t kills = 0;
    std::string name = "";
    uint32_t ownerId = 0;
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
        s.text1b(name, 16);
        s.value4b(ownerId);
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