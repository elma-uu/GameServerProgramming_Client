#include "Map.h"
#include "Game.h"
#include "SpriteManager.h"
#include <fstream>
#include <string>
#include <cstring>

unsigned char Map::sMapData[MAP_ROWS][MAP_COLS] = {};
bool          Map::sMapLoaded = false;

Map::Map()
    : mMapTilesX(0), mMapTilesY(0), mTileSize(0)
    , mMapWidthPixel(0), mMapHeightPixel(0)
{
}

Map::~Map()
{
}

void Map::Initialize(int mapTilesX, int mapTilesY, int tileSize)
{
    mMapTilesX = mapTilesX;
    mMapTilesY = mapTilesY;
    mTileSize  = tileSize;
    mMapWidthPixel  = mapTilesX * tileSize;
    mMapHeightPixel = mapTilesY * tileSize;
}

bool Map::LoadMapFile()
{
    memset(sMapData, 1, sizeof(sMapData));  // default: all walls

    // Try several paths relative to exe location
    WCHAR exeDir[MAX_PATH];
    GetModuleFileNameW(nullptr, exeDir, MAX_PATH);
    WCHAR* last = wcsrchr(exeDir, L'\\');
    if (last) *(last + 1) = L'\0';

    const wchar_t* candidates[] = {
        L"Resource\\Map\\map.txt",
        L"..\\Resource\\Map\\map.txt",
        L"..\\..\\game\\Resource\\Map\\map.txt",
        L"..\\..\\..\\game\\Resource\\Map\\map.txt",
        L"game\\Resource\\Map\\map.txt",
    };

    for (const wchar_t* rel : candidates) {
        WCHAR full[MAX_PATH];
        swprintf_s(full, MAX_PATH, L"%s%s", exeDir, rel);

        std::ifstream f(full);
        if (!f.is_open()) continue;

        std::string line;
        int row = 0;
        while (row < MAP_ROWS && std::getline(f, line)) {
            for (int col = 0; col < MAP_COLS && col < (int)line.size(); ++col)
                sMapData[row][col] = (line[col] == '0') ? 0 : 1;
            ++row;
        }
        sMapLoaded = true;
        return true;
    }
    return false;
}

bool Map::IsWalkable(int wx, int wy)
{
    int col = wx - MAP_ORIGIN_X;
    int row = wy - MAP_ORIGIN_Y;
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS)
        return true;  // outside defined map = open world
    return sMapData[row][col] == 0;
}

void Map::Render(HDC hdc)
{
    Camera* camera = GAME.GetCamera();

    int cameraX = camera->GetCameraX();
    int cameraY = camera->GetCameraY();
    int viewportWidth  = camera->GetViewportWidth();
    int viewportHeight = camera->GetViewportHeight();

    int startTileX = cameraX / mTileSize;
    int startTileY = cameraY / mTileSize;
    int endTileX   = (cameraX + viewportWidth)  / mTileSize + 1;
    int endTileY   = (cameraY + viewportHeight) / mTileSize + 1;

    if (startTileX < 0) startTileX = 0;
    if (startTileY < 0) startTileY = 0;
    if (endTileX > mMapTilesX) endTileX = mMapTilesX;
    if (endTileY > mMapTilesY) endTileY = mMapTilesY;

    for (int ty = startTileY; ty <= endTileY; ++ty)
    {
        for (int tx = startTileX; tx <= endTileX; ++tx)
        {
            bool isWall = !IsWalkable(tx, ty);
            int screenX = tx * mTileSize - cameraX;
            int screenY = ty * mTileSize - cameraY;
            SpriteManager::DrawTile(hdc, !isWall, screenX, screenY, mTileSize);
        }
    }
}
