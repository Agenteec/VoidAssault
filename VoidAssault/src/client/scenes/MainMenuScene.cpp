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

    ServerConfig& sCfg = ConfigManager::GetServer();
    strcpy(serverNameBuffer, sCfg.serverName.c_str());
    hostMaxPlayers = (float)sCfg.maxPlayers;
    hostTps = (float)sCfg.tickRate;
    isPublicServer = true; 
    currentState = MenuState::MAIN;
        activeMpTab = 0;

    virtualKeyboard.Hide();
}

void MainMenuScene::Exit() {
    ClientConfig& cfg = ConfigManager::GetClient();
    cfg.playerName = nameBuffer;
    cfg.lastIp = ipBuffer;
    cfg.lastPort = atoi(portBuffer);

    ServerConfig& sCfg = ConfigManager::GetServer();
    sCfg.serverName = serverNameBuffer;
    sCfg.maxPlayers = (int)hostMaxPlayers;
    sCfg.tickRate = (int)hostTps;

    ConfigManager::Save();
}

void MainMenuScene::Update(float dt) {
    ResourceManager::Get().UpdateBackground(dt);
}

void MainMenuScene::RefreshLobbyList() {
    if (isRefreshing) return;
    isRefreshing = true;

    {
        std::lock_guard<std::mutex> lock(lobbyMutex);
        lobbyList.clear();
    }

    std::thread([this]() {
                auto tempClient = ENetClient::alloc();
        ClientConfig& cCfg = ConfigManager::GetClient();

        if (tempClient->connect(cCfg.masterServerIp, cCfg.masterServerPort)) {
            Buffer buffer; OutputAdapter adapter(buffer);
            bitsery::Serializer<OutputAdapter> serializer(std::move(adapter));
            serializer.value1b(GamePacket::MASTER_LIST_REQ);
            serializer.adapter().flush();
            tempClient->send(DeliveryType::RELIABLE, StreamBuffer::alloc(buffer.data(), buffer.size()));

            auto start = std::chrono::steady_clock::now();
            bool gotResponse = false;

                        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
                auto msgs = tempClient->poll();
                for (auto& msg : msgs) {
                    if (msg->type() == MessageType::DATA) {
                        const auto& buf = msg->stream()->buffer();
                        size_t offset = msg->stream()->tellg();
                        if (!buf.empty() && offset < buf.size()) {
                            InputAdapter ia(buf.begin() + offset, buf.end());
                            bitsery::Deserializer<InputAdapter> des(std::move(ia));
                            uint8_t type; des.value1b(type);
                            if (type == GamePacket::MASTER_LIST_RES) {
                                MasterListResPacket res; des.object(res);

                                                                std::lock_guard<std::mutex> lock(lobbyMutex);
                                lobbyList = res.lobbies;
                                gotResponse = true;
                            }
                        }
                    }
                }
                if (gotResponse) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            tempClient->disconnect();
        }
        isRefreshing = false;
        }).detach();
}

bool MainMenuScene::DrawInputField(Rectangle bounds, char* buffer, int bufferSize, bool& editMode) {
#if defined(PLATFORM_ANDROID) || defined(ANDROID)
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
        ResourceManager::DrawSciFiPanel(menuRect, "Alpha 0.7", font);
        float currentY = panelY + panelPadding;

                if (GuiButton({ cx - btnW / 2, currentY, btnW, btnH }, ConfigManager::Text("btn_singleplayer"))) {
            std::thread([this]() {
                int realPort = game->StartHost(7777, false);
                if (realPort > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    game->netClient->connect("127.0.0.1", realPort);
                }
                }).detach();
        }
        currentY += btnH + spacing;

                if (GuiButton({ cx - btnW / 2, currentY, btnW, btnH }, ConfigManager::Text("btn_multiplayer"))) {
            currentState = MenuState::MULTIPLAYER;
            if (activeMpTab == 0) RefreshLobbyList();
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
        float panelW = w * 0.9f;
        if (panelW > 900 * uiScale) panelW = 900 * uiScale;         float panelH = h * 0.8f;
        float panelY = (h - panelH) / 2.0f;

        Rectangle panelRect = { cx - panelW / 2, panelY, panelW, panelH };
        ResourceManager::DrawSciFiPanel(panelRect, ConfigManager::Text("btn_multiplayer"), font);

        float contentPad = 20 * uiScale;
        float contentX = panelRect.x + contentPad;
        float contentY = panelRect.y + contentPad * 2.0f;         float contentW = panelRect.width - (contentPad * 2);
        float fieldH = 40 * uiScale;
        float lblH = 25 * uiScale;
        float spacing = 15 * uiScale;

                GuiLabel({ contentX, contentY, contentW, lblH }, ConfigManager::Text("lbl_nickname"));
        contentY += lblH;
        DrawInputField({ contentX, contentY, contentW * 0.6f, fieldH }, nameBuffer, 32, isEditingName);
        if (!isEditingName && !kbdActive) ConfigManager::GetClient().playerName = nameBuffer;
        contentY += fieldH + spacing;

                int prevTab = activeMpTab;

                GuiToggleGroup({ contentX, contentY, contentW/3, fieldH }, ConfigManager::Text("tabs_multiplayer"), &activeMpTab);

                if (activeMpTab == 0 && prevTab != 0) RefreshLobbyList();

        contentY += fieldH + spacing;

                if (activeMpTab == 0) {
                        if (GuiButton({ contentX + contentW - 120 * uiScale, contentY, 120 * uiScale, 30 * uiScale }, isRefreshing ? "..." : ConfigManager::Text("btn_refresh"))) {
                RefreshLobbyList();
            }
            contentY += 35 * uiScale;

            DrawRectangleLines(contentX, contentY, contentW, 30 * uiScale, GRAY);
            DrawTextEx(font, ConfigManager::Text("header_browser_name"), { contentX + 10, contentY + 5 }, 20 * uiScale,1, DARKGRAY);
            DrawTextEx(font, ConfigManager::Text("header_browser_players"), { contentX + contentW * 0.5f, contentY + 5 }, 20 * uiScale, 1, DARKGRAY);
            DrawTextEx(font, ConfigManager::Text("header_browser_wave"), { contentX + contentW * 0.7f, contentY + 5 }, 20 * uiScale, 1, DARKGRAY);
            contentY += 35 * uiScale;

            float itemH = 40 * uiScale;
            float listMaxY = panelRect.y + panelRect.height - 60 * uiScale;

                        std::lock_guard<std::mutex> lock(lobbyMutex);

            if (lobbyList.empty() && !isRefreshing) {
                DrawTextEx(font, ConfigManager::Text("msg_no_servers"), { contentX + 10, contentY + 10 }, 20 * uiScale, 1, GRAY);
            }

            for (const auto& lobby : lobbyList) {
                if (contentY + itemH > listMaxY) break; 
                Rectangle itemRect = { contentX, contentY, contentW, itemH };
                if (CheckCollisionPointRec(GetMousePosition(), itemRect)) DrawRectangleRec(itemRect, Fade(Theme::COL_ACCENT, 0.2f));
                DrawRectangleLinesEx(itemRect, 1, LIGHTGRAY);

                DrawText(lobby.name.c_str(), itemRect.x + 10, itemRect.y + 10, 20 * uiScale, BLACK);
                DrawText(TextFormat("%d/%d", lobby.currentPlayers, lobby.maxPlayers), itemRect.x + contentW * 0.5f, itemRect.y + 10, 20 * uiScale, BLACK);
                DrawText(TextFormat("%d", lobby.wave), itemRect.x + contentW * 0.7f, itemRect.y + 10, 20 * uiScale, BLACK);

                if (GuiButton({ itemRect.x + contentW - 110 * uiScale, itemRect.y + 5, 100 * uiScale, 30 * uiScale }, ConfigManager::Text("btn_join"))) {
                    std::string targetIp = (lobby.ip.empty() || lobby.ip == "Unknown") ? "127.0.0.1" : lobby.ip;
                    game->ConnectToLobby(targetIp, lobby.port, lobby.id);
                }
                contentY += itemH + 5;
            }
        }
                else if (activeMpTab == 1) {
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
                else if (activeMpTab == 2) {
            GuiLabel({ contentX, contentY, contentW, lblH }, "Server Name");
            contentY += lblH;
            DrawInputField({ contentX, contentY, contentW, fieldH }, serverNameBuffer, 32, isEditingServerName);
            contentY += fieldH + spacing;

            GuiLabel({ contentX, contentY, contentW, lblH }, "Max Players");
            contentY += lblH;
            GuiSlider({ contentX, contentY, contentW, fieldH }, "1", "32", &hostMaxPlayers, 1, 32);
            contentY += fieldH + spacing;

                        if (GuiCheckBox({ contentX, contentY, 20 * uiScale, 20 * uiScale }, ConfigManager::Text("chk_public"), &isPublicServer)) {
                            }
            contentY += 30 * uiScale + spacing;

            GuiLabel({ contentX, contentY, contentW, lblH }, ConfigManager::Text("lbl_port"));
            contentY += lblH;
            DrawInputField({ contentX, contentY, contentW * 0.3f, fieldH }, portBuffer, 16, isEditingPort);
            contentY += fieldH + spacing * 2.0f;

            if (GuiButton({ contentX, contentY, contentW, fieldH * 1.2f }, ConfigManager::Text("btn_create_lobby"))) {
                ConfigManager::GetServer().serverName = serverNameBuffer;
                ConfigManager::GetServer().maxPlayers = (int)hostMaxPlayers;
                ConfigManager::GetServer().tickRate = (int)hostTps;

                int port = atoi(portBuffer);
                bool publicSrv = isPublicServer;

                std::thread([this, port, publicSrv]() {
                    int p = game->StartHost(port, publicSrv);
                    if (p > 0) {
                        int delay = publicSrv ? 1000 : 200;
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        game->netClient->connect("127.0.0.1", p);
                    }
                    }).detach();
            }
        }

                float backBtnH = 50 * uiScale;
        float backBtnY = panelRect.y + panelRect.height + spacing;
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

        float contentX = panelRect.x + 30 * uiScale;
        float contentY = panelRect.y + 45 * uiScale;
        float contentW = panelRect.width - 60 * uiScale;
        float elemH = 40 * uiScale;
        float spacing = 25 * uiScale;

        GuiLabel({ contentX, contentY, contentW, 25 * uiScale }, ConfigManager::Text("lbl_master_vol"));
        contentY += 30 * uiScale;
        float vol = game->audio.GetVolume();
        if (GuiSlider({ contentX, contentY, contentW, elemH }, "0%", "100%", &vol, 0.0f, 1.0f)) game->audio.SetVolume(vol);
        contentY += elemH + spacing;

        GuiLabel({ contentX, contentY, contentW, 25 * uiScale }, ConfigManager::Text("lbl_language"));
        contentY += 30 * uiScale;
        std::string langBtnText = "< " + ConfigManager::GetCurrentLangName() + " >";
        if (GuiButton({ contentX, contentY, contentW, elemH }, langBtnText.c_str())) ConfigManager::CycleLanguage();

        float backBtnH = 50 * uiScale;
        float backBtnY = panelRect.y + panelRect.height + spacing;
        if (GuiButton({ cx - (150 * uiScale), backBtnY, 300 * uiScale, backBtnH }, ConfigManager::Text("btn_back"))) currentState = MenuState::MAIN;
    }

    if (kbdActive) {
        GuiEnable();
        DrawRectangle(0, 0, w, h, Fade(BLACK, 0.7f));
        virtualKeyboard.Draw();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && GetMouseY() < h * 0.4f) virtualKeyboard.Hide();
    }
}