#include "Camera.h"

Camera::Camera()
    : mCameraX(0)
    , mCameraY(0)
    , mTargetX(0)
    , mTargetY(0)
    , mViewportWidth(0)
    , mViewportHeight(0)
    , mMapWidth(0)
    , mMapHeight(0)
    , mTileSize(0)
    , mCameraEasing(0.1f)
{
}

Camera::~Camera()
{
}

void Camera::Initialize(int mapWidth, int mapHeight, int tileSize, int viewTilesX, int viewTilesY)
{
    mMapWidth = mapWidth * tileSize;
    mMapHeight = mapHeight * tileSize;
    mTileSize = tileSize;
    mViewportWidth = viewTilesX * tileSize;
    mViewportHeight = viewTilesY * tileSize;

    // 초기 카메라 위치를 맵 중심에 설정
    mCameraX = (mMapWidth - mViewportWidth) / 2;
    mCameraY = (mMapHeight - mViewportHeight) / 2;
    mTargetX = mCameraX;
    mTargetY = mCameraY;

    ClampCamera();
}

void Camera::SetTarget(int targetX, int targetY)
{
    // Avatar 위치를 카메라의 중심으로 설정
    mTargetX = targetX - (mViewportWidth / 2);
    mTargetY = targetY - (mViewportHeight / 2);
}

void Camera::Update()
{
    // 카메라를 스무딩하면서 이동
    mCameraX = (int)(mCameraX + (mTargetX - mCameraX) * mCameraEasing);
    mCameraY = (int)(mCameraY + (mTargetY - mCameraY) * mCameraEasing);

    ClampCamera();
}

void Camera::ClampCamera()
{
    // 카메라가 맵 범위 내에 있도록 제한
    if (mCameraX < 0)
        mCameraX = 0;
    if (mCameraY < 0)
        mCameraY = 0;
    if (mCameraX + mViewportWidth > mMapWidth)
        mCameraX = mMapWidth - mViewportWidth;
    if (mCameraY + mViewportHeight > mMapHeight)
        mCameraY = mMapHeight - mViewportHeight;
}

void Camera::WorldToScreen(int worldX, int worldY, int& screenX, int& screenY) const
{
    screenX = worldX - mCameraX;
    screenY = worldY - mCameraY;
}

void Camera::ScreenToWorld(int screenX, int screenY, int& worldX, int& worldY) const
{
    worldX = screenX + mCameraX;
    worldY = screenY + mCameraY;
}

bool Camera::IsInViewport(int worldX, int worldY, int width, int height) const
{
    // 객체 범위와 카메라 뷰포트 범위가 겹치는지 확인 (AABB collision)
    return !(worldX + width < mCameraX ||
        worldX > mCameraX + mViewportWidth ||
        worldY + height < mCameraY ||
        worldY > mCameraY + mViewportHeight);
}
