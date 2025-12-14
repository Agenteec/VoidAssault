#pragma once
#include "raylib.h"
#include "raygui.h"

namespace Theme {

    static const Color COL_BACKGROUND = { 240, 240, 240, 255 };

    static const Color COL_GRID = { 205, 205, 205, 255 };

    static const Color COL_PANEL = { 255, 255, 255, 245 };
    static const Color COL_ACCENT = { 0, 178, 225, 255 };
    static const Color COL_ACCENT_DIM = { 200, 200, 210, 255 };
    static const Color COL_TEXT = { 40, 40, 50, 255 };
    static const Color COL_WARNING = { 230, 80, 80, 255 };

    static const Color COLOR_BLUE = { 0, 178, 225, 255 };
    static const Color COLOR_RED = { 241, 78, 84, 255 };
    static const Color COLOR_GREEN = { 0, 225, 110, 255 };
    static const Color COLOR_PURPLE = { 191, 127, 245, 255 };

    inline void ApplyTheme() {
        GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0xF5F5F5FF);
        GuiSetStyle(DEFAULT, LINE_COLOR, 0xC8C8D2FF);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0x282832FF);
        GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, 0x0078C8FF);
        GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, 0x005A96FF);
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0xFFFFFFFF);
        GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, 0xEBEBEBFF);
        GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, 0xDCDCDCFF);
        GuiSetStyle(DEFAULT, BORDER_WIDTH, 2);
    }
}