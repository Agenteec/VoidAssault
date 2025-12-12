#pragma once
#include "raylib.h"
#include "raygui.h"

namespace Theme {
   
    static const Color COL_BACKGROUND = { 10, 10, 20, 255 };
    static const Color COL_PANEL = { 20, 30, 45, 230 };
    static const Color COL_ACCENT = { 0, 240, 255, 255 };
    static const Color COL_ACCENT_DIM = { 0, 120, 130, 255 };
    static const Color COL_TEXT = { 240, 240, 255, 255 };
    static const Color COL_WARNING = { 255, 60, 60, 255 };


    inline void ApplyTheme() {
        GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0x141e2dff);
        GuiSetStyle(DEFAULT, LINE_COLOR, 0x00f0ffff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0xf0f0ffff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, 0x00f0ffff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, 0xffffffff);
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0x141e2dff);
        GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, 0x20354dff);
        GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, 0x00707fff);
        GuiSetStyle(DEFAULT, BORDER_WIDTH, 2);
    }
}