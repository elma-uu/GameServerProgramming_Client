#pragma once

#include <Windows.h>
#include <cmath>

class Camera
{
public:
    Camera();
    ~Camera();

    // 카메라 초기화
    void Initialize(int mapWidth, int mapHeight, int tileSize, int viewTilesX, int viewTilesY);

    // 카메라 중심을 Avatar 위치로 설정
    void SetTarget(int targetX, int targetY);

    // 월드 좌표를 스크린 좌표로 변환
    void WorldToScreen(int worldX, int worldY, int& screenX, int& screenY) const;

    // 스크린 좌표를 월드 좌표로 변환
    void ScreenToWorld(int screenX, int screenY, int& worldX, int& worldY) const;

    // 카메라 업데이트
    void Update();

    // Getter
    int GetCameraX() const { return mCameraX; }
    int GetCameraY() const { return mCameraY; }
    int GetViewportWidth() const { return mViewportWidth; }
    int GetViewportHeight() const { return mViewportHeight; }

    // 특정 월드 영역이 뷰포트 안에 있는지 확인
    bool IsInViewport(int worldX, int worldY, int width, int height) const;

private:
    // 카메라가 맵 범위 내에 있도록 제한
    void ClampCamera();

private:
    // 카메라 위치 (픽셀 단위)
    int mCameraX;
    int mCameraY;

    // 목표 위치 (Avatar 위치)
    int mTargetX;
    int mTargetY;

    // 뷰포트 크기 (픽셀 단위)
    int mViewportWidth;
    int mViewportHeight;

    // 맵 크기 (픽셀 단위)
    int mMapWidth;
    int mMapHeight;

    // 타일 크기 (픽셀 단위)
    int mTileSize;

    // 카메라 움직임 스무딩
    float mCameraEasing;
};
