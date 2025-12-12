#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include "../common/NetworkPackets.h"
#include <iostream>
#include <chrono>

ServerHost::ServerHost() : running(false) {
    netServer = ENetServer::alloc();
}

ServerHost::~ServerHost() {
    Stop();
}

bool ServerHost::Start(int port) {
    if (running) return false;

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

    while (running && netServer->isRunning()) {
        auto currentTime = clock::now();
        std::chrono::duration<double> frameTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += frameTime.count();
        snapshotTimer += frameTime.count();

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
                    PlayerInputPacket input;
                    des.object(input);

                    if (des.adapter().error() == bitsery::ReaderError::NoError) {
                        if (gameScene.objects.count(peerId)) {
                            auto p = std::dynamic_pointer_cast<Player>(gameScene.objects[peerId]);
                            if (p) {
                                p->ApplyInput(input.movement);
                                p->aimTarget = input.aimTarget;
                                p->wantsToShoot = input.isShooting;
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
    snap.serverTime = GetTime();

    for (auto& [id, obj] : gameScene.objects) {
        EntityState state;
        state.id = obj->id;

        if (obj->body) {
            state.position = ToRay(cpBodyGetPosition(obj->body));
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