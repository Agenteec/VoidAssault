#pragma once
#include "../../engine/Scenes/Scene.h"
#include "../../common/NetworkPackets.h"
#include "../ParticleSystem.h"
#include "../VirtualJoystick.h"
#include "raymath.h"
#include <map>
#include <vector>
#include <memory>

class GameClient;

struct InterpolatedEntity {
    EntityState current;
    EntityState previous;
    float lastUpdateTime;
    Vector2 renderPos;
    float renderRot;

    InterpolatedEntity() : lastUpdateTime(0), renderRot(0) {}

    void PushState(const EntityState& newState) {
        previous = current;
        current = newState;
        lastUpdateTime = (float)GetTime();
        if (previous.id == 0) previous = current;
    }

    void UpdateInterpolation() {
        double serverTickRate = 0.05;
        double t = (GetTime() - lastUpdateTime) / serverTickRate;
        if (t > 1.0) t = 1.0;
        if (t < 0.0) t = 0.0;

        renderPos = Vector2Lerp(previous.position, current.position, (float)t);
        renderRot = Lerp(previous.rotation, current.rotation, (float)t);
    }
};

class GameplayScene : public Scene {
    Camera2D camera = { 0 };
    std::map<uint32_t, InterpolatedEntity> worldEntities;
    uint32_t myPlayerId = 0;
    std::unique_ptr<VirtualJoystick> leftStick;
    std::unique_ptr<VirtualJoystick> rightStick;

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