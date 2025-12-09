#pragma once
#include "GameObject.h"
#include "../PhysicsUtils.h"
#include "raymath.h"

class Player : public GameObject {
public:
    float speed = 200.0f; // Сила/Скорость движения
    float shootCooldown = 0.0f;
    const float FIRE_RATE = 0.2f;

    bool wantsToShoot = false;
    Vector2 aimTarget = { 0,0 };
    bool spawnBulletSignal = false;
    Vector2 bulletDir = { 0,0 };

    Player(uint32_t id, Vector2 startPos, cpSpace* space) : GameObject(id, EntityType::PLAYER) {
        spaceRef = space;
        color = BLUE;

        // 1. Создаем тело (масса 10, инерция для круга)
        cpFloat radius = 20.0;
        cpFloat mass = 10.0;
        cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);

        body = cpSpaceAddBody(space, cpBodyNew(mass, moment));
        cpBodySetPosition(body, ToCp(startPos));

        // Отключаем вращение от коллизий (бесконечная инерция)
        cpBodySetMoment(body, INFINITY);

        // 2. Создаем форму (коллизию)
        shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
        cpShapeSetFriction(shape, 0.7);
        cpShapeSetElasticity(shape, 0.1); // Упругость
        cpShapeSetCollisionType(shape, COLLISION_PLAYER);

        // Pointer на самого себя для логики коллизий
        cpShapeSetUserData(shape, (void*)this);
    }

    void Update(float dt) override {
        if (shootCooldown > 0) shootCooldown -= dt;

        // Позиция для стрельбы и камеры
        Vector2 pos = ToRay(cpBodyGetPosition(body));

        if (wantsToShoot && shootCooldown <= 0) {
            shootCooldown = FIRE_RATE;
            spawnBulletSignal = true;
            bulletDir = Vector2Subtract(aimTarget, pos);
        }

        // Вычисляем угол поворота (визуально)
        Vector2 diff = Vector2Subtract(aimTarget, pos);
        rotation = atan2(diff.y, diff.x) * RAD2DEG;

        // Передаем угол в физику (хоть она и не вращается от ударов)
        cpBodySetAngle(body, atan2(diff.y, diff.x));

        // Линейное затухание (трение о воздух/землю)
        // В Chipmunk это делается через damping space, но можно и вручную:
        cpVect vel = cpBodyGetVelocity(body);
        cpBodySetVelocity(body, cpvmult(vel, 0.90f)); // Торможение
    }

    void ApplyInput(Vector2 move) {
        if (body) {
            // Прямая установка скорости для четкого управления
            // (или можно использовать forces для "ледяного" управления)
            cpBodySetVelocity(body, cpv(move.x * speed, move.y * speed));
        }
    }
};