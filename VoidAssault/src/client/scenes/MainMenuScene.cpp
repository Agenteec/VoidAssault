#include "../raygui_wrapper.h"
#include "MainMenuScene.h"
#include "GameplayScene.h"
#include "../../client/ResourceManager.h"
#include "../../client/Theme.h"

MainMenuScene::MainMenuScene(GameClient* g) : Scene(g) {}

void MainMenuScene::Draw() {
    ClearBackground(Theme::BG_COLOR);
    float time = (float)GetTime() * 20.0f;
    for (int x = 0; x < game->GetWidth(); x += 50)
        DrawLine(x + (int)time % 50, 0, x + (int)time % 50, game->GetHeight(), Fade(Theme::GRID_COLOR, 0.5f));

    const char* title = "VOID ASSAULT";
    int fontSize = 80;
    int cx = game->GetWidth() / 2;
    int cy = 150;

    ResourceManager::DrawTextCentered(title, cx + 4, cy + 4, fontSize, Fade(BLACK, 0.3f));
    ResourceManager::DrawTextCentered(title, cx, cy, fontSize, Theme::COLOR_BLUE);

    DrawText("WASD to Move, LMB to Shoot", cx - 100, game->GetHeight() - 50, 20, DARKGRAY);
}

void MainMenuScene::DrawGUI() {
    float cx = game->GetWidth() / 2.0f - 100;
    float cy = game->GetHeight() / 2.0f;

    if (GuiTextBox(Rectangle{ cx, cy, 200, 30 }, ipBuffer, 64, ipEditMode)) {
        ipEditMode = !ipEditMode;
    }

    if (GuiButton(Rectangle{ cx, cy + 40, 200, 30 }, "CONNECT")) {
        game->network.Connect(ipBuffer, 7777);
        game->ChangeScene(std::make_shared<GameplayScene>(game));
    }

    if (GuiButton(Rectangle{ cx, cy + 80, 200, 30 }, "HOST LAN GAME")) {
        game->StartHost();
        game->network.Connect("127.0.0.1", 7777);
        game->ChangeScene(std::make_shared<GameplayScene>(game));
    }

    if (GuiButton(Rectangle{ cx, cy + 130, 200, 30 }, "QUIT")) {
        CloseWindow();
    }
}