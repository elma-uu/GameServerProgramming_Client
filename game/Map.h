#pragma once

#include <Windows.h>

class Map
{
public:
    Map();
    ~Map();

    void Initialize(int mapTilesX, int mapTilesY, int tileSize);
    void Render(HDC hdc);

private:
    int mMapTilesX;      // 맵의 타일 개수 (X) - 2000
    int mMapTilesY;      // 맵의 타일 개수 (Y) - 2000
    int mTileSize;       // 한 타일의 크기 (픽셀) - 100
    int mMapWidthPixel;  // 맵의 전체 가로 크기 (픽셀) - 200,000 (2000*100)
    int mMapHeightPixel; // 맵의 전체 세로 크기 (픽셀) - 200,000 (2000*100)
};
