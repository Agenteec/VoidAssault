#pragma once
#include "GameObject.h"
#include "../PhysicsUtils.h"

class Construct : public GameObject {
public:
    float lifetime = -1.0f;
    uint32_t level = 1;
    uint32_t ownerId;

    Construct(uint32_t id, EntityType type, Vector2 pos, uint32_t owner, cpSpace* space)
        : GameObject(id, type), ownerId(owner) {
        spaceRef = space;
    }

    virtual void Upgrade() {
        level++;
        maxHealth *= 1.4f;         health = maxHealth;     }

    virtual void Update(float dt) override {
        if (lifetime > 0) {
            lifetime -= dt;
            if (lifetime <= 0) destroyFlag = true;
        }
    }
};

class Wall : public Construct {
public:
    Wall(uint32_t id, Vector2 pos, uint32_t owner, cpSpace* space)
        : Construct(id, EntityType::WALL, pos, owner, space) {
        maxHealth = 400.0f;
        health = maxHealth;
        color = GRAY; 
        body = cpSpaceAddBody(space, cpBodyNewStatic());
        cpBodySetPosition(body, ToCp(pos));

        shape = cpSpaceAddShape(space, cpBoxShapeNew(body, 50, 50, 0));
        cpShapeSetElasticity(shape, 0.5f);
        cpShapeSetFriction(shape, 0.5f);
        cpShapeSetCollisionType(shape, COLLISION_WALL);
        cpShapeSetUserData(shape, (void*)this);
    }

        };
class Turret : public Construct {
public:
    float range = 400.0f;
    float reloadTime = 1.0f;     float cooldown = 0.0f;
    float damage = 15.0f;    
    Turret(uint32_t id, Vector2 pos, uint32_t owner, cpSpace* space)
        : Construct(id, EntityType::TURRET, pos, owner, space) {
        maxHealth = 200.0f;         health = maxHealth;
        color = PURPLE;

        body = cpSpaceAddBody(space, cpBodyNewStatic());
        cpBodySetPosition(body, ToCp(pos));

        shape = cpSpaceAddShape(space, cpCircleShapeNew(body, 25.0f, cpvzero));         cpShapeSetCollisionType(shape, COLLISION_WALL);
        cpShapeSetUserData(shape, (void*)this);
    }

    void Upgrade() override {
        Construct::Upgrade();         damage *= 1.3f;               reloadTime *= 0.85f;          range += 20.0f;
    }

    void Update(float dt) override {
        Construct::Update(dt);
        if (cooldown > 0) cooldown -= dt;
    }
};
class Mine : public Construct {
public:
    float damage = 200.0f;
    float triggerRadius = 35.0f;
    float splashRadius = 100.0f;

    Mine(uint32_t id, Vector2 pos, uint32_t owner, cpSpace* space)
        : Construct(id, EntityType::MINE, pos, owner, space) {
        maxHealth = 50.0f;
        health = maxHealth;
        color = ORANGE;

        body = cpSpaceAddBody(space, cpBodyNewStatic());
        cpBodySetPosition(body, ToCp(pos));

                shape = cpSpaceAddShape(space, cpCircleShapeNew(body, 15.0f, cpvzero));
        cpShapeSetSensor(shape, true);
        cpShapeSetCollisionType(shape, COLLISION_BULLET);
        cpShapeSetUserData(shape, (void*)this);
    }

    void Upgrade() override {
        Construct::Upgrade();
        damage += 100.0f;              splashRadius += 10.0f;     }
};