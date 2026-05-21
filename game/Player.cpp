#include "Player.h"
#include "Player.h"
#include "Input.h"
#include "Game.h"
#include "Camera.h"
#include "Time.h"

// 한 프레임당 이동 거리 (테스트용)
// 60FPS 기준: 200픽셀/초 ÷ 60fps = 약 3.33픽셀/프레임
const int PLAYER_SIZE = 50;    // player sprite size
const int TILE_SIZE = PLAYER_SIZE;  // 맵 한칸 크기 = 플레이어 크기
const float MOVE_TIME = 0.5f;  // 0.5초에 1칸
const int PLAYER_SPEED = (int)(TILE_SIZE / MOVE_TIME);  // 100 픽셀/초
const int FRAME_MOVE = 2;      // 테스트: 프레임당 2픽셀 이동

Player::Player()
{
    SetPosition(100000, 100000);  // 맵 중앙에 시작
}

Player::~Player()
{
}

void Player::Update()
{
    // 화살표 키 입력 처리 - 프레임당 고정 거리 이동
    if (Input::GetKey(eKeyCode::Left))
    {
        SetPosition(GetX() - FRAME_MOVE, GetY());
    }
    if (Input::GetKey(eKeyCode::Right))
    {
        SetPosition(GetX() + FRAME_MOVE, GetY());
    }
    if (Input::GetKey(eKeyCode::Up))
    {
        SetPosition(GetX(), GetY() - FRAME_MOVE);
    }
    if (Input::GetKey(eKeyCode::Down))
    {
        SetPosition(GetX(), GetY() + FRAME_MOVE);
    }

    // 맵 범위 제한 (2000x2000 타일 = 200,000x200,000 픽셀)
    const int MAP_MAX = 200000;
    int x = GetX();
    int y = GetY();

    if (x < 0) SetPosition(0, y);
    else if (x > MAP_MAX) SetPosition(MAP_MAX, y);

    if (y < 0) SetPosition(x, 0);
    else if (y > MAP_MAX) SetPosition(x, MAP_MAX);
}

void Player::LateUpdate()
{
    // 카메라를 플레이어 위치로 업데이트
    GAME.GetCamera()->SetTarget(GetX(), GetY());
}

void Player::Render(HDC hdc)
{
    Camera* camera = GAME.GetCamera();
    int screenX, screenY;
    camera->WorldToScreen(GetX(), GetY(), screenX, screenY);

    // 플레이어를 화면에 그리기 (사각형) - 흰색
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, whiteBrush);

    Rectangle(hdc, 
        screenX - PLAYER_SIZE / 2, 
        screenY - PLAYER_SIZE / 2,
        screenX + PLAYER_SIZE / 2, 
        screenY + PLAYER_SIZE / 2);

    SelectObject(hdc, oldBrush);
    DeleteObject(whiteBrush);
}