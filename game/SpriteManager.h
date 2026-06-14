#pragma once
#include "framework.h"
#include <string>

// Character visual IDs (must match protocol_2026.h C2S_CharSelect visual_id)
enum { CHAR_BASE = 0, CHAR_ALICE, CHAR_METAL_PLATE, CHAR_PICKAX, CHAR_RED_LOTUS, CHAR_COUNT };

constexpr int SPRITE_IDLE_FRAMES = 5;
constexpr int SPRITE_RUN_FRAMES  = 8;

// Monster visual IDs (sent in S2C_ADD_OBJECT.visual_id for NPC objects)
// 0=Dog  1=Small  2=Big_Normal  3=Magician_Ice
enum { MON_DOG=0, MON_SMALL=1, MON_BIG_NORMAL=2, MON_MAGICIAN=3, MON_COUNT=4 };

// Town NPC visual IDs (4=Ability 5=Restaurant 6=Shop)
enum { NPC_ABILITY=4, NPC_RESTAURANT=5, NPC_SHOP=6, NPC_TOWN_COUNT=3 };
// Frame counts for each town NPC sprite sheet
constexpr int NPC_ABILITY_FRAMES    = 6;
constexpr int NPC_RESTAURANT_FRAMES = 6;
constexpr int NPC_SHOP_FRAMES       = 4;

// Per-monster frame counts (measured from actual sprite sheet dimensions)
// Dog         idle=100/20=5   move=140/20=7
// Small       idle=14/14=1    move=84/14=6
// Big_Normal  idle=165/33=5   move=198/33=6
// Magician_Ice idle=192/32=6  move=none
struct MonFrameInfo { int idleFrames; int moveFrames; };
constexpr MonFrameInfo MON_FRAMES[4] = {
    { 5, 7 },   // Dog
    { 1, 6 },   // Small
    { 5, 6 },   // Big_Normal
    { 6, 0 },   // Magician_Ice (no move sheet)
};
// Keep legacy constant for existing code that uses it
constexpr int MON_IDLE_FRAMES = 5;

class SpriteManager
{
public:
    static void Init();
    static void Shutdown();

    // Draw one player animation frame centered at (screenX, screenY).
    static void DrawSprite(HDC hdc, int charId, bool isRunning, int frame,
                           int screenX, int screenY, int drawW, int drawH, bool flipH);

    // Draw idle frame 0 — used on the character-select screen.
    static void DrawPreview(HDC hdc, int charId, int cx, int cy, int size);

    // Draw one monster animation frame centered at (screenX, screenY).
    // isMoving selects move sheet vs idle sheet; flipH mirrors left/right.
    static void DrawMonster(HDC hdc, int monType, bool isMoving, int frame,
                            int screenX, int screenY, int drawW, int drawH, bool flipH = false);

    // Draw a town NPC sprite (Ability/Restaurant/Shop) centered at (screenX, screenY).
    static void DrawTownNpc(HDC hdc, int visualId, int frame,
                            int screenX, int screenY, int drawW, int drawH);

    // Draw a UI image (stat icon or button) at (x, y) with given size.
    static void DrawUiImage(HDC hdc, int uiIdx, int x, int y, int w, int h);
    // uiIdx: 0=STR 1=INT 2=DEX 3=LUK 4=plus_button

    // Draw a shop item icon at (x, y) with given size.
    static void DrawItemImage(HDC hdc, int itemIdx, int x, int y, int w, int h);
    // itemIdx: 0=HP Potion  1=Teleport Scroll

    static bool IsLoaded() { return sLoaded; }

private:
    // ---- player sprites ----
    struct CharSprites {
        Gdiplus::Bitmap* idle = nullptr;
        Gdiplus::Bitmap* run  = nullptr;
    };
    static CharSprites  sSprites[CHAR_COUNT];

    // ---- monster sprites ----
    struct MonSprites {
        Gdiplus::Bitmap* idle = nullptr;
        Gdiplus::Bitmap* move = nullptr;  // nullptr for types that have no move sheet
    };
    static MonSprites   sMonsters[MON_COUNT];

    // ---- town NPC sprites (single sheet, no move animation) ----
    static Gdiplus::Bitmap* sNpcSheets[NPC_TOWN_COUNT]; // [0]=Ability [1]=Restaurant [2]=Shop

    // ---- UI images: [0]=STR [1]=INT [2]=DEX [3]=LUK [4]=plus_button ----
    static Gdiplus::Bitmap* sUiImages[5];

    static std::wstring FindNpcDir();
    static Gdiplus::Bitmap* LoadNpcBmp(const std::wstring& dir, const wchar_t* fileName);
    static std::wstring FindUiDir();
    static Gdiplus::Bitmap* LoadUiBmp(const std::wstring& dir, const wchar_t* fileName);
    static std::wstring FindItemDir();
    static Gdiplus::Bitmap* LoadItemBmp(const std::wstring& dir, const wchar_t* fileName);

    // ---- shop item images: [0]=potion  [1]=teleport ----
    static Gdiplus::Bitmap* sItemImages[2];

    static bool         sLoaded;
    static ULONG_PTR    sGdiplusToken;

    static std::wstring FindCharacterDir();
    static std::wstring FindMonsterDir();

    static Gdiplus::Bitmap* LoadBmp(const std::wstring& dir,
                                    const char* charFolder,
                                    const char* stateName);
    static Gdiplus::Bitmap* LoadMonBmp(const std::wstring& monDir,
                                       const wchar_t* subFolder,
                                       const wchar_t* fileName);

    static void DrawFrame(Gdiplus::Graphics& g, Gdiplus::Bitmap* sheet,
                          int frameIdx, int totalFrames,
                          int destX, int destY, int drawW, int drawH, bool flipH);
};
