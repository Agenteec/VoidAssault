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
        CreateMapBoundaries();
    }

    ~GameScene() {
        objects.clear();
        cpSpaceFree(space);
    }

    void CreateMapBoundaries() {
        cpBody* staticBody = cpSpaceGetStaticBody(space);

        float w = (width > 0) ? width : 2000;
        float h = (height > 0) ? height : 2000;
        float thick = 20.0f;

        auto addWall = [&](Vector2 a, Vector2 b) {
            cpShape* wall = cpSpaceAddShape(space, cpSegmentShapeNew(staticBody, ToCp(a), ToCp(b), thick));
            cpShapeSetElasticity(wall, 0.0f);
            cpShapeSetFriction(wall, 1.0f);
            cpShapeSetCollisionType(wall, COLLISION_WALL);
            };

        addWall({ 0,0 }, { w, 0 });
        addWall({ 0,h }, { w, h });
        addWall({ 0,0 }, { 0, h });
        addWall({ w,0 }, { w, h });
    }

    std::shared_ptr<Player> CreatePlayerWithId(uint32_t id) {
        Vector2 startPos = { width / 2.0f, height / 2.0f };


        auto p = std::make_shared<Player>(id, startPos, space);
        objects[id] = p;
        return p;
    }

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
                        float rx = (float)(rand() % ((int)width - 200) + 100);
                        float ry = (float)(rand() % ((int)height - 200) + 100);

                        cpVect randPos = cpv(rx, ry);
                        cpBodySetPosition(pObj->body, randPos);
                        cpBodySetVelocity(pObj->body, cpvzero);
                    }
                }
            }
        }
    }
};