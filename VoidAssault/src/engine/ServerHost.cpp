#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include <iostream>

ServerHost::ServerHost() : running(false) {}

ServerHost::~ServerHost() {
    Stop();
}

bool ServerHost::Start(int port) {
    if (running) return false;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    server = enet_host_create(&address, 32, 2, 0, 0);
    if (!server) {
        std::cerr << "An error occurred while trying to create an ENet server host.\n";
        return false;
    }

    running = true;
    // Запускаем цикл в отдельном потоке
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
    std::cout << "Server started loop.\n";
    const double dt = 1.0 / 60.0;
    double accumulator = 0.0;
    double lastTime = GetTime();

    while (running) {
        double currentTime = GetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += frameTime;

        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                std::cout << "A new client connected from...\n";// << event.peer->address.host.u.Word << ":" << event.peer->address.port << ".\n";

                auto player = gameScene.CreatePlayer();

                event.peer->data = (void*)(uintptr_t)player->id;

                InitPacket initPkt;
                initPkt.type = PacketType::server_INIT;
                initPkt.yourPlayerId = player->id;

                ENetPacket* packet = enet_packet_create(&initPkt, sizeof(InitPacket), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, 0, packet);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
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
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                uint32_t playerId = (uint32_t)(uintptr_t)event.peer->data;
                gameScene.objects.erase(playerId);
                std::cout << "Client disconnected.\n";
            }
        }

        // Физический апдейт
        while (accumulator >= dt) {
            gameScene.Update((float)dt);
            accumulator -= dt;
        }

        // Рассылка снапшота (раз в 2 тика, например, или каждый тик)
        static double snapshotTimer = 0;
        snapshotTimer += frameTime;
        if (snapshotTimer > 0.05) { // 20 раз в секунду
            snapshotTimer = 0;
            auto data = gameScene.SerializeSnapshot();

            ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_UNSEQUENCED);
            enet_host_broadcast(server, 1, packet);
        }

        // Небольшой sleep, чтобы не грузить CPU на 100% в пустом цикле
        WaitTime(0.001);
    }
}