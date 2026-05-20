#pragma once

#include <Windows.h>

class MiniMap
{
public:
    MiniMap();
    ~MiniMap();

    void Initialize(int mapTilesX, int mapTilesY, int tileSize);
    void Render(HDC hdc);

private:
    // 미니맵 위치 및 크기
    static const int MINIMAP_WIDTH = 150;
    static const int MINIMAP_HEIGHT = 150;
    static const int MINIMAP_X = 10;
    static const int MINIMAP_Y = 10;

    // 맵 정보
    int mMapTilesX;
    int mMapTilesY;
    int mTileSize;
    int mMapWidthPixel;
    int mMapHeightPixel;

    // 미니맵 스케일
    float mScaleX;
    float mScaleY;
};
