#include "MainMenuScene.h"
#include "raygui.h"
#include "GameClient.h"
#include "engine/Utils/ConfigManager.h"
#include <cstring> // для strcpy

enum MenuState { MAIN, MULTIPLAYER, SETTINGS };
static MenuState currentMenuState = MAIN;

MainMenuScene::MainMenuScene(GameClient* g) : Scene(g) {}

void MainMenuScene::Enter() {
    // Копируем данные из конфига в буферы при входе
    ClientConfig& cfg = ConfigManager::GetClient();
    ServerConfig& sCfg = ConfigManager::GetServer();

    strcpy(ipBuffer, cfg.lastIp.c_str());
    sprintf(portBuffer, "%d", cfg.lastPort > 0 ? cfg.lastPort : sCfg.port);
    strcpy(nameBuffer, cfg.playerName.c_str());
}

void MainMenuScene::Draw() {
    ClearBackground(GetColor(0x181818FF));
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    const char* title = ConfigManager::Text("title_void_assault");
    DrawText(title, w / 2 - MeasureText(title, 60) / 2, 50, 60, WHITE);

    if (currentMenuState == MAIN) {
        float btnW = 300; float btnH = 50; float startY = 200; float spacing = 70;

        // Одиночная игра (Хост + Клиент)
        if (GuiButton({ (float)w / 2 - btnW / 2, startY, btnW, btnH }, ConfigManager::Text("btn_singleplayer"))) {
            game->StartHost(7777);
            // После старта хоста подключаемся к самому себе
            game->network.Connect("127.0.0.1", 7777);
            // Сцена сменится при получении пакета коннекта или вручную тут
        }

        // Сетевая игра
        if (GuiButton({ (float)w / 2 - btnW / 2, startY + spacing, btnW, btnH }, ConfigManager::Text("btn_multiplayer"))) {
            currentMenuState = MULTIPLAYER;
        }

        // Настройки
        if (GuiButton({ (float)w / 2 - btnW / 2, startY + spacing * 2, btnW, btnH }, ConfigManager::Text("btn_settings"))) {
            currentMenuState = SETTINGS;
        }

        // Выход
        if (GuiButton({ (float)w / 2 - btnW / 2, startY + spacing * 3, btnW, btnH }, ConfigManager::Text("btn_exit"))) {
            CloseWindow();
        }
    }
    else if (currentMenuState == MULTIPLAYER) {
        // Настройка имени
        GuiGroupBox({ (float)w / 2 - 200, 130, 400, 60 }, ConfigManager::Text("lbl_player_setup"));
        GuiLabel({ (float)w / 2 - 180, 150, 100, 30 }, ConfigManager::Text("lbl_nickname"));
        if (GuiTextBox({ (float)w / 2 - 80, 150, 260, 30 }, nameBuffer, 32, isEditingName)) {
            isEditingName = !isEditingName;
            ConfigManager::GetClient().playerName = std::string(nameBuffer);
            ConfigManager::Save();
        }

        // Вкладки
        activeTab = GuiToggleGroup({ (float)w / 2 - 200, 220, 200, 40 }, ConfigManager::Text("tabs_multiplayer"), &activeTab);

        float startY = 280;

        if (activeTab == 0) { // ПОДКЛЮЧЕНИЕ
            GuiLabel({ (float)w / 2 - 180, startY, 50, 30 }, ConfigManager::Text("lbl_ip"));
            if (GuiTextBox({ (float)w / 2 - 130, startY, 200, 30 }, ipBuffer, 64, isEditingIp)) isEditingIp = !isEditingIp;

            GuiLabel({ (float)w / 2 + 80, startY, 50, 30 }, ConfigManager::Text("lbl_port"));
            if (GuiTextBox({ (float)w / 2 + 120, startY, 80, 30 }, portBuffer, 16, isEditingPort)) isEditingPort = !isEditingPort;

            if (GuiButton({ (float)w / 2 - 150, startY + 60, 300, 50 }, ConfigManager::Text("btn_join"))) {
                ClientConfig& cfg = ConfigManager::GetClient();
                cfg.lastIp = std::string(ipBuffer);
                cfg.lastPort = atoi(portBuffer);
                ConfigManager::Save();

                game->network.Connect(cfg.lastIp, cfg.lastPort);
                // Переход в игру произойдет автоматически при событии ENET_CONNECT в GameClient::Run
            }
        }
        else if (activeTab == 1) { // СОЗДАНИЕ
            GuiLabel({ (float)w / 2 - 50, startY, 50, 30 }, ConfigManager::Text("lbl_port"));
            if (GuiTextBox({ (float)w / 2, startY, 100, 30 }, portBuffer, 16, isEditingPort)) isEditingPort = !isEditingPort;

            if (GuiButton({ (float)w / 2 - 150, startY + 60, 300, 50 }, ConfigManager::Text("btn_create_lobby"))) {
                int port = atoi(portBuffer);
                ConfigManager::GetServer().port = port;
                ConfigManager::Save();

                game->StartHost(port);
                game->network.Connect("127.0.0.1", port);
            }
        }

        if (GuiButton({ 20, 20, 100, 40 }, ConfigManager::Text("btn_back"))) {
            currentMenuState = MAIN;
        }
    }
    else if (currentMenuState == SETTINGS) {
        // Смена языка
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