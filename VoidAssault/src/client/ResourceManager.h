#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include "Theme.h"

class ResourceManager {
public:
    static ResourceManager& Get() {
        static ResourceManager instance;
        return instance;
    }

    void Load() {}

    void Unload() {}

    void UpdateBackground(float dt) {}

    void DrawBackground() {
        ClearBackground(Theme::COL_BACKGROUND);

        int w = GetScreenWidth();
        int h = GetScreenHeight();
        int gridSize = 50;

        for (int i = 0; i < w; i += gridSize) DrawLine(i, 0, i, h, Theme::COL_ACCENT_DIM);
        for (int i = 0; i < h; i += gridSize) DrawLine(0, i, w, i, Theme::COL_ACCENT_DIM);
    }

    static void DrawSciFiPanel(Rectangle rect, const char* title, Font font) {
        DrawRectangleRec(rect, Theme::COL_PANEL);
        DrawRectangleLinesEx(rect, 2, Theme::COL_ACCENT_DIM);

        float lineLen = 15.0f;
        DrawLineEx({ rect.x, rect.y }, { rect.x + lineLen, rect.y }, 3, Theme::COL_ACCENT);
        DrawLineEx({ rect.x, rect.y }, { rect.x, rect.y + lineLen }, 3, Theme::COL_ACCENT);
        DrawLineEx({ rect.x + rect.width, rect.y + rect.height }, { rect.x + rect.width - lineLen, rect.y + rect.height }, 3, Theme::COL_ACCENT);
        DrawLineEx({ rect.x + rect.width, rect.y + rect.height }, { rect.x + rect.width, rect.y + rect.height - lineLen }, 3, Theme::COL_ACCENT);

        if (title) {
            float fontSize = 24.0f;
            float spacing = 1.0f;
            Vector2 textSize = MeasureTextEx(font, title, fontSize, spacing);

            float labelX = rect.x + 20;
            float labelY = rect.y - 12;
            float labelW = textSize.x + 20;
            float labelH = 24;

            DrawRectangle(labelX - 2, labelY, labelW + 4, labelH, Theme::COL_BACKGROUND);
            DrawRectangleLines(labelX, labelY, labelW, labelH, Theme::COL_ACCENT_DIM);
            DrawTextEx(font, title, { labelX + 10, rect.y - 12 }, fontSize, spacing, Theme::COL_ACCENT);
        }
    }
};