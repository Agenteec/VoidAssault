#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include "../common/NetworkPackets.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <iomanip>

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

    if (!netServer->start(port)) {
        std::cerr << "SERVER: Failed to start on port " << port << std::endl;
        return false;
    }

    std::cout << "SERVER: Started on port " << port << std::endl;
    running = true;
    serverThread = std::thread(&ServerHost::ServerLoop, this);
    return true;
}

void ServerHost::Stop() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.join();
    }
    netServer->stop();
}

void ServerHost::ServerLoop() {
    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();
    double accumulator = 0.0;
    const double dt = 1.0 / 60.0;
    double snapshotTimer = 0.0;

    double debugLogTimer = 0.0;

    std::cout << "SERVER: Loop started. Tick rate: " << dt << std::endl;

    while (running && netServer->isRunning()) {
        auto currentTime = clock::now();
        std::chrono::duration<double> frameTimeWrapper = currentTime - lastTime;
        double frameTime = frameTimeWrapper.count();
        lastTime = currentTime;

        if (std::isnan(frameTime) || frameTime < 0.0) {
            frameTime = 0.001;
            std::cerr << "WARNING: NaN frame time detected!" << std::endl;
        }

        if (frameTime > 0.25) {
            // std::cerr << "WARNING: Lag spike detected: " << frameTime << "s" << std::endl;
            frameTime = 0.25;
        }

        accumulator += frameTime;
        snapshotTimer += frameTime;
        debugLogTimer += frameTime;

        if (debugLogTimer > 5.0) {
            std::cout << "SERVER ALIVE: Time: " << GetSystemTime()
                << " Clients: " << netServer->numClients()
                << " Objects: " << gameScene.objects.size() << std::endl;
            debugLogTimer = 0.0;
        }


        auto msgs = netServer->poll();

        for (auto& msg : msgs) {
            uint32_t peerId = msg->peerId();

            if (msg->type() == MessageType::CONNECT) {
                std::cout << "SERVER: Client " << peerId << " connected.\n";

                auto player = gameScene.CreatePlayerWithId(peerId);

                InitPacket initPkt;
                initPkt.playerId = player->id;

                Buffer buffer;
                OutputAdapter adapter(buffer);
                bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));

                serializer.value1b(GamePacket::INIT);
                serializer.object(initPkt);
                serializer.adapter().flush();

                auto stream = StreamBuffer::alloc(buffer.data(), buffer.size());
                netServer->send(peerId, DeliveryType::RELIABLE, stream);
            }
            else if (msg->type() == MessageType::DISCONNECT) {
                std::cout << "SERVER: Client " << peerId << " disconnected.\n";
                gameScene.objects.erase(peerId);
            }
            else if (msg->type() == MessageType::DATA) {
                const auto& buffer = msg->stream()->buffer();
                size_t offset = msg->stream()->tellg();

                if (buffer.empty() || offset >= buffer.size()) continue;

                InputAdapter adapter(buffer.begin() + offset, buffer.end());
                bitsery::Deserializer<InputAdapter> des(std::move(adapter));

                uint8_t packetType;
                des.value1b(packetType);

                if (packetType == GamePacket::INPUT) {
                    PlayerInputPacket input = {};
                    des.object(input);

                    if (des.adapter().error() == bitsery::ReaderError::NoError) {
                        if (gameScene.objects.count(peerId)) {
                            auto p = std::dynamic_pointer_cast<Player>(gameScene.objects[peerId]);
                            if (p) {
                              
                                if (std::isfinite(input.movement.x) && std::isfinite(input.movement.y)) {
                                    p->ApplyInput(input.movement);
                                    p->aimTarget = input.aimTarget;
                                    p->wantsToShoot = input.isShooting;
                                }
                            }
                        }
                    }
                }
            }
        }


        int physicsSteps = 0;
        while (accumulator >= dt) {
            gameScene.Update((float)dt);
            accumulator -= dt;
            physicsSteps++;

           
            if (physicsSteps > 5) {
                accumulator = 0;
                break;
            }
        }

        if (snapshotTimer >= 0.05) {
            BroadcastSnapshot();
            snapshotTimer = 0;
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
            if (!std::isfinite(pos.x) || !std::isfinite(pos.y)) {
                pos = cpv(0, 0);
                cpBodySetPosition(obj->body, pos);
                cpBodySetVelocity(obj->body, cpv(0, 0));
                std::cerr << "WARN: Resetting body " << id << " due to NaN position" << std::endl;
            }

            state.position = ToRay(pos);
            state.rotation = (float)cpBodyGetAngle(obj->body) * RAD2DEG;
        }
        else {
            state.position = { 0,0 }; state.rotation = 0;
        }

        state.health = obj->health;
        state.maxHealth = obj->maxHealth;
        state.type = obj->type;
        state.radius = (obj->type == EntityType::BULLET) ? 5.0f : 20.0f;
        state.color = obj->color;

        snap.entities.push_back(state);
    }

    Buffer buffer;
    OutputAdapter adapter(buffer);
    bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));

    uint8_t type = GamePacket::SNAPSHOT;
    serializer.value1b(type);
    serializer.object(snap);
    serializer.adapter().flush();

    auto stream = StreamBuffer::alloc(buffer.data(), buffer.size());
    netServer->broadcast(DeliveryType::UNRELIABLE, stream);
}