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
    uint32_t nextId = 1000; // Начинаем с 1000, чтобы не пересекаться с PeerID (0-64), хотя это не критично
    float width = 2000;
    float height = 2000;

    GameScene() {
        space = cpSpaceNew();
        cpSpaceSetGravity(space, cpv(0, 0));
        CreateMapBoundaries();
    }

    ~GameScene() {
        objects.clear();
        cpSpaceFree(space);
    }

    void CreateMapBoundaries() {
        cpBody* staticBody = cpSpaceGetStaticBody(space);
        float w = width;
        float h = height;
        float thick = 20.0f;

        auto addWall = [&](Vector2 a, Vector2 b) {
            cpShape* wall = cpSpaceAddShape(space, cpSegmentShapeNew(staticBody, ToCp(a), ToCp(b), thick));
            cpShapeSetElasticity(wall, 0.5f);
            cpShapeSetFriction(wall, 1.0f);
            cpShapeSetCollisionType(wall, COLLISION_WALL);
            };

        addWall({ 0,0 }, { w, 0 });
        addWall({ 0,h }, { w, h });
        addWall({ 0,0 }, { 0, h });
        addWall({ w,0 }, { w, h });
    }

    // Создание игрока с конкретным ID (от ENet)
    std::shared_ptr<Player> CreatePlayerWithId(uint32_t id) {
        Vector2 startPos = { width / 2, height / 2 };
        auto p = std::make_shared<Player>(id, startPos, space);
        objects[id] = p;
        return p;
    }

    // Обычное создание (для ботов)
    std::shared_ptr<Player> CreatePlayer() {
        return CreatePlayerWithId(nextId++);
    }

    void Update(float dt) {
        cpSpaceStep(space, dt);

        std::vector<std::shared_ptr<Bullet>> newBullets;

        for (auto it = objects.begin(); it != objects.end();) {
            auto& obj = it->second;
            obj->Update(dt);

            if (obj->type == EntityType::PLAYER) {
                auto p = std::dynamic_pointer_cast<Player>(obj);
                if (p && p->spawnBulletSignal) {
                    p->spawnBulletSignal = false;
                    Vector2 spawnPos = ToRay(cpBodyGetPosition(p->body));
                    // ID пули генерируем сами
                    auto b = std::make_shared<Bullet>(nextId++, spawnPos, p->bulletDir, p->id, space);
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

        HandleDamageManual();
    }

    void HandleDamageManual() {
        for (auto& [bid, bObj] : objects) {
            if (bObj->type != EntityType::BULLET) continue;
            auto bullet = std::dynamic_pointer_cast<Bullet>(bObj);
            Vector2 bPos = ToRay(cpBodyGetPosition(bullet->body));

            for (auto& [pid, pObj] : objects) {
                if (pObj->type != EntityType::PLAYER) continue;
                if (pid == bullet->ownerId) continue;

                Vector2 pPos = ToRay(cpBodyGetPosition(pObj->body));
                if (Vector2Distance(bPos, pPos) < 25.0f) {
                    pObj->health -= 10;
                    bullet->destroyFlag = true;

                    if (pObj->health <= 0) {
                        pObj->health = 100;
                        cpVect randPos = cpv(GetRandomValue(100, (int)width - 100), GetRandomValue(100, (int)height - 100));
                        cpBodySetPosition(pObj->body, randPos);
                        cpBodySetVelocity(pObj->body, cpvzero);
                    }
                }
            }
        }
    }
};