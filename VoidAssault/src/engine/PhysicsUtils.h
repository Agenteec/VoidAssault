#pragma once
#include "raylib_compatibility.h"
#include "chipmunk/chipmunk.h"


inline cpVect ToCp(Vector2 v) {
    return cpv(v.x, v.y);
}

inline Vector2 ToRay(cpVect v) {
    return Vector2{ (float)v.x, (float)v.y };
}