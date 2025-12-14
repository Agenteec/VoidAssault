#pragma once
#include "../../engine/Scenes/Scene.h"
#include "../../common/NetworkPackets.h"
#include "SnapshotManager.h"
#include "../vircontrols/VirtualJoystick.h"
#include <memory>
#include "Theme.h"

class GameClient;
class GameplayScene : public Scene {
    Camera2D camera = { 0 };

    SnapshotManager snapshotManager;

    double clientTime = 0.0;
    double lastServerTime = 0.0;

    uint32_t myPlayerId = 0;

    Vector2 predictedPos = { 0, 0 };
    float predictedRot = 0.0f;
    bool isPredictedInit = false;

        float gunAnimOffset = 0.0f; 
        uint32_t myLevel = 1;
    float myCurrentXp = 0.0f;
    float myMaxXp = 100.0f;
    float myHealth = 100.0f;
    float myMaxHealth = 100.0f;
    float myDamage = 0.0f;
    float mySpeed = 0.0f;

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

    void Update(float dt) override;
    void Draw() override;
    void DrawGUI() override;
};