#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef NOGDI
#define NOGDI
#endif


// void PlaySoundA(...) -> void PlaySoundA_Win32(...)
#define CloseWindow     CloseWindow_Win32
#define ShowCursor      ShowCursor_Win32
#define Rectangle       Rectangle_Win32
#define DrawText        DrawText_Win32
#define DrawTextA       DrawTextA_Win32
#define DrawTextEx      DrawTextEx_Win32
#define DrawTextExA     DrawTextExA_Win32
#define LoadImage       LoadImage_Win32
#define LoadImageA      LoadImageA_Win32
#define PlaySound       PlaySound_Win32
#define PlaySoundA      PlaySoundA_Win32
#define PlaySoundW      PlaySoundW_Win32

#include <windows.h>
#include <mmsystem.h>


#undef CloseWindow
#undef ShowCursor
#undef Rectangle
#undef DrawText
#undef DrawTextA
#undef DrawTextEx
#undef DrawTextExA
#undef LoadImage
#undef LoadImageA
#undef PlaySound
#undef PlaySoundA
#undef PlaySoundW