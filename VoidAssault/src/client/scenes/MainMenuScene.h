#pragma once
#include "../../engine/Scenes/Scene.h"

class GameClient;

class MainMenuScene : public Scene {
    char ipBuffer[64] = "";
    char portBuffer[16] = "";
    char nameBuffer[32] = "";

    bool isEditingIp = false;
    bool isEditingPort = false;
    bool isEditingName = false;

    // Состояние табов
    int activeTab = 0; // 0 = Connect, 1 = Host

public:
    MainMenuScene(GameClient* g);
    virtual ~MainMenuScene() = default;

    void Enter() override;
    void Exit() override {}
    void Update(float dt) override {}
    void Draw() override;
    void DrawGUI() override {}
};