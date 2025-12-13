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
#if defined(PLATFORM_ANDROID) || defined(ANDROID) || defined(ANDROID)
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
	float uiScale = game->GetUIScale();

	const char* title = ConfigManager::Text("title_void_assault");
	float fontSize = 60.0f * uiScale;

	Font font = ConfigManager::GetFont();
	Vector2 titleSize = MeasureTextEx(font, title, fontSize, 2);

	float titleY = h * 0.1f;

	DrawTextEx(font, title, { cx - titleSize.x / 2 + 4, titleY + 4 }, fontSize, 2, Fade(BLACK, 0.5f));
	DrawTextEx(font, title, { cx - titleSize.x / 2, titleY }, fontSize, 2, Theme::COL_ACCENT);
}
void MainMenuScene::DrawGUI() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    float cx = w / 2.0f;
    float cy = h / 2.0f;
    float uiScale = game->GetUIScale();
    Font font = ConfigManager::GetFont();
    GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20 * uiScale));

    bool kbdActive = virtualKeyboard.IsActive();
    if (kbdActive) GuiDisable();

    if (currentState == MenuState::MAIN) {
        float btnW = 320 * uiScale;
        float btnH = 60 * uiScale;
        float spacing = 25 * uiScale;
        int btnCount = 4;

        float totalHeight = (btnH * btnCount) + (spacing * (btnCount - 1));
        float panelPadding = 40 * uiScale;
        float panelW = btnW + (panelPadding * 2);
        float panelH = totalHeight + (panelPadding * 2);

        float panelY = (h - panelH) / 2.0f + (h * 0.05f);

        Rectangle menuRect = { cx - panelW / 2, panelY, panelW, panelH };
        ResourceManager::DrawSciFiPanel(menuRect, "Alpha 0.0.3", font);

        float currentY = panelY + panelPadding;

        if (GuiButton({ cx - btnW / 2, currentY, btnW, btnH }, ConfigManager::Text("btn_singleplayer"))) {
            std::thread([this]() {
                int realPort = game->StartHost(7777);
                if (realPort > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    game->netClient->connect("127.0.0.1", realPort);
                }
                }).detach();
        }
        currentY += btnH + spacing;

        if (GuiButton({ cx - btnW / 2, currentY, btnW, btnH }, ConfigManager::Text("btn_multiplayer"))) {
            currentState = MenuState::MULTIPLAYER;
        }
        currentY += btnH + spacing;

        if (GuiButton({ cx - btnW / 2, currentY, btnW, btnH }, ConfigManager::Text("btn_settings"))) {
            currentState = MenuState::SETTINGS;
        }
        currentY += btnH + spacing;

        if (GuiButton({ cx - btnW / 2, currentY, btnW, btnH }, ConfigManager::Text("btn_exit"))) {
            CloseWindow();
        }
    }
    else if (currentState == MenuState::MULTIPLAYER) {
        float panelW = w * 0.8f;
        if (panelW > 600 * uiScale) panelW = 600 * uiScale;

        float panelH = h * 0.7f;
        float panelY = (h - panelH) / 2.0f;

        Rectangle panelRect = { cx - panelW / 2, panelY, panelW, panelH };
        ResourceManager::DrawSciFiPanel(panelRect, ConfigManager::Text("btn_multiplayer"), font);

        float contentPad = 30 * uiScale;
        float contentX = panelRect.x + contentPad;
        float contentY = panelRect.y + contentPad * 1.5f;
        float contentW = panelRect.width - (contentPad * 2);
        float fieldH = 40 * uiScale;
        float lblH = 25 * uiScale;
        float spacing = 15 * uiScale;

        GuiLabel({ contentX, contentY, contentW, lblH }, ConfigManager::Text("lbl_nickname"));
        contentY += lblH;
        DrawInputField({ contentX, contentY, contentW, fieldH }, nameBuffer, 32, isEditingName);
        if (!isEditingName && !kbdActive) ConfigManager::GetClient().playerName = nameBuffer;
        contentY += fieldH + spacing;

        GuiToggleGroup({ contentX, contentY, contentW / 2, fieldH }, ConfigManager::Text("tabs_multiplayer"), &activeMpTab);
        contentY += fieldH + spacing * 1.5f;

        if (activeMpTab == 0) {
            GuiLabel({ contentX, contentY, contentW, lblH }, ConfigManager::Text("lbl_ip"));
            contentY += lblH;
            DrawInputField({ contentX, contentY, contentW, fieldH }, ipBuffer, 64, isEditingIp);
            contentY += fieldH + spacing;

            GuiLabel({ contentX, contentY, contentW, lblH }, ConfigManager::Text("lbl_port"));
            contentY += lblH;
            DrawInputField({ contentX, contentY, contentW * 0.3f, fieldH }, portBuffer, 16, isEditingPort);
            contentY += fieldH + spacing * 1.5f;

            if (GuiButton({ contentX, contentY, contentW, fieldH * 1.2f }, ConfigManager::Text("btn_join"))) {
                ConfigManager::GetClient().lastIp = ipBuffer;
                ConfigManager::GetClient().lastPort = atoi(portBuffer);
                ConfigManager::Save();
                game->netClient->connect(ipBuffer, atoi(portBuffer));
            }
        }
        else {
            GuiLabel({ contentX, contentY, contentW, lblH }, ConfigManager::Text("lbl_port"));
            contentY += lblH;
            DrawInputField({ contentX, contentY, contentW * 0.3f, fieldH }, portBuffer, 16, isEditingPort);
            contentY += fieldH + spacing * 2.0f;

            if (GuiButton({ contentX, contentY, contentW, fieldH * 1.2f }, ConfigManager::Text("btn_create_lobby"))) {
                int port = atoi(portBuffer);
                int p = game->StartHost(port);
                if (p > 0) game->netClient->connect("127.0.0.1", p);
            }
        }

        float backBtnH = 50 * uiScale;
        float backBtnY = panelRect.y + panelRect.height + spacing;
        if (backBtnY + backBtnH > h) backBtnY = h - backBtnH - spacing;

        if (GuiButton({ cx - (150 * uiScale), backBtnY, 300 * uiScale, backBtnH }, ConfigManager::Text("btn_back"))) {
            currentState = MenuState::MAIN;
        }
    }
    else if (currentState == MenuState::SETTINGS) {
        float panelW = w * 0.7f;
        if (panelW > 500 * uiScale) panelW = 500 * uiScale;
        float panelH = 400 * uiScale;
        float panelY = (h - panelH) / 2.0f;

        Rectangle panelRect = { cx - panelW / 2, panelY, panelW, panelH };
        ResourceManager::DrawSciFiPanel(panelRect, ConfigManager::Text("btn_settings"), font);

        float contentPad = 30 * uiScale;
        float contentX = panelRect.x + contentPad;
        float contentY = panelRect.y + contentPad * 1.5f;
        float contentW = panelRect.width - (contentPad * 2);
        float elemH = 40 * uiScale;
        float spacing = 25 * uiScale;

        GuiLabel({ contentX, contentY, contentW, 25 * uiScale }, ConfigManager::Text("lbl_master_vol"));
        contentY += 30 * uiScale;
        float vol = game->audio.GetVolume();
        if (GuiSlider({ contentX, contentY, contentW, elemH }, "0%", "100%", &vol, 0.0f, 1.0f)) {
            game->audio.SetVolume(vol);
        }
        contentY += elemH + spacing;

        GuiLabel({ contentX, contentY, contentW, 25 * uiScale }, ConfigManager::Text("lbl_language"));
        contentY += 30 * uiScale;
        std::string langBtnText = "< " + ConfigManager::GetCurrentLangName() + " >";
        if (GuiButton({ contentX, contentY, contentW, elemH }, langBtnText.c_str())) {
            ConfigManager::CycleLanguage();
        }

        float backBtnH = 50 * uiScale;
        float backBtnY = panelRect.y + panelRect.height + spacing;
        if (GuiButton({ cx - (150 * uiScale), backBtnY, 300 * uiScale, backBtnH }, ConfigManager::Text("btn_back"))) {
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