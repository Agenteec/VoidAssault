#pragma once
#include "GameObject.h"
#include "../PhysicsUtils.h"
#include "raymath.h"
#include "../../common/NetworkPackets.h"

class Enemy : public GameObject {
public:
    float speed = 100.0f;
    float damage = 10.0f;
    float xpReward = 20.0f;
    int scrapReward = 1;
    uint8_t enemyType = EnemyType::BASIC;

    Enemy(uint32_t id, Vector2 pos, uint8_t type, cpSpace* space) : GameObject(id, EntityType::ENEMY) {
        spaceRef = space;
        enemyType = type;
        cpFloat radius = 20.0;
        cpFloat mass = 5.0;

        switch (type) {
        case EnemyType::BASIC:
            speed = 100.0f; maxHealth = 30.0f; damage = 10.0f; xpReward = 20.0f; scrapReward = 1; radius = 20.0f; break;
        case EnemyType::FAST:
            speed = 180.0f; maxHealth = 15.0f; damage = 15.0f; xpReward = 30.0f; scrapReward = 2; radius = 15.0f; mass = 2.0f; break;
        case EnemyType::TANK:
            speed = 60.0f; maxHealth = 100.0f; damage = 20.0f; xpReward = 60.0f; scrapReward = 5; radius = 30.0f; mass = 20.0f; break;
        case EnemyType::BOSS:
            speed = 40.0f; maxHealth = 1000.0f; damage = 40.0f; xpReward = 500.0f; scrapReward = 50; radius = 60.0f; mass = 200.0f; break;
        }

        health = maxHealth;

        cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);
        body = cpSpaceAddBody(space, cpBodyNew(mass, moment));
        cpBodySetPosition(body, ToCp(pos));
        shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
        cpShapeSetElasticity(shape, 0.2);
        cpShapeSetFriction(shape, 0.9);
        cpShapeSetCollisionType(shape, COLLISION_PLAYER); // Игрок и враг имеют один тип коллизии, чтобы толкаться
        cpShapeSetUserData(shape, (void*)this);
    }

    void Update(float dt) override {
        // Регенерация (медленная)
        if (health < maxHealth) {
            health += maxHealth * 0.005f * dt; // 0.5% в сек
        }

        cpVect vel = cpBodyGetVelocity(body);
        float currentSpeed = cpvlength(vel);
        if (currentSpeed > speed) {
            cpBodySetVelocity(body, cpvmult(cpvnormalize(vel), speed));
        }
    }

    void MoveTowards(Vector2 targetPos) {
        if (!body) return;
        Vector2 pos = ToRay(cpBodyGetPosition(body));
        Vector2 dir = Vector2Subtract(targetPos, pos);
        float angle = atan2(dir.y, dir.x);
        cpBodySetAngle(body, angle);
        rotation = angle * RAD2DEG;
        if (Vector2Length(dir) < 5.0f) { cpBodySetVelocity(body, cpvzero); return; }

        Vector2 moveDir = Vector2Normalize(dir);
        cpVect newVel = cpv(moveDir.x * speed, moveDir.y * speed);
        cpVect currentVel = cpBodyGetVelocity(body);
        cpBodySetVelocity(body, cpvlerp(currentVel, newVel, 0.15f));
    }
};