#include "SettingsScene.h"
#include "engine/Utils/ConfigManager.h"
#include "ResourceManager.h"

void SettingsScene::Draw() {
    ResourceManager::Get().UpdateBackground(GetFrameTime());
    ResourceManager::Get().DrawBackground();
}

void SettingsScene::DrawGUI() {
    float w = (float)game->GetWidth();
    float h = (float)game->GetHeight();
    float cx = w / 2.0f;
    float cy = h / 2.0f - 150;

    Rectangle panelRect = { cx - 250, cy - 20, 500, 350 };
    ResourceManager::DrawSciFiPanel(panelRect, ConfigManager::Text("btn_settings"), ConfigManager::GetFont());

    float contentX = cx - 200;
    float currentY = cy + 40;
    float width = 400;

    GuiLabel({ contentX, currentY, width, 20 }, ConfigManager::Text("lbl_master_vol"));
    currentY += 25;
    float vol = game->audio.GetVolume();
    if (GuiSlider({ contentX, currentY, width, 30 }, "0%", "100%", &vol, 0.0f, 1.0f)) {
        game->audio.SetVolume(vol);
    }
    currentY += 60;

    GuiLabel({ contentX, currentY, width, 20 }, ConfigManager::Text("lbl_language"));
    currentY += 25;

    std::string langBtnText = "< " + ConfigManager::GetCurrentLangName() + " >";
    if (GuiButton({ contentX, currentY, width, 40 }, langBtnText.c_str())) {
        ConfigManager::CycleLanguage();
    }
    currentY += 80;

    if (GuiButton({ cx - 100, panelRect.y + panelRect.height + 20, 200, 40 }, ConfigManager::Text("btn_back"))) {
        game->ReturnToMenu();
    }
}