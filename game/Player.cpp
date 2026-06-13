#include "Player.h"
#include "Player.h"
#include "Input.h"
#include "Game.h"
#include "Camera.h"
#include "Time.h"

// �� �����Ӵ� �̵� �Ÿ� (�׽�Ʈ��)
// 60FPS ����: 200�ȼ�/�� �� 60fps = �� 3.33�ȼ�/������
const int PLAYER_SIZE = 50;    // player sprite size
const int TILE_SIZE = PLAYER_SIZE;  // �� ��ĭ ũ�� = �÷��̾� ũ��
const float MOVE_TIME = 0.5f;  // 0.5�ʿ� 1ĭ
const int PLAYER_SPEED = (int)(TILE_SIZE / MOVE_TIME);  // 100 �ȼ�/��
const int FRAME_MOVE = 2;      // �׽�Ʈ: �����Ӵ� 2�ȼ� �̵�

Player::Player()
{
    // �� �߾ӿ� ���� (Ÿ��: 1000, 1000 �߽� = �ȼ� 50025, 50025)
    SetPosition(50025, 50025);
    // mLastSentX/Y�� Ÿ�� ��ǥ�� ����
    mLastSentX = 1000;
    mLastSentY = 1000;
    mLastDirection = UP;  // �⺻ ����
}

Player::~Player()
{
}

void Player::Update()
{
    // block movement while in chat mode
    if (Input::IsChatMode()) return;

    if (Input::GetKey(eKeyCode::Left))
    {
        SetPosition(GetX() - FRAME_MOVE, GetY());
        mLastDirection = LEFT;
    }
    if (Input::GetKey(eKeyCode::Right))
    {
        SetPosition(GetX() + FRAME_MOVE, GetY());
        mLastDirection = RIGHT;
    }
    if (Input::GetKey(eKeyCode::Up))
    {
        SetPosition(GetX(), GetY() - FRAME_MOVE);
        mLastDirection = UP;
    }
    if (Input::GetKey(eKeyCode::Down))
    {
        SetPosition(GetX(), GetY() + FRAME_MOVE);
        mLastDirection = DOWN;
    }

    // �� ���� ���� (2000x2000 Ÿ�� = 100,000x100,000 �ȼ�)
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
    // ī�޶� �÷��̾� ��ġ�� ������Ʈ
    GAME.GetCamera()->SetTarget(GetX(), GetY());

    // ��ġ ���� ����: �ȼ� ��ǥ�� Ÿ�� ��ǥ�� ��ȯ�Ͽ� ������ ����
    int currentTileX = GetX() / TILE_SIZE;
    int currentTileY = GetY() / TILE_SIZE;

    if (mLastSentX != currentTileX || mLastSentY != currentTileY)
    {
        // ��ġ�� ����Ǿ����Ƿ� ������ �̵� ��Ŷ ����
        printf("Tile changed - From: (%d, %d) -> To: (%d, %d), Pixel: (%d, %d)\n", 
            mLastSentX, mLastSentY, currentTileX, currentTileY, GetX(), GetY());
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

    // �÷��̾ ȭ�鿡 �׸��� (�簢��) - ���
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, whiteBrush);

    Rectangle(hdc, 
        screenX - PLAYER_SIZE / 2, 
        screenY - PLAYER_SIZE / 2,
        screenX + PLAYER_SIZE / 2, 
        screenY + PLAYER_SIZE / 2);

    SelectObject(hdc, oldBrush);
    DeleteObject(whiteBrush);

    // ���� ����Ʈ�� ��� ��ü ������
    RenderObjects(hdc);
}

void Player::AddObject(int objectId, const std::string& objName, int visualId,
    int x, int y, int hp, int max_hp, unsigned long long exp, unsigned char level)
{
    RenderObject obj;
    obj.object_id = objectId;
    obj.obj_name = objName;
    obj.visual_id = visualId;
    // Network.cpp���� �̹� �ȼ� ��ǥ�� ��ȯ�Ǿ� ���޵�
    obj.x = x;
    obj.y = y;
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
        // Network.cpp���� �̹� �ȼ� ��ǥ�� ��ȯ�Ǿ� ���޵�
        it->second.x = x;
        it->second.y = y;
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

        // ��ü�� ȭ�鿡 �׸��� (�簢��) - �Ķ��� (�÷��̾�� ����)
        HBRUSH blueBrush = CreateSolidBrush(RGB(0, 0, 255));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, blueBrush);

        Rectangle(hdc,
            screenX - PLAYER_SIZE / 2,
            screenY - PLAYER_SIZE / 2,
            screenX + PLAYER_SIZE / 2,
            screenY + PLAYER_SIZE / 2);

        SelectObject(hdc, oldBrush);
        DeleteObject(blueBrush);

        // ��ü �̸� ��� (HP ���� ����)
        char statusText[64];
        sprintf_s(statusText, sizeof(statusText), "%s(HP:%d/%d)", 
            obj.obj_name.c_str(), obj.hp, obj.max_hp);

        // �ؽ�Ʈ�� ��ü ���� ���
        TextOutA(hdc, screenX - 20, screenY - 30, statusText, strlen(statusText));
    }
}

std::string Player::GetObjectName(int objectId) const
{
    auto it = mRenderList.find(objectId);
    if (it != mRenderList.end())
        return it->second.obj_name;
    return "";
}

void Player::SendMoveToServer(int tileX, int tileY)
{
    // Game Ŭ������ SendPlayerMove �޼��带 ȣ��
    // (�̸� ���� WinSock2 �ߺ� include ������ ����)
    extern void SendPlayerMovePacket(int x, int y);
    SendPlayerMovePacket(tileX, tileY);
}