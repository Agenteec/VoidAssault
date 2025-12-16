#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include "../common/NetworkPackets.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include "Utils/ConfigManager.h"
#if defined(__linux__) || defined(__APPLE__)
#include <signal.h>
#endif

double GetSystemTime() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - start;
    return diff.count();
}

ServerHost::ServerHost() : running(false) {
    netServer = ENetServer::alloc();
}

ServerHost::~ServerHost() {
    Stop();
}

bool ServerHost::Start(int port) {
    if (running) return false;

#if defined(__linux__) || defined(__APPLE__)
    signal(SIGPIPE, SIG_IGN);
#endif

    gameScene.pvpFactor = ConfigManager::GetServer().pvpDamageFactor;
    std::cout << "SERVER: PvP Damage Factor set to " << gameScene.pvpFactor << "\n";

    if (!netServer->start(port)) return false;

    std::cout << "SERVER: Started on port " << port << std::endl;
    running = true;
    serverThread = std::thread(&ServerHost::ServerLoop, this);
    return true;
}

void ServerHost::Stop() {
    running = false;
    if (serverThread.joinable()) serverThread.join();
    netServer->stop();
}

void ServerHost::ServerLoop() {
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();
    double accumulator = 0.0;
    const double dt = 1.0 / 60.0;
    double snapshotTimer = 0.0;
    double statsTimer = 0.0;

    waveCount = 1;
    waveTimer = 0.0;
    timeToNextWave = 5.0;

    while (running && netServer->isRunning()) {
        auto currentTime = clock::now();
        double frameTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

                if (frameTime > 0.25) frameTime = 0.25;

                if (netServer->numClients() == 0) {
            bool hasEntities = false;
            for (auto& pair : gameScene.objects) {
                if (pair.second->type != EntityType::PLAYER && pair.second->type != EntityType::WALL) {
                    hasEntities = true; break;
                }
            }

            if (waveCount > 1 || hasEntities) {
                auto it = gameScene.objects.begin();
                while (it != gameScene.objects.end()) {
                    if (it->second->type == EntityType::ENEMY ||
                        it->second->type == EntityType::BULLET ||
                        it->second->type == EntityType::ARTIFACT) {
                        it = gameScene.objects.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
                waveCount = 1;
                waveTimer = 0;
                timeToNextWave = 5.0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

                        auto msgs = netServer->poll();
            for (auto& msg : msgs) {
                if (msg->type() == MessageType::CONNECT) {
                    uint32_t peerId = msg->peerId();
                    std::cout << "Client " << peerId << " connected (waking up).\n";
                    auto player = gameScene.CreatePlayerWithId(peerId);

                    InitPacket initPkt; initPkt.playerId = player->id;
                    Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
                    ser.value1b(GamePacket::INIT); ser.object(initPkt); ser.adapter().flush();
                    netServer->send(peerId, DeliveryType::RELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
                }
            }
            continue;
        }

        accumulator += frameTime;
        snapshotTimer += frameTime;
        statsTimer += frameTime;
        waveTimer += frameTime;

        if (waveTimer >= timeToNextWave) {
            waveTimer = 0.0;
            timeToNextWave = 20.0 + (waveCount * 2.0);

            int currentEnemies = 0;
            for (auto& [id, obj] : gameScene.objects) { if (obj->type == EntityType::ENEMY) currentEnemies++; }

            if (currentEnemies < 120) {
                int enemiesToSpawn = 5 + (waveCount * 2);
                if (enemiesToSpawn > 60) enemiesToSpawn = 60;

                std::cout << "SERVER: Spawning Wave " << waveCount << " (" << enemiesToSpawn << " enemies)\n";
                for (int i = 0; i < enemiesToSpawn; i++) {
                    gameScene.SpawnEnemy();
                }

                if (waveCount % 5 == 0) {
                    gameScene.SpawnEnemy(EnemyType::BOSS);
                }

                waveCount++;
            }
        }

        auto msgs = netServer->poll();
        for (auto& msg : msgs) {
            uint32_t peerId = msg->peerId();

            if (msg->type() == MessageType::CONNECT) {
                std::cout << "Client " << peerId << " connected.\n";
                auto player = gameScene.CreatePlayerWithId(peerId);

                InitPacket initPkt; initPkt.playerId = player->id;
                Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
                ser.value1b(GamePacket::INIT); ser.object(initPkt); ser.adapter().flush();
                netServer->send(peerId, DeliveryType::RELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
            }
            else if (msg->type() == MessageType::DISCONNECT) {
                std::cout << "Client " << peerId << " disconnected.\n";
                gameScene.objects.erase(peerId);
            }
            else if (msg->type() == MessageType::DATA) {
                const auto& buf = msg->stream()->buffer();
                size_t offset = msg->stream()->tellg();
                if (!buf.empty() && offset < buf.size()) {
                    InputAdapter ia(buf.begin() + offset, buf.end());
                    bitsery::Deserializer<InputAdapter> des(std::move(ia));
                    uint8_t type; des.value1b(type);

                    if (type == GamePacket::JOIN) {
                        JoinPacket pkt; des.object(pkt);
                        if (des.adapter().error() == bitsery::ReaderError::NoError) {
                            if (gameScene.objects.count(peerId)) {
                                auto p = std::dynamic_pointer_cast<Player>(gameScene.objects[peerId]);
                                if (p) {
                                    p->name = pkt.name;
                                    std::cout << "Player " << peerId << " set name: " << p->name << "\n";
                                }
                            }
                        }
                    }
                    else if (type == GamePacket::INPUT) {
                        PlayerInputPacket inp; des.object(inp);
                        if (des.adapter().error() == bitsery::ReaderError::NoError) {
                            if (gameScene.objects.count(peerId)) {
                                auto p = std::dynamic_pointer_cast<Player>(gameScene.objects[peerId]);
                                if (p) {
                                    p->ApplyInput(inp.movement);
                                    p->aimTarget = inp.aimTarget;
                                    p->wantsToShoot = inp.isShooting;
                                }
                            }
                        }
                    }
                    else if (type == GamePacket::ACTION) {
                        ActionPacket act; des.object(act);
                        if (des.adapter().error() == bitsery::ReaderError::NoError) {
                            if (gameScene.objects.count(peerId)) {
                                if (act.type == ActionType::UPGRADE_BUILDING) {
                                    gameScene.TryUpgrade(peerId, act.target);
                                }
                                else {
                                    gameScene.TryBuild(peerId, act.type, act.target);
                                }
                            }
                        }
                    }
                    else if (type == GamePacket::ADMIN_CMD) {
                        AdminCommandPacket pkt; des.object(pkt);
                        if (des.adapter().error() == bitsery::ReaderError::NoError) {
                            gameScene.HandleAdminCommand(peerId, pkt);
                        }
                    }
                }
            }
        }

                        int maxPhysicsSteps = 5;
        int steps = 0;
        while (accumulator >= dt && steps < maxPhysicsSteps) {
            gameScene.Update((float)dt);
            accumulator -= dt;
            steps++;
        }
        if (accumulator > dt) accumulator = 0.0; 
        if (!gameScene.pendingEvents.empty()) {
            for (const auto& evt : gameScene.pendingEvents) {
                Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
                ser.value1b(GamePacket::EVENT);
                ser.object(evt);
                ser.adapter().flush();
                netServer->broadcast(DeliveryType::UNRELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
            }
            gameScene.pendingEvents.clear();
        }

        if (snapshotTimer >= 0.033) {             BroadcastSnapshot();
            snapshotTimer = 0;
        }

        if (statsTimer >= 0.2) {             for (auto& [id, obj] : gameScene.objects) {
                if (obj->type == EntityType::PLAYER) {
                    auto p = std::dynamic_pointer_cast<Player>(obj);

                    PlayerStatsPacket stats;
                    stats.level = p->level;
                    stats.currentXp = p->currentXp;
                    stats.maxXp = p->maxXp;
                    stats.maxHealth = p->maxHealth;
                    stats.damage = p->curDamage;
                    stats.speed = p->curSpeed;
                    stats.scrap = p->scrap;
                    stats.kills = p->kills;
                    stats.inventory.assign(std::begin(p->inventory), std::end(p->inventory));
                    stats.isAdmin = p->isAdmin;

                    Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
                    ser.value1b(GamePacket::STATS);
                    ser.object(stats);
                    ser.adapter().flush();

                    netServer->send(id, DeliveryType::UNRELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
                }
            }
            statsTimer = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void ServerHost::BroadcastSnapshot() {
    if (netServer->numClients() == 0) return;

    WorldSnapshotPacket snap;
    snap.serverTime = GetSystemTime();
    snap.wave = waveCount;

    for (auto& [id, obj] : gameScene.objects) {
        EntityState state;
        state.id = obj->id;

        if (obj->body) {
            cpVect pos = cpBodyGetPosition(obj->body);
            state.position = ToRay(pos);
            state.rotation = obj->rotation;
        }
        else {
            state.position = { 0,0 };
            state.rotation = 0;
        }

        state.health = obj->health;
        state.maxHealth = obj->maxHealth;
        state.type = obj->type;
        state.color = obj->color;

        state.level = 1;
        state.kills = 0;
        state.radius = 20.0f;
        state.subtype = 0;
        state.ownerId = 0;

        if (obj->type == EntityType::PLAYER) {
            auto p = std::dynamic_pointer_cast<Player>(obj);
            if (p) {
                state.level = p->level;
                state.kills = p->kills;
                state.name = p->name;
            }
        }
        else if (obj->type == EntityType::BULLET) {
            state.radius = 5.0f;
        }
        else if (obj->type == EntityType::ENEMY) {
            auto e = std::dynamic_pointer_cast<Enemy>(obj);
            if (e) state.subtype = e->enemyType;
        }
        else if (obj->type == EntityType::WALL) {
            state.radius = 25.0f;
            auto c = std::dynamic_pointer_cast<Construct>(obj);
            if (c) { state.ownerId = c->ownerId; state.level = c->level; }
        }
        else if (obj->type == EntityType::TURRET) {
            state.radius = 20.0f;
            auto c = std::dynamic_pointer_cast<Construct>(obj);
            if (c) { state.ownerId = c->ownerId; state.level = c->level; }
        }
        else if (obj->type == EntityType::MINE) {
            state.radius = 15.0f;
            auto c = std::dynamic_pointer_cast<Construct>(obj);
            if (c) { state.ownerId = c->ownerId; state.level = c->level; }
        }

        snap.entities.push_back(state);
    }

    Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
    ser.value1b(GamePacket::SNAPSHOT);
    ser.object(snap);
    ser.adapter().flush();
    netServer->broadcast(DeliveryType::UNRELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
}