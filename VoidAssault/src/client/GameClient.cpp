#include "GameClient.h"
#include "scenes/MainMenuScene.h" 
#include "scenes/GameplayScene.h"

GameClient::GameClient() {
    InitWindow(screenWidth, screenHeight, "Void Assault");
    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    // Инициализация ENetClient
    netClient = ENetClient::alloc();

    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
}

GameClient::~GameClient() {
    StopHost();
    if (currentScene) currentScene->Exit();

    if (netClient) netClient->disconnect();

    CloseWindow();
}

void GameClient::ChangeScene(std::shared_ptr<Scene> newScene) {
    nextScene = newScene;
}

void GameClient::ReturnToMenu() {
    if (netClient) netClient->disconnect();
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
        }

        float dt = GetFrameTime();

        // --- Сетевой цикл ---
        if (netClient) {
            auto msgs = netClient->poll();

            for (auto& msg : msgs) {
                if (msg->type() == MessageType::CONNECT) {
                    TraceLog(LOG_INFO, ">> CLIENT: Connected to server!");
                    // Если мы не в игре, переходим
                    if (!std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
                        ChangeScene(std::make_shared<GameplayScene>(this));
                    }
                }
                else if (msg->type() == MessageType::DISCONNECT) {
                    TraceLog(LOG_INFO, ">> CLIENT: Disconnected.");
                    if (std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
                        ReturnToMenu();
                    }
                }
                else if (msg->type() == MessageType::DATA) {
                    // Передаем данные в текущую сцену через OnMessage
                    if (currentScene) {
                        currentScene->OnMessage(msg);
                    }
                }
            }
        }
        // --------------------

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