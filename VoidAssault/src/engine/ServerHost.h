#pragma once
#if defined(_WIN32)
#include "fix_win32_compatibility.h"
#endif

#include <enet.h>

#include <thread>
#include <atomic>
#include "Scenes/GameScene.h"

class ServerHost {
    ENetHost* server = nullptr;
    GameScene gameScene;
    std::atomic<bool> running;
    std::thread serverThread;

    ENetHost* gameServerHost = nullptr;

    ENetHost* masterClientHost = nullptr;
    ENetPeer* masterPeer = nullptr;
    double lastHeartbeatTime = 0;

public:
    ServerHost();
    ~ServerHost();

    bool Start(int port);
    void Stop();

    void ServerLoop();

    void ConnectToMaster();
    void SendHeartbeat();
};