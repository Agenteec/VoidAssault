// client\scenes\SettingsScene.cpp
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
	int w = game->GetWidth();
	int h = game->GetHeight();
	float cx = w / 2.0f;
	float uiScale = game->GetUIScale();
	GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20 * uiScale));

	float panelW = w * 0.8f;
	if (panelW > 600 * uiScale) panelW = 600 * uiScale;

	int items = 2;
#if !defined(PLATFORM_ANDROID) && !defined(ANDROID)
	items++;
#endif
	float elemH = 40 * uiScale;
	float spacing = 30 * uiScale;
	float padding = 40 * uiScale;
	float panelH = (padding * 2) + (items * (elemH + spacing + 30 * uiScale));
	float panelY = (h - panelH) / 2.0f;

	Rectangle panelRect = { cx - panelW / 2, panelY, panelW, panelH };
	ResourceManager::DrawSciFiPanel(panelRect, ConfigManager::Text("btn_settings"), ConfigManager::GetFont());

	float contentX = panelRect.x + padding;
	float currentY = panelRect.y + padding + 10 * uiScale;
	float contentW = panelRect.width - (padding * 2);

	GuiLabel({ contentX, currentY, contentW, 25 * uiScale }, ConfigManager::Text("lbl_master_vol"));
	currentY += 30 * uiScale;
	float vol = game->audio.GetVolume();
	if (GuiSlider({ contentX, currentY, contentW, elemH }, "0%", "100%", &vol, 0.0f, 1.0f)) {
		game->audio.SetVolume(vol);
	}
	currentY += elemH + spacing;

	GuiLabel({ contentX, currentY, contentW, 25 * uiScale }, ConfigManager::Text("lbl_language"));
	currentY += 30 * uiScale;
	std::string langBtnText = "< " + ConfigManager::GetCurrentLangName() + " >";
	if (GuiButton({ contentX, currentY, contentW, elemH }, langBtnText.c_str())) {
		ConfigManager::CycleLanguage();
	}
	currentY += elemH + spacing;
#if !defined(PLATFORM_ANDROID) && !defined(ANDROID)
	GuiLabel({ contentX, currentY, contentW, 25 * uiScale }, "Resolution");
	currentY += 30 * uiScale;
	std::string resText = std::to_string((int)resolutions[currentResIndex].x) + "x" + std::to_string((int)resolutions[currentResIndex].y);
	if (GuiButton({ contentX, currentY, contentW, elemH }, resText.c_str())) {
		currentResIndex = (currentResIndex + 1) % resolutions.size();
		ConfigManager::GetClient().resolutionWidth = (int)resolutions[currentResIndex].x;
		ConfigManager::GetClient().resolutionHeight = (int)resolutions[currentResIndex].y;
		ConfigManager::Save();
		SetWindowSize((int)resolutions[currentResIndex].x, (int)resolutions[currentResIndex].y);
	}
	currentY += elemH + spacing;
#endif
	float backBtnH = 50 * uiScale;
	float backBtnY = panelRect.y + panelRect.height + spacing;

	if (GuiButton({ cx - (150 * uiScale), backBtnY, 300 * uiScale, backBtnH }, ConfigManager::Text("btn_back"))) {
		game->ReturnToMenu();
	}
}