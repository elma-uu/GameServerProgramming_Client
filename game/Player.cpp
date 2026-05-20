#include "Player.h"
#include "Player.h"
#include "Input.h"
#include "Game.h"
#include "Camera.h"

const int PLAYER_SPEED = 100; // pixels per second
const int PLAYER_SIZE = 50;   // player sprite size

Player::Player()
{
    SetPosition(1000, 1000); // 맵 중앙 근처에 시작
}

Player::~Player()
{
}

void Player::Update()
{
    // 화살표 키 입력 처리
    if (Input::GetKey(eKeyCode::Left))
    {
        SetPosition(GetX() - PLAYER_SPEED, GetY());
    }
    if (Input::GetKey(eKeyCode::Right))
    {
        SetPosition(GetX() + PLAYER_SPEED, GetY());
    }
    if (Input::GetKey(eKeyCode::Up))
    {
        SetPosition(GetX(), GetY() - PLAYER_SPEED);
    }
    if (Input::GetKey(eKeyCode::Down))
    {
        SetPosition(GetX(), GetY() + PLAYER_SPEED);
    }

    // 맵 범위 제한 (2000x2000)
    if (GetX() < 0) SetPosition(0, GetY());
    if (GetX() > 2000) SetPosition(2000, GetY());
    if (GetY() < 0) SetPosition(GetX(), 0);
    if (GetY() > 2000) SetPosition(GetX(), 2000);
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