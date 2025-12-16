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
        cpFloat mass = 20.0;

        switch (type) {
        case EnemyType::BASIC:             
            speed = 100.0f; 
            maxHealth = 30.0f;
            damage = 50.0f;             
            xpReward = 20.0f; 
            scrapReward = 1;
            radius = 20.0f;
            mass = 20.0f; 
            break;
        case EnemyType::FAST:            
            speed = 190.0f; 
            maxHealth = 20.0f;
            damage = 40.0f;             
            xpReward = 30.0f; 
            scrapReward = 2; 
            radius = 15.0f; 
            mass = 10.0f;
            break;
        case EnemyType::TANK:   
            speed = 60.0f; 
            maxHealth = 250.0f;             
            damage = 100.0f;           
            xpReward = 100.0f; 
            scrapReward = 10; 
            radius = 35.0f; 
            mass = 100.0f;
            break;
        case EnemyType::BOSS:
            speed = 45.0f;
            maxHealth = 5000.0f;
            damage = 300.0f;        
            xpReward = 1000.0f;
            scrapReward = 100;
            radius = 70.0f;
            mass = 1000.0f;
            break;
        }
        health = maxHealth;

        cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);
        body = cpSpaceAddBody(space, cpBodyNew(mass, moment));
        cpBodySetPosition(body, ToCp(pos));
        shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));

        cpShapeSetElasticity(shape, 0.0f);
        cpShapeSetFriction(shape, 1.0f);
        cpShapeSetCollisionType(shape, COLLISION_PLAYER);
        cpShapeSetUserData(shape, (void*)this);
    }

    void Update(float dt) override {
        if (health < maxHealth) health += maxHealth * 0.005f * dt;

        cpVect currentVel = cpBodyGetVelocity(body);
        cpBodySetVelocity(body, cpvmult(currentVel, 0.90f));

        float currentSpeed = cpvlength(currentVel);
        if (currentSpeed > speed * 3.0f) {
            cpBodySetVelocity(body, cpvmult(cpvnormalize(currentVel), speed * 3.0f));
        }
    }

    void MoveTowards(Vector2 targetPos) {
        if (!body) return;
        Vector2 pos = ToRay(cpBodyGetPosition(body));
        Vector2 dir = Vector2Subtract(targetPos, pos);
        float angle = atan2(dir.y, dir.x);
        cpBodySetAngle(body, angle);
        rotation = angle * RAD2DEG;

        if (Vector2Length(dir) < 5.0f) return;
        Vector2 moveDir = Vector2Normalize(dir);
        cpVect desiredVel = cpv(moveDir.x * speed, moveDir.y * speed);
        cpVect currentVel = cpBodyGetVelocity(body);
        cpBodySetVelocity(body, cpvlerp(currentVel, desiredVel, 0.1f));
    }
};