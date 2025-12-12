#pragma once
#include "raylib.h"
#include "raygui.h"

namespace Theme {
   
    static const Color COL_BACKGROUND = { 240, 240, 245, 255 };
    static const Color COL_PANEL = { 255, 255, 255, 245 };
    static const Color COL_ACCENT = { 0, 120, 200, 255 };
    static const Color COL_ACCENT_DIM = { 200, 200, 210, 255 };
    static const Color COL_TEXT = { 40, 40, 50, 255 };
    static const Color COL_WARNING = { 200, 50, 50, 255 };

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