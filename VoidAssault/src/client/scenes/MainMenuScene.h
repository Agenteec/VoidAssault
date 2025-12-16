#pragma once
#include "raylib_compatibility.h"
#include "engine/Scenes/Scene.h"
#include "../vircontrols/InGameKeyboard.h"
#include "common/NetworkPackets.h"
#include "common/enet/ENetClient.h"
#include <string>
#include <vector>
#include <mutex> 
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

        char serverNameBuffer[32] = "";
    float hostMaxPlayers = 8;
    float hostTps = 60;
    bool isPublicServer = true; 
    bool isEditingIp = false;
    bool isEditingPort = false;
    bool isEditingName = false;
    bool isEditingServerName = false;

    MenuState currentState = MenuState::MAIN;
    int activeMpTab = 0; 
    InGameKeyboard virtualKeyboard;

        std::vector<LobbyInfo> lobbyList;
    std::mutex lobbyMutex;     bool isRefreshing = false;

    
    bool DrawInputField(Rectangle bounds, char* buffer, int bufferSize, bool& editMode);
    void RefreshLobbyList();

public:
    MainMenuScene(GameClient* g);
    virtual ~MainMenuScene() = default;

    void Enter() override;
    void Exit() override;
    void Update(float dt) override;
    void Draw() override;
    void DrawGUI() override;
};