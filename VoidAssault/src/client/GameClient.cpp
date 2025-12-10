#include "GameClient.h"
#include "scenes/MainMenuScene.h" 
#include "scenes/GameplayScene.h"



GameClient::GameClient() {

    InitWindow(screenWidth, screenHeight, "Void Assault");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    if (enet_initialize() != 0) {
        TraceLog(LOG_ERROR, "Failed to init ENet");
    }

    if (!network.Init()) {
        TraceLog(LOG_ERROR, "Failed to create ENet client host");
    }
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
}

GameClient::~GameClient() {
    StopHost();
    if (currentScene) currentScene->Exit();
    network.Shutdown();
    enet_deinitialize();
    CloseWindow();
}

void GameClient::ChangeScene(std::shared_ptr<Scene> newScene) {
    nextScene = newScene;
}

void GameClient::StartHost() {
    if (!localServer) {
        localServer = std::make_unique<ServerHost>();
        if (localServer->Start(7777)) {
            TraceLog(LOG_INFO, "Local Server Started!");
        }
        else {
            TraceLog(LOG_ERROR, "Failed to start local server!");
        }
    }
}
void GameClient::StartHost(int port) {
    StopHost();

    localServer = std::make_unique<ServerHost>();
    if (localServer->Start(port)) {
        TraceLog(LOG_INFO, "Local Server Started on port %d", port);
    }
    else {
        TraceLog(LOG_ERROR, "Failed to start local server on port %d!", port);
        localServer.reset();
    }
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
        }

        float dt = GetFrameTime();

        ENetEvent event;
        while (enet_host_service(network.clientHost, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                network.isConnected = true;
                TraceLog(LOG_INFO, "Connected to server!");
                ChangeScene(std::make_shared<GameplayScene>(this));
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                network.isConnected = false;
                TraceLog(LOG_INFO, "Disconnected from server.");
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (currentScene) {
                    currentScene->OnPacketReceived(event.packet->data, event.packet->dataLength);
                }
                enet_packet_destroy(event.packet);
                break;
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