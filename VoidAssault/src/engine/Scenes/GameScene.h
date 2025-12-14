#pragma once
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include "ECS/Player.h"
#include "ECS/Bullet.h"
#include "PhysicsUtils.h"

class GameScene {
public:
    cpSpace* space;
    std::map<uint32_t, std::shared_ptr<GameObject>> objects;
    uint32_t nextId = 1000;

    float width = 2000;
    float height = 2000;

    GameScene() {
        space = cpSpaceNew();
        cpSpaceSetGravity(space, cpv(0, 0));
        cpSpaceSetIdleSpeedThreshold(space, 0);         CreateMapBoundaries();
    }

    ~GameScene() {
        objects.clear();
        cpSpaceFree(space);
    }

    void CreateMapBoundaries() {
        cpBody* staticBody = cpSpaceGetStaticBody(space);
        float w = width;
        float h = height;
        float thickness = 2000.0f;

        auto addWallBox = [&](float x, float y, float boxW, float boxH) {
            cpVect verts[] = {
                cpv(x, y),
                cpv(x, y + boxH),
                cpv(x + boxW, y + boxH),
                cpv(x + boxW, y)
            };
            cpShape* wall = cpSpaceAddShape(space, cpPolyShapeNew(staticBody, 4, verts, cpTransformIdentity, 0.0));
            cpShapeSetElasticity(wall, 0.0f);             cpShapeSetFriction(wall, 0.0f);               cpShapeSetCollisionType(wall, COLLISION_WALL);
            };

        addWallBox(-thickness, -thickness, thickness, h + thickness * 2);         addWallBox(w, -thickness, thickness, h + thickness * 2);                  addWallBox(-thickness, -thickness, w + thickness * 2, thickness);         addWallBox(-thickness, h, w + thickness * 2, thickness);
    }

    std::shared_ptr<Player> CreatePlayerWithId(uint32_t id) {
        Vector2 startPos = { width / 2.0f, height / 2.0f };
        auto p = std::make_shared<Player>(id, startPos, space);
        objects[id] = p;
        return p;
    }

    void Update(float dt) {
        cpSpaceSetIterations(space, 10);
        cpSpaceStep(space, dt);

        EnforceMapBoundaries();

        std::vector<std::shared_ptr<Bullet>> newBullets;

        for (auto it = objects.begin(); it != objects.end();) {
            auto& obj = it->second;
            obj->Update(dt);

            if (obj->type == EntityType::PLAYER) {
                auto p = std::dynamic_pointer_cast<Player>(obj);
                if (p && p->spawnBulletSignal) {
                    p->spawnBulletSignal = false;

                    Vector2 playerPos = ToRay(cpBodyGetPosition(p->body));
                    Vector2 dir = Vector2Normalize(p->bulletDir);

                    float playerRadius = 20.0f + (p->level - 1) * 2.0f;

                    float barrelLength = playerRadius * 1.8f;
                    float spawnOffset = barrelLength + 5.0f;

                    Vector2 spawnPos = Vector2Add(playerPos, Vector2Scale(dir, spawnOffset));

                    auto b = std::make_shared<Bullet>(nextId++, spawnPos, dir, p->id, space);

                    cpVect tankVel = cpv(p->currentVelocity.x * 0.4f, p->currentVelocity.y * 0.4f);
                    cpBodySetVelocity(b->body, cpvadd(cpBodyGetVelocity(b->body), tankVel));

                    newBullets.push_back(b);
                }
            }

            if (obj->destroyFlag) {
                it = objects.erase(it);
            }
            else {
                ++it;
            }
        }

        for (auto& b : newBullets) objects[b->id] = b;

        HandleCollisionsAndDamage();
    }

    void EnforceMapBoundaries() {
        for (auto& [id, obj] : objects) {
            if (obj->type == EntityType::BULLET) continue;
            if (!obj->body) continue;

            cpVect pos = cpBodyGetPosition(obj->body);
            cpVect vel = cpBodyGetVelocity(obj->body);

            float r = 20.0f;
            if (obj->type == EntityType::PLAYER) {
                auto p = std::dynamic_pointer_cast<Player>(obj);
                r = 20.0f + (p->level - 1) * 2.0f;
            }

            bool clamped = false;

            if (pos.x < r) {
                pos.x = r;
                if (vel.x < 0) vel.x = 0;                 clamped = true;
            }
            if (pos.x > width - r) {
                pos.x = width - r;
                if (vel.x > 0) vel.x = 0;
                clamped = true;
            }
            if (pos.y < r) {
                pos.y = r;
                if (vel.y < 0) vel.y = 0;
                clamped = true;
            }
            if (pos.y > height - r) {
                pos.y = height - r;
                if (vel.y > 0) vel.y = 0;
                clamped = true;
            }

            if (clamped) {
                cpBodySetPosition(obj->body, pos);
                cpBodySetVelocity(obj->body, vel);
            }
        }
    }

    void HandleCollisionsAndDamage() {
        for (auto& [bid, bObj] : objects) {
            if (bObj->type != EntityType::BULLET) continue;
            auto bullet = std::dynamic_pointer_cast<Bullet>(bObj);
            Vector2 bPos = ToRay(cpBodyGetPosition(bullet->body));

            if (bPos.x <= 5.0f || bPos.x >= width - 5.0f || bPos.y <= 5.0f || bPos.y >= height - 5.0f) {
                bullet->destroyFlag = true;
                continue;
            }

            for (auto& [pid, pObj] : objects) {
                if (pObj->type != EntityType::PLAYER) continue;
                if (pid == bullet->ownerId) continue;

                auto playerTarget = std::dynamic_pointer_cast<Player>(pObj);
                Vector2 pPos = ToRay(cpBodyGetPosition(playerTarget->body));

                float targetRadius = 20.0f + (playerTarget->level - 1) * 2.0f;

                if (Vector2Distance(bPos, pPos) < (targetRadius + 5.0f)) {
                    bullet->destroyFlag = true;

                    float damageDealt = 10.0f;
                    if (objects.count(bullet->ownerId)) {
                        auto attacker = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]);
                        if (attacker) damageDealt = attacker->curDamage;
                    }

                    playerTarget->health -= damageDealt;

                    if (playerTarget->health <= 0) {
                        playerTarget->health = playerTarget->maxHealth;
                        float rx = (float)(rand() % ((int)width - 300) + 150);
                        float ry = (float)(rand() % ((int)height - 300) + 150);
                        cpBodySetPosition(playerTarget->body, cpv(rx, ry));
                        cpBodySetVelocity(playerTarget->body, cpvzero);

                        if (objects.count(bullet->ownerId)) {
                            auto attacker = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]);
                            if (attacker) attacker->AddXp(50.0f + (playerTarget->level * 10.0f));
                        }
                    }
                    else {
                        if (objects.count(bullet->ownerId)) {
                            auto attacker = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]);
                            if (attacker) attacker->AddXp(5.0f);
                        }
                    }
                }
            }
        }
    }
};