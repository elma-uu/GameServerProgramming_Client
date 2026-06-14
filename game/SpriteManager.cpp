#include "SpriteManager.h"
#pragma comment(lib, "gdiplus.lib")

SpriteManager::CharSprites SpriteManager::sSprites[CHAR_COUNT] = {};
bool         SpriteManager::sLoaded       = false;
ULONG_PTR    SpriteManager::sGdiplusToken = 0;

static const char* kCharFolders[CHAR_COUNT] = {
    "base", "alice", "metalPlate", "pickax", "redLotus"
};

// Fallback colors shown when a sprite sheet couldn't be loaded
static const COLORREF kFallbackColors[CHAR_COUNT] = {
    RGB(200, 200, 200),   // base   – light grey
    RGB(180, 150, 220),   // alice  – lavender
    RGB( 80, 140, 200),   // metalPlate – steel blue
    RGB(120, 190,  80),   // pickax – olive green
    RGB(220,  80,  80),   // redLotus   – red
};

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

void SpriteManager::Init()
{
    if (sLoaded) return;

    Gdiplus::GdiplusStartupInput si;
    Gdiplus::GdiplusStartup(&sGdiplusToken, &si, nullptr);

    std::wstring dir = FindCharacterDir();
    if (!dir.empty()) {
        for (int i = 0; i < CHAR_COUNT; ++i) {
            sSprites[i].idle = LoadBmp(dir, kCharFolders[i], "idle");
            sSprites[i].run  = LoadBmp(dir, kCharFolders[i], "run");
        }
    }

    sLoaded = true;
}

void SpriteManager::Shutdown()
{
    for (int i = 0; i < CHAR_COUNT; ++i) {
        delete sSprites[i].idle; sSprites[i].idle = nullptr;
        delete sSprites[i].run;  sSprites[i].run  = nullptr;
    }
    if (sGdiplusToken) {
        Gdiplus::GdiplusShutdown(sGdiplusToken);
        sGdiplusToken = 0;
    }
    sLoaded = false;
}

// ---------------------------------------------------------------------------
// Path discovery
// ---------------------------------------------------------------------------

std::wstring SpriteManager::FindCharacterDir()
{
    WCHAR exeDir[MAX_PATH];
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    WCHAR* last = wcsrchr(exeDir, L'\\');
    if (last) *(last + 1) = L'\0';

    // Try paths relative to the exe directory
    const wchar_t* candidates[] = {
        L"..\\..\\game\\Resource\\Character\\",
        L"..\\..\\..\\game\\Resource\\Character\\",
        L"game\\Resource\\Character\\",
        L"Resource\\Character\\",
        L"..\\Resource\\Character\\",
    };

    for (const wchar_t* rel : candidates) {
        WCHAR full[MAX_PATH];
        swprintf_s(full, MAX_PATH, L"%s%s", exeDir, rel);
        WCHAR probe[MAX_PATH];
        swprintf_s(probe, MAX_PATH, L"%sbase\\player_idle.png", full);
        if (GetFileAttributesW(probe) != INVALID_FILE_ATTRIBUTES)
            return std::wstring(full);
    }
    return L"";
}

Gdiplus::Bitmap* SpriteManager::LoadBmp(const std::wstring& dir,
                                         const char* charFolder,
                                         const char* stateName)
{
    wchar_t wFolder[64], wState[32];
    MultiByteToWideChar(CP_ACP, 0, charFolder, -1, wFolder, 64);
    MultiByteToWideChar(CP_ACP, 0, stateName,  -1, wState,  32);

    wchar_t path[MAX_PATH];
    swprintf_s(path, MAX_PATH, L"%s%s\\player_%s.png", dir.c_str(), wFolder, wState);

    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    return bmp;
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------

void SpriteManager::DrawFrame(Gdiplus::Graphics& g, Gdiplus::Bitmap* sheet,
                               int frameIdx, int totalFrames,
                               int destX, int destY, int drawW, int drawH, bool flipH)
{
    if (!sheet) return;

    int sheetW = (int)sheet->GetWidth();
    int sheetH = (int)sheet->GetHeight();
    if (sheetW <= 0 || sheetH <= 0 || totalFrames <= 0) return;

    int frameW = sheetW / totalFrames;
    if (frameW <= 0) return;

    int fi   = frameIdx % totalFrames;
    int srcX = fi * frameW;

    if (flipH) {
        // Reflect horizontally about the vertical centre of destRect
        float cx = (float)(destX + drawW / 2);
        Gdiplus::Matrix flip(-1.0f, 0.0f, 0.0f, 1.0f, cx * 2.0f, 0.0f);
        Gdiplus::Matrix prev;
        g.GetTransform(&prev);
        g.SetTransform(&flip);
        g.DrawImage(sheet,
            Gdiplus::Rect(destX, destY, drawW, drawH),
            srcX, 0, frameW, sheetH,
            Gdiplus::UnitPixel);
        g.SetTransform(&prev);
    }
    else {
        g.DrawImage(sheet,
            Gdiplus::Rect(destX, destY, drawW, drawH),
            srcX, 0, frameW, sheetH,
            Gdiplus::UnitPixel);
    }
}

void SpriteManager::DrawSprite(HDC hdc, int charId, bool isRunning, int frame,
                                int screenX, int screenY, int drawW, int drawH, bool flipH)
{
    if (charId < 0 || charId >= CHAR_COUNT) charId = CHAR_BASE;

    Gdiplus::Bitmap* sheet = isRunning ? sSprites[charId].run : sSprites[charId].idle;
    int totalFrames        = isRunning ? SPRITE_RUN_FRAMES : SPRITE_IDLE_FRAMES;

    int destX = screenX - drawW / 2;
    int destY = screenY - drawH / 2;

    if (!sheet) {
        // Fallback: solid colour rectangle
        HBRUSH br  = CreateSolidBrush(kFallbackColors[charId]);
        HBRUSH old = (HBRUSH)SelectObject(hdc, br);
        Rectangle(hdc, destX, destY, destX + drawW, destY + drawH);
        SelectObject(hdc, old);
        DeleteObject(br);
        return;
    }

    Gdiplus::Graphics g(hdc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    DrawFrame(g, sheet, frame, totalFrames, destX, destY, drawW, drawH, flipH);
}

void SpriteManager::DrawPreview(HDC hdc, int charId, int cx, int cy, int size)
{
    DrawSprite(hdc, charId, false, 0, cx, cy, size, size, false);
}
