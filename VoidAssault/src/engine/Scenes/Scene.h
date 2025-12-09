#pragma once

#include "raylib.h"
#include <vector>
#include <cstdint>

class GameClient;

class Scene {
protected:
    GameClient* game;

public:
    Scene(GameClient* _game) : game(_game) {}
    virtual ~Scene() = default;

    virtual void Enter() = 0;
    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;
    virtual void DrawGUI() = 0;
    virtual void Exit() = 0;
    virtual void OnPacketReceived(const uint8_t* data, size_t size) {};
};