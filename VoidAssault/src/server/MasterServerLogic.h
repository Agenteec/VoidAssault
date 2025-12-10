#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <enet.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include "../common/NetworkPackets.h"

struct ActiveLobby {
    ENetAddress address;
    uint16_t gamePort;
    std::string name;
    std::string map;
    int players;
    int maxPlayers;
    double lastHeartbeatTime;
};

class MasterServer {
    ENetHost* host = nullptr;
    std::map<std::string, ActiveLobby> lobbies;

public:
    bool Start(int port) {
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        host = enet_host_create(&address, 32, 2, 0, 0);
        return host != nullptr;
    }

    void Update() {
        if (!host) return;

        ENetEvent event;
        while (enet_host_service(host, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                OnPacket(event.peer, event.packet);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                break;
            }
        }

        double now = GetTime();
        for (auto it = lobbies.begin(); it != lobbies.end();) {
            if (now - it->second.lastHeartbeatTime > 15.0) {
                std::cout << "Server timed out: " << it->second.name << "\n";
                it = lobbies.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void OnPacket(ENetPeer* peer, ENetPacket* packet) {
        if (packet->dataLength < sizeof(PacketHeader)) return;
        PacketHeader* header = (PacketHeader*)packet->data;

        char ipStr[64];
        enet_address_get_host_ip(&peer->address, ipStr, 64);
        std::string key = std::string(ipStr) + ":" + std::to_string(peer->address.port);

        if (header->type == PacketType::master_REGISTER) {
            auto* pkt = (MasterRegisterPacket*)packet->data;
            ActiveLobby lobby;
            lobby.address = peer->address;
            lobby.gamePort = pkt->gamePort;
            lobby.name = pkt->serverName;
            lobby.map = pkt->mapName;
            lobby.maxPlayers = pkt->maxPlayers;
            lobby.players = 0;
            lobby.lastHeartbeatTime = GetTime();

            lobbies[key] = lobby;
            std::cout << "Registered server: " << lobby.name << " at " << ipStr << "\n";
        }
        else if (header->type == PacketType::master_HEARTBEAT) {
            auto* pkt = (MasterHeartbeatPacket*)packet->data;
            if (lobbies.count(key)) {
                lobbies[key].lastHeartbeatTime = GetTime();
                lobbies[key].players = pkt->currentPlayers;
            }
        }
        else if (header->type == PacketType::master_LIST_REQ) {
            SendList(peer);
        }
    }

    void SendList(ENetPeer* peer) {
        size_t dataSize = sizeof(MasterListResponsePacket) + (lobbies.size() * sizeof(LobbyInfo));
        std::vector<uint8_t> buffer(dataSize);

        MasterListResponsePacket* header = (MasterListResponsePacket*)buffer.data();
        header->type = PacketType::master_LIST_RES;
        header->count = (uint16_t)lobbies.size();

        LobbyInfo* infos = (LobbyInfo*)(buffer.data() + sizeof(MasterListResponsePacket));
        int i = 0;
        for (const auto& pair : lobbies) {
            const auto& lobby = pair.second;
            infos[i].id = i;
            infos[i].players = lobby.players;
            infos[i].maxPlayers = lobby.maxPlayers;
            infos[i].port = lobby.gamePort;

            char ipBuf[64];
            enet_address_get_host_ip(&lobby.address, ipBuf, 64);
            strncpy(infos[i].ip, ipBuf, 16);

            strncpy(infos[i].name, lobby.name.c_str(), 31);
            strncpy(infos[i].mapName, lobby.map.c_str(), 31);
            i++;
        }

        ENetPacket* packet = enet_packet_create(buffer.data(), buffer.size(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);
    }

    void Stop() {
        if (host) enet_host_destroy(host);
    }
};