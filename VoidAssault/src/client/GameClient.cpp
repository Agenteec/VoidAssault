#include "GameClient.h"
#include "scenes/MainMenuScene.h"
#include "scenes/GameplayScene.h"
#include "../engine/Utils/ConfigManager.h"
#include "Theme.h"
GameClient::GameClient() {
	ClientConfig& cfg = ConfigManager::GetClient();
	screenWidth = cfg.resolutionWidth;
	screenHeight = cfg.resolutionHeight;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
	InitWindow(0, 0, "Void Assault");
	screenWidth = GetScreenWidth();
	screenHeight = GetScreenHeight();
#else
	
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screenWidth, screenHeight, "Void Assault");
#endif

	SetTargetFPS(cfg.targetFPS);
	SetWindowMinSize(800, 600);

	netClient = ENetClient::alloc();

	float scale = GetUIScale();
	GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20 * scale));
}
GameClient::~GameClient() {
	StopHost();
	if (currentScene) currentScene->Exit();
	if (netClient) netClient->disconnect();
	CloseWindow();
}
void GameClient::ChangeScene(std::shared_ptr<Scene> newScene) {
	nextScene = newScene;
}
void GameClient::ReturnToMenu() {
	if (netClient) netClient->disconnect();
	ChangeScene(std::make_shared<MainMenuScene>(this));
}
int GameClient::StartHost(int startPort) {
	StopHost();
	localServer = std::make_unique<ServerHost>();
	for (int p = startPort; p < startPort + 10; p++) {
		if (localServer->Start(p)) {
			TraceLog(LOG_INFO, "Local Server Started on port %d", p);
			return p;
		}
	}
	TraceLog(LOG_ERROR, "Failed to start local server. All ports busy?");
	localServer.reset();
	return -1;
}
void GameClient::StopHost() {
	if (localServer) {
		localServer->Stop();
		localServer.reset();
		TraceLog(LOG_INFO, "Local Server Stopped.");
	}
}
float GameClient::GetUIScale() const {
	// ВАЖНО: Берем текущие размеры окна, а не закешированные
	int h = GetScreenHeight();
	float scale = (float)h / 720.0f;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
	// Для Android делаем интерфейс покрупнее (HUD)
	scale *= 1.5f;
#endif

	if (scale < 0.8f) scale = 0.8f; // Минимальный скейл
	return scale;
}
void GameClient::Run() {
	ChangeScene(std::make_shared<MainMenuScene>(this));
	while (!WindowShouldClose()) {
		if (nextScene) {
			if (currentScene) currentScene->Exit();
			currentScene = nextScene;
			currentScene->Enter();
			nextScene = nullptr;
		}
		float dt = GetFrameTime();

		if (netClient) {
			auto msgs = netClient->poll();

			for (auto& msg : msgs) {
				if (msg->type() == MessageType::CONNECT) {
					TraceLog(LOG_INFO, ">> CLIENT: Connected to server!");
					if (!std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
						ChangeScene(std::make_shared<GameplayScene>(this));
					}
				}
				else if (msg->type() == MessageType::DISCONNECT) {
					TraceLog(LOG_INFO, ">> CLIENT: Disconnected.");
					if (std::dynamic_pointer_cast<GameplayScene>(currentScene)) {
						ReturnToMenu();
					}
				}
				else if (msg->type() == MessageType::DATA) {
					if (currentScene) {
						currentScene->OnMessage(msg);
					}
				}
			}
		}

		if (currentScene) {
			currentScene->Update(dt);
			BeginDrawing();
			ClearBackground(Theme::COL_BACKGROUND);
			currentScene->Draw();
			currentScene->DrawGUI();
			EndDrawing();
		}
	}
}