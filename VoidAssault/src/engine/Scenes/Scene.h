#pragma once
#include <vector>
#include <memory>
#include "net/Message.h"

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

    virtual void OnMessage(Message::Shared msg) {};
};