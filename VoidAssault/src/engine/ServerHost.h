#pragma once
#if defined(_WIN32)
#include "fix_win32_compatibility.h"
#endif

#include "enet/ENetServer.h"
#include "enet/ENetClient.h"
#include "Scenes/GameScene.h"
#include <thread>
#include <atomic>

class ServerHost {
    ENetServer::Shared netServer;
    GameScene gameScene;

    std::vector<uint32_t> relayClientIds;

    ENetClient::Shared masterClient;
    double masterHeartbeatTimer = 0.0;
    bool connectedToMaster = false;
    bool useMasterServer = false;

    std::atomic<bool> running{ false };
    std::thread serverThread;

    double waveTimer = 0.0;
    double timeToNextWave = 5.0;
    int waveCount = 1;

public:
    ServerHost();
    ~ServerHost();
    bool Start(int port, bool registerOnMaster);
    void Stop();

    void ServerLoop();
    void BroadcastSnapshot();


private:
    void RegisterWithMaster();
    void UpdateMasterHeartbeat(float dt);
    void ProcessGamePacket(uint32_t peerId, StreamBuffer::Shared stream);
    void BroadcastToAll(DeliveryType type, StreamBuffer::Shared stream);
    void SendToClient(uint32_t peerId, DeliveryType type, StreamBuffer::Shared stream);
};