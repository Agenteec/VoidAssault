#pragma once
#include "raygui_wrapper.h"
#include "enet/ENetClient.h" 
#include "../engine/Scenes/Scene.h"
#include "../engine/ServerHost.h"
#include "AudioManager.h"
#include <memory>

class Scene;

class GameClient {
public:
    int screenWidth = 1280;
    int screenHeight = 720;


    ENetClient::Shared netClient;

    std::unique_ptr<ServerHost> localServer;
    AudioManager audio;

    std::shared_ptr<Scene> currentScene;
    std::shared_ptr<Scene> nextScene;

    GameClient();
    ~GameClient();

    void Run();
    void ChangeScene(std::shared_ptr<Scene> newScene);
    void ReturnToMenu();

    int StartHost(int port);
    void StopHost();

    int GetWidth() const { return GetScreenWidth(); }
    int GetHeight() const { return GetScreenHeight(); }
};