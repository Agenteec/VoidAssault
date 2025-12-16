#pragma once
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include <cmath>
#include "../ECS/Player.h"
#include "../ECS/Bullet.h"
#include "../ECS/Enemy.h"
#include "../ECS/Construct.h"
#include "../ECS/Artifact.h"
#include "../PhysicsUtils.h"
#include "../../common/NetworkPackets.h"

class GameScene {
public:
    cpSpace* space;
    std::map<uint32_t, std::shared_ptr<GameObject>> objects;
    std::vector<EventPacket> pendingEvents;
    float pvpFactor = 1.0f;
    uint32_t nextId = 1000;
    float width = 4000;
    float height = 4000;
    const float GRID_SIZE = 50.0f;

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
        float thickness = 2000.0f;
        auto addWall = [&](float x, float y, float w, float h) {
            cpVect verts[] = { cpv(x, y), cpv(x, y + h), cpv(x + w, y + h), cpv(x + w, y) };
            cpShape* s = cpSpaceAddShape(space, cpPolyShapeNew(staticBody, 4, verts, cpTransformIdentity, 0.0));
            cpShapeSetElasticity(s, 0.0f);
            cpShapeSetFriction(s, 0.5f);
            cpShapeSetCollisionType(s, COLLISION_WALL);
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
        case AdminCmdType::GIVE_SCRAP:
            p->scrap += cmd.value;
            break;
        case AdminCmdType::GIVE_XP:
            p->AddXp((float)cmd.value);
            break;
        case AdminCmdType::KILL_ALL_ENEMIES: {
            for (auto& [id, obj] : objects) {
                if (obj->type == EntityType::ENEMY) {
                    obj->TakeDamage(999999.0f, 0);                 }
            }
        } break;
        case AdminCmdType::SPAWN_BOSS:
            SpawnEnemy(EnemyType::BOSS);
            break;

                    case AdminCmdType::CLEAR_BUILDINGS: {
            for (auto& [id, obj] : objects) {
                if (obj->type == EntityType::WALL ||
                    obj->type == EntityType::TURRET ||
                    obj->type == EntityType::MINE) {
                    obj->destroyFlag = true;
                                        if (obj->body) {
                        Vector2 pos = ToRay(cpBodyGetPosition(obj->body));
                        pendingEvents.push_back({ 1, pos, GRAY });
                    }
                }
            }
        } break;

                                                  case AdminCmdType::RESET_SERVER: {
                        auto it = objects.begin();
            while (it != objects.end()) {
                if (it->second->type != EntityType::PLAYER) {
                                        if (it->second->body) {
                                            }
                    it = objects.erase(it);
                }
                else {
                                        auto pl = std::dynamic_pointer_cast<Player>(it->second);
                    if (pl) {
                        pl->Reset();
                                                float rx = (width / 2.0f) + (float)(rand() % 400 - 200);
                        float ry = (height / 2.0f) + (float)(rand() % 400 - 200);
                        cpBodySetPosition(pl->body, cpv(rx, ry));
                        cpBodySetVelocity(pl->body, cpvzero);
                    }
                    ++it;
                }
            }
                                } break;
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

                if (buildType == ActionType::BUILD_TURRET) {
            int myTurrets = 0;
            for (auto& [id, obj] : objects) {
                if (obj->type == EntityType::TURRET) {
                    auto t = std::dynamic_pointer_cast<Turret>(obj);
                    if (t && t->ownerId == playerId) myTurrets++;
                }
            }
            if (myTurrets >= 5) return;
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
        if (objects.size() == 0) p->isAdmin = true;         objects[id] = p;
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

                std::vector<std::shared_ptr<Enemy>> enemies;
        std::vector<std::shared_ptr<Player>> players;
        std::vector<std::shared_ptr<Bullet>> bullets;
        std::vector<std::shared_ptr<Construct>> structures;
        std::vector<std::shared_ptr<Mine>> mines;
        std::vector<std::shared_ptr<Artifact>> artifacts;

        enemies.reserve(objects.size());
        bullets.reserve(objects.size());

        for (auto& [id, obj] : objects) {
            if (obj->destroyFlag) continue;
            switch (obj->type) {
            case EntityType::ENEMY: enemies.push_back(std::dynamic_pointer_cast<Enemy>(obj)); break;
            case EntityType::PLAYER: players.push_back(std::dynamic_pointer_cast<Player>(obj)); break;
            case EntityType::BULLET: bullets.push_back(std::dynamic_pointer_cast<Bullet>(obj)); break;
            case EntityType::WALL:
            case EntityType::TURRET: structures.push_back(std::dynamic_pointer_cast<Construct>(obj)); break;
            case EntityType::MINE: mines.push_back(std::dynamic_pointer_cast<Mine>(obj)); break;
            case EntityType::ARTIFACT: artifacts.push_back(std::dynamic_pointer_cast<Artifact>(obj)); break;
            }
        }

                for (auto& enemy : enemies) {
            if (enemy->destroyFlag || enemy->health <= 0) continue;
            Vector2 ePos = ToRay(cpBodyGetPosition(enemy->body));
            float eRad = (enemy->enemyType == EnemyType::BOSS) ? 70.0f : ((enemy->enemyType == EnemyType::TANK) ? 35.0f : 20.0f);

            for (auto& pObj : players) {
                if (pObj->destroyFlag || pObj->health <= 0) continue;
                Vector2 pPos = ToRay(cpBodyGetPosition(pObj->body));

                if (Vector2Distance(ePos, pPos) < (eRad + 25.0f + 8.0f)) {
                    float enemyDpsMult = 2.0f;
                    pObj->TakeDamage(enemy->damage * enemyDpsMult * 0.016f, currentTime);

                    if (pObj->health <= 0) {
                        pObj->Reset();
                        cpBodySetPosition(pObj->body, cpv(rand() % (int)width, rand() % (int)height));
                        pendingEvents.push_back({ 1, pPos, RED });
                    }
                    enemy->TakeDamage(pObj->curBodyDmg * 0.2f * 0.016f, currentTime);
                    Vector2 knockDir = Vector2Normalize(Vector2Subtract(pPos, ePos));
                    cpBodyApplyImpulseAtLocalPoint(pObj->body, ToCp(Vector2Scale(knockDir, 150.0f)), cpvzero);
                }
            }

            for (auto& str : structures) {
                if (str->destroyFlag || str->health <= 0) continue;
                Vector2 sPos = ToRay(cpBodyGetPosition(str->body));
                float sRad = (str->type == EntityType::WALL) ? 35.0f : 25.0f;
                if (Vector2Distance(ePos, sPos) < (eRad + sRad + 8.0f)) {
                    str->TakeDamage(enemy->damage * 2.0f * 0.016f, currentTime);
                    if (str->health <= 0) {
                        str->destroyFlag = true;
                        pendingEvents.push_back({ 1, sPos, GRAY });
                    }
                    enemy->TakeDamage(5.0f * 0.016f, currentTime);
                }
            }
        }

                for (auto& mine : mines) {
            if (mine->destroyFlag) continue;
            Vector2 mPos = ToRay(cpBodyGetPosition(mine->body));
            bool triggered = false;
            for (auto& enemy : enemies) {
                float eRad = (enemy->enemyType == EnemyType::TANK) ? 35.0f : 20.0f;
                if (Vector2Distance(mPos, ToRay(cpBodyGetPosition(enemy->body))) < (mine->triggerRadius + eRad)) {
                    triggered = true; break;
                }
            }
            if (triggered) {
                mine->destroyFlag = true;
                pendingEvents.push_back({ 1, mPos, ORANGE });
                pendingEvents.push_back({ 2, mPos, RED });
                for (auto& enemy : enemies) {
                    if (enemy->destroyFlag) continue;
                    if (Vector2Distance(mPos, ToRay(cpBodyGetPosition(enemy->body))) <= mine->splashRadius) {
                        enemy->TakeDamage(mine->damage, currentTime);
                        if (enemy->health <= 0) {
                            enemy->destroyFlag = true;
                            pendingEvents.push_back({ 1, ToRay(cpBodyGetPosition(enemy->body)), RED });
                            if (objects.count(mine->ownerId) && objects[mine->ownerId]->type == EntityType::PLAYER) {
                                auto p = std::dynamic_pointer_cast<Player>(objects[mine->ownerId]);
                                p->AddXp(enemy->xpReward); p->scrap += enemy->scrapReward; p->kills++;
                            }
                        }
                    }
                }
            }
        }

                for (auto& art : artifacts) {
            if (art->destroyFlag) continue;
            Vector2 aPos = ToRay(cpBodyGetPosition(art->body));
            for (auto& p : players) {
                if (Vector2Distance(aPos, ToRay(cpBodyGetPosition(p->body))) < 40.0f) {
                    if (p->AddItemToInventory(art->bonusType)) {
                        art->destroyFlag = true;
                        pendingEvents.push_back({ 2, aPos, GOLD });
                    }
                }
            }
        }

                for (auto& bullet : bullets) {
            if (bullet->destroyFlag) continue;
            Vector2 bPos = ToRay(cpBodyGetPosition(bullet->body));

            if (bPos.x <= -50 || bPos.x >= width + 50 || bPos.y <= -50 || bPos.y >= height + 50) {
                bullet->destroyFlag = true; continue;
            }

            bool hit = false;
            float dmg = 10.0f;

                        if (objects.count(bullet->ownerId)) {
                auto owner = objects[bullet->ownerId];
                if (owner->type == EntityType::PLAYER) dmg = std::dynamic_pointer_cast<Player>(owner)->curDamage;
                else if (owner->type == EntityType::TURRET) dmg = std::dynamic_pointer_cast<Turret>(owner)->damage;
            }

                        for (auto& enemy : enemies) {
                if (enemy->destroyFlag || enemy->health <= 0) continue;
                float targetRadius = (enemy->enemyType == EnemyType::BOSS) ? 75.0f : ((enemy->enemyType == EnemyType::TANK) ? 40.0f : 30.0f);

                if (Vector2Distance(bPos, ToRay(cpBodyGetPosition(enemy->body))) <= targetRadius) {
                    hit = true;
                    enemy->TakeDamage(dmg, currentTime);
                    if (enemy->health <= 0) {
                        enemy->destroyFlag = true;
                        pendingEvents.push_back({ 1, ToRay(cpBodyGetPosition(enemy->body)), RED });
                        if (objects.count(bullet->ownerId) && objects[bullet->ownerId]->type == EntityType::PLAYER) {
                            auto p = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]);
                            p->AddXp(enemy->xpReward); p->scrap += enemy->scrapReward; p->kills++;
                        }
                        int dropChance = (enemy->enemyType == EnemyType::BOSS) ? 100 : (enemy->enemyType == EnemyType::TANK ? 25 : 5);
                        if (rand() % 100 < dropChance) newArtifacts.push_back(std::make_shared<Artifact>(nextId++, ToRay(cpBodyGetPosition(enemy->body)), space));
                    }
                    break;
                }
            }
            if (hit) { bullet->destroyFlag = true; pendingEvents.push_back({ 0, bPos, WHITE }); continue; }

                        if (pvpFactor > 0.001f) {                 for (auto& p : players) {
                                        if (p->id == bullet->ownerId) continue;
                    if (p->destroyFlag || p->health <= 0) continue;

                                                            if (objects.count(bullet->ownerId) && objects[bullet->ownerId]->type != EntityType::PLAYER) continue;

                    float pRad = 25.0f;
                    if (Vector2Distance(bPos, ToRay(cpBodyGetPosition(p->body))) <= pRad) {
                        hit = true;

                                                float pvpDmg = dmg * pvpFactor;

                        p->TakeDamage(pvpDmg, currentTime);
                        pendingEvents.push_back({ 0, bPos, RED }); 
                        if (p->health <= 0) {
                            p->Reset();
                            cpBodySetPosition(p->body, cpv(rand() % (int)width, rand() % (int)height));
                            pendingEvents.push_back({ 1, ToRay(cpBodyGetPosition(p->body)), RED });

                                                        if (objects.count(bullet->ownerId) && objects[bullet->ownerId]->type == EntityType::PLAYER) {
                                auto killer = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]);
                                killer->kills++;
                                killer->scrap += p->level * 10;                             }
                        }
                        break;
                    }
                }
            }
            if (hit) { bullet->destroyFlag = true; continue; }

                                    for (auto& str : structures) {
                if (str->destroyFlag) continue;
                if (str->ownerId == bullet->ownerId) continue; 
                                if (pvpFactor <= 0.001f && objects.count(bullet->ownerId) && objects[bullet->ownerId]->type == EntityType::PLAYER) continue;

                float sRad = (str->type == EntityType::WALL) ? 35.0f : 25.0f;
                if (Vector2Distance(bPos, ToRay(cpBodyGetPosition(str->body))) <= sRad) {
                    hit = true;
                                        str->TakeDamage(dmg, currentTime);
                    if (str->health <= 0) {
                        str->destroyFlag = true;
                        pendingEvents.push_back({ 1, ToRay(cpBodyGetPosition(str->body)), GRAY });
                    }
                    break;
                }
            }
            if (hit) { bullet->destroyFlag = true; pendingEvents.push_back({ 0, bPos, WHITE }); continue; }
        }

        for (auto& a : newArtifacts) objects[a->id] = a;
    }

};