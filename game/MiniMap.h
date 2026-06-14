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
    static const int MINIMAP_WIDTH  = 150;
    static const int MINIMAP_HEIGHT = 150;
    static const int MINIMAP_X = 800 - MINIMAP_WIDTH  - 10;  // 640  (right side)
    static const int MINIMAP_Y = 600 - MINIMAP_HEIGHT - 10;  // 440  (bottom)

    // �� ����
    int mMapTilesX;
    int mMapTilesY;
    int mTileSize;
    int mMapWidthPixel;
    int mMapHeightPixel;

    // �̴ϸ� ������
    float mScaleX;
    float mScaleY;
};
