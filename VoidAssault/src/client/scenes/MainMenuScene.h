#pragma once
#include "../../engine/Scenes/Scene.h"
#include "../../client/GameClient.h"

class MainMenuScene : public Scene {
    char ipBuffer[64] = "127.0.0.1";
    bool ipEditMode = false;

public:
    MainMenuScene(GameClient* g);
    virtual ~MainMenuScene() = default;

    void Enter() override {}
    void Exit() override {}
    void Update(float dt) override {}
    void Draw() override;
    void DrawGUI() override;
};