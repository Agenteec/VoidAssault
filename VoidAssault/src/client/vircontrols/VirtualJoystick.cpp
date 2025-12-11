#include "VirtualJoystick.h"
#include "raymath.h"

#ifndef MAX_TOUCH_POINTS
#define MAX_TOUCH_POINTS 10
#endif

VirtualJoystick::VirtualJoystick() {

}

VirtualJoystick::VirtualJoystick(Vector2 pos, float rStick, float rBody) {
    position = pos;
    stickPosition = pos;
    radiusStick = rStick;
    radiusBody = rBody;
    radiusInteraction = rBody * 1.5f;
}

void VirtualJoystick::SetPosition(Vector2 pos) {
    position = pos;
    if (!dragging) {
        stickPosition = pos;
    }
}

void VirtualJoystick::SetPosition(float x, float y) {
    SetPosition({ x, y });
}

bool VirtualJoystick::IsTouchInBounds(Vector2 touchPos) const {
    return Vector2Distance(touchPos, position) <= radiusInteraction;
}

void VirtualJoystick::Update() {
    bool inputFound = false;
    Vector2 inputPos = { 0, 0 };


    if (dragging) {
        if (touchId == -2) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                inputPos = GetMousePosition();
                inputFound = true;
            }
        }
        else {
            if (touchId < GetTouchPointCount()) {
                inputPos = GetTouchPosition(touchId);
                inputFound = true;
            }
        }

        if (!inputFound) {
            Reset();
            return;
        }
    }
    else {
        int count = GetTouchPointCount();
        for (int i = 0; i < count && i < MAX_TOUCH_POINTS; i++) {
            Vector2 touchPos = GetTouchPosition(i);
            if (IsTouchInBounds(touchPos)) {
                dragging = true;
                touchId = i;
                inputPos = touchPos;
                inputFound = true;
                break;
            }
        }

        if (!inputFound && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            if (IsTouchInBounds(mousePos)) {
                dragging = true;
                touchId = -2;
                inputPos = mousePos;
                inputFound = true;
            }
        }
    }

    if (dragging) {
        Vector2 direction = Vector2Subtract(inputPos, position);
        float distance = Vector2Length(direction);

        if (distance > radiusBody) {
            direction = Vector2Scale(Vector2Normalize(direction), radiusBody);
            stickPosition = Vector2Add(position, direction);
        }
        else {
            stickPosition = inputPos;
        }

        if (radiusBody > 0) {
            axis = Vector2Scale(direction, 1.0f / radiusBody);
        }

        if (Vector2Length(axis) > 1.0f) {
            axis = Vector2Normalize(axis);
        }
    }
    else {
        Reset();
    }
}

void VirtualJoystick::Reset() {
    stickPosition = position;
    dragging = false;
    touchId = -1;
    axis = { 0, 0 };
}

void VirtualJoystick::Draw() {

    DrawCircleV(position, radiusBody, colorBody);
    DrawRing(position, radiusBody, radiusBody + 2, 0, 360, 24, Fade(colorBody, 0.8f));

    DrawCircleV(stickPosition, radiusStick, colorStick);

    if (dragging) {
        DrawCircleV(stickPosition, radiusStick * 0.8f, Fade(WHITE, 0.3f));
    }
}