#pragma once
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include "ECS/Player.h"
#include "ECS/Bullet.h"
#include "ECS/Enemy.h"
#include "ECS/Construct.h"
#include "ECS/Artifact.h"
#include "PhysicsUtils.h"
#include "../../common/NetworkPackets.h"

class GameScene {
public:
    cpSpace* space;
    std::map<uint32_t, std::shared_ptr<GameObject>> objects;
    std::vector<EventPacket> pendingEvents;

    uint32_t nextId = 1000;
    float width = 4000;
    float height = 4000;
    const float GRID_SIZE = 50.0f;

    GameScene() {
        space = cpSpaceNew();
        cpSpaceSetGravity(space, cpv(0, 0));
        CreateMapBoundaries();
    }

    ~GameScene() { objects.clear(); cpSpaceFree(space); }

    void CreateMapBoundaries() {
        cpBody* staticBody = cpSpaceGetStaticBody(space);
        float thickness = 2000.0f;
        auto addWall = [&](float x, float y, float w, float h) {
            cpVect verts[] = { cpv(x, y), cpv(x, y + h), cpv(x + w, y + h), cpv(x + w, y) };
            cpShape* s = cpSpaceAddShape(space, cpPolyShapeNew(staticBody, 4, verts, cpTransformIdentity, 0.0));
            cpShapeSetElasticity(s, 0.0f); cpShapeSetFriction(s, 0.5f); cpShapeSetCollisionType(s, COLLISION_WALL);
            };
        addWall(-thickness, -thickness, thickness, height + thickness * 2);
        addWall(width, -thickness, thickness, height + thickness * 2);
        addWall(-thickness, -thickness, width + thickness * 2, thickness);
        addWall(-thickness, height, width + thickness * 2, thickness);
    }

    Vector2 SnapToGrid(Vector2 pos) {
        float x = roundf(pos.x / GRID_SIZE) * GRID_SIZE;
        float y = roundf(pos.y / GRID_SIZE) * GRID_SIZE;
        return { x, y };
    }

    void HandleAdminCommand(uint32_t playerId, const AdminCommandPacket& cmd) {
        if (!objects.count(playerId)) return;
        auto p = std::dynamic_pointer_cast<Player>(objects[playerId]);
        if (!p) return;
        if (cmd.cmdType == AdminCmdType::LOGIN) p->isAdmin = true;
        if (!p->isAdmin) return;

        switch (cmd.cmdType) {
        case AdminCmdType::GIVE_SCRAP: p->scrap += cmd.value; break;
        case AdminCmdType::GIVE_XP: p->AddXp((float)cmd.value); break;
        case AdminCmdType::KILL_ALL_ENEMIES: {
            for (auto& [id, obj] : objects) {
                if (obj->type == EntityType::ENEMY) {
                    obj->health = 0;
                    obj->TakeDamage(9999, 0);
                }
            }
        } break;
        case AdminCmdType::SPAWN_BOSS: SpawnEnemy(EnemyType::BOSS); break;
        }
    }

    
    void TryBuild(uint32_t playerId, uint8_t buildType, Vector2 rawPos) {
        if (!objects.count(playerId)) return;
        auto p = std::dynamic_pointer_cast<Player>(objects[playerId]);
        if (!p) return;

        Vector2 pos = SnapToGrid(rawPos);
        if (pos.x < GRID_SIZE || pos.x > width - GRID_SIZE || pos.y < GRID_SIZE || pos.y > height - GRID_SIZE) return;
        if (Vector2Distance(ToRay(cpBodyGetPosition(p->body)), pos) > 400.0f) return;

        for (auto& [id, obj] : objects) {
            if (obj->type == EntityType::WALL || obj->type == EntityType::TURRET || obj->type == EntityType::MINE) {
                if (Vector2Distance(ToRay(cpBodyGetPosition(obj->body)), pos) < (GRID_SIZE / 2.0f)) return;
            }
        }

        int cost = 0;
        if (buildType == ActionType::BUILD_WALL) cost = 10;
        else if (buildType == ActionType::BUILD_TURRET) cost = 50;
        else if (buildType == ActionType::BUILD_MINE) cost = 25;

        if (p->scrap >= cost) {
            p->scrap -= cost;
            std::shared_ptr<GameObject> obj = nullptr;
            if (buildType == ActionType::BUILD_WALL) obj = std::make_shared<Wall>(nextId++, pos, p->id, space);
            else if (buildType == ActionType::BUILD_TURRET) obj = std::make_shared<Turret>(nextId++, pos, p->id, space);
            else if (buildType == ActionType::BUILD_MINE) obj = std::make_shared<Mine>(nextId++, pos, p->id, space);

            if (obj) {
                objects[obj->id] = obj;
                pendingEvents.push_back({ 1, pos, WHITE });
            }
        }
    }

    void TryUpgrade(uint32_t playerId, Vector2 rawPos) {
        if (!objects.count(playerId)) return;
        auto p = std::dynamic_pointer_cast<Player>(objects[playerId]);
        for (auto& [id, obj] : objects) {
            if (obj->type == EntityType::WALL || obj->type == EntityType::TURRET || obj->type == EntityType::MINE) {
                Vector2 objPos = ToRay(cpBodyGetPosition(obj->body));
                if (Vector2Distance(rawPos, objPos) < 30.0f) {
                    auto c = std::dynamic_pointer_cast<Construct>(obj);
                    int cost = 20 * c->level;
                    if (p->scrap >= cost) {
                        p->scrap -= cost;
                        c->Upgrade();
                        pendingEvents.push_back({ 2, objPos, GREEN });
                    }
                    return;
                }
            }
        }
    }

    std::shared_ptr<Player> CreatePlayerWithId(uint32_t id) {
        Vector2 startPos = { width / 2.0f, height / 2.0f };
        if (objects.count(id)) objects.erase(id);
        auto p = std::make_shared<Player>(id, startPos, space);
        p->Reset();
        if (objects.size() == 0) p->isAdmin = true;
        objects[id] = p;
        return p;
    }

    std::shared_ptr<Enemy> SpawnEnemy(uint8_t forcedType = 255) {
        float spawnX, spawnY;
        int side = rand() % 4;
        float offset = 50.0f;
        if (side == 0) { spawnX = -offset; spawnY = (float)(rand() % (int)height); }
        else if (side == 1) { spawnX = width + offset; spawnY = (float)(rand() % (int)height); }
        else if (side == 2) { spawnX = (float)(rand() % (int)width); spawnY = -offset; }
        else { spawnX = (float)(rand() % (int)width); spawnY = height + offset; }

        uint8_t type = EnemyType::BASIC;
        if (forcedType != 255) {
            type = forcedType;
        }
        else {
            int chance = rand() % 100;
            if (chance < 60) type = EnemyType::BASIC;
            else if (chance < 85) type = EnemyType::FAST;
            else if (chance < 98) type = EnemyType::TANK;
            else type = EnemyType::BOSS;
        }

        auto enemy = std::make_shared<Enemy>(nextId++, Vector2{ spawnX, spawnY }, type, space);
        objects[enemy->id] = enemy;
        return enemy;
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
                    auto b = std::make_shared<Bullet>(nextId++, Vector2Add(playerPos, Vector2Scale(dir, 35.0f)), dir, p->id, space);

                    b->lifeTime = p->curBulletPen;
                    float spd = p->curBulletSpeed;

                    cpBodySetVelocity(b->body, cpvadd(cpvmult(ToCp(dir), spd), cpvmult(cpBodyGetVelocity(p->body), 0.2f)));
                    newBullets.push_back(b);
                }
            }

            if (obj->type == EntityType::TURRET) {
                auto t = std::dynamic_pointer_cast<Turret>(obj);
                if (t->cooldown <= 0) {
                    float minDist = t->range;
                    std::shared_ptr<GameObject> target = nullptr;
                    Vector2 tPos = ToRay(cpBodyGetPosition(t->body));
                    for (auto& [eid, eObj] : objects) {
                        if (eObj->type == EntityType::ENEMY) {
                            float d = Vector2Distance(tPos, ToRay(cpBodyGetPosition(eObj->body)));
                            if (d < minDist) { minDist = d; target = eObj; }
                        }
                    }
                    if (target) {
                        t->cooldown = t->reloadTime;
                        Vector2 ePos = ToRay(cpBodyGetPosition(target->body));
                        Vector2 dir = Vector2Normalize(Vector2Subtract(ePos, tPos));
                        auto b = std::make_shared<Bullet>(nextId++, Vector2Add(tPos, Vector2Scale(dir, 35.0f)), dir, t->ownerId, space);
                        newBullets.push_back(b);
                    }
                }
            }

            if (obj->type == EntityType::MINE) {
                auto m = std::dynamic_pointer_cast<Mine>(obj);
                Vector2 mPos = ToRay(cpBodyGetPosition(m->body));
                for (auto& [eid, eObj] : objects) {
                    if (eObj->type == EntityType::ENEMY) {
                        if (Vector2Distance(mPos, ToRay(cpBodyGetPosition(eObj->body))) < 35.0f) {
                            pendingEvents.push_back({ 1, mPos, ORANGE });
                            eObj->TakeDamage(m->damage, GetTime());
                            m->destroyFlag = true;
                            break;
                        }
                    }
                }
            }

            if (obj->type == EntityType::ENEMY) {
                auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
                float minDist = 999999.0f;
                Vector2 targetPos = { width / 2, height / 2 };
                Vector2 myPos = ToRay(cpBodyGetPosition(obj->body));

                for (const auto& [pid, pObj] : objects) {
                    if (pObj->type == EntityType::PLAYER || pObj->type == EntityType::TURRET) {
                        float dist = Vector2Distance(myPos, ToRay(cpBodyGetPosition(pObj->body)));
                        if (dist < minDist) { minDist = dist; targetPos = ToRay(cpBodyGetPosition(pObj->body)); }
                    }
                }
                enemy->MoveTowards(targetPos);
            }

            if (obj->destroyFlag) it = objects.erase(it);
            else ++it;
        }
        for (auto& b : newBullets) objects[b->id] = b;

        HandleCollisionsAndDamage();
    }

    void EnforceMapBoundaries() {
        for (auto& [id, obj] : objects) {
            if (obj->type == EntityType::BULLET || !obj->body || cpBodyGetType(obj->body) == CP_BODY_TYPE_STATIC) continue;
            cpVect pos = cpBodyGetPosition(obj->body);
            cpVect vel = cpBodyGetVelocity(obj->body);
            float r = 20.0f;
            bool clamped = false;
            if (pos.x < r) { pos.x = r; if (vel.x < 0) vel.x = 0; clamped = true; }
            if (pos.x > width - r) { pos.x = width - r; if (vel.x > 0) vel.x = 0; clamped = true; }
            if (pos.y < r) { pos.y = r; if (vel.y < 0) vel.y = 0; clamped = true; }
            if (pos.y > height - r) { pos.y = height - r; if (vel.y > 0) vel.y = 0; clamped = true; }
            if (clamped) { cpBodySetPosition(obj->body, pos); cpBodySetVelocity(obj->body, vel); }
        }
    }

    void HandleCollisionsAndDamage() {
        std::vector<std::shared_ptr<GameObject>> newArtifacts;
        double currentTime = GetTime();

        for (auto& [eid, eObj] : objects) {
            if (eObj->type != EntityType::ENEMY) continue;
            auto enemy = std::dynamic_pointer_cast<Enemy>(eObj);
            Vector2 ePos = ToRay(cpBodyGetPosition(enemy->body));
            float eRad = 20.0f;

            for (auto& [pid, pObj] : objects) {
                if (pObj->type != EntityType::PLAYER && pObj->type != EntityType::WALL && pObj->type != EntityType::TURRET) continue;

                Vector2 pPos = ToRay(cpBodyGetPosition(pObj->body));
                float pRad = 25.0f;
                if (pObj->type == EntityType::WALL) pRad = 35.0f;
                float bodyDmg = enemy->damage;
                if (pObj->type == EntityType::PLAYER) {
                    auto p = std::dynamic_pointer_cast<Player>(pObj);
                    enemy->TakeDamage(p->curBodyDmg * 0.5f, currentTime);
                }

                if (Vector2Distance(ePos, pPos) < (eRad + pRad)) {
                    pObj->TakeDamage(bodyDmg * 0.016f, currentTime);
                    if (cpBodyGetType(pObj->body) != CP_BODY_TYPE_STATIC) {
                        Vector2 knockDir = Vector2Normalize(Vector2Subtract(pPos, ePos));
                        cpBodyApplyImpulseAtLocalPoint(pObj->body, ToCp(Vector2Scale(knockDir, 50.0f)), cpvzero);
                    }
                }
            }
        }

        for (auto& [aid, aObj] : objects) {
            if (aObj->type != EntityType::ARTIFACT) continue;
            Vector2 aPos = ToRay(cpBodyGetPosition(aObj->body));
            for (auto& [pid, pObj] : objects) {
                if (pObj->type != EntityType::PLAYER) continue;
                auto p = std::dynamic_pointer_cast<Player>(pObj);
                if (Vector2Distance(aPos, ToRay(cpBodyGetPosition(p->body))) < 40.0f) {
                    auto art = std::dynamic_pointer_cast<Artifact>(aObj);
                    if (p->AddItemToInventory(art->bonusType)) {
                        aObj->destroyFlag = true;
                        pendingEvents.push_back({ 2, aPos, GOLD });
                    }
                }
            }
        }

        for (auto& [bid, bObj] : objects) {
            if (bObj->type != EntityType::BULLET) continue;
            auto bullet = std::dynamic_pointer_cast<Bullet>(bObj);
            Vector2 bPos = ToRay(cpBodyGetPosition(bullet->body));

            if (bPos.x <= -50 || bPos.x >= width + 50 || bPos.y <= -50 || bPos.y >= height + 50) {
                bullet->destroyFlag = true; continue;
            }

            for (auto& [tid, tObj] : objects) {
                if (tid == bullet->ownerId || tObj->type == EntityType::BULLET ||
                    tObj->type == EntityType::ARTIFACT || tObj->type == EntityType::MINE) continue;

                if (objects.count(bullet->ownerId)) {
                    if (objects[bullet->ownerId]->type == EntityType::PLAYER &&
                        (tObj->type == EntityType::TURRET || tObj->type == EntityType::WALL)) continue;
                }

                float targetRadius = 25.0f;
                if (tObj->type == EntityType::WALL) targetRadius = 35.0f;
                else if (tObj->type == EntityType::ENEMY) {
                    auto e = std::dynamic_pointer_cast<Enemy>(tObj);
                    if (e && e->enemyType == EnemyType::BOSS) targetRadius = 60.0f;
                }

                if (Vector2Distance(bPos, ToRay(cpBodyGetPosition(tObj->body))) < targetRadius) {
                    bullet->destroyFlag = true;
                    pendingEvents.push_back({ 0, bPos, WHITE });

                    float dmg = 10.0f;
                    if (objects.count(bullet->ownerId) && objects[bullet->ownerId]->type == EntityType::PLAYER) {
                        auto p = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]);
                        dmg = p->curDamage;
                    }
                    else if (objects.count(bullet->ownerId) && objects[bullet->ownerId]->type == EntityType::TURRET) {
                        auto t = std::dynamic_pointer_cast<Turret>(objects[bullet->ownerId]);
                        dmg = t->damage;
                    }

                    tObj->TakeDamage(dmg, currentTime);

                    if (tObj->health <= 0) {
                        Color debrisCol = GRAY;
                        if (tObj->type == EntityType::ENEMY) debrisCol = RED;

                        pendingEvents.push_back({ 1, ToRay(cpBodyGetPosition(tObj->body)), debrisCol });

                        if (tObj->type == EntityType::PLAYER) {
                            auto p = std::dynamic_pointer_cast<Player>(tObj);
                            p->Reset();
                            cpBodySetPosition(p->body, cpv(rand() % (int)width, rand() % (int)height));
                        }
                        else if (tObj->type == EntityType::ENEMY) {
                            tObj->destroyFlag = true;
                            auto e = std::dynamic_pointer_cast<Enemy>(tObj);

                            for (auto& [pid, pObj] : objects) {
                                if (pObj->type == EntityType::PLAYER) {
                                    auto p = std::dynamic_pointer_cast<Player>(pObj);
                                    p->AddXp(e->xpReward);
                                    p->scrap += e->scrapReward;
                                }
                            }

                            if (objects.count(bullet->ownerId) && objects[bullet->ownerId]->type == EntityType::PLAYER) {
                                auto p = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]);
                                p->kills++;
                            }

                                                        int dropChance = 0;
                            if (e->enemyType == EnemyType::BOSS) dropChance = 100;
                            else if (e->enemyType == EnemyType::TANK) dropChance = 15;
                            else dropChance = 0;

                            if (rand() % 100 < dropChance) {
                                auto art = std::make_shared<Artifact>(nextId++, ToRay(cpBodyGetPosition(tObj->body)), space);
                                newArtifacts.push_back(art);
                            }
                        }
                        else if (tObj->type == EntityType::WALL || tObj->type == EntityType::TURRET) {
                            tObj->destroyFlag = true;
                        }
                    }
                }
            }
        }
        for (auto& a : newArtifacts) objects[a->id] = a;
    }
};