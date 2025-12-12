#define _CRT_SECURE_NO_WARNINGS
#include "MainMenuScene.h"
#include "raygui_wrapper.h"
#include "../GameClient.h"
#include "engine/Utils/ConfigManager.h"
#include "ResourceManager.h" 
#include "Theme.h"          
#include <cstring>
#include <string>
#include <thread>
#include <chrono>

MainMenuScene::MainMenuScene(GameClient* g) : Scene(g) {}

void MainMenuScene::Enter() {
    Theme::ApplyTheme();
    ResourceManager::Get().Load();

    ClientConfig& cfg = ConfigManager::GetClient();
    strcpy(ipBuffer, cfg.lastIp.c_str());
    sprintf(portBuffer, "%d", cfg.lastPort);
    strcpy(nameBuffer, cfg.playerName.c_str());

    currentState = MenuState::MAIN;
    virtualKeyboard.Hide();
}

void MainMenuScene::Exit() {
    ClientConfig& cfg = ConfigManager::GetClient();
    cfg.playerName = nameBuffer;
    cfg.lastIp = ipBuffer;
    cfg.lastPort = atoi(portBuffer);
    ConfigManager::Save();
}

void MainMenuScene::Update(float dt) {
    ResourceManager::Get().UpdateBackground(dt);
}

bool MainMenuScene::DrawInputField(Rectangle bounds, char* buffer, int bufferSize, bool& editMode) {
#if defined(PLATFORM_ANDROID) || defined(ANDROID) || defined(__ANDROID__)
    if (GuiTextBox(bounds, buffer, bufferSize, false)) {
        if (!virtualKeyboard.IsActive()) virtualKeyboard.Show(buffer, bufferSize);
    }
    return false;
#else
    if (GuiTextBox(bounds, buffer, bufferSize, editMode)) {
        editMode = !editMode;
        return true;
    }
    return false;
#endif
}

void MainMenuScene::Draw() {
    ResourceManager::Get().DrawBackground();

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float cx = w / 2.0f;

    const char* title = ConfigManager::Text("title_void_assault");
    int fontSize = (h < 500) ? 40 : 60;
#if defined(PLATFORM_ANDROID)
    fontSize = 70;
#endif

    Font font = ConfigManager::GetFont();
    Vector2 titleSize = MeasureTextEx(font, title, (float)fontSize, 2);
    DrawTextEx(font, title, { cx - titleSize.x / 2 + 4, 64 }, (float)fontSize, 2, Fade(BLACK, 0.5f));
    DrawTextEx(font, title, { cx - titleSize.x / 2, 60 }, (float)fontSize, 2, Theme::COL_ACCENT);
}

void MainMenuScene::DrawGUI() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float cx = w / 2.0f;
    Font font = ConfigManager::GetFont();

    bool kbdActive = virtualKeyboard.IsActive();
    if (kbdActive) GuiDisable();

    if (currentState == MenuState::MAIN) {
        float startY = 220;
        float btnW = 320;
        float btnH = 50;
        float spacing = 20;

        Rectangle menuRect = { cx - btnW / 2 - 20, startY - 20, btnW + 40, (btnH + spacing) * 4 + 40 };
        ResourceManager::DrawSciFiPanel(menuRect, "Alpha 0.0.2", font);

        if (GuiButton({ cx - btnW / 2, startY, btnW, btnH }, ConfigManager::Text("btn_singleplayer"))) {
            std::thread([this]() {
                int realPort = game->StartHost(7777);
                if (realPort > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    game->netClient->connect("127.0.0.1", realPort);
                }
                }).detach();
        }
        startY += btnH + spacing;

        if (GuiButton({ cx - btnW / 2, startY, btnW, btnH }, ConfigManager::Text("btn_multiplayer"))) {
            currentState = MenuState::MULTIPLAYER;
        }
        startY += btnH + spacing;

        if (GuiButton({ cx - btnW / 2, startY, btnW, btnH }, ConfigManager::Text("btn_settings"))) {
            currentState = MenuState::SETTINGS;
        }
        startY += btnH + spacing;

        if (GuiButton({ cx - btnW / 2, startY, btnW, btnH }, ConfigManager::Text("btn_exit"))) {
            CloseWindow();
        }
    }
    else if (currentState == MenuState::MULTIPLAYER) {
        float panelW = 600;
        float panelH = 440;
        float panelY = 100;

        Rectangle panelRect = { cx - panelW / 2, panelY, panelW, panelH };
        ResourceManager::DrawSciFiPanel(panelRect, ConfigManager::Text("btn_multiplayer"), font);

        float contentX = panelRect.x + 40;
        float contentY = panelRect.y + 45;
        float contentW = panelW - 80;

        GuiLabel({ contentX, contentY, contentW, 20 }, ConfigManager::Text("lbl_nickname"));
        contentY += 20;
        DrawInputField({ contentX, contentY, contentW, 30 }, nameBuffer, 32, isEditingName);
        if (!isEditingName && !kbdActive) ConfigManager::GetClient().playerName = nameBuffer;

        contentY += 40;


        GuiToggleGroup({ contentX, contentY, contentW/2, 40 }, ConfigManager::Text("tabs_multiplayer"), &activeMpTab);
        contentY += 50;

        if (activeMpTab == 0) {

            GuiLabel({ contentX, contentY, contentW, 20 }, ConfigManager::Text("lbl_ip"));
            contentY += 20;
            DrawInputField({ contentX, contentY, contentW, 30 }, ipBuffer, 64, isEditingIp);
            contentY += 35;

            GuiLabel({ contentX, contentY, contentW, 20 }, ConfigManager::Text("lbl_port"));
            contentY += 20;
            DrawInputField({ contentX, contentY, 100, 30 }, portBuffer, 16, isEditingPort);
            contentY += 45;

            if (GuiButton({ contentX, contentY, contentW, 40 }, ConfigManager::Text("btn_join"))) {
                ConfigManager::GetClient().lastIp = ipBuffer;
                ConfigManager::GetClient().lastPort = atoi(portBuffer);
                ConfigManager::Save();
                game->netClient->connect(ipBuffer, atoi(portBuffer));
            }
        }
        else {
            GuiLabel({ contentX, contentY, contentW, 20 }, ConfigManager::Text("lbl_port"));
            contentY += 20;
            DrawInputField({ contentX, contentY, 100, 30 }, portBuffer, 16, isEditingPort);
            contentY += 45;

            if (GuiButton({ contentX, contentY, contentW, 40 }, ConfigManager::Text("btn_create_lobby"))) {
                int port = atoi(portBuffer);
                int p = game->StartHost(port);
                if (p > 0) game->netClient->connect("127.0.0.1", p);
            }
        }

        float backBtnY = panelRect.y + panelRect.height + 15;
        if (backBtnY + 40 > GetScreenHeight()) {
            backBtnY = GetScreenHeight() - 50.0f;
        }

        if (GuiButton({ cx - 100, backBtnY, 200, 40 }, ConfigManager::Text("btn_back"))) {
            currentState = MenuState::MAIN;
        }
    }
    else if (currentState == MenuState::SETTINGS) {
        float panelW = 400;
        float panelH = 350;
        float panelY = h / 2.0f - panelH / 2.0f;

        Rectangle panelRect = { cx - panelW / 2, panelY, panelW, panelH };
        ResourceManager::DrawSciFiPanel(panelRect, ConfigManager::Text("btn_settings"), font);

        float contentX = panelRect.x + 30;
        float contentY = panelRect.y + 50;
        float contentW = panelW - 60;

        GuiLabel({ contentX, contentY, contentW, 20 }, ConfigManager::Text("lbl_master_vol"));
        contentY += 25;
        float vol = game->audio.GetVolume();
        if (GuiSlider({ contentX, contentY, contentW, 30 }, "0%", "100%", &vol, 0.0f, 1.0f)) {
            game->audio.SetVolume(vol);
        }
        contentY += 60;

        GuiLabel({ contentX, contentY, contentW, 20 }, ConfigManager::Text("lbl_language"));
        contentY += 25;
        std::string langBtnText = "< " + ConfigManager::GetCurrentLangName() + " >";
        if (GuiButton({ contentX, contentY, contentW, 40 }, langBtnText.c_str())) {
            ConfigManager::CycleLanguage();
        }
        contentY += 60;

        if (GuiButton({ cx - 100, panelRect.y + panelRect.height + 20, 200, 40 }, ConfigManager::Text("btn_back"))) {
            game->ReturnToMenu();
            currentState = MenuState::MAIN;
        }
    }

    if (kbdActive) {
        GuiEnable();
        DrawRectangle(0, 0, w, h, Fade(BLACK, 0.7f));
        virtualKeyboard.Draw();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && GetMouseY() < h * 0.4f) {
            virtualKeyboard.Hide();
        }
    }
}