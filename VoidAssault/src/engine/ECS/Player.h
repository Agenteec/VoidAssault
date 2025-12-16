// engine/ECS/Player.h
#pragma once
#include "GameObject.h"
#include "../PhysicsUtils.h"
#include "raymath.h"
#include <vector>

struct ArtifactStats {
    float damageMult = 0.0f;
    float speedMult = 0.0f;
    float healthFlat = 0.0f;
    float reloadMult = 0.0f;
};

class Player : public GameObject {
public:
    const float BASE_SPEED = 220.0f;
    const float BASE_RELOAD = 0.5f;
    const float BASE_DAMAGE = 20.0f;
    const float BASE_HP = 100.0f;

    uint32_t level = 1;
    float currentXp = 0.0f;
    float maxXp = 100.0f;
    uint32_t scrap = 0;
    uint32_t kills = 0;

    uint8_t inventory[6];
    ArtifactStats artifacts;

    float shootCooldown = 0.0f;
    bool wantsToShoot = false;
    Vector2 aimTarget = { 0,0 };
    bool spawnBulletSignal = false;
    Vector2 bulletDir = { 0,0 };
    Vector2 knockback = { 0, 0 };

    float curSpeed = 0;
    float curReload = 0;
    float curDamage = 0;

    Player(uint32_t id, Vector2 startPos, cpSpace* space) : GameObject(id, EntityType::PLAYER) {
        spaceRef = space;
        color = { 0, 120, 215, 255 };
        for (int i = 0; i < 6; ++i) inventory[i] = ArtifactType::EMPTY;

        cpFloat radius = 20.0;
        cpFloat mass = 5.0;
        cpFloat moment = cpMomentForCircle(mass, 0, radius, cpvzero);

        body = cpSpaceAddBody(space, cpBodyNew(mass, moment));
        cpBodySetPosition(body, ToCp(startPos));
        cpBodySetMoment(body, INFINITY);

        shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
        cpShapeSetFriction(shape, 0.0f);
        cpShapeSetElasticity(shape, 0.0f);
        cpShapeSetCollisionType(shape, COLLISION_PLAYER);
        cpShapeSetUserData(shape, (void*)this);

        RecalculateStats();
        health = maxHealth;
    }

    void Reset() {
        level = 1;
        currentXp = 0;
        maxXp = 100.0f;
        scrap = 0;
        kills = 0;
        health = maxHealth;
        for (int i = 0; i < 6; ++i) inventory[i] = ArtifactType::EMPTY;
        RecalculateStats();
    }

    // Автоматическая прокачка
    void RecalculateStats() {
        artifacts = { 0,0,0,0 };
        for (int i = 0; i < 6; i++) {
            if (inventory[i] == ArtifactType::EMPTY) continue;
            switch (inventory[i]) {
            case ArtifactType::DAMAGE: artifacts.damageMult += 0.15f; break;
            case ArtifactType::SPEED: artifacts.speedMult += 0.10f; break;
            case ArtifactType::HEALTH: artifacts.healthFlat += 50.0f; break;
            case ArtifactType::RELOAD: artifacts.reloadMult += 0.10f; break;
            }
        }

        float lvlScale = (float)(level - 1);

        // Формулы автоматического роста
        maxHealth = (BASE_HP + artifacts.healthFlat) + (lvlScale * 20.0f); // +20 HP за уровень
        curDamage = BASE_DAMAGE + (lvlScale * 3.0f);                       // +3 урона за уровень
        curDamage *= (1.0f + artifacts.damageMult);

        curSpeed = BASE_SPEED + (lvlScale * 2.0f);                         // Немного скорости
        curSpeed *= (1.0f + artifacts.speedMult);

        curReload = BASE_RELOAD * (1.0f - (lvlScale * 0.02f) - artifacts.reloadMult); // Чуть быстрее перезарядка
        if (curReload < 0.1f) curReload = 0.1f;

        if (health > maxHealth) health = maxHealth;
    }

    bool AddItemToInventory(uint8_t type) {
        for (int i = 0; i < 6; i++) {
            if (inventory[i] == ArtifactType::EMPTY) {
                inventory[i] = type;
                RecalculateStats();
                return true;
            }
        }
        return false;
    }

    void AddXp(float amount) {
        currentXp += amount;
        while (currentXp >= maxXp) {
            currentXp -= maxXp;
            level++;
            maxXp *= 1.2f;
            RecalculateStats();
            health = maxHealth; // Полное лечение при уровне
        }
    }

    void Update(float dt) override {
        // Регенерация
        double currentTime = GetTime();
        float baseRegen = maxHealth * 0.01f; // 1%
        if (currentTime - lastDamageTime > 5.0) baseRegen *= 5.0f; // 5% вне боя

        health += baseRegen * dt;
        if (health > maxHealth) health = maxHealth;

        if (shootCooldown > 0) shootCooldown -= dt;
        knockback = Vector2Lerp(knockback, { 0,0 }, dt * 5.0f);

        Vector2 pos = ToRay(cpBodyGetPosition(body));

        if (wantsToShoot && shootCooldown <= 0) {
            shootCooldown = curReload;
            spawnBulletSignal = true;
            bulletDir = Vector2Subtract(aimTarget, pos);
            Vector2 recoilDir = Vector2Normalize(bulletDir);
            knockback = Vector2Subtract(knockback, Vector2Scale(recoilDir, 150.0f));
        }

        Vector2 diff = Vector2Subtract(aimTarget, pos);
        rotation = atan2(diff.y, diff.x) * RAD2DEG;
        cpBodySetAngle(body, atan2(diff.y, diff.x));
    }

    void ApplyInput(Vector2 move) {
        if (!body) return;
        if (!std::isfinite(move.x) || !std::isfinite(move.y)) move = { 0, 0 };
        float len = Vector2Length(move);
        if (len > 1.0f) move = Vector2Normalize(move);

        Vector2 targetVel = Vector2Scale(move, curSpeed);
        Vector2 finalVel = Vector2Add(targetVel, knockback);
        cpBodySetVelocity(body, cpv(finalVel.x, finalVel.y));
        cpBodySetAngularVelocity(body, 0.0f);
    }
};