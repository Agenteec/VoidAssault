#include "GameClient.h"
#include "raygui_wrapper.h"
#include "scenes/MainMenuScene.h" 
#include "scenes/GameplayScene.h"
#include <queue>
#include <vector>

// Буфер пакетов (чтобы не терять Init при загрузке)
std::queue<std::pair<std::vector<uint8_t>, size_t>> packetQueue;

GameClient::GameClient() {
    InitWindow(screenWidth, screenHeight, "Void Assault");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    if (!network.Init()) {
        TraceLog(LOG_ERROR, "Failed to create ENet client host");
    }

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
}

GameClient::~GameClient() {
    StopHost();
    if (currentScene) currentScene->Exit();
    network.Shutdown();
    CloseWindow();
}

void GameClient::ChangeScene(std::shared_ptr<Scene> newScene) {
    nextScene = newScene;
}

void GameClient::ReturnToMenu() {
    network.Disconnect();
    ChangeScene(std::make_shared<MainMenuScene>(this));
}

int GameClient::StartHost(int startPort) {
    StopHost();
    localServer = std::make_unique<ServerHost>();

    for (int p = startPort; p < startPort + 10; p++) {
        if (localServer->Start(p)) {
            TraceLog(LOG_INFO, "Local Server Started on port %d", p);
            return p;
        }
    }

    TraceLog(LOG_ERROR, "Failed to start local server. All ports busy?");
    localServer.reset();
    return -1;
}

void GameClient::StopHost() {
    if (localServer) {
        localServer->Stop();
        localServer.reset();
        TraceLog(LOG_INFO, "Local Server Stopped.");
    }
}

void GameClient::Run() {
    ChangeScene(std::make_shared<MainMenuScene>(this));

    while (!WindowShouldClose()) {
        if (nextScene) {
            if (currentScene) currentScene->Exit();
            currentScene = nextScene;
            currentScene->Enter();
            nextScene = nullptr;

            while (!packetQueue.empty()) {
                auto& p = packetQueue.front();
                currentScene->OnPacketReceived(p.first.data(), p.first.size());
                packetQueue.pop();
            }
        }

        float dt = GetFrameTime();

        if (network.clientHost) {
            ENetEvent event;
            while (enet_host_service(network.clientHost, &event, 10) > 0) {
                switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    TraceLog(LOG_INFO, ">> CLIENT: Connected! Switching to Gameplay...");
                    network.isConnected = true;
                    ChangeScene(std::make_shared<GameplayScene>(this));
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    network.isConnected = false;
                    TraceLog(LOG_INFO, ">> CLIENT: Disconnected.");
                    if (std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
                        ChangeScene(std::make_shared<MainMenuScene>(this));
                    }
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
                    if (nextScene != nullptr) {
                        std::vector<uint8_t> data(event.packet->data, event.packet->data + event.packet->dataLength);
                        packetQueue.push({ data, event.packet->dataLength });
                    }
                    else if (currentScene) {
                        currentScene->OnPacketReceived(event.packet->data, event.packet->dataLength);
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
            }
        }

        if (currentScene) {
            currentScene->Update(dt);
            BeginDrawing();
            ClearBackground(RAYWHITE);
            currentScene->Draw();
            currentScene->DrawGUI();
            EndDrawing();
        }
    }
}