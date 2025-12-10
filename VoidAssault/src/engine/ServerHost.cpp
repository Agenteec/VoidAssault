#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

ServerHost::ServerHost() : running(false) {}

ServerHost::~ServerHost() {
    Stop();
}

bool ServerHost::Start(int port) {
    if (running) return false;

    if (enet_initialize() != 0) {
        std::cerr << "SERVER: ENet init failed.\n";
        return false;
    }

    ENetAddress address;
    memset(&address, 0, sizeof(address));
    address.host = ENET_HOST_ANY;
    address.port = (enet_uint16)port;

    server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server) {
        std::cerr << "SERVER: Failed to bind port " << port << "!\n";
        return false;
    }

    std::cout << "SERVER: Started on port " << port << "\n";

    running = true;
    serverThread = std::thread(&ServerHost::ServerLoop, this);

    return true;
}

void ServerHost::Stop() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.join();
    }
    if (server) {
        enet_host_destroy(server);
        server = nullptr;
    }
}

void ServerHost::ServerLoop() {
    std::cout << "SERVER: Event loop running.\n";

    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();
    double accumulator = 0.0;
    const double dt = 1.0 / 60.0;

    while (running) {
        auto currentTime = clock::now();
        std::chrono::duration<double> frameTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += frameTime.count();

        ENetEvent event;
        while (enet_host_service(server, &event, 15) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                std::cout << "SERVER: Client connected.\n";

                auto player = gameScene.CreatePlayer();
                event.peer->data = (void*)(uintptr_t)player->id;

                InitPacket initPkt;
                initPkt.type = PacketType::server_INIT;
                initPkt.yourPlayerId = player->id;

                ENetPacket* packet = enet_packet_create(&initPkt, sizeof(InitPacket), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, 0, packet);

                enet_host_flush(server);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength >= sizeof(PacketHeader)) {
                    PacketHeader* header = (PacketHeader*)event.packet->data;
                    if (header->type == PacketType::client_INPUT) {
                        PlayerInputPacket* input = (PlayerInputPacket*)event.packet->data;
                        uint32_t playerId = (uint32_t)(uintptr_t)event.peer->data;

                        if (gameScene.objects.count(playerId)) {
                            auto p = std::dynamic_pointer_cast<Player>(gameScene.objects[playerId]);
                            if (p) {
                                p->ApplyInput(input->movement);
                                p->aimTarget = input->aimTarget;
                                p->wantsToShoot = input->isShooting;
                            }
                        }
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.peer->data) {
                    uint32_t playerId = (uint32_t)(uintptr_t)event.peer->data;
                    gameScene.objects.erase(playerId);
                }
                std::cout << "SERVER: Client disconnected.\n";
                break;
            }
        }

        while (accumulator >= dt) {
            gameScene.Update((float)dt);
            accumulator -= dt;
        }

        static double snapshotTimer = 0;
        snapshotTimer += frameTime.count();
        if (snapshotTimer > 0.05) {
            snapshotTimer = 0;
            auto data = gameScene.SerializeSnapshot();
            ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_UNSEQUENCED);
            enet_host_broadcast(server, 1, packet);
        }
    }
}