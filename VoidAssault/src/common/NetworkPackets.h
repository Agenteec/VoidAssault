#pragma once
#include "raylib.h"
#include <cstdint>
#include <vector>

#include "serial/StreamBuffer.h" 

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
};


inline StreamBuffer::Shared& operator<<(StreamBuffer::Shared& stream, const Vector2& v) {
    stream->write(v.x);
    stream->write(v.y);
    return stream;
}

inline StreamBuffer::Shared& operator>>(StreamBuffer::Shared& stream, Vector2& v) {
    stream->read(v.x);
    stream->read(v.y);
    return stream;
}

inline StreamBuffer::Shared& operator<<(StreamBuffer::Shared& stream, const Color& c) {
    stream->write(c.r);
    stream->write(c.g);
    stream->write(c.b);
    stream->write(c.a);
    return stream;
}

inline StreamBuffer::Shared& operator>>(StreamBuffer::Shared& stream, Color& c) {
    stream->read(c.r);
    stream->read(c.g);
    stream->read(c.b);
    stream->read(c.a);
    return stream;
}


inline StreamBuffer::Shared& operator<<(StreamBuffer::Shared& stream, const PlayerInputPacket& p) {
    stream->write(p.movement);
    stream->write(p.aimTarget);
    stream->write(p.isShooting);
    return stream;
}

inline StreamBuffer::Shared& operator>>(StreamBuffer::Shared& stream, PlayerInputPacket& p) {
    stream->read(p.movement);
    stream->read(p.aimTarget);
    stream->read(p.isShooting);
    return stream;
}

inline StreamBuffer::Shared& operator<<(StreamBuffer::Shared& stream, const EntityState& e) {
    stream->write(e.id);
    stream->write(e.position);
    stream->write(e.rotation);
    stream->write(e.health);
    stream->write(e.maxHealth);
    stream->write((uint8_t)e.type);
    stream->write(e.radius);
    stream << e.color;
    return stream;
}

inline StreamBuffer::Shared& operator>>(StreamBuffer::Shared& stream, EntityState& e) {
    stream->read(e.id);
    stream->read(e.position);
    stream->read(e.rotation);
    stream->read(e.health);
    stream->read(e.maxHealth);
    uint8_t t; stream->read(t); e.type = (EntityType)t;
    stream->read(e.radius);
    stream >> e.color;
    return stream;
}