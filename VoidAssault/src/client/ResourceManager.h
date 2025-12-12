#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include "Theme.h"

struct Star {
    Vector2 pos;
    float speed;
    float brightness;
};

class ResourceManager {
    std::vector<Star> stars;
public:

    static ResourceManager& Get() {
        static ResourceManager instance;
        return instance;
    }

    void Load() {
        stars.clear();
        for (int i = 0; i < 150; i++) {
            stars.push_back({
                {(float)GetRandomValue(0, GetScreenWidth()), (float)GetRandomValue(0, GetScreenHeight())},
                (float)GetRandomValue(10, 50) / 10.0f,
                (float)GetRandomValue(100, 255) / 255.0f
                });
        }
    }

    void Unload() {}

    void UpdateBackground(float dt) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        for (auto& star : stars) {
            star.pos.y += star.speed * dt * 60.0f;
            if (star.pos.y > h) {
                star.pos.y = 0;
                star.pos.x = GetRandomValue(0, w);
            }
        }
    }

    void DrawBackground() {
        ClearBackground(Theme::COL_BACKGROUND);
        for (const auto& star : stars) {
            Color c = WHITE;
            c.a = (unsigned char)(star.brightness * 255);
            DrawPixelV(star.pos, c);
        }
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        for (int i = 0; i < w; i += 50) DrawLine(i, 0, i, h, Fade(Theme::COL_ACCENT_DIM, 0.03f));
        for (int i = 0; i < h; i += 50) DrawLine(0, i, w, i, Fade(Theme::COL_ACCENT_DIM, 0.03f));
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
            float fontSize = 20.0f;
            float spacing = 1.0f;
            Vector2 textSize = MeasureTextEx(font, title, fontSize, spacing);

            float labelX = rect.x + 20;
            float labelY = rect.y - 12;
            float labelW = textSize.x + 20;
            float labelH = 24;

            DrawRectangle(labelX - 2, labelY, labelW + 4, labelH, Theme::COL_BACKGROUND);

            DrawRectangleLines(labelX, labelY, labelW, labelH, Theme::COL_ACCENT_DIM);

            DrawTextEx(font, title, { labelX + 10, rect.y - 10 }, fontSize, spacing, Theme::COL_ACCENT);
        }
    }
};