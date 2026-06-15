#pragma once

#include <Windows.h>

// Must match server MapData.h
constexpr int MAP_ORIGIN_X = 950;
constexpr int MAP_ORIGIN_Y = 950;
constexpr int MAP_COLS     = 100;
constexpr int MAP_ROWS     = 100;

class Map
{
public:
    Map();
    ~Map();

    void Initialize(int mapTilesX, int mapTilesY, int tileSize);
    bool LoadMapFile();   // loads Resource/Map/map.txt
    void Render(HDC hdc);

    // Returns false if world tile (wx,wy) is a wall. Tiles outside the map = walkable.
    static bool IsWalkable(int wx, int wy);

private:
    int mMapTilesX;
    int mMapTilesY;
    int mTileSize;
    int mMapWidthPixel;
    int mMapHeightPixel;

    static unsigned char sMapData[MAP_ROWS][MAP_COLS];
    static bool          sMapLoaded;
};
