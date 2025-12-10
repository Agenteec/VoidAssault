#define _CRT_SECURE_NO_WARNINGS // Исправлено: для strcpy/sprintf
#include "MainMenuScene.h"
#include "raygui.h"
#include "GameClient.h"
#include "engine/Utils/ConfigManager.h"
#include <cstring>
#include <string>

enum MenuState { MAIN, MULTIPLAYER, SETTINGS };
static MenuState currentMenuState = MAIN;
static int mpTab = 0; // 0=Internet, 1=LAN (Disabled), 2=Direct/Favorites
static int selectedServerIndex = -1;
static std::string statusMessage = "";

MainMenuScene::MainMenuScene(GameClient* g) : Scene(g) {}

void MainMenuScene::Enter() {
    ClientConfig& cfg = ConfigManager::GetClient();
    ServerConfig& sCfg = ConfigManager::GetServer();

    strcpy(ipBuffer, cfg.lastIp.c_str());
    sprintf(portBuffer, "%d", cfg.lastPort > 0 ? cfg.lastPort : sCfg.port);
    strcpy(nameBuffer, cfg.playerName.c_str());
    statusMessage = "";
    selectedServerIndex = -1;
}

void MainMenuScene::Draw() {
    ClearBackground(GetColor(0x181818FF));
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    const char* title = ConfigManager::Text("title_void_assault");
    DrawText(title, w / 2 - MeasureText(title, 60) / 2, 40, 60, WHITE);

    if (currentMenuState == MAIN) {
        float btnW = 300; float btnH = 50; float startY = 180; float spacing = 65;

        if (GuiButton({ (float)w / 2 - btnW / 2, startY, btnW, btnH }, ConfigManager::Text("btn_singleplayer"))) {
            game->StartHost(7777);
            if (game->localServer) {
                game->network.Connect("127.0.0.1", 7777);
            }
            else {
                statusMessage = "Error: Port 7777 is busy!";
            }
        }

        // --- MULTIPLAYER ---
        if (GuiButton({ (float)w / 2 - btnW / 2, startY + spacing, btnW, btnH }, ConfigManager::Text("btn_multiplayer"))) {
            currentMenuState = MULTIPLAYER;
        }

        // --- SETTINGS ---
        if (GuiButton({ (float)w / 2 - btnW / 2, startY + spacing * 2, btnW, btnH }, ConfigManager::Text("btn_settings"))) {
            currentMenuState = SETTINGS;
        }

        // --- EXIT ---
        if (GuiButton({ (float)w / 2 - btnW / 2, startY + spacing * 3, btnW, btnH }, ConfigManager::Text("btn_exit"))) {
            CloseWindow();
        }

        if (!statusMessage.empty()) {
            DrawText(statusMessage.c_str(), w / 2 - MeasureText(statusMessage.c_str(), 20) / 2, h - 50, 20, RED);
        }
    }
    else if (currentMenuState == MULTIPLAYER) {

        GuiGroupBox({ (float)w / 2 - 250, 100, 500, 60 }, ConfigManager::Text("lbl_player_setup"));
        GuiLabel({ (float)w / 2 - 230, 120, 80, 30 }, "Nickname:");
        if (GuiTextBox({ (float)w / 2 - 150, 120, 300, 30 }, nameBuffer, 32, isEditingName)) {
            isEditingName = !isEditingName;
            ConfigManager::GetClient().playerName = std::string(nameBuffer);
            ConfigManager::Save();
        }

        GuiToggleGroup({ (float)w / 2 - 300, 180, 200, 40 }, "ONLINE;LAN (Disabled);DIRECT / HOST", &mpTab);

        float listX = (float)w / 2 - 300;
        float listY = 240;
        float listW = 600;
        float listH = 300;

        GuiGroupBox({ listX, listY, listW, listH }, "Server List");

        if (mpTab == 0) { // ONLINE LIST
            float startY = 280;

            // Кнопка Refresh
            if (GuiButton({ (float)w / 2 + 150, startY - 40, 100, 30 }, "REFRESH")) {
                game->network.RequestServerList();
            }

            // Статус
            if (game->network.isWaitingForList) {
                GuiLabel({ (float)w / 2 - 50, startY, 200, 30 }, "Fetching list...");
            }
            else if (game->network.lobbyList.empty()) {
                GuiLabel({ (float)w / 2 - 80, startY, 200, 30 }, "No servers found.");
            }
            else {
                int yOffset = 0;
                for (const auto& lobby : game->network.lobbyList) {
                    std::string text = std::string(lobby.name) + " | Map: " + lobby.mapName +
                        " | " + std::to_string(lobby.players) + "/" + std::to_string(lobby.maxPlayers);

                    if (GuiButton({ (float)w / 2 - 200, startY + (float)yOffset, 300, 30 }, text.c_str())) {
                        std::string ip = lobby.ip;
                        int port = lobby.port;
                        game->network.Connect(ip, port);
                    }
                    yOffset += 40;
                }
            }
        }
        else if (mpTab == 1) {
            // LAN (Убрано из кода по запросу, заглушка)
            GuiLabel({ listX + 20, listY + 40, 300, 30 }, "LAN Scanning disabled.");
        }
        else if (mpTab == 2) { // DIRECT / FAVORITES
            float subY = listY + 20;

            GuiLabel({ listX + 20, subY, 200, 30 }, "--- CREATE LOBBY ---");
            GuiLabel({ listX + 20, subY + 40, 50, 30 }, "Port:");
            if (GuiTextBox({ listX + 70, subY + 40, 80, 30 }, portBuffer, 6, isEditingPort)) isEditingPort = !isEditingPort;

            if (GuiButton({ listX + 170, subY + 40, 150, 30 }, "HOST SERVER")) {
                int port = atoi(portBuffer);
                ConfigManager::GetServer().port = port;
                ConfigManager::Save();
                game->StartHost(port);
                if (game->localServer) {
                    game->network.Connect("127.0.0.1", port);
                }
                else {
                    statusMessage = "Failed to bind port (Already in use?)";
                }
            }

            subY += 100;
            DrawLine((int)listX, (int)subY, (int)(listX + listW), (int)subY, GRAY);
            subY += 20;

            GuiLabel({ listX + 20, subY, 200, 30 }, "--- DIRECT CONNECT ---");
            GuiLabel({ listX + 20, subY + 40, 50, 30 }, "IP:");
            if (GuiTextBox({ listX + 70, subY + 40, 200, 30 }, ipBuffer, 64, isEditingIp)) isEditingIp = !isEditingIp;

            if (GuiButton({ listX + 280, subY + 40, 120, 30 }, "ADD FAVORITE")) {
                SavedServer sv;
                sv.ip = std::string(ipBuffer);
                sv.port = atoi(portBuffer);
                sv.name = "Custom Server";
                ConfigManager::GetClient().favoriteServers.push_back(sv);
                ConfigManager::Save();
            }

            if (GuiButton({ listX + 410, subY + 40, 150, 30 }, "CONNECT")) {
                int port = atoi(portBuffer);
                game->network.Connect(ipBuffer, port);
            }

            subY += 80;
            GuiLabel({ listX + 20, subY, 200, 20 }, "Favorites:");
            auto& favs = ConfigManager::GetClient().favoriteServers;
            for (int i = 0; i < favs.size() && i < 3; i++) {
                std::string label = favs[i].ip + ":" + std::to_string(favs[i].port);
                if (GuiButton({ listX + 20, subY + 25 + (i * 35.0f), 300, 30 }, label.c_str())) {
                    strcpy(ipBuffer, favs[i].ip.c_str());
                    sprintf(portBuffer, "%d", favs[i].port);
                    game->network.Connect(favs[i].ip, favs[i].port);
                }
            }
        }
        if (GuiButton({ 20, 20, 100, 40 }, ConfigManager::Text("btn_back"))) {
            currentMenuState = MAIN;
            statusMessage = "";
        }

        if (!statusMessage.empty()) {
            DrawText(statusMessage.c_str(), listX, listY + listH + 70, 20, RED);
        }
    }
    else if (currentMenuState == SETTINGS) {
        if (GuiButton({ (float)w / 2 - 100, 200, 200, 40 }, "Language: RU / EN")) {
            ClientConfig& cfg = ConfigManager::GetClient();
            cfg.language = (cfg.language == "ru") ? "en" : "ru";
            ConfigManager::LoadLanguage(cfg.language);
            ConfigManager::Save();
        }

        if (GuiButton({ 20, 20, 100, 40 }, ConfigManager::Text("btn_back"))) {
            currentMenuState = MAIN;
        }
    }
}