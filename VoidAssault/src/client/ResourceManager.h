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
        // Загружаем шрифт. Если файла нет, Raylib загрузит дефолтный, но с фильтрацией
        // В реальном проекте положи файл .ttf в папку assets
        // mainFont = LoadFontEx("assets/font.ttf", 64, 0, 0);

        // Пока используем дефолтный, но ставим фильтр для гладкости
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