#include "MiniMap.h"
#include "Game.h"

MiniMap::MiniMap()
    : mMapTilesX(0)
    , mMapTilesY(0)
    , mTileSize(0)
    , mMapWidthPixel(0)
    , mMapHeightPixel(0)
    , mScaleX(0.0f)
    , mScaleY(0.0f)
{
}

MiniMap::~MiniMap()
{
}

void MiniMap::Initialize(int mapTilesX, int mapTilesY, int tileSize)
{
    mMapTilesX = mapTilesX;
    mMapTilesY = mapTilesY;
    mTileSize = tileSize;
    mMapWidthPixel = mapTilesX * tileSize;
    mMapHeightPixel = mapTilesY * tileSize;

    // 미니맵 스케일 계산
    mScaleX = (float)MINIMAP_WIDTH / mMapWidthPixel;
    mScaleY = (float)MINIMAP_HEIGHT / mMapHeightPixel;
}

void MiniMap::Render(HDC hdc)
{
    Camera* camera = GAME.GetCamera();
    Player* player = GAME.GetPlayer();

    // 미니맵 배경 (검은색)
    HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, blackBrush);
    Rectangle(hdc, MINIMAP_X, MINIMAP_Y, MINIMAP_X + MINIMAP_WIDTH, MINIMAP_Y + MINIMAP_HEIGHT);
    SelectObject(hdc, oldBrush);
    DeleteObject(blackBrush);

    // 미니맵 테두리 (흰색)
    HPEN whitePen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    HPEN oldPen = (HPEN)SelectObject(hdc, whitePen);
    Rectangle(hdc, MINIMAP_X, MINIMAP_Y, MINIMAP_X + MINIMAP_WIDTH, MINIMAP_Y + MINIMAP_HEIGHT);
    SelectObject(hdc, oldPen);
    DeleteObject(whitePen);

    // 카메라 뷰포트 표시 (초록색)
    int cameraX = camera->GetCameraX();
    int cameraY = camera->GetCameraY();
    int viewportWidth = camera->GetViewportWidth();
    int viewportHeight = camera->GetViewportHeight();

    int viewMinX = (int)(cameraX * mScaleX);
    int viewMinY = (int)(cameraY * mScaleY);
    int viewMaxX = (int)((cameraX + viewportWidth) * mScaleX);
    int viewMaxY = (int)((cameraY + viewportHeight) * mScaleY);

    HPEN greenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
    oldPen = (HPEN)SelectObject(hdc, greenPen);
    Rectangle(hdc, 
        MINIMAP_X + viewMinX, 
        MINIMAP_Y + viewMinY, 
        MINIMAP_X + viewMaxX, 
        MINIMAP_Y + viewMaxY);
    SelectObject(hdc, oldPen);
    DeleteObject(greenPen);

    // 플레이어 위치 표시 (빨간색)
    int playerX = player->GetX();
    int playerY = player->GetY();

    int playerMinimapX = (int)(playerX * mScaleX);
    int playerMinimapY = (int)(playerY * mScaleY);

    HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
    oldBrush = (HBRUSH)SelectObject(hdc, redBrush);
    Rectangle(hdc,
        MINIMAP_X + playerMinimapX - 3,
        MINIMAP_Y + playerMinimapY - 3,
        MINIMAP_X + playerMinimapX + 3,
        MINIMAP_Y + playerMinimapY + 3);
    SelectObject(hdc, oldBrush);
    DeleteObject(redBrush);
}
