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
    std::atomic<bool> running{ false };
    std::thread serverThread;

public:
    ServerHost();
    ~ServerHost();

    bool Start(int port);
    void Stop();

    void ServerLoop();
};