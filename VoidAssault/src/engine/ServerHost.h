#pragma once
#if defined(_WIN32)
#include "fix_win32_compatibility.h"
#endif

#include "enet/ENetServer.h"
#include "Scenes/GameScene.h"
#include <thread>
#include <atomic>

class ServerHost {
    ENetServer::Shared netServer;
    GameScene gameScene;

    std::atomic<bool> running{ false };
    std::thread serverThread;

public:
    ServerHost();
    ~ServerHost();

    bool Start(int port);
    void Stop();

    void ServerLoop();
    void BroadcastSnapshot();
};