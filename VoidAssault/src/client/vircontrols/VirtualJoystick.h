#pragma once
#include <raylib.h>

class VirtualJoystick {
public:
    VirtualJoystick();
    VirtualJoystick(Vector2 pos, float radiusStick, float radiusBody);

    void Update();
    void Draw();
    void Reset();


    Vector2 GetAxis() const { return axis; }

    const Vector2& GetPosition() const { return position; }
    void SetPosition(Vector2 pos);
    void SetPosition(float x, float y);

    bool IsDragging() const { return dragging; }


    void SetColors(Color body, Color stick) {
        colorBody = body;
        colorStick = stick;
    }

    void SetRadii(float body, float stick, float interaction) {
        radiusBody = body;
        radiusStick = stick;
        radiusInteraction = interaction;
    }

private:
    int touchId = -1;
    bool dragging = false;

    Vector2 position = { 0, 0 };
    Vector2 stickPosition = { 0, 0 };
    Vector2 axis = { 0, 0 };


    float radiusStick = 20.0f;
    float radiusBody = 50.0f;
    float radiusInteraction = 60.0f;


    Color colorStick = { 200, 200, 200, 255 };
    Color colorBody = { 50, 50, 50, 150 };


    bool IsTouchInBounds(Vector2 touchPos) const;
};