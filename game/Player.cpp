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
    SetPosition(50000, 50000);  // 맵 중앙에 시작 (타일: 1000, 1000)
    mLastSentX = 50000;
    mLastSentY = 50000;
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

    // 맵 범위 제한 (2000x2000 타일 = 100,000x100,000 픽셀)
    const int MAP_MAX = 100000;
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

    // 위치 변경 감지: 픽셀 좌표를 타일 좌표로 변환하여 서버에 전송
    int currentTileX = GetX() / TILE_SIZE;
    int currentTileY = GetY() / TILE_SIZE;

    if (mLastSentX != currentTileX || mLastSentY != currentTileY)
    {
        // 위치가 변경되었으므로 서버에 이동 패킷 전송
        // SendMoveToServer 메서드를 통해 전송 (Game.cpp에서 실제 전송)
        SendMoveToServer(currentTileX, currentTileY);

        mLastSentX = currentTileX;
        mLastSentY = currentTileY;
    }
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

    // 렌더 리스트의 모든 객체 렌더링
    RenderObjects(hdc);
}

void Player::AddObject(int objectId, const std::string& objName, int visualId,
    int x, int y, int hp, int max_hp, unsigned long long exp, unsigned char level)
{
    RenderObject obj;
    obj.object_id = objectId;
    obj.obj_name = objName;
    obj.visual_id = visualId;
    // 서버에서 보내는 타일 좌표를 픽셀 좌표로 변환 (TILE_SIZE = 50)
    obj.x = x * TILE_SIZE;
    obj.y = y * TILE_SIZE;
    obj.hp = hp;
    obj.max_hp = max_hp;
    obj.exp = exp;
    obj.level = level;

    mRenderList[objectId] = obj;
}

void Player::RemoveObject(int objectId)
{
    auto it = mRenderList.find(objectId);
    if (it != mRenderList.end())
    {
        mRenderList.erase(it);
    }
}

void Player::UpdateObjectPosition(int objectId, int x, int y)
{
    auto it = mRenderList.find(objectId);
    if (it != mRenderList.end())
    {
        // 서버에서 보내는 타일 좌표를 픽셀 좌표로 변환 (TILE_SIZE = 50)
        it->second.x = x * TILE_SIZE;
        it->second.y = y * TILE_SIZE;
    }
}

void Player::UpdateObjectStatus(int objectId, int hp, int max_hp, unsigned long long exp, unsigned char level)
{
    auto it = mRenderList.find(objectId);
    if (it != mRenderList.end())
    {
        it->second.hp = hp;
        it->second.max_hp = max_hp;
        it->second.exp = exp;
        it->second.level = level;
    }
}

void Player::RenderObjects(HDC hdc)
{
    Camera* camera = GAME.GetCamera();

    for (auto& pair : mRenderList)
    {
        const RenderObject& obj = pair.second;
        int screenX, screenY;
        camera->WorldToScreen(obj.x, obj.y, screenX, screenY);

        // 객체를 화면에 그리기 (사각형) - 파란색 (플레이어와 구분)
        HBRUSH blueBrush = CreateSolidBrush(RGB(0, 0, 255));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, blueBrush);

        Rectangle(hdc,
            screenX - PLAYER_SIZE / 2,
            screenY - PLAYER_SIZE / 2,
            screenX + PLAYER_SIZE / 2,
            screenY + PLAYER_SIZE / 2);

        SelectObject(hdc, oldBrush);
        DeleteObject(blueBrush);

        // 객체 이름 출력 (HP 정보 포함)
        char statusText[64];
        sprintf_s(statusText, sizeof(statusText), "%s(HP:%d/%d)", 
            obj.obj_name.c_str(), obj.hp, obj.max_hp);

        // 텍스트를 객체 위에 출력
        TextOutA(hdc, screenX - 20, screenY - 30, statusText, strlen(statusText));
    }
}

void Player::SendMoveToServer(int tileX, int tileY)
{
    // Game 클래스의 SendPlayerMove 메서드를 호출
    // (이를 통해 WinSock2 중복 include 문제를 피함)
    extern void SendPlayerMovePacket(int x, int y);
    SendPlayerMovePacket(tileX, tileY);
}