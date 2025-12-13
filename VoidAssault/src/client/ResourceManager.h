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
        float scale = (float)h / 720.0f;
        int gridSize = (int)(50 * (scale < 1.0f ? 1.0f : scale));

        for (int i = 0; i < w; i += gridSize) DrawLine(i, 0, i, h, Theme::COL_ACCENT_DIM);
        for (int i = 0; i < h; i += gridSize) DrawLine(0, i, w, i, Theme::COL_ACCENT_DIM);
    }

    static void DrawSciFiPanel(Rectangle rect, const char* title, Font font) {
        DrawRectangleRec(rect, Theme::COL_PANEL);
        DrawRectangleLinesEx(rect, 2, Theme::COL_ACCENT_DIM);

        float minDim = (rect.width < rect.height) ? rect.width : rect.height;
        float lineLen = minDim * 0.1f;
        if (lineLen > 30.0f) lineLen = 30.0f;

        float lineThick = 3.0f;

        DrawLineEx({ rect.x, rect.y }, { rect.x + lineLen, rect.y }, lineThick, Theme::COL_ACCENT);
        DrawLineEx({ rect.x, rect.y }, { rect.x, rect.y + lineLen }, lineThick, Theme::COL_ACCENT);
        DrawLineEx({ rect.x + rect.width, rect.y + rect.height }, { rect.x + rect.width - lineLen, rect.y + rect.height }, lineThick, Theme::COL_ACCENT);
        DrawLineEx({ rect.x + rect.width, rect.y + rect.height }, { rect.x + rect.width, rect.y + rect.height - lineLen }, lineThick, Theme::COL_ACCENT);

        if (title) {
            float fontSize = rect.height * 0.08f;
            if (fontSize < 24.0f) fontSize = 24.0f;
            if (fontSize > 48.0f) fontSize = 48.0f;

            float spacing = 1.0f;
            Vector2 textSize = MeasureTextEx(font, title, fontSize, spacing);

            float labelPad = 10.0f;
            float labelX = rect.x + 20;
            float labelY = rect.y - (textSize.y / 2);
            float labelW = textSize.x + (labelPad * 2);
            float labelH = textSize.y;

            DrawRectangle(labelX - 2, labelY, labelW + 4, labelH, Theme::COL_BACKGROUND);
            DrawRectangleLines(labelX, labelY, labelW, labelH, Theme::COL_ACCENT_DIM);
            DrawTextEx(font, title, { labelX + labelPad, labelY }, fontSize, spacing, Theme::COL_ACCENT);
        }
    }
};