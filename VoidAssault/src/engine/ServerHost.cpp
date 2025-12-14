#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include "../common/NetworkPackets.h"
#include <iostream>
#include <chrono>
#include <cmath>

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

    while (running && netServer->isRunning()) {
        auto currentTime = clock::now();
        double frameTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        if (frameTime > 0.25) frameTime = 0.25;

        accumulator += frameTime;
        snapshotTimer += frameTime;
        statsTimer += frameTime;

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
                gameScene.objects.erase(peerId);
            }
            else if (msg->type() == MessageType::DATA) {
                const auto& buf = msg->stream()->buffer();
                size_t offset = msg->stream()->tellg();
                if (!buf.empty() && offset < buf.size()) {
                    InputAdapter ia(buf.begin() + offset, buf.end());
                    bitsery::Deserializer<InputAdapter> des(std::move(ia));
                    uint8_t type; des.value1b(type);
                    if (type == GamePacket::INPUT) {
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
                }
            }
        }

        while (accumulator >= dt) {
            gameScene.Update((float)dt);
            accumulator -= dt;
        }

        if (snapshotTimer >= 0.033) {
            BroadcastSnapshot();
            snapshotTimer = 0;
        }

        if (statsTimer >= 0.2) {
            for (auto& [id, obj] : gameScene.objects) {
                if (obj->type == EntityType::PLAYER) {
                    auto p = std::dynamic_pointer_cast<Player>(obj);

                    PlayerStatsPacket stats;
                    stats.level = p->level;
                    stats.currentXp = p->currentXp;
                    stats.maxXp = p->maxXp;
                    stats.maxHealth = p->maxHealth;
                    stats.damage = p->curDamage;
                    stats.speed = p->curSpeed;

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

    for (auto& [id, obj] : gameScene.objects) {
        EntityState state;
        state.id = obj->id;

        if (obj->body) {
            cpVect pos = cpBodyGetPosition(obj->body);
            state.position = ToRay(pos);
            state.rotation = (float)cpBodyGetAngle(obj->body) * RAD2DEG;
        }
        else {
            state.position = { 0,0 }; state.rotation = 0;
        }

        state.health = obj->health;
        state.maxHealth = obj->maxHealth;
        state.type = obj->type;
        state.color = obj->color;

        state.level = 1;
        state.radius = 20.0f;

        if (obj->type == EntityType::PLAYER) {
            auto p = std::dynamic_pointer_cast<Player>(obj);
            if (p) state.level = p->level;
        }
        else if (obj->type == EntityType::BULLET) {
            state.radius = 5.0f;
        }

        snap.entities.push_back(state);
    }

    Buffer buf; OutputAdapter ad(buf); bitsery::Serializer<OutputAdapter> ser(std::move(ad));
    ser.value1b(GamePacket::SNAPSHOT);
    ser.object(snap);
    ser.adapter().flush();
    netServer->broadcast(DeliveryType::UNRELIABLE, StreamBuffer::alloc(buf.data(), buf.size()));
}