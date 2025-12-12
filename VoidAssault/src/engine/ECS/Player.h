#pragma once
#include "GameObject.h"
#include "../PhysicsUtils.h"
#include "raymath.h"

class Player : public GameObject {
public:
    float speed = 200.0f;
    float shootCooldown = 0.0f;
    const float FIRE_RATE = 0.2f;

    bool wantsToShoot = false;
    Vector2 aimTarget = { 0,0 };
    bool spawnBulletSignal = false;
    Vector2 bulletDir = { 0,0 };

    Player(uint32_t id, Vector2 startPos, cpSpace* space) : GameObject(id, EntityType::PLAYER) {
        spaceRef = space;
        color = BLUE;

        cpFloat radius = 20.0;
        cpFloat mass = 10.0;
        cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);

        body = cpSpaceAddBody(space, cpBodyNew(mass, moment));
        cpBodySetPosition(body, ToCp(startPos));

        cpBodySetMoment(body, INFINITY);
        shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
        cpShapeSetFriction(shape, 0.7);
        cpShapeSetElasticity(shape, 0.1);
        cpShapeSetCollisionType(shape, COLLISION_PLAYER);

        cpShapeSetUserData(shape, (void*)this);
    }

    void Update(float dt) override {
        if (shootCooldown > 0) shootCooldown -= dt;

        
        Vector2 pos = ToRay(cpBodyGetPosition(body));

        if (wantsToShoot && shootCooldown <= 0) {
            shootCooldown = FIRE_RATE;
            spawnBulletSignal = true;
            bulletDir = Vector2Subtract(aimTarget, pos);
        }

        Vector2 diff = Vector2Subtract(aimTarget, pos);
        rotation = atan2(diff.y, diff.x) * RAD2DEG;

        cpBodySetAngle(body, atan2(diff.y, diff.x));

        cpVect vel = cpBodyGetVelocity(body);
        cpBodySetVelocity(body, cpvmult(vel, 0.90f));
    }

    void ApplyInput(Vector2 move) {
        if (!body) return;


        if (!std::isfinite(move.x) || !std::isfinite(move.y)) {
            move = { 0, 0 };
        }


        float length = Vector2Length(move);
        if (length > 1.0f) {
            move = Vector2Normalize(move);

            if (!std::isfinite(move.x)) move = { 0, 0 };
        }

        float targetX = move.x * speed;
        float targetY = move.y * speed;

        if (!std::isfinite(targetX)) targetX = 0;
        if (!std::isfinite(targetY)) targetY = 0;

        cpBodySetVelocity(body, cpv(targetX, targetY));

        cpBodySetAngularVelocity(body, 0.0f);
    }
};