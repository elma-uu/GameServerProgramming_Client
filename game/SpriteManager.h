#pragma once
#include "framework.h"
#include <string>

// Character visual IDs (must match protocol_2026.h C2S_CharSelect visual_id)
enum { CHAR_BASE = 0, CHAR_ALICE, CHAR_METAL_PLATE, CHAR_PICKAX, CHAR_RED_LOTUS, CHAR_COUNT };

constexpr int SPRITE_IDLE_FRAMES = 5;
constexpr int SPRITE_RUN_FRAMES  = 8;

class SpriteManager
{
public:
    static void Init();
    static void Shutdown();

    // Draw one animation frame centered at (screenX, screenY).
    // isRunning selects run vs idle sheet. flipH mirrors the sprite left.
    static void DrawSprite(HDC hdc, int charId, bool isRunning, int frame,
                           int screenX, int screenY, int drawW, int drawH, bool flipH);

    // Draw idle frame 0 — used on the character-select screen.
    static void DrawPreview(HDC hdc, int charId, int cx, int cy, int size);

    static bool IsLoaded() { return sLoaded; }

private:
    struct CharSprites {
        Gdiplus::Bitmap* idle = nullptr;
        Gdiplus::Bitmap* run  = nullptr;
    };

    static CharSprites  sSprites[CHAR_COUNT];
    static bool         sLoaded;
    static ULONG_PTR    sGdiplusToken;

    static std::wstring FindCharacterDir();
    static Gdiplus::Bitmap* LoadBmp(const std::wstring& dir,
                                    const char* charFolder,
                                    const char* stateName);
    static void DrawFrame(Gdiplus::Graphics& g, Gdiplus::Bitmap* sheet,
                          int frameIdx, int totalFrames,
                          int destX, int destY, int drawW, int drawH, bool flipH);
};
