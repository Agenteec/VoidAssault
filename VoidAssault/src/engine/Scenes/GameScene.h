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
                    obj->TakeDamage(999999.0f, 0); // Гарантированное убийство
                }
            }
        } break;
        case AdminCmdType::SPAWN_BOSS:
            SpawnEnemy(EnemyType::BOSS);
            break;

            // NEW: Очистка построек
        case AdminCmdType::CLEAR_BUILDINGS: {
            for (auto& [id, obj] : objects) {
                if (obj->type == EntityType::WALL ||
                    obj->type == EntityType::TURRET ||
                    obj->type == EntityType::MINE) {
                    obj->destroyFlag = true;
                    // Создаем эффект исчезновения
                    if (obj->body) {
                        Vector2 pos = ToRay(cpBodyGetPosition(obj->body));
                        pendingEvents.push_back({ 1, pos, GRAY });
                    }
                }
            }
        } break;

                                          // NEW: Полный сброс сервера
        case AdminCmdType::RESET_SERVER: {
            // Удаляем всех, кроме игроков
            auto it = objects.begin();
            while (it != objects.end()) {
                if (it->second->type != EntityType::PLAYER) {
                    // Эффект исчезновения для всего
                    if (it->second->body) {
                        // Чтобы не спамить пакетами, можно не добавлять эффекты при полном сбросе
                    }
                    it = objects.erase(it);
                }
                else {
                    // Сброс игроков
                    auto pl = std::dynamic_pointer_cast<Player>(it->second);
                    if (pl) {
                        pl->Reset();
                        // Телепорт в случайную точку около центра
                        float rx = (width / 2.0f) + (float)(rand() % 400 - 200);
                        float ry = (height / 2.0f) + (float)(rand() % 400 - 200);
                        cpBodySetPosition(pl->body, cpv(rx, ry));
                        cpBodySetVelocity(pl->body, cpvzero);
                    }
                    ++it;
                }
            }
            // Сбрасываем ID генератор, чтобы не улетал в бесконечность (безопасно)
            // Но лучше оставить как есть, чтобы не было коллизий ID у старых пакетов
        } break;
        }
    }

    void TryBuild(uint32_t playerId, uint8_t buildType, Vector2 rawPos) {
        if (!objects.count(playerId)) return;
        auto p = std::dynamic_pointer_cast<Player>(objects[playerId]);
        if (!p) return;

        Vector2 pos = SnapToGrid(rawPos);
        // Проверка границ карты
        if (pos.x < GRID_SIZE || pos.x > width - GRID_SIZE || pos.y < GRID_SIZE || pos.y > height - GRID_SIZE) return;
        // Проверка дистанции стройки
        if (Vector2Distance(ToRay(cpBodyGetPosition(p->body)), pos) > 400.0f) return;

        // Проверка наложения на другие постройки
        for (auto& [id, obj] : objects) {
            if (obj->type == EntityType::WALL || obj->type == EntityType::TURRET || obj->type == EntityType::MINE) {
                if (Vector2Distance(ToRay(cpBodyGetPosition(obj->body)), pos) < (GRID_SIZE / 2.0f)) return;
            }
        }

        // Лимит турелей
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
                // Радиус клика для апгрейда
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
        if (objects.size() == 0) p->isAdmin = true; // Первый игрок - админ
        objects[id] = p;
        return p;
    }

    std::shared_ptr<Enemy> SpawnEnemy(uint8_t forcedType = 255) {
        float spawnX, spawnY;
        int side = rand() % 4;
        float offset = 50.0f;

        // Спавн за пределами экрана
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

            // Логика стрельбы игрока
            if (obj->type == EntityType::PLAYER) {
                auto p = std::dynamic_pointer_cast<Player>(obj);
                if (p && p->spawnBulletSignal) {
                    p->spawnBulletSignal = false;
                    Vector2 playerPos = ToRay(cpBodyGetPosition(p->body));
                    Vector2 dir = Vector2Normalize(p->bulletDir);
                    auto b = std::make_shared<Bullet>(nextId++, Vector2Add(playerPos, Vector2Scale(dir, 35.0f)), dir, p->id, space);

                    b->lifeTime = p->curBulletPen;
                    float spd = p->curBulletSpeed;

                    // Добавляем инерцию от движения игрока
                    cpBodySetVelocity(b->body, cpvadd(cpvmult(ToCp(dir), spd), cpvmult(cpBodyGetVelocity(p->body), 0.2f)));
                    newBullets.push_back(b);
                }
            }

            // Логика стрельбы турели
            if (obj->type == EntityType::TURRET) {
                auto t = std::dynamic_pointer_cast<Turret>(obj);
                if (t->cooldown <= 0) {
                    float minDist = t->range;
                    std::shared_ptr<GameObject> target = nullptr;
                    Vector2 tPos = ToRay(cpBodyGetPosition(t->body));

                    // Поиск ближайшего врага
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

            // Логика мин
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

            // Логика движения врагов
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
            // Пули могут вылетать (чтобы удаляться), статика не двигается
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

        // 1. Взаимодействие ВРАГ <-> ИГРОК/ПОСТРОЙКА (Таран)
        for (auto& [eid, eObj] : objects) {
            if (eObj->type != EntityType::ENEMY) continue;
            auto enemy = std::dynamic_pointer_cast<Enemy>(eObj);

            if (enemy->destroyFlag || enemy->health <= 0) continue;

            Vector2 ePos = ToRay(cpBodyGetPosition(enemy->body));

            // Определяем радиус врага для корректной коллизии
            float eRad = 20.0f;
            if (enemy->enemyType == EnemyType::TANK) eRad = 35.0f;
            if (enemy->enemyType == EnemyType::BOSS) eRad = 70.0f;

            for (auto& [pid, pObj] : objects) {
                if (pObj->type != EntityType::PLAYER && pObj->type != EntityType::WALL && pObj->type != EntityType::TURRET) continue;
                if (pObj->destroyFlag || pObj->health <= 0) continue;

                Vector2 pPos = ToRay(cpBodyGetPosition(pObj->body));
                float pRad = 25.0f;
                if (pObj->type == EntityType::WALL) pRad = 35.0f;

                // Проверка столкновения
                if (Vector2Distance(ePos, pPos) < (eRad + pRad + 8.0f)) {

                    // --- Враг наносит урон Игроку/Структуре ---
                    float enemyDpsMult = 2.0f;
                    pObj->TakeDamage(enemy->damage * enemyDpsMult * 0.016f, currentTime);

                    // --- [FIX] ПРОВЕРКА СМЕРТИ ИГРОКА/СТРУКТУРЫ ---
                    if (pObj->health <= 0) {
                        if (pObj->type == EntityType::PLAYER) {
                            // Логика смерти игрока
                            auto pl = std::dynamic_pointer_cast<Player>(pObj);
                            pl->Reset();
                            // Респавн в случайном месте
                            cpBodySetPosition(pl->body, cpv(rand() % (int)width, rand() % (int)height));
                            pendingEvents.push_back({ 1, pPos, RED }); // Эффект смерти
                        }
                        else {
                            // Логика уничтожения постройки
                            pObj->destroyFlag = true;
                            pendingEvents.push_back({ 1, pPos, GRAY }); // Эффект обломков
                        }
                    }
                    // ----------------------------------------------

                    // --- Ответный урон врагу (шипы) ---
                    float ramDamage = 0.0f;
                    if (pObj->type == EntityType::PLAYER) {
                        auto p = std::dynamic_pointer_cast<Player>(pObj);
                        ramDamage = p->curBodyDmg * 0.2f * 0.016f;
                    }
                    else {
                        ramDamage = 5.0f * 0.016f;
                    }
                    enemy->TakeDamage(ramDamage, currentTime);

                    // Отталкивание
                    if (cpBodyGetType(pObj->body) != CP_BODY_TYPE_STATIC) {
                        Vector2 knockDir = Vector2Normalize(Vector2Subtract(pPos, ePos));
                        cpBodyApplyImpulseAtLocalPoint(pObj->body, ToCp(Vector2Scale(knockDir, 150.0f)), cpvzero);
                    }

                    // Смерть врага
                    if (enemy->health <= 0 && !enemy->destroyFlag) {
                        enemy->destroyFlag = true;
                        pendingEvents.push_back({ 1, ePos, RED });
                        if (pObj->type == EntityType::PLAYER) {
                            auto p = std::dynamic_pointer_cast<Player>(pObj);
                            p->AddXp(enemy->xpReward); p->scrap += enemy->scrapReward; p->kills++;
                        }
                        // Дроп
                        int dropChance = (enemy->enemyType == EnemyType::BOSS) ? 100 : (enemy->enemyType == EnemyType::TANK ? 25 : 5);
                        if (rand() % 100 < dropChance) {
                            newArtifacts.push_back(std::make_shared<Artifact>(nextId++, ePos, space));
                        }
                        break;
                    }
                }
            }
        }

        // 2. МИНЫ (Splash Damage)
        for (auto& [mid, mObj] : objects) {
            if (mObj->type != EntityType::MINE) continue;
            if (mObj->destroyFlag) continue;

            auto mine = std::dynamic_pointer_cast<Mine>(mObj);
            Vector2 mPos = ToRay(cpBodyGetPosition(mine->body));
            bool triggered = false;

            // Проверка триггера
            for (auto& [eid, eObj] : objects) {
                if (eObj->type == EntityType::ENEMY) {
                    auto enemy = std::dynamic_pointer_cast<Enemy>(eObj);

                    // [FIX] Учет радиуса врага при срабатывании мины
                    float enemyRadius = 20.0f;
                    if (enemy->enemyType == EnemyType::TANK) enemyRadius = 35.0f;
                    if (enemy->enemyType == EnemyType::BOSS) enemyRadius = 70.0f;

                    // Если расстояние < (радиус срабатывания + радиус врага), то БУМ
                    if (Vector2Distance(mPos, ToRay(cpBodyGetPosition(eObj->body))) < (mine->triggerRadius + enemyRadius)) {
                        triggered = true;
                        break;
                    }
                }
            }

            if (triggered) {
                mine->destroyFlag = true;
                pendingEvents.push_back({ 1, mPos, ORANGE }); // Взрыв
                pendingEvents.push_back({ 2, mPos, RED });    // Волна (визуал)

                // [FIX] Нанесение урона по площади
                for (auto& [eid, eObj] : objects) {
                    if (eObj->type == EntityType::ENEMY && !eObj->destroyFlag) {
                        float dist = Vector2Distance(mPos, ToRay(cpBodyGetPosition(eObj->body)));

                        // Здесь используем splashRadius. Можно также учесть радиус врага, 
                        // но для взрыва обычно берется центр.
                        if (dist <= mine->splashRadius) {
                            auto enemy = std::dynamic_pointer_cast<Enemy>(eObj);
                            enemy->TakeDamage(mine->damage, currentTime);

                            if (enemy->health <= 0) {
                                enemy->destroyFlag = true;
                                pendingEvents.push_back({ 1, ToRay(cpBodyGetPosition(enemy->body)), RED });

                                // Награда владельцу
                                if (objects.count(mine->ownerId) && objects[mine->ownerId]->type == EntityType::PLAYER) {
                                    auto p = std::dynamic_pointer_cast<Player>(objects[mine->ownerId]);
                                    p->AddXp(enemy->xpReward); p->scrap += enemy->scrapReward; p->kills++;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 3. Сбор артефактов
        for (auto& [aid, aObj] : objects) {
            if (aObj->type != EntityType::ARTIFACT) continue;
            if (aObj->destroyFlag) continue;
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

        // 4. Пули (стандартная логика, без изменений, кроме проверки владельца на существование)
        for (auto& [bid, bObj] : objects) {
            if (bObj->type != EntityType::BULLET) continue;
            if (bObj->destroyFlag) continue;
            auto bullet = std::dynamic_pointer_cast<Bullet>(bObj);
            Vector2 bPos = ToRay(cpBodyGetPosition(bullet->body));

            // Удаление пуль за границей
            if (bPos.x <= -50 || bPos.x >= width + 50 || bPos.y <= -50 || bPos.y >= height + 50) {
                bullet->destroyFlag = true; continue;
            }

            for (auto& [tid, tObj] : objects) {
                if (tid == bullet->ownerId || tObj->type == EntityType::BULLET || tObj->type == EntityType::ARTIFACT || tObj->type == EntityType::MINE) continue;
                if (tObj->destroyFlag || tObj->health <= 0) continue;

                // Friendly fire check
                if (objects.count(bullet->ownerId)) {
                    if (objects[bullet->ownerId]->type == EntityType::PLAYER && (tObj->type == EntityType::TURRET || tObj->type == EntityType::WALL)) continue;
                }

                float targetRadius = 25.0f;
                if (tObj->type == EntityType::WALL) targetRadius = 35.0f;
                else if (tObj->type == EntityType::ENEMY) {
                    auto e = std::dynamic_pointer_cast<Enemy>(tObj);
                    if (e->enemyType == EnemyType::BOSS) targetRadius = 75.0f;
                    else if (e->enemyType == EnemyType::TANK) targetRadius = 40.0f;
                    else targetRadius = 30.0f;
                }

                if (Vector2Distance(bPos, ToRay(cpBodyGetPosition(tObj->body))) <= targetRadius) {
                    bullet->destroyFlag = true;
                    pendingEvents.push_back({ 0, bPos, WHITE }); // Hit effect

                    float dmg = 10.0f;
                    if (objects.count(bullet->ownerId)) {
                        if (objects[bullet->ownerId]->type == EntityType::PLAYER) {
                            auto p = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]); dmg = p->curDamage;
                        }
                        else if (objects[bullet->ownerId]->type == EntityType::TURRET) {
                            auto t = std::dynamic_pointer_cast<Turret>(objects[bullet->ownerId]); dmg = t->damage;
                        }
                    }

                    tObj->TakeDamage(dmg, currentTime);

                    if (tObj->health <= 0) {
                        Color debrisCol = GRAY; if (tObj->type == EntityType::ENEMY) debrisCol = RED;
                        pendingEvents.push_back({ 1, ToRay(cpBodyGetPosition(tObj->body)), debrisCol });

                        if (tObj->type == EntityType::PLAYER) {
                            auto p = std::dynamic_pointer_cast<Player>(tObj); p->Reset();
                            cpBodySetPosition(p->body, cpv(rand() % (int)width, rand() % (int)height));
                        }
                        else if (tObj->type == EntityType::ENEMY) {
                            tObj->destroyFlag = true; auto e = std::dynamic_pointer_cast<Enemy>(tObj);

                            // Награда всем игрокам или владельцу пули (здесь всем для простоты, или владельцу)
                            if (objects.count(bullet->ownerId) && objects[bullet->ownerId]->type == EntityType::PLAYER) {
                                auto p = std::dynamic_pointer_cast<Player>(objects[bullet->ownerId]);
                                p->AddXp(e->xpReward); p->scrap += e->scrapReward; p->kills++;
                            }

                            int dropChance = (e->enemyType == EnemyType::BOSS) ? 100 : (e->enemyType == EnemyType::TANK ? 25 : 5);
                            if (rand() % 100 < dropChance) newArtifacts.push_back(std::make_shared<Artifact>(nextId++, ToRay(cpBodyGetPosition(tObj->body)), space));
                        }
                        else {
                            // Уничтожение постройки
                            tObj->destroyFlag = true;
                        }
                    }
                }
            }
        }
        for (auto& a : newArtifacts) objects[a->id] = a;
    }

};