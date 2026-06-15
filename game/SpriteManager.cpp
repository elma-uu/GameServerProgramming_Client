#include "SpriteManager.h"
#pragma comment(lib, "gdiplus.lib")

SpriteManager::CharSprites SpriteManager::sSprites[CHAR_COUNT]   = {};
SpriteManager::MonSprites  SpriteManager::sMonsters[MON_COUNT]   = {};
Gdiplus::Bitmap*           SpriteManager::sNpcSheets[NPC_TOWN_COUNT] = {};
Gdiplus::Bitmap*           SpriteManager::sUiImages[5]               = {};
Gdiplus::Bitmap*           SpriteManager::sItemImages[2]             = {};
Gdiplus::Bitmap*           SpriteManager::sBossIdle                  = nullptr;
Gdiplus::Bitmap*           SpriteManager::sBossHand                  = nullptr;
Gdiplus::Bitmap*           SpriteManager::sBossHandAttack            = nullptr;
Gdiplus::Bitmap*           SpriteManager::sBossLaserHead             = nullptr;
Gdiplus::Bitmap*           SpriteManager::sBossLaserBody             = nullptr;
Gdiplus::Bitmap*           SpriteManager::sBossSword                 = nullptr;
bool         SpriteManager::sLoaded       = false;
ULONG_PTR    SpriteManager::sGdiplusToken = 0;

// monster subfolder names (Resource/monster/Skeleton/<name>/)
static const wchar_t* kMonFolders[MON_COUNT] = {
    L"Dog", L"Small", L"Big_Normal", L"Magician_Ice"
};
// fallback colours when sprite not loaded
static const COLORREF kMonColors[MON_COUNT] = {
    RGB(180, 130,  80),   // Dog         – sandy brown
    RGB(100, 180,  80),   // Small       – lime green
    RGB( 80, 100, 180),   // Big_Normal  – steel blue
    RGB(160,  80, 200),   // Magician    – purple
};

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

    // --- character sprites ---
    std::wstring dir = FindCharacterDir();
    if (!dir.empty()) {
        for (int i = 0; i < CHAR_COUNT; ++i) {
            sSprites[i].idle = LoadBmp(dir, kCharFolders[i], "idle");
            sSprites[i].run  = LoadBmp(dir, kCharFolders[i], "run");
        }
    }

    // --- monster sprites ---
    std::wstring monDir = FindMonsterDir();
    if (!monDir.empty()) {
        for (int i = 0; i < MON_COUNT; ++i) {
            sMonsters[i].idle = LoadMonBmp(monDir, kMonFolders[i], L"idle");
            sMonsters[i].move = LoadMonBmp(monDir, kMonFolders[i], L"move");
        }
        // Only Big_Normal has an attack sheet
        sMonsters[MON_BIG_NORMAL].attack = LoadMonBmp(monDir, L"Big_Normal", L"attack");
    }

    // --- town NPC sprites ---
    std::wstring npcDir = FindNpcDir();
    if (!npcDir.empty()) {
        sNpcSheets[0] = LoadNpcBmp(npcDir, L"Ability");
        sNpcSheets[1] = LoadNpcBmp(npcDir, L"restaurant");
        sNpcSheets[2] = LoadNpcBmp(npcDir, L"shop");
    }

    // --- UI images ---
    std::wstring uiDir = FindUiDir();
    if (!uiDir.empty()) {
        sUiImages[0] = LoadUiBmp(uiDir, L"STR");
        sUiImages[1] = LoadUiBmp(uiDir, L"INT");
        sUiImages[2] = LoadUiBmp(uiDir, L"DEX");
        sUiImages[3] = LoadUiBmp(uiDir, L"LUK");
        sUiImages[4] = LoadUiBmp(uiDir, L"plus_button");
    }

    // --- item images ---
    std::wstring itemDir = FindItemDir();
    if (!itemDir.empty()) {
        sItemImages[0] = LoadItemBmp(itemDir, L"potion");
        sItemImages[1] = LoadItemBmp(itemDir, L"teleport");
    }

    // --- boss sprites ---
    std::wstring bossDir = FindBossDir();
    if (!bossDir.empty()) {
        sBossIdle = LoadBossSheet(bossDir);
        sBossHand = LoadHandSheet(bossDir);

        // Hand attack sheet: Resource/Boss/Belial/Hand/attack.png
        std::wstring attackPath = bossDir + L"Hand\\attack.png";
        sBossHandAttack = Gdiplus::Bitmap::FromFile(attackPath.c_str());
        if (sBossHandAttack && sBossHandAttack->GetLastStatus() != Gdiplus::Ok) {
            delete sBossHandAttack; sBossHandAttack = nullptr;
        }

        // Laser sprites: Resource/Boss/Belial/Laser/head.png + body.png
        auto loadLaser = [](const std::wstring& path) -> Gdiplus::Bitmap* {
            Gdiplus::Bitmap* b = Gdiplus::Bitmap::FromFile(path.c_str());
            if (!b || b->GetLastStatus() != Gdiplus::Ok) { delete b; return nullptr; }
            return b;
        };
        sBossLaserHead = loadLaser(bossDir + L"Laser\\head.png");
        sBossLaserBody = loadLaser(bossDir + L"Laser\\body.png");

        // Sword (static sprite, no animation)
        Gdiplus::Bitmap* sw = Gdiplus::Bitmap::FromFile((bossDir + L"Sword\\default.png").c_str());
        if (sw && sw->GetLastStatus() == Gdiplus::Ok) sBossSword = sw;
        else { delete sw; sBossSword = nullptr; }
    }

    sLoaded = true;
}

void SpriteManager::Shutdown()
{
    for (int i = 0; i < CHAR_COUNT; ++i) {
        delete sSprites[i].idle; sSprites[i].idle = nullptr;
        delete sSprites[i].run;  sSprites[i].run  = nullptr;
    }
    for (int i = 0; i < MON_COUNT; ++i) {
        delete sMonsters[i].idle;   sMonsters[i].idle   = nullptr;
        delete sMonsters[i].move;   sMonsters[i].move   = nullptr;
        delete sMonsters[i].attack; sMonsters[i].attack = nullptr;
    }
    for (int i = 0; i < NPC_TOWN_COUNT; ++i) {
        delete sNpcSheets[i]; sNpcSheets[i] = nullptr;
    }
    for (int i = 0; i < 5; ++i) {
        delete sUiImages[i]; sUiImages[i] = nullptr;
    }
    for (int i = 0; i < 2; ++i) {
        delete sItemImages[i]; sItemImages[i] = nullptr;
    }
    delete sBossIdle;       sBossIdle       = nullptr;
    delete sBossHand;       sBossHand       = nullptr;
    delete sBossHandAttack; sBossHandAttack = nullptr;
    delete sBossLaserHead;  sBossLaserHead  = nullptr;
    delete sBossLaserBody;  sBossLaserBody  = nullptr;
    delete sBossSword;      sBossSword      = nullptr;
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

// ---------------------------------------------------------------------------
// Monster path discovery
// ---------------------------------------------------------------------------

std::wstring SpriteManager::FindMonsterDir()
{
    WCHAR exeDir[MAX_PATH];
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    WCHAR* last = wcsrchr(exeDir, L'\\');
    if (last) *(last + 1) = L'\0';

    const wchar_t* candidates[] = {
        L"..\\..\\game\\Resource\\monster\\Skeleton\\",
        L"..\\..\\..\\game\\Resource\\monster\\Skeleton\\",
        L"game\\Resource\\monster\\Skeleton\\",
        L"Resource\\monster\\Skeleton\\",
        L"..\\Resource\\monster\\Skeleton\\",
    };

    for (const wchar_t* rel : candidates) {
        WCHAR full[MAX_PATH];
        swprintf_s(full, MAX_PATH, L"%s%s", exeDir, rel);
        WCHAR probe[MAX_PATH];
        swprintf_s(probe, MAX_PATH, L"%sDog\\idle.png", full);
        if (GetFileAttributesW(probe) != INVALID_FILE_ATTRIBUTES)
            return std::wstring(full);
    }
    return L"";
}

Gdiplus::Bitmap* SpriteManager::LoadMonBmp(const std::wstring& monDir,
                                             const wchar_t* subFolder,
                                             const wchar_t* fileName)
{
    wchar_t path[MAX_PATH];
    swprintf_s(path, MAX_PATH, L"%s%s\\%s.png", monDir.c_str(), subFolder, fileName);

    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    return bmp;
}

// ---------------------------------------------------------------------------
// Monster drawing
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Town NPC path discovery and loading
// ---------------------------------------------------------------------------

std::wstring SpriteManager::FindNpcDir()
{
    WCHAR exeDir[MAX_PATH];
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    WCHAR* last = wcsrchr(exeDir, L'\\');
    if (last) *(last + 1) = L'\0';

    const wchar_t* candidates[] = {
        L"..\\..\\game\\Resource\\NPC\\",
        L"..\\..\\..\\game\\Resource\\NPC\\",
        L"game\\Resource\\NPC\\",
        L"Resource\\NPC\\",
        L"..\\Resource\\NPC\\",
    };

    for (const wchar_t* rel : candidates) {
        WCHAR full[MAX_PATH];
        swprintf_s(full, MAX_PATH, L"%s%s", exeDir, rel);
        WCHAR probe[MAX_PATH];
        swprintf_s(probe, MAX_PATH, L"%sAbility.png", full);
        if (GetFileAttributesW(probe) != INVALID_FILE_ATTRIBUTES)
            return std::wstring(full);
    }
    return L"";
}

Gdiplus::Bitmap* SpriteManager::LoadNpcBmp(const std::wstring& dir, const wchar_t* fileName)
{
    wchar_t path[MAX_PATH];
    swprintf_s(path, MAX_PATH, L"%s%s.png", dir.c_str(), fileName);
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    return bmp;
}

// ---------------------------------------------------------------------------
// UI image path discovery, loading, drawing
// ---------------------------------------------------------------------------

std::wstring SpriteManager::FindUiDir()
{
    WCHAR exeDir[MAX_PATH];
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    WCHAR* last = wcsrchr(exeDir, L'\\');
    if (last) *(last + 1) = L'\0';

    const wchar_t* candidates[] = {
        L"..\\..\\game\\Resource\\Ui\\",
        L"..\\..\\..\\game\\Resource\\Ui\\",
        L"game\\Resource\\Ui\\",
        L"Resource\\Ui\\",
        L"..\\Resource\\Ui\\",
    };

    for (const wchar_t* rel : candidates) {
        WCHAR full[MAX_PATH];
        swprintf_s(full, MAX_PATH, L"%s%s", exeDir, rel);
        WCHAR probe[MAX_PATH];
        swprintf_s(probe, MAX_PATH, L"%sSTR.png", full);
        if (GetFileAttributesW(probe) != INVALID_FILE_ATTRIBUTES)
            return std::wstring(full);
    }
    return L"";
}

Gdiplus::Bitmap* SpriteManager::LoadUiBmp(const std::wstring& dir, const wchar_t* fileName)
{
    wchar_t path[MAX_PATH];
    swprintf_s(path, MAX_PATH, L"%s%s.png", dir.c_str(), fileName);
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    return bmp;
}

void SpriteManager::DrawUiImage(HDC hdc, int uiIdx, int x, int y, int w, int h)
{
    if (uiIdx < 0 || uiIdx >= 5) return;
    Gdiplus::Bitmap* bmp = sUiImages[uiIdx];
    if (!bmp) return;

    Gdiplus::Graphics g(hdc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.DrawImage(bmp, x, y, w, h);
}

// ---------------------------------------------------------------------------
// Town NPC drawing
// ---------------------------------------------------------------------------

void SpriteManager::DrawTownNpc(HDC hdc, int visualId, int frame,
                                 int screenX, int screenY, int drawW, int drawH)
{
    int idx = visualId - NPC_ABILITY; // 4->0, 5->1, 6->2
    if (idx < 0 || idx >= NPC_TOWN_COUNT) return;

    const int frameCounts[NPC_TOWN_COUNT] = {
        NPC_ABILITY_FRAMES, NPC_RESTAURANT_FRAMES, NPC_SHOP_FRAMES
    };

    int destX = screenX - drawW / 2;
    int destY = screenY - drawH / 2;

    Gdiplus::Bitmap* sheet = sNpcSheets[idx];
    if (!sheet) {
        // Fallback: colored rectangle
        static const COLORREF kNpcColors[NPC_TOWN_COUNT] = {
            RGB(100, 200, 255), // Ability - light blue
            RGB(255, 180, 80),  // Restaurant - orange
            RGB(120, 220, 120), // Shop - green
        };
        HBRUSH br  = CreateSolidBrush(kNpcColors[idx]);
        HBRUSH old = (HBRUSH)SelectObject(hdc, br);
        Rectangle(hdc, destX, destY, destX + drawW, destY + drawH);
        SelectObject(hdc, old);
        DeleteObject(br);
        return;
    }

    Gdiplus::Graphics g(hdc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    DrawFrame(g, sheet, frame % frameCounts[idx], frameCounts[idx],
              destX, destY, drawW, drawH, false);
}

void SpriteManager::DrawMonster(HDC hdc, int monType, bool isMoving, bool isAttacking, int frame,
                                 int screenX, int screenY, int drawW, int drawH, bool flipH)
{
    if (monType < 0 || monType >= MON_COUNT) monType = MON_DOG;

    const MonFrameInfo& fi = MON_FRAMES[monType];

    // Priority: attack > move > idle
    Gdiplus::Bitmap* sheet = nullptr;
    int totalFrames = fi.idleFrames;
    if (isAttacking && sMonsters[monType].attack && fi.attackFrames > 0) {
        sheet       = sMonsters[monType].attack;
        totalFrames = fi.attackFrames;
    } else if (isMoving && sMonsters[monType].move && fi.moveFrames > 0) {
        sheet       = sMonsters[monType].move;
        totalFrames = fi.moveFrames;
    } else {
        sheet       = sMonsters[monType].idle;
        totalFrames = fi.idleFrames;
    }
    if (totalFrames <= 0) totalFrames = 1;

    int destX = screenX - drawW / 2;
    int destY = screenY - drawH / 2;

    if (!sheet) {
        HBRUSH br  = CreateSolidBrush(kMonColors[monType]);
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

// ---------------------------------------------------------------------------
// Shop item images (Resource/item/)
// ---------------------------------------------------------------------------

std::wstring SpriteManager::FindItemDir()
{
    WCHAR exeDir[MAX_PATH];
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    WCHAR* last = wcsrchr(exeDir, L'\\');
    if (last) *(last + 1) = L'\0';

    const wchar_t* candidates[] = {
        L"..\\..\\game\\Resource\\item\\",
        L"..\\..\\..\\game\\Resource\\item\\",
        L"game\\Resource\\item\\",
        L"Resource\\item\\",
        L"..\\Resource\\item\\",
    };

    for (const wchar_t* rel : candidates) {
        WCHAR full[MAX_PATH];
        swprintf_s(full, MAX_PATH, L"%s%s", exeDir, rel);
        WCHAR probe[MAX_PATH];
        swprintf_s(probe, MAX_PATH, L"%spotion.png", full);
        if (GetFileAttributesW(probe) != INVALID_FILE_ATTRIBUTES)
            return std::wstring(full);
    }
    return L"";
}

Gdiplus::Bitmap* SpriteManager::LoadItemBmp(const std::wstring& dir, const wchar_t* fileName)
{
    wchar_t path[MAX_PATH];
    swprintf_s(path, MAX_PATH, L"%s%s.png", dir.c_str(), fileName);
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    return bmp;
}

void SpriteManager::DrawItemImage(HDC hdc, int itemIdx, int x, int y, int w, int h)
{
    if (itemIdx < 0 || itemIdx >= 2) return;
    Gdiplus::Bitmap* bmp = sItemImages[itemIdx];
    if (!bmp) return;

    Gdiplus::Graphics g(hdc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.DrawImage(bmp, x, y, w, h);
}

// ---------------------------------------------------------------------------
// Boss Belial sprites (Resource/Boss/Belial/)
// ---------------------------------------------------------------------------

std::wstring SpriteManager::FindBossDir()
{
    WCHAR exeDir[MAX_PATH];
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    WCHAR* last = wcsrchr(exeDir, L'\\');
    if (last) *(last + 1) = L'\0';

    const wchar_t* candidates[] = {
        L"..\\..\\game\\Resource\\Boss\\Belial\\",
        L"..\\..\\..\\game\\Resource\\Boss\\Belial\\",
        L"game\\Resource\\Boss\\Belial\\",
        L"Resource\\Boss\\Belial\\",
        L"..\\Resource\\Boss\\Belial\\",
    };

    for (const wchar_t* rel : candidates) {
        WCHAR full[MAX_PATH];
        swprintf_s(full, MAX_PATH, L"%s%s", exeDir, rel);
        WCHAR probe[MAX_PATH];
        swprintf_s(probe, MAX_PATH, L"%sHead\\idle.png", full);
        if (GetFileAttributesW(probe) != INVALID_FILE_ATTRIBUTES)
            return std::wstring(full);
    }
    return L"";
}

Gdiplus::Bitmap* SpriteManager::LoadBossSheet(const std::wstring& dir)
{
    wchar_t path[MAX_PATH];
    swprintf_s(path, MAX_PATH, L"%sHead\\idle.png", dir.c_str());
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    return bmp;
}

void SpriteManager::DrawBoss(HDC hdc, int frame, int screenX, int screenY, int drawW, int drawH)
{
    int destX = screenX - drawW / 2;
    int destY = screenY - drawH / 2;

    if (!sBossIdle) {
        HBRUSH br  = CreateSolidBrush(RGB(160, 0, 0));
        HBRUSH old = (HBRUSH)SelectObject(hdc, br);
        Rectangle(hdc, destX, destY, destX + drawW, destY + drawH);
        SelectObject(hdc, old);
        DeleteObject(br);
        return;
    }

    Gdiplus::Graphics g(hdc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    DrawFrame(g, sBossIdle, frame % BOSS_BELIAL_IDLE_FRAMES, BOSS_BELIAL_IDLE_FRAMES,
              destX, destY, drawW, drawH, false);
}

// ---------------------------------------------------------------------------
// Boss Belial hand sprite (Resource/Boss/Belial/Hand/idle.png, 9 frames)
// flipH=true flips horizontally to produce the right hand from the left-hand sheet.
// ---------------------------------------------------------------------------

Gdiplus::Bitmap* SpriteManager::LoadHandSheet(const std::wstring& dir)
{
    wchar_t path[MAX_PATH];
    swprintf_s(path, MAX_PATH, L"%sHand\\idle.png", dir.c_str());
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(path);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    return bmp;
}

void SpriteManager::DrawHand(HDC hdc, int frame, int screenX, int screenY,
                              int drawW, int drawH, bool flipH, bool isAttacking)
{
    int destX = screenX - drawW / 2;
    int destY = screenY - drawH / 2;

    Gdiplus::Bitmap* sheet    = isAttacking ? sBossHandAttack : sBossHand;
    int              numFrames = isAttacking ? BOSS_BELIAL_HAND_ATTACK_FRAMES
                                             : BOSS_BELIAL_HAND_FRAMES;

    if (!sheet) {
        HBRUSH br  = CreateSolidBrush(isAttacking ? RGB(200, 80, 0) : RGB(80, 0, 120));
        HBRUSH old = (HBRUSH)SelectObject(hdc, br);
        Rectangle(hdc, destX, destY, destX + drawW, destY + drawH);
        SelectObject(hdc, old);
        DeleteObject(br);
        return;
    }

    Gdiplus::Graphics g(hdc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    DrawFrame(g, sheet, frame % numFrames, numFrames, destX, destY, drawW, drawH, flipH);
}

// Draw the laser beam for a given animation frame (0-6).
// Head sprites are drawn at the hand positions; body fills between them.
// Left hand head is drawn normally; right hand head is flipped horizontally.
void SpriteManager::DrawLaser(HDC hdc, int screenCenterY, int leftHandX, int rightHandX,
                               int tileH, int laserFrame)
{
    constexpr int LASER_FRAMES = 7;
    int frame = max(0, min(laserFrame, LASER_FRAMES - 1));

    // Laser visual height = 3 tiles (center ± 1)
    int laserH = tileH * 3;
    int top    = screenCenterY - laserH / 2;

    // Head width: use sprite's natural per-frame width if loaded, else 1 tile
    int headW = tileH;
    if (sBossLaserHead) {
        UINT sw = sBossLaserHead->GetWidth();
        UINT sh = sBossLaserHead->GetHeight();
        if (sh > 0) headW = (int)((float)sw / LASER_FRAMES * laserH / sh);
    }

    int bodyLeft  = leftHandX  + headW / 2;
    int bodyRight = rightHandX - headW / 2;
    int bodyW     = bodyRight - bodyLeft;

    Gdiplus::Graphics g(hdc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    if (sBossLaserBody && bodyW > 0) {
        // Tile the body sprite between the two heads
        UINT sw = sBossLaserBody->GetWidth();
        UINT sh = sBossLaserBody->GetHeight();
        if (sw > 0 && sh > 0) {
            int segW = (int)((float)sw / LASER_FRAMES * laserH / sh);
            if (segW < 1) segW = 1;
            for (int x = bodyLeft; x < bodyRight; x += segW) {
                int w = min(segW, bodyRight - x);
                Gdiplus::Rect dest(x, top, w, laserH);
                // Source: one frame column from the body sheet
                INT srcX = (INT)((float)sw / LASER_FRAMES * frame);
                INT srcW = (INT)((float)sw / LASER_FRAMES);
                g.DrawImage(sBossLaserBody, dest,
                            srcX, 0, srcW, (INT)sh, Gdiplus::UnitPixel);
            }
        }
    } else if (!sBossLaserBody && bodyW > 0) {
        // Fallback body
        HPEN np = (HPEN)GetStockObject(NULL_PEN);
        HPEN op = (HPEN)SelectObject(hdc, np);
        HBRUSH br = CreateSolidBrush(RGB(255, 40, 40));
        HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
        Rectangle(hdc, bodyLeft, top, bodyRight, top + laserH);
        SelectObject(hdc, ob); SelectObject(hdc, op);
        DeleteObject(br);
    }

    auto drawHead = [&](int centerX, bool flipH) {
        int destX = centerX - headW / 2;
        if (sBossLaserHead) {
            DrawFrame(g, sBossLaserHead, frame, LASER_FRAMES,
                      destX, top, headW, laserH, flipH);
        } else {
            // Fallback head
            HPEN np = (HPEN)GetStockObject(NULL_PEN);
            HPEN op = (HPEN)SelectObject(hdc, np);
            HBRUSH br = CreateSolidBrush(RGB(255, 140, 0));
            HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
            Rectangle(hdc, destX, top, destX + headW, top + laserH);
            SelectObject(hdc, ob); SelectObject(hdc, op);
            DeleteObject(br);
        }
    };

    drawHead(leftHandX,  false);  // left hand head — normal
    drawHead(rightHandX, true);   // right hand head — flipped
}

// Draw a falling sword (static sprite) centered at (screenCenterX, screenCenterY).
// drawW = 1 tile wide, drawH = 3 tiles tall.
void SpriteManager::DrawSword(HDC hdc, int screenCenterX, int screenCenterY,
                               int drawW, int drawH)
{
    int destX = screenCenterX - drawW / 2;
    int destY = screenCenterY - drawH / 2;

    if (sBossSword) {
        Gdiplus::Graphics g(hdc);
        g.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        Gdiplus::Rect dest(destX, destY, drawW, drawH);
        g.DrawImage(sBossSword, dest,
                    0, 0, (INT)sBossSword->GetWidth(), (INT)sBossSword->GetHeight(),
                    Gdiplus::UnitPixel);
    } else {
        // Fallback: thin blue-white rectangle (sword silhouette)
        HBRUSH br = CreateSolidBrush(RGB(180, 200, 255));
        RECT r = { destX + drawW / 3, destY, destX + drawW * 2 / 3, destY + drawH };
        FillRect(hdc, &r, br);
        DeleteObject(br);
    }
}
