#define _CRT_SECURE_NO_WARNINGS
#include "MainMenuScene.h"
#include "raygui_wrapper.h"
#include "../GameClient.h"
#include "engine/Utils/ConfigManager.h"
#include <cstring>
#include <string>
#include <thread>
#include <chrono>

enum MenuState { STATE_MAIN, STATE_MULTIPLAYER, STATE_SETTINGS };
static MenuState currentState = STATE_MAIN;

static int mpTab = 0;
static char ipBuffer[64] = "";
static char portBuffer[16] = "";
static char nameBuffer[32] = "";
static bool editIp = false;
static bool editPort = false;
static bool editName = false;

MainMenuScene::MainMenuScene(GameClient* g) : Scene(g) {}

void MainMenuScene::Enter() {
    ClientConfig& cfg = ConfigManager::GetClient();
    strcpy(ipBuffer, cfg.lastIp.c_str());
    sprintf(portBuffer, "%d", cfg.lastPort);
    strcpy(nameBuffer, cfg.playerName.c_str());
    currentState = STATE_MAIN;
}

void MainMenuScene::Draw() {
    ClearBackground(GetColor(0x181818FF));
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float cx = w / 2.0f;

    const char* title = "VOID ASSAULT";
    DrawText(title, cx - MeasureText(title, 60) / 2, 50, 60, WHITE);

    if (currentState == STATE_MAIN) {
        float startY = 200;
        float btnW = 300; float btnH = 50; float spacing = 70;

        if (GuiButton({ cx - btnW / 2, startY, btnW, btnH }, ConfigManager::Text("btn_singleplayer"))) {
            //std::thread([this]() {
                int realPort = game->StartHost(7777);
                if (realPort > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    game->netClient->connect("127.0.0.1", realPort);
                }
                //}).detach();
        }

        if (GuiButton({ cx - btnW / 2, startY + spacing, btnW, btnH }, ConfigManager::Text("btn_multiplayer"))) {
            currentState = STATE_MULTIPLAYER;
        }

        if (GuiButton({ cx - btnW / 2, startY + spacing * 2, btnW, btnH }, ConfigManager::Text("btn_settings"))) {
            currentState = STATE_SETTINGS;
        }

        if (GuiButton({ cx - btnW / 2, startY + spacing * 3, btnW, btnH }, ConfigManager::Text("btn_exit"))) {
            CloseWindow();
        }
    }
    else if (currentState == STATE_MULTIPLAYER) {
        GuiLabel({ cx - 150, 130, 300, 20 }, "Nickname:");
        if (GuiTextBox({ cx - 150, 150, 300, 30 }, nameBuffer, 32, editName)) {
            editName = !editName;
            ConfigManager::GetClient().playerName = nameBuffer;
            ConfigManager::Save();
        }

        GuiToggleGroup({ cx - 200, 210, 200, 40 }, "JOIN GAME;HOST GAME", &mpTab);

        float panelY = 270;
        GuiGroupBox({ cx - 250, panelY, 500, 300 }, mpTab == 0 ? "Join Server" : "Create Server");

        if (mpTab == 0) {
            float inY = panelY + 40;
            GuiLabel({ cx - 200, inY, 50, 30 }, "IP:");
            if (GuiTextBox({ cx - 150, inY, 200, 30 }, ipBuffer, 64, editIp)) editIp = !editIp;

            GuiLabel({ cx + 60, inY, 50, 30 }, "Port:");
            if (GuiTextBox({ cx + 100, inY, 80, 30 }, portBuffer, 16, editPort)) editPort = !editPort;

            if (GuiButton({ cx - 100, inY + 60, 200, 40 }, "CONNECT")) {
                ConfigManager::GetClient().lastIp = ipBuffer;
                ConfigManager::GetClient().lastPort = atoi(portBuffer);
                ConfigManager::Save();
                game->netClient->connect(ipBuffer, atoi(portBuffer));
            }

            GuiLabel({ cx - 200, inY + 120, 200, 20 }, "Favorites:");
            auto& favs = ConfigManager::GetClient().favoriteServers;
            for (size_t i = 0; i < favs.size() && i < 3; i++) {
                std::string label = favs[i].name + " (" + favs[i].ip + ")";
                if (GuiButton({ cx - 200, inY + 145 + (i * 35.0f), 400, 30 }, label.c_str())) {
                    game->netClient->connect(favs[i].ip, favs[i].port);
                }
            }
        }
        else {
            float inY = panelY + 80;
            GuiLabel({ cx - 100, inY, 200, 20 }, "Local Port:");
            if (GuiTextBox({ cx - 50, inY + 25, 100, 30 }, portBuffer, 16, editPort)) editPort = !editPort;

            if (GuiButton({ cx - 120, inY + 80, 240, 50 }, "START HOST & PLAY")) {
                int port = atoi(portBuffer);

                int p = game->StartHost(port);
                if (p > 0) {
                    //std::this_thread::sleep_for(std::chrono::milliseconds(50));

                    game->netClient->connect("127.0.0.1", p);
                }
            }
        }

        if (GuiButton({ 30, (float)h - 70, 120, 40 }, "BACK")) {
            currentState = STATE_MAIN;
        }
    }
    else if (currentState == STATE_SETTINGS) {
        float startY = 200;
        GuiLabel({ cx - 150, startY, 300, 20 }, "Master Volume");
        float vol = game->audio.GetVolume();
        if (GuiSlider({ cx - 150, startY + 25, 300, 30 }, "0", "1", &vol, 0, 1)) {
            game->audio.SetVolume(vol);
        }

        startY += 80;
        if (GuiButton({ cx - 150, startY, 300, 40 }, "Toggle Fullscreen")) {
            ToggleFullscreen();
        }

        if (GuiButton({ 30, (float)h - 70, 120, 40 }, "BACK")) {
            currentState = STATE_MAIN;
        }
    }
}