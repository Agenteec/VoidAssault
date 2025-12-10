#pragma once
#include <enet.h>
#include <string>
#include <iostream>
#include <vector> // Исправлено: C2039
#include "../common/NetworkPackets.h"
#include "../engine/Utils/ConfigManager.h" // Исправлено: чтобы видеть ConfigManager

class NetworkManager {
public:
    ENetHost* clientHost = nullptr;
    ENetPeer* serverPeer = nullptr;
    bool isConnected = false;

    std::vector<LobbyInfo> lobbyList;
    bool isWaitingForList = false;

    // Метод ScanLAN и поле foundServers удалены, так как 
    // используется Master Server подход (по требованию)

    bool Init() {
        if (enet_initialize() != 0) return false;
        clientHost = enet_host_create(NULL, 1, 2, 0, 0);
        return (clientHost != nullptr);
    }

    void RequestServerList() {
        if (!Init()) return;

        std::string ip = ConfigManager::GetClient().masterServerIp;
        int port = ConfigManager::GetClient().masterServerPort;

        Connect(ip, port);
        isWaitingForList = true;
    }

    void Connect(const std::string& ip, int port) {
        if (!clientHost) Init(); // Гарантируем инициализацию

        ENetAddress address;
        enet_address_set_host(&address, ip.c_str());
        address.port = port;
        serverPeer = enet_host_connect(clientHost, &address, 2, 0);
    }

    void Disconnect() {
        if (serverPeer) {
            enet_peer_disconnect(serverPeer, 0);
            enet_host_flush(clientHost);
            serverPeer = nullptr;
        }
        isConnected = false;
    }

    void Update() {
        if (!clientHost) return;

        ENetEvent event;
        while (enet_host_service(clientHost, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (isWaitingForList) {
                    // Как только подключились к мастеру - просим список
                    MasterListRequestPacket req;
                    req.type = PacketType::master_LIST_REQ;
                    ENetPacket* p = enet_packet_create(&req, sizeof(req), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(serverPeer, 0, p);
                }
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (isWaitingForList) {
                    PacketHeader* h = (PacketHeader*)event.packet->data;
                    if (h->type == PacketType::master_LIST_RES) {
                        MasterListResponsePacket* res = (MasterListResponsePacket*)event.packet->data;
                        lobbyList.clear();

                        LobbyInfo* infos = (LobbyInfo*)(event.packet->data + sizeof(MasterListResponsePacket));
                        for (int i = 0; i < res->count; i++) {
                            lobbyList.push_back(infos[i]);
                        }

                        Disconnect();
                        isWaitingForList = false;
                    }
                    enet_packet_destroy(event.packet);
                }
                break;
            }
        }
    }

    void SendInput(const PlayerInputPacket& input) {
        if (!isConnected || !serverPeer) return;
        ENetPacket* packet = enet_packet_create(&input, sizeof(PlayerInputPacket), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(serverPeer, 0, packet);
    }

    void Shutdown() {
        if (clientHost) enet_host_destroy(clientHost);
        enet_deinitialize();
    }
};