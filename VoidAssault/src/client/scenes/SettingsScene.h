#pragma once
#include "../../engine/Scenes/Scene.h"
#include "../../client/GameClient.h"
#include "../Theme.h"

class SettingsScene : public Scene {
public:
    SettingsScene(GameClient* g) : Scene(g) {}

    void Enter() override {}
    void Exit() override {}
    void Update(float dt) override {} // Логика в DrawGUI

    void Draw() override {
        ClearBackground(Theme::BG_COLOR);
        ResourceManager::DrawTextCentered("SETTINGS", game->GetWidth() / 2, 100, 60, Theme::COLOR_BLUE);
    }

    void DrawGUI() override {
        float cx = game->GetWidth() / 2.0f - 150;
        float cy = 200;

        // Управление громкостью
        GuiLabel(Rectangle{ cx, cy - 30, 300, 20 }, "Master Volume");
        float vol = game->audio.GetVolume();
        if (GuiSlider(Rectangle{ cx, cy, 300, 30 }, "0", "1", &vol, 0.0f, 1.0f)) {
            game->audio.SetVolume(vol);
        }
        cy += 80;

        // Полноэкранный режим (для ПК)
        static bool fullscreen = IsWindowFullscreen();
        if (GuiCheckBox(Rectangle{ cx, cy, 30, 30 }, "Fullscreen", &fullscreen)) {
            ToggleFullscreen();
        }
        cy += 80;

        // Кнопка назад
        if (GuiButton(Rectangle{ cx, game->GetHeight() - 100.0f, 300, 50 }, "BACK")) {
            // Возвращаемся в главное меню (нужен forward declaration или include)
            // Для простоты в GameClient сделаем метод ReturnToMenu
            game->ReturnToMenu();
        }
    }
};