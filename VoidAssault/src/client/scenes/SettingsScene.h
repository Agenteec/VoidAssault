#pragma once
#include "../../engine/Scenes/Scene.h"
#include "../../client/GameClient.h"
#include <vector>
class SettingsScene : public Scene {
	std::vector<Vector2> resolutions = {
	{1280, 720},
	{1366, 768},
	{1600, 900},
	{1920, 1080}
	};
	int currentResIndex = 0;
public:
	SettingsScene(GameClient* g) : Scene(g) {}
	void Enter() override;
	void Exit() override;
	void Update(float dt) override;

	void Draw() override;

	void DrawGUI() override;
};