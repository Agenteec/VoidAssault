#pragma once
#include <enet.h>
#include <string>
#include <vector>
#include <iostream>
#include "../common/NetworkPackets.h"

class NetworkManager {
public:
    ENetHost* clientHost = nullptr;
    ENetPeer* serverPeer = nullptr;
    bool isConnected = false;

    bool Init() {
        if (clientHost) return true;
        if (enet_initialize() != 0) return false;

        clientHost = enet_host_create(NULL, 32, 2, 0, 0);
        return (clientHost != nullptr);
    }

    void Connect(const std::string& ip, int port) {
        if (!Init()) return;

        if (serverPeer) {
            enet_peer_disconnect_now(serverPeer, 0);
            serverPeer = nullptr;
        }
        isConnected = false;

        ENetAddress address;
        enet_address_set_host(&address, ip.c_str());
        address.port = (enet_uint16)port;

        std::cout << "CLIENT: Connecting to " << ip << ":" << port << "...\n";

        serverPeer = enet_host_connect(clientHost, &address, 2, 0);

        if (serverPeer) {
            enet_host_flush(clientHost);
        }
        else {
            std::cout << "CLIENT: Failed to create peer.\n";
        }
    }

    void Disconnect() {
        if (serverPeer) {
            enet_peer_disconnect(serverPeer, 0);
            enet_host_flush(clientHost);
            serverPeer = nullptr;
        }
        isConnected = false;
    }

    void SendInput(const PlayerInputPacket& input) {
        if (!isConnected || !serverPeer) return;
        ENetPacket* packet = enet_packet_create(&input, sizeof(PlayerInputPacket), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(serverPeer, 0, packet);
    }

    void Shutdown() {
        if (clientHost) enet_host_destroy(clientHost);
    }
};