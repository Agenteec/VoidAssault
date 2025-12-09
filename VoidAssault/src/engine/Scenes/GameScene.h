#pragma once
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include "../ECS/Player.h"
#include "../ECS/Bullet.h"
#include "../PhysicsUtils.h"

class GameScene {
public:
    cpSpace* space;
    std::map<uint32_t, std::shared_ptr<GameObject>> objects;
    uint32_t nextId = 1;
    float width = 2000;
    float height = 2000;

    GameScene() {
        space = cpSpaceNew();
        // Гравитации нет
        cpSpaceSetGravity(space, cpv(0, 0));

        CreateMapBoundaries();
    }

    ~GameScene() {
        // Сначала очищаем объекты (их деструкторы удалят тела из space)
        objects.clear();
        // Потом удаляем space
        cpSpaceFree(space);
    }

    void CreateMapBoundaries() {
        // Статическое тело для стен (не добавляем его в space, только shape)
        cpBody* staticBody = cpSpaceGetStaticBody(space);

        float w = width;
        float h = height;
        float thick = 20.0f;

        // 4 Стены (Segment Shapes)
        auto addWall = [&](Vector2 a, Vector2 b) {
            cpShape* wall = cpSpaceAddShape(space, cpSegmentShapeNew(staticBody, ToCp(a), ToCp(b), thick));
            cpShapeSetElasticity(wall, 0.5f);
            cpShapeSetFriction(wall, 1.0f);
            cpShapeSetCollisionType(wall, COLLISION_WALL);
            };

        addWall({ 0,0 }, { w, 0 });      // Top
        addWall({ 0,h }, { w, h });      // Bottom
        addWall({ 0,0 }, { 0, h });      // Left
        addWall({ w,0 }, { w, h });      // Right
    }

    std::shared_ptr<Player> CreatePlayer() {
        Vector2 startPos = { width / 2, height / 2 };
        auto p = std::make_shared<Player>(nextId++, startPos, space);
        objects[p->id] = p;
        return p;
    }

    void Update(float dt) {
        // Шаг физики (фиксированный шаг для стабильности)
        cpSpaceStep(space, dt);

        // Логика объектов и спавн пуль
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
                it = objects.erase(it); // Деструктор GameObject очистит Chipmunk ресурсы
            }
            else {
                ++it;
            }
        }

        for (auto& b : newBullets) objects[b->id] = b;

        // Обработка урона (Manual Query)
        // Chipmunk имеет систему коллбеков, но для совместимости с вашей логикой
        // сделаем простую проверку расстояний, но физика отталкивания будет от Chipmunk.
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

                // Простая проверка радиуса (20 танк + 5 пуля)
                if (Vector2Distance(bPos, pPos) < 25.0f) {
                    pObj->health -= 10;
                    bullet->destroyFlag = true;

                    if (pObj->health <= 0) {
                        // Respawn
                        pObj->health = 100;
                        cpVect randPos = cpv(GetRandomValue(100, (int)width - 100), GetRandomValue(100, (int)height - 100));
                        cpBodySetPosition(pObj->body, randPos);
                        cpBodySetVelocity(pObj->body, cpvzero);
                    }
                }
            }
        }
    }

    std::vector<uint8_t> SerializeSnapshot() {
        size_t size = sizeof(WorldSnapshotPacket) + (objects.size() * sizeof(EntityState));
        std::vector<uint8_t> buffer(size);

        WorldSnapshotPacket* header = (WorldSnapshotPacket*)buffer.data();
        header->type = PacketType::server_SNAPSHOT;
        header->entityCount = (uint32_t)objects.size();

        EntityState* states = (EntityState*)(buffer.data() + sizeof(WorldSnapshotPacket));
        int i = 0;
        for (auto& [id, obj] : objects) {
            states[i].id = obj->id;

            // Берем координаты из Chipmunk
            if (obj->body) {
                states[i].position = ToRay(cpBodyGetPosition(obj->body));
                states[i].rotation = (float)cpBodyGetAngle(obj->body) * RAD2DEG;
            }

            states[i].health = obj->health;
            states[i].maxHealth = obj->maxHealth;
            states[i].type = obj->type;
            states[i].radius = (obj->type == EntityType::BULLET) ? 5.0f : 20.0f;
            states[i].color = obj->color;
            i++;
        }
        return buffer;
    }
};