#pragma once
#include "GameObject.h"
#include "../PhysicsUtils.h"
#include "raymath.h"

class Bullet : public GameObject {
public:
    uint32_t ownerId;
    float lifeTime = 2.0f;

    Bullet(uint32_t id, Vector2 pos, Vector2 dir, uint32_t owner, cpSpace* space)
        : GameObject(id, EntityType::BULLET), ownerId(owner)
    {
        spaceRef = space;
        color = BLACK;
        float moveSpeed = 600.0f;

        cpFloat radius = 5.0;
        cpFloat mass = 1.0;
        // Пуля - кинематический или легкий динамический объект
        cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);

        body = cpSpaceAddBody(space, cpBodyNew(mass, moment));
        cpBodySetPosition(body, ToCp(pos));

        shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
        cpShapeSetElasticity(shape, 0.8);
        cpShapeSetCollisionType(shape, COLLISION_BULLET);
        cpShapeSetUserData(shape, (void*)this);

        // Задаем скорость
        Vector2 normDir = Vector2Normalize(dir);
        cpBodySetVelocity(body, cpv(normDir.x * moveSpeed, normDir.y * moveSpeed));
    }

    void Update(float dt) override {
        lifeTime -= dt;
        if (lifeTime <= 0) destroyFlag = true;
    }
};