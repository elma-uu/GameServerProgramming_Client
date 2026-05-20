#include "Map.h"
#include "Game.h"

Map::Map()
    : mMapTilesX(0)
    , mMapTilesY(0)
    , mTileSize(0)
    , mMapWidthPixel(0)
    , mMapHeightPixel(0)
{
}

Map::~Map()
{
}

void Map::Initialize(int mapTilesX, int mapTilesY, int tileSize)
{
    mMapTilesX = mapTilesX;
    mMapTilesY = mapTilesY;
    mTileSize = tileSize;
    mMapWidthPixel = mapTilesX * tileSize;
    mMapHeightPixel = mapTilesY * tileSize;
}

void Map::Render(HDC hdc)
{
    Camera* camera = GAME.GetCamera();

    // 펜 생성 (회색 그리드)
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);

    int cameraX = camera->GetCameraX();
    int cameraY = camera->GetCameraY();
    int viewportWidth = camera->GetViewportWidth();
    int viewportHeight = camera->GetViewportHeight();

    // 시작 타일 계산
    int startTileX = cameraX / mTileSize;
    int startTileY = cameraY / mTileSize;

    // 끝 타일 계산 (뷰포트를 벗어날 때까지)
    int endTileX = (cameraX + viewportWidth) / mTileSize + 1;
    int endTileY = (cameraY + viewportHeight) / mTileSize + 1;

    // 범위 제한
    if (startTileX < 0) startTileX = 0;
    if (startTileY < 0) startTileY = 0;
    if (endTileX > mMapTilesX) endTileX = mMapTilesX;
    if (endTileY > mMapTilesY) endTileY = mMapTilesY;

    // 세로 선 그리기
    for (int x = startTileX; x <= endTileX; x++)
    {
        int worldX = x * mTileSize;
        int screenX = worldX - cameraX;

        MoveToEx(hdc, screenX, 0, nullptr);
        LineTo(hdc, screenX, viewportHeight);
    }

    // 가로 선 그리기
    for (int y = startTileY; y <= endTileY; y++)
    {
        int worldY = y * mTileSize;
        int screenY = worldY - cameraY;

        MoveToEx(hdc, 0, screenY, nullptr);
        LineTo(hdc, viewportWidth, screenY);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
}
