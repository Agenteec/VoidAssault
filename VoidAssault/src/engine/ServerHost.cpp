#define _CRT_SECURE_NO_WARNINGS
#include "ServerHost.h"
#include "Utils/ConfigManager.h"
#include <iostream>
#include <cstring> 
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
    serverThread = std::thread(&ServerHost::ServerLoop, this);
    ConnectToMaster();
    return true;
}

void ServerHost::ConnectToMaster() {
    masterClientHost = enet_host_create(NULL, 1, 2, 0, 0);
    if (!masterClientHost) return;

    ENetAddress addr;
    std::string ip = ConfigManager::GetClient().masterServerIp;
    int port = ConfigManager::GetClient().masterServerPort;

    enet_address_set_host(&addr, ip.c_str());
    addr.port = port;

    masterPeer = enet_host_connect(masterClientHost, &addr, 2, 0);
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
    // Очистка мастера
    if (masterClientHost) {
        enet_host_destroy(masterClientHost);
        masterClientHost = nullptr;
    }
}


void ServerHost::ServerLoop() {
    std::cout << "Server started loop on port " << server->address.port << ".\n";
    const double dt = 1.0 / 60.0;
    double accumulator = 0.0;
    double lastTime = GetTime();

    while (running) {
        double currentTime = GetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += frameTime;

        // --- Master Server Events ---
        if (masterClientHost) {
            ENetEvent mEvent;
            while (enet_host_service(masterClientHost, &mEvent, 0) > 0) {
                if (mEvent.type == ENET_EVENT_TYPE_CONNECT) {
                    std::cout << "Connected to Master Server!\n";
                    MasterRegisterPacket reg;
                    reg.type = PacketType::master_REGISTER;
                    reg.gamePort = server->address.port;
                    reg.maxPlayers = 8;
                    // Исправлено: strncpy
                    strncpy(reg.serverName, ConfigManager::GetServer().serverName.c_str(), 31);
                    strncpy(reg.mapName, "Map 1", 31);

                    ENetPacket* p = enet_packet_create(&reg, sizeof(reg), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(masterPeer, 0, p);
                }
                // Disconnect handling...
            }

            // Heartbeat
            if (GetTime() - lastHeartbeatTime > 5.0 && masterPeer) {
                lastHeartbeatTime = GetTime();
                MasterHeartbeatPacket hb;
                hb.type = PacketType::master_HEARTBEAT;
                hb.currentPlayers = (uint8_t)gameScene.objects.size();
                ENetPacket* p = enet_packet_create(&hb, sizeof(hb), ENET_PACKET_FLAG_UNSEQUENCED);
                enet_peer_send(masterPeer, 0, p);
            }
        }

        // --- Game Server Events ---
        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                std::cout << "Client connected.\n";
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
                if (event.packet->dataLength < sizeof(PacketHeader)) break;
                PacketHeader* header = (PacketHeader*)event.packet->data;

                if (header->type == PacketType::client_PING) {
                    PingPacket* pingIn = (PingPacket*)event.packet->data;
                    PingPacket pongOut;
                    pongOut.type = PacketType::server_PONG; // Теперь определено
                    pongOut.timestamp = pingIn->timestamp;
                    ENetPacket* p = enet_packet_create(&pongOut, sizeof(PingPacket), ENET_PACKET_FLAG_UNSEQUENCED);
                    enet_peer_send(event.peer, 0, p);
                }
                else if (header->type == PacketType::client_INPUT) {
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
                break;
            }
        }

        // Physics Step
        while (accumulator >= dt) {
            gameScene.Update((float)dt);
            accumulator -= dt;
        }

        // Snapshots
        static double snapshotTimer = 0;
        snapshotTimer += frameTime;
        if (snapshotTimer > 0.05) {
            auto data = gameScene.SerializeSnapshot();
            ENetPacket* packet = enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_UNSEQUENCED);
            enet_host_broadcast(server, 1, packet);
        }

        WaitTime(0.001);
    }
}