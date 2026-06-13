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
    SetPosition(50025, 50025);
    mLastSentX = 1000;
    mLastSentY = 1000;
    mLastDirection = UP;
    mStr = 5; mIntl = 5; mDex = 5; mLuk = 5;
    mStatPoints = 0;
    mAttackCooldown = 0.0f;
}

Player::~Player()
{
}

void Player::Update()
{
    // block movement and stat keys while in chat mode
    if (Input::IsChatMode()) return;

    // Attack cooldown countdown
    if (mAttackCooldown > 0.0f)
        mAttackCooldown -= Time::DeltaTime();

    // Attack: press S, 0.5s cooldown
    if (Input::GetKeyDown(eKeyCode::S) && mAttackCooldown <= 0.0f)
    {
        SendAttackPacket();
        mAttackCooldown = 0.5f;
    }

    // stat investment: press 1-4 when stat points are available
    if (mStatPoints > 0)
    {
        if (Input::GetKeyDown(eKeyCode::Key1)) SendStatInvestPacket(STAT_STR);
        if (Input::GetKeyDown(eKeyCode::Key2)) SendStatInvestPacket(STAT_INT);
        if (Input::GetKeyDown(eKeyCode::Key3)) SendStatInvestPacket(STAT_DEX);
        if (Input::GetKeyDown(eKeyCode::Key4)) SendStatInvestPacket(STAT_LUK);
    }

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

    // Level at top-left corner of player rect
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 80));
    char lvBuf[16];
    sprintf_s(lvBuf, "Lv.%d", (int)mLevel);
    TextOutA(hdc, screenX - PLAYER_SIZE / 2, screenY - PLAYER_SIZE / 2,
        lvBuf, static_cast<int>(strlen(lvBuf)));

    RenderObjects(hdc);
    RenderStats(hdc);
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

        SetBkMode(hdc, TRANSPARENT);

        // Level at top-left corner of object rect
        SetTextColor(hdc, RGB(255, 255, 80));
        char lvText[16];
        sprintf_s(lvText, sizeof(lvText), "Lv.%d", (int)obj.level);
        TextOutA(hdc, screenX - PLAYER_SIZE / 2, screenY - PLAYER_SIZE / 2,
            lvText, static_cast<int>(strlen(lvText)));

        // Name and HP above the object
        SetTextColor(hdc, RGB(255, 255, 255));
        char statusText[64];
        sprintf_s(statusText, sizeof(statusText), "%s(HP:%d/%d)",
            obj.obj_name.c_str(), obj.hp, obj.max_hp);
        TextOutA(hdc, screenX - 20, screenY - 30, statusText, static_cast<int>(strlen(statusText)));
    }
}

std::string Player::GetObjectName(int objectId) const
{
    auto it = mRenderList.find(objectId);
    if (it != mRenderList.end())
        return it->second.obj_name;
    return "";
}

void Player::RenderStats(HDC hdc)
{
    const int PX = 600;
    const int PY = 10;
    const int PW = 195;
    const int LH = 18;

    int rows = mStatPoints > 0 ? 5 : 3;

    HBRUSH bgBrush = CreateSolidBrush(RGB(20, 20, 20));
    HPEN nullPen   = (HPEN)GetStockObject(NULL_PEN);
    HBRUSH oldBr   = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN oldPen    = (HPEN)SelectObject(hdc, nullPen);
    Rectangle(hdc, PX - 2, PY - 2, PX + PW + 2, PY + rows * LH + 4);
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPen);
    DeleteObject(bgBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(200, 220, 255));

    char buf[64];
    sprintf_s(buf, "STR:%3d   INT:%3d", (int)mStr, (int)mIntl);
    TextOutA(hdc, PX + 4, PY, buf, static_cast<int>(strlen(buf)));

    sprintf_s(buf, "DEX:%3d   LUK:%3d", (int)mDex, (int)mLuk);
    TextOutA(hdc, PX + 4, PY + LH, buf, static_cast<int>(strlen(buf)));

    sprintf_s(buf, "Points: %d", (int)mStatPoints);
    SetTextColor(hdc, mStatPoints > 0 ? RGB(255, 255, 80) : RGB(150, 150, 150));
    TextOutA(hdc, PX + 4, PY + LH * 2, buf, static_cast<int>(strlen(buf)));

    if (mStatPoints > 0)
    {
        SetTextColor(hdc, RGB(180, 255, 180));
        const char* hint1 = "1:STR  2:INT";
        const char* hint2 = "3:DEX  4:LUK";
        TextOutA(hdc, PX + 4, PY + LH * 3, hint1, static_cast<int>(strlen(hint1)));
        TextOutA(hdc, PX + 4, PY + LH * 4, hint2, static_cast<int>(strlen(hint2)));
    }
}

void Player::SendStatInvestPacket(STAT_TYPE statType)
{
    extern void SendStatInvest(STAT_TYPE st);
    SendStatInvest(statType);
}

void Player::SendMoveToServer(int tileX, int tileY)
{
    extern void SendPlayerMovePacket(int x, int y);
    SendPlayerMovePacket(tileX, tileY);
}

void Player::SendAttackPacket()
{
    extern void SendAttackToServer();
    SendAttackToServer();
}