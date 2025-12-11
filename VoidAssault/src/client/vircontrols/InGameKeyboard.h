#pragma once
#include "raylib.h"
#include "raygui.h"
#include <string>
#include <vector>
#include <cstring>

#if defined(ANDROID) || defined(PLATFORM_ANDROID)
    #define IS_MOBILE_PLATFORM
#endif

class InGameKeyboard {
private:
#ifdef IS_MOBILE_PLATFORM
    bool active = false;
    char* targetBuffer = nullptr;
    int maxLen = 0;
    
    bool isShift = false;
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
        isShift = false;
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

        float previewH = 50.0f;
        float previewY = startY - previewH;

        DrawRectangle(0, (int)previewY, screenW, (int)(kbdH + previewH), Fade(GetColor(0x202020FF), 0.98f));
        DrawLine(0, (int)previewY, screenW, (int)previewY, GRAY);

        Rectangle previewRect = { 0, previewY, (float)screenW, previewH };
        GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
        
        GuiTextBox(previewRect, targetBuffer, maxLen, true);
        

        if (targetBuffer) {

            char tempC = targetBuffer[cursorIndex];
            targetBuffer[cursorIndex] = '\0';
            int textWidth = MeasureText(targetBuffer, GuiGetStyle(DEFAULT, TEXT_SIZE));
            targetBuffer[cursorIndex] = tempC;

            int textPadding = GuiGetStyle(TEXTBOX, TEXT_PADDING);
            int cursorX = (int)previewRect.x + textPadding + textWidth + 2;
            int cursorY = (int)previewRect.y + (int)previewH / 2 - 10;

            frameCounter++;
            if ((frameCounter / 30) % 2 == 0) {
                DrawRectangle(cursorX, cursorY, 2, 20, RED);
            }
        }

        float padding = 4.0f;
        
        int totalRows = 4;
        if (isSymbols) totalRows = 4;

        float rowHeight = (kbdH - (padding * (totalRows + 2))) / (totalRows + 1);

        for (int r = 0; r < totalRows; r++) {
            std::string rowStr;
            
            if (isSymbols) {
                rowStr = rowsSymbols[r];
            } else {
                if (r == 0) rowStr = rowNumbers;
                else rowStr = (isShift ? rowsAlphaUpper : rowsAlphaLower)[r - 1];
            }

            int numKeys = (int)rowStr.length();
            
            float availableWidth = screenW;
            if (r == 0 && !isSymbols) availableWidth -= (rowHeight * 1.5f + padding); 

            float keyWidth = (availableWidth - (padding * (numKeys + 1))) / (float)numKeys;
            if (keyWidth > rowHeight * 1.4f) keyWidth = rowHeight * 1.4f;

            float rowTotalWidth = numKeys * keyWidth + (numKeys - 1) * padding;
            float startX = (screenW - rowTotalWidth) / 2.0f;
            
            if (r == 0 && !isSymbols) startX = padding;

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


        if (!isSymbols) {
            float bsWidth = rowHeight * 1.5f;
            Rectangle bsRect = { screenW - bsWidth - padding, startY + padding, bsWidth, rowHeight };
            if (GuiButton(bsRect, "#118#")) { 
                DeleteChar();
            }
        }

        float bottomY = startY + padding + totalRows * (rowHeight + padding);
        
        float navBtnW = screenW * 0.12f;
        float shiftBtnW = screenW * 0.15f;
        float spaceW = screenW - (shiftBtnW + navBtnW * 2 + padding * 5);
        
        float cx = padding;

        const char* modeLabel = isSymbols ? "ABC" : (isShift ? "#113#" : "#112#");
        if (GuiButton({ cx, bottomY, shiftBtnW, rowHeight }, modeLabel)) {
            if (isSymbols) {
                isSymbols = false; 
            } else {
                isShift = !isShift;
            }
        }
        cx += shiftBtnW + padding;

        
        if (!isSymbols) {

        }
        

        float btnW = screenW / 6.0f; 
        cx = padding;

        if (GuiButton({cx, bottomY, btnW, rowHeight}, isSymbols ? "ABC" : "?123")) {
            isSymbols = !isSymbols;
        }
        cx += btnW + padding;

        if (GuiButton({cx, bottomY, btnW, rowHeight}, "#114#")) {
            if (cursorIndex > 0) cursorIndex--;
        }
        cx += btnW + padding;


        if (GuiButton({cx, bottomY, btnW * 2, rowHeight}, "SPACE")) {
            InsertChar(' ');
        }
        cx += btnW * 2 + padding;


        if (GuiButton({cx, bottomY, btnW, rowHeight}, "#115#")) {
            if (targetBuffer && cursorIndex < strlen(targetBuffer)) cursorIndex++;
        }
        cx += btnW + padding;

        if (GuiButton({screenW - btnW - padding, bottomY, btnW, rowHeight}, "OK")) {
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