#pragma once
#include "../../engine/Scenes/Scene.h"
#include "../vircontrols/InGameKeyboard.h"
#include <string>

class GameClient;

enum class MenuState {
    MAIN,
    MULTIPLAYER,
    SETTINGS
};

class MainMenuScene : public Scene {
private:
    char ipBuffer[64] = "";
    char portBuffer[16] = "";
    char nameBuffer[32] = "";

    bool isEditingIp = false;
    bool isEditingPort = false;
    bool isEditingName = false;

    MenuState currentState = MenuState::MAIN;
    int activeMpTab = 0; // 0 = Connect, 1 = Host

    InGameKeyboard virtualKeyboard;

    bool DrawInputField(Rectangle bounds, char* buffer, int bufferSize, bool& editMode);

public:
    MainMenuScene(GameClient* g);
    virtual ~MainMenuScene() = default;

    void Enter() override;
    void Exit() override;
    void Update(float dt) override;
    void Draw() override;
    void DrawGUI() override;
};