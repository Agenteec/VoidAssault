#pragma once
#include "raylib_compatibility.h"
#include "chipmunk/chipmunk.h"


// Конвертация Raylib Vector2 -> Chipmunk cpVect
inline cpVect ToCp(Vector2 v) {
    return cpv(v.x, v.y);
}

// Конвертация Chipmunk cpVect -> Raylib Vector2
inline Vector2 ToRay(cpVect v) {
    return Vector2{ (float)v.x, (float)v.y };
}