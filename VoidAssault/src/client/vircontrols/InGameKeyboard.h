#pragma once
#include "raylib.h"
#include "raygui.h"
#include <string>
#include <vector>
#include <cstring>
#include <cmath>

#if defined(ANDROID) || defined(PLATFORM_ANDROID) || defined(__ANDROID__)
#define IS_MOBILE_PLATFORM
#endif

class InGameKeyboard {
private:
#ifdef IS_MOBILE_PLATFORM
    bool active = false;
    char* targetBuffer = nullptr;
    int maxLen = 0;

    bool isCaps = false;
    bool isSymbols = false;
    int cursorIndex = 0;
    int frameCounter = 0;


    const std::string rowNumbers = "1234567890";

    const std::vector<std::string> rowsAlphaLower = {
        "qwertyuiop",
        "asdfghjkl",
        "zxcvbnm."
    };

    const std::vector<std::string> rowsAlphaUpper = {
        "QWERTYUIOP",
        "ASDFGHJKL",
        "ZXCVBNM."
    };


    const std::vector<std::string> rowsSymbols = {
        "1234567890",
        "!@#$%^&*()", 
        "-=_+[]{}\\|",
        ";:'\",.<>/?"
    };
#endif

public:
    void Show(char* buffer, int bufferSize) {
#ifdef IS_MOBILE_PLATFORM
        targetBuffer = buffer;
        maxLen = bufferSize;
        active = true;
        isCaps = false;
        isSymbols = false;
        cursorIndex = (targetBuffer) ? (int)strlen(targetBuffer) : 0;
#endif
    }

    void Hide() {
#ifdef IS_MOBILE_PLATFORM
        active = false;
        targetBuffer = nullptr;
#endif
    }

    bool IsActive() const {
#ifdef IS_MOBILE_PLATFORM
        return active;
#else
        return false;
#endif
    }

    void Draw() {
#ifdef IS_MOBILE_PLATFORM
        if (!active) return;

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        float kbdH = screenH * 0.45f;
        float startY = screenH - kbdH;

 
        float previewH = 60.0f;
        float previewY = startY - previewH;

        DrawRectangle(0, (int)previewY, screenW, (int)(kbdH + previewH), Fade(GetColor(0x202020FF), 0.98f));
        DrawLine(0, (int)previewY, screenW, (int)previewY, GRAY);

        Rectangle previewRect = { 0, previewY, (float)screenW, previewH };

        int oldAlign = GuiGetStyle(TEXTBOX, TEXT_ALIGNMENT);
        GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

        GuiTextBox(previewRect, targetBuffer, maxLen, false);

        GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, oldAlign);

        if (targetBuffer) {
            Font font = GuiGetFont();
            int fontSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
            int fontSpacing = GuiGetStyle(DEFAULT, TEXT_SPACING);
            int textPadding = GuiGetStyle(TEXTBOX, TEXT_PADDING);

            char tempC = targetBuffer[cursorIndex];
            targetBuffer[cursorIndex] = '\0';
            Vector2 size = MeasureTextEx(font, targetBuffer, (float)fontSize, (float)fontSpacing);
            targetBuffer[cursorIndex] = tempC;

            int cursorX = (int)previewRect.x + textPadding + (int)size.x + 2;
            int cursorY = (int)previewRect.y + (int)previewH / 2 - fontSize / 2;

            frameCounter++;
            if ((frameCounter / 30) % 2 == 0) {
                DrawRectangle(cursorX, cursorY, 2, fontSize, RED);
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), previewRect)) {
                float clickLocalX = GetMousePosition().x - (previewRect.x + textPadding);

                int bestIndex = 0;
                float minDiff = 9999.0f;
                int len = (int)strlen(targetBuffer);

                for (int i = 0; i <= len; i++) {
                    char tmp = targetBuffer[i];
                    targetBuffer[i] = '\0';
                    Vector2 width = MeasureTextEx(font, targetBuffer, (float)fontSize, (float)fontSpacing);
                    targetBuffer[i] = tmp;

                    float diff = std::abs(width.x - clickLocalX);
                    if (diff < minDiff) {
                        minDiff = diff;
                        bestIndex = i;
                    }
                }
                cursorIndex = bestIndex;
            }
        }

        float padding = 4.0f;
        int totalRows = 4; 

        float rowHeight = (kbdH - (padding * (totalRows + 2))) / (totalRows + 1);

        for (int r = 0; r < totalRows; r++) {
            std::string rowStr;

            if (isSymbols) {
                rowStr = rowsSymbols[r];
            }
            else {
                if (r == 0) rowStr = rowNumbers;
                else rowStr = (isCaps ? rowsAlphaUpper : rowsAlphaLower)[r - 1];
            }

            int numKeys = (int)rowStr.length();

            float availableWidth = screenW;
            if (r == 0) availableWidth -= (rowHeight * 1.6f + padding * 2);

            float keyWidth = (availableWidth - (padding * (numKeys + 1))) / (float)numKeys;

            float maxKeyW = rowHeight * 1.3f;
            if (keyWidth > maxKeyW) keyWidth = maxKeyW;

            float rowTotalWidth = numKeys * keyWidth + (numKeys - 1) * padding;

            float startX = (screenW - rowTotalWidth) / 2.0f;

            if (r == 0) {
                startX = (availableWidth - rowTotalWidth) / 2.0f;
                if (startX < padding) startX = padding;
            }

            for (int i = 0; i < numKeys; i++) {
                Rectangle btnRect = {
                    startX + i * (keyWidth + padding),
                    startY + padding + r * (rowHeight + padding),
                    keyWidth,
                    rowHeight
                };

                char keyLabel[2] = { rowStr[i], '\0' };
                if (GuiButton(btnRect, keyLabel)) {
                    InsertChar(rowStr[i]);
                }
            }
        }

        {
            float bsWidth = rowHeight * 1.6f;
            float bsX = screenW - bsWidth - padding;
            Rectangle bsRect = { bsX, startY + padding, bsWidth, rowHeight };
            if (GuiButton(bsRect, "#118#")) {
                DeleteChar();
            }
        }


        float bottomY = startY + padding + totalRows * (rowHeight + padding);

        float modeBtnW = screenW * 0.15f;
        float okBtnW = screenW * 0.15f;
        float spaceW = screenW - (modeBtnW + okBtnW + padding * 4);

        float currentX = padding;

        const char* modeLabel = isSymbols ? "ABC" : (isCaps ? "#113#" : "#112#");

        if (GuiButton({ currentX, bottomY, modeBtnW, rowHeight }, modeLabel)) {
            if (isSymbols) {
                isSymbols = false;
            }
            else {

                isCaps = !isCaps;
            }
        }

        float btnSmallW = screenW * 0.15f;
        float btnSpaceW = screenW - (btnSmallW * 3 + padding * 5);

        currentX = padding;

        if (GuiButton({ currentX, bottomY, btnSmallW, rowHeight }, isSymbols ? "ABC" : "?123")) {
            isSymbols = !isSymbols;
        }
        currentX += btnSmallW + padding;

        if (!isSymbols) {
            if (GuiButton({ currentX, bottomY, btnSmallW, rowHeight }, isCaps ? "#113#" : "#112#")) {
                isCaps = !isCaps;
            }
        }
        else {
            
            GuiLabel({ currentX, bottomY, btnSmallW, rowHeight }, "");
        }
        currentX += btnSmallW + padding;

        if (GuiButton({ currentX, bottomY, btnSpaceW, rowHeight }, "SPACE")) {
            InsertChar(' ');
        }
        currentX += btnSpaceW + padding;

        if (GuiButton({ currentX, bottomY, btnSmallW, rowHeight }, "OK")) {
            Hide();
        }

#endif
    }

private:
#ifdef IS_MOBILE_PLATFORM
    void InsertChar(char c) {
        if (!targetBuffer) return;

        int len = (int)strlen(targetBuffer);
        if (len >= maxLen - 1) return;

        memmove(targetBuffer + cursorIndex + 1, targetBuffer + cursorIndex, len - cursorIndex + 1);

        targetBuffer[cursorIndex] = c;
        cursorIndex++;
    }

    void DeleteChar() {
        if (!targetBuffer || cursorIndex <= 0) return;

        int len = (int)strlen(targetBuffer);

        memmove(targetBuffer + cursorIndex - 1, targetBuffer + cursorIndex, len - cursorIndex + 1);

        cursorIndex--;
    }
#endif
};