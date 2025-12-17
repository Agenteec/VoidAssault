#include "../common/enet/ENetServer.h"
#include "../common/NetworkPackets.h"
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>

struct ActiveLobby {
    uint32_t id;
    std::string ip;
    uint16_t port;
    std::string name;
    uint8_t currentPlayers;
    uint8_t maxPlayers;
    uint8_t wave;
    double lastHeartbeatTime;
    uint32_t peerId;
};

std::map<uint32_t, ActiveLobby> lobbies;
std::mutex lobbyMutex;
uint32_t nextLobbyId = 1;

double GetTime() {
    return std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv) {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet.\n";
        return 1;
    }

    ENetServer::Shared server = ENetServer::alloc();
    int port = 8080;
    if (server->start(port, 1024)) {
        std::cout << "MASTER SERVER: Started on port " << port << std::endl;
    }
    else {
        std::cerr << "MASTER SERVER: Failed to start on port " << port << std::endl;
        return -1;
    }

    while (server->isRunning()) {
        auto msgs = server->poll();

        double now = GetTime();

        for (auto& msg : msgs) {
            uint32_t peerId = msg->peerId();

            if (msg->type() == MessageType::DISCONNECT) {
                std::lock_guard<std::mutex> lock(lobbyMutex);
                for (auto it = lobbies.begin(); it != lobbies.end(); ) {
                    if (it->second.peerId == peerId) {
                        std::cout << "Lobby removed (Disconnect): " << it->second.name << "\n";
                        it = lobbies.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
            else if (msg->type() == MessageType::DATA) {
                const auto& buf = msg->stream()->buffer();
                size_t offset = msg->stream()->tellg();
                if (!buf.empty() && offset < buf.size()) {
                    InputAdapter ia(buf.begin() + offset, buf.end());
                    bitsery::Deserializer<InputAdapter> des(std::move(ia));
                    uint8_t type; des.value1b(type);

                    if (type == GamePacket::MASTER_REGISTER) {
                        MasterRegisterPacket pkt; des.object(pkt);
                        if (des.adapter().error() == bitsery::ReaderError::NoError) {
                            std::lock_guard<std::mutex> lock(lobbyMutex);


                            ActiveLobby lobby;
                            lobby.id = nextLobbyId++;
                            lobby.peerId = peerId;
                            lobby.port = pkt.gamePort;
                            lobby.name = pkt.serverName;
                            lobby.maxPlayers = pkt.maxPlayers;
                            lobby.currentPlayers = 0;
                            lobby.wave = 1;
                            lobby.lastHeartbeatTime = now;

                            lobby.ip = "Unknown";

                            lobbies[lobby.id] = lobby;
                            std::cout << "Lobby Registered: " << lobby.name << " (ID: " << lobby.id << ")\n";
                        }
                    }
                    else if (type == GamePacket::MASTER_HEARTBEAT) {
                        MasterHeartbeatPacket pkt; des.object(pkt);
                        if (des.adapter().error() == bitsery::ReaderError::NoError) {
                            std::lock_guard<std::mutex> lock(lobbyMutex);
                            for (auto& pair : lobbies) {
                                if (pair.second.peerId == peerId) {
                                    pair.second.currentPlayers = pkt.currentPlayers;
                                    pair.second.wave = pkt.wave;
                                    pair.second.lastHeartbeatTime = now;
                                    break;
                                }
                            }
                        }
                    }
                    else if (type == GamePacket::MASTER_LIST_REQ) {
                        std::lock_guard<std::mutex> lock(lobbyMutex);
                        MasterListResPacket res;
                        for (const auto& pair : lobbies) {
                            LobbyInfo info;
                            info.id = pair.second.id;
                            info.name = pair.second.name;
                            info.ip = pair.second.ip;
                            info.port = pair.second.port;
                            info.currentPlayers = pair.second.currentPlayers;
                            info.maxPlayers = pair.second.maxPlayers;
                            info.wave = pair.second.wave;
                            res.lobbies.push_back(info);
                        }

                        Buffer resBuf; OutputAdapter resAd(resBuf);
                        bitsery::Serializer<OutputAdapter> ser(std::move(resAd));
                        ser.value1b(GamePacket::MASTER_LIST_RES);
                        ser.object(res);
                        ser.adapter().flush();
                        server->send(peerId, DeliveryType::RELIABLE, StreamBuffer::alloc(resBuf.data(), resBuf.size()));

                    }
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(lobbyMutex);
            for (auto it = lobbies.begin(); it != lobbies.end(); ) {
                if (now - it->second.lastHeartbeatTime > 30.0) {
                    std::cout << "Lobby timed out: " << it->second.name << "\n";
                    it = lobbies.erase(it);
                }
                else {
                    ++it;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    enet_deinitialize();
    return 0;
}