#pragma once
#include "../../engine/Scenes/Scene.h"
#include "../../common/NetworkPackets.h"
#include "SnapshotManager.h"
#include "../vircontrols/VirtualJoystick.h"
#include "../ParticleSystem.h"
#include <memory>
#include <vector>
#include "Theme.h"

class GameClient;
class GameplayScene : public Scene {
    Camera2D camera = { 0 };

    SnapshotManager snapshotManager;
    ParticleSystem particles;

    double clientTime = 0.0;
    double lastServerTime = 0.0;

    uint32_t myPlayerId = 0;
    Vector2 predictedPos = { 0, 0 };
    float predictedRot = 0.0f;
    bool isPredictedInit = false;
    float gunAnimOffset = 0.0f;

    // Player Stats
    uint32_t myLevel = 1;
    float myCurrentXp = 0.0f;
    float myMaxXp = 100.0f;
    float myHealth = 100.0f;
    float myMaxHealth = 100.0f;
    float myDamage = 0.0f;
    float mySpeed = 0.0f;
    uint32_t myScrap = 0;
    uint32_t myKills = 0;
    bool isAdmin = false;


    uint32_t currentWave = 1;

    int selectedBuildType = 0;
    std::vector<uint8_t> myInventory{ 255,255,255,255,255,255 };
    std::vector<uint32_t> lastFrameEntityIds;

    bool showAdminPanel = false;
    bool showLeaderboard = true;

#if defined(PLATFORM_ANDROID) || defined(ANDROID)
    std::unique_ptr<VirtualJoystick> leftStick;
    std::unique_ptr<VirtualJoystick> rightStick;
#endif

public:
    GameplayScene(GameClient* g);
    virtual ~GameplayScene() = default;

    void Enter() override;
    void Exit() override;

    void OnMessage(Message::Shared msg) override;
    void SendAction(const ActionPacket& act);
    void SendAdminCmd(uint8_t cmd, uint32_t val);

    void Update(float dt) override;
    void Draw() override;
    void DrawGUI() override;

    void DrawMinimap(const std::vector<EntityState>& entities);
    void DrawLeaderboard(const std::vector<EntityState>& entities);
    void DrawAdminPanel();
};