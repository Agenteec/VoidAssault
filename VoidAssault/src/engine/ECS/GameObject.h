#pragma once
#if defined(_WIN32)
#include "fix_win32_compatibility.h"
#endif
#include "raylib.h"
#include "chipmunk/chipmunk.h"
#include "../../common/NetworkPackets.h"

// Определяем типы коллизий для фильтрации (понадобится в будущем)
enum CollisionType {
    COLLISION_PLAYER = 1,
    COLLISION_BULLET = 2,
    COLLISION_WALL = 3
};

class GameObject {
public:
    uint32_t id;
    EntityType type;

    // Ссылки на физические объекты
    cpBody* body = nullptr;
    cpShape* shape = nullptr;
    cpSpace* spaceRef = nullptr; // Ссылка на мир для очистки

    bool destroyFlag = false;
    Color color = RED;
    float rotation = 0.0f;
    float health = 100.0f;
    float maxHealth = 100.0f;

    GameObject(uint32_t _id, EntityType _type) : id(_id), type(_type) {}

    virtual ~GameObject() {
        if (spaceRef) {
            if (shape) {
                cpSpaceRemoveShape(spaceRef, shape);
                cpShapeFree(shape);
            }
            if (body) {
                cpSpaceRemoveBody(spaceRef, body);
                cpBodyFree(body);
            }
        }
    }

    virtual void Update(float dt) = 0;
};