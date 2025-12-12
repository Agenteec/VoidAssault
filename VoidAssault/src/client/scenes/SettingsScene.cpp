#include "SettingsScene.h"
#include "engine/Utils/ConfigManager.h"
#include "ResourceManager.h"
#include <string>
void SettingsScene::Enter() {
#if !defined(PLATFORM_ANDROID) && !defined(ANDROID)
	currentResIndex = 0;
	int w = ConfigManager::GetClient().resolutionWidth;
	int h = ConfigManager::GetClient().resolutionHeight;
	for (size_t i = 0; i < resolutions.size(); i++) {
		if (resolutions[i].x == w && resolutions[i].y == h) {
			currentResIndex = (int)i;
			break;
		}
	}
#endif
}
void SettingsScene::Exit() {}
void SettingsScene::Update(float dt) {}
void SettingsScene::Draw() {
	ResourceManager::Get().UpdateBackground(GetFrameTime());
	ResourceManager::Get().DrawBackground();
}
void SettingsScene::DrawGUI() {
	float w = (float)game->GetWidth();
	float h = (float)game->GetHeight();
	float cx = w / 2.0f;
	float cy = h / 2.0f - 200;
	Rectangle panelRect = { cx - 250, cy - 20, 500, 450 };
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
	currentY += 60;
#if !defined(PLATFORM_ANDROID) && !defined(ANDROID)
	GuiLabel({ contentX, currentY, width, 20 }, "Resolution");
	currentY += 25;
	std::string resText = std::to_string((int)resolutions[currentResIndex].x) + "x" + std::to_string((int)resolutions[currentResIndex].y);
	if (GuiButton({ contentX, currentY, width, 40 }, resText.c_str())) {
		currentResIndex = (currentResIndex + 1) % resolutions.size();
		ConfigManager::GetClient().resolutionWidth = (int)resolutions[currentResIndex].x;
		ConfigManager::GetClient().resolutionHeight = (int)resolutions[currentResIndex].y;
		ConfigManager::Save();
		SetWindowSize((int)resolutions[currentResIndex].x, (int)resolutions[currentResIndex].y);
	}
	currentY += 60;
#endif
	if (GuiButton({ cx - 100, panelRect.y + panelRect.height + 20, 200, 40 }, ConfigManager::Text("btn_back"))) {
		game->ReturnToMenu();
	}
}