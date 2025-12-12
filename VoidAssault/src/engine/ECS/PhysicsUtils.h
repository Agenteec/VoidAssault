#pragma once
#include "raylib_compatibility.h"
#include "chipmunk/chipmunk.h"
#include <cmath>

inline float SanitizeFloat(float val) {
    if (!std::isfinite(val)) return 0.0f;
    return val;
}

inline double SanitizeDouble(double val) {
    if (!std::isfinite(val)) return 0.0;
    return val;
}

inline cpVect ToCp(Vector2 v) {
    return cpv(SanitizeFloat(v.x), SanitizeFloat(v.y));
}

inline Vector2 ToRay(cpVect v) {
    return Vector2{ (float)SanitizeDouble(v.x), (float)SanitizeDouble(v.y) };
}