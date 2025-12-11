#pragma once
#include "raylib.h"
#include <string>

class ResourceManager {
public:
    Font mainFont;

    static ResourceManager& Get() {
        static ResourceManager instance;
        return instance;
    }

    void Load() {
        mainFont = GetFontDefault();
        SetTextureFilter(mainFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    void Unload() {
        // UnloadFont(mainFont);
    }

    static void DrawTextCentered(const char* text, int cx, int cy, int fontSize, Color color) {
        int width = MeasureText(text, fontSize);
        DrawText(text, cx - width / 2, cy - fontSize / 2, fontSize, color);
    }
};