#pragma once
#include "../../engine/Scenes/Scene.h"
#include "../../client/GameClient.h"


class SettingsScene : public Scene {
public:
    SettingsScene(GameClient* g) : Scene(g) {}

    void Enter() override;
    void Exit() override;
    void Update(float dt) override;

    void Draw() override;

    void DrawGUI() override;
};