#include "Player.h"
#include "Player.h"
#include "Input.h"
#include "Game.h"
#include "Camera.h"
#include "Time.h"
#include "SpriteManager.h"
#include <cmath>

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
    mAoeCooldown = 0.0f;
    mPartyId = -1;
    mShowPartyUI = false;
    mPartyUISelection = -1;
    mVisualId   = 0;
    mAnimFrame  = 0;
    mAnimTimer  = 0.0f;
    mFacingLeft = false;
    mIsMoving   = false;
}

Player::~Player()
{
}

void Player::Update()
{
    // block movement and stat keys while in chat mode
    if (Input::IsChatMode()) return;

    // P key: toggle party interface
    if (Input::GetKeyDown(eKeyCode::P)) {
        mShowPartyUI = !mShowPartyUI;
        if (mShowPartyUI) {
            mPartyUISelection = (mPartyId >= 0) ? -2 : -1;
            SendPartyListReq();
        }
    }

    // Party UI modal: absorbs all other input
    if (mShowPartyUI) {
        if (Input::GetKeyDown(eKeyCode::ESC)) {
            mShowPartyUI = false;
            return;
        }
        int minSel = (mPartyId >= 0) ? -2 : -1;
        int maxSel = (int)mPartyList.size() - 1;
        if (Input::GetKeyDown(eKeyCode::Up) && mPartyUISelection > minSel)
            mPartyUISelection--;
        if (Input::GetKeyDown(eKeyCode::Down) && mPartyUISelection < maxSel)
            mPartyUISelection++;
        if (Input::GetKeyDown(eKeyCode::SPACE)) {
            if (mPartyUISelection == -1 && mPartyId < 0) {
                SendPartyCreate();
                mShowPartyUI = false;
            } else if (mPartyUISelection == -2 && mPartyId >= 0) {
                SendPartyLeave();
                mShowPartyUI = false;
            } else if (mPartyUISelection >= 0 &&
                       mPartyUISelection < (int)mPartyList.size() &&
                       mPartyId < 0) {
                SendPartyJoin(mPartyList[mPartyUISelection].party_id);
                mShowPartyUI = false;
            }
        }
        return;
    }

    // Attack cooldown countdown
    if (mAttackCooldown > 0.0f)
        mAttackCooldown -= Time::DeltaTime();
    if (mAoeCooldown > 0.0f)
        mAoeCooldown -= Time::DeltaTime();

    // Attack: press S, 0.5s cooldown
    if (Input::GetKeyDown(eKeyCode::S) && mAttackCooldown <= 0.0f)
    {
        SendAttackPacket();
        mAttackCooldown = 0.5f;
    }

    // AoE Attack: press A, 3s cooldown
    if (Input::GetKeyDown(eKeyCode::A) && mAoeCooldown <= 0.0f)
    {
        SendAoeAttackPacket();
        mAoeCooldown = 3.0f;
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

    // Map boundary clamp (2000x2000 tiles = 100,000x100,000 pixels)
    const int MAP_MAX = 100000;
    int x = GetX();
    int y = GetY();

    if (x < 0) SetPosition(0, y);
    else if (x > MAP_MAX) SetPosition(MAP_MAX, y);

    if (y < 0) SetPosition(x, 0);
    else if (y > MAP_MAX) SetPosition(x, MAP_MAX);

    // Sprite animation — track movement & facing direction
    bool movingNow = Input::GetKey(eKeyCode::Left)  || Input::GetKey(eKeyCode::Right) ||
                     Input::GetKey(eKeyCode::Up)     || Input::GetKey(eKeyCode::Down);
    mIsMoving = movingNow;

    if (Input::GetKey(eKeyCode::Left))  mFacingLeft = true;
    if (Input::GetKey(eKeyCode::Right)) mFacingLeft = false;

    int numFrames = mIsMoving ? SPRITE_RUN_FRAMES : SPRITE_IDLE_FRAMES;
    mAnimTimer += Time::DeltaTime();
    if (mAnimTimer >= 0.1f) {
        mAnimTimer = 0.0f;
        mAnimFrame = (mAnimFrame + 1) % numFrames;
    }

    // Smooth interpolation for render objects (monsters & other players)
    const float MOVE_SPEED = 100.0f; // pixels per second (one tile = 50px, arrives in 0.5s)
    float dt = Time::DeltaTime();
    for (auto& [id, obj] : mRenderList) {
        if (!obj.isMoving) continue;
        float dx   = obj.targetX - obj.x;
        float dy   = obj.targetY - obj.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 1.0f) {
            obj.x = obj.targetX;
            obj.y = obj.targetY;
            obj.isMoving = false;
        } else {
            float step = MOVE_SPEED * dt;
            if (step > dist) step = dist;
            obj.x += dx / dist * step;
            obj.y += dy / dist * step;
        }
    }
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
    // Draw player sprite (fallback to coloured rect if sprites not loaded)
    SpriteManager::DrawSprite(hdc, mVisualId, mIsMoving, mAnimFrame,
        screenX, screenY, PLAYER_SIZE, PLAYER_SIZE, mFacingLeft);

    SetBkMode(hdc, TRANSPARENT);

    // Username above the player rect
    if (!mMyUsername.empty()) {
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutA(hdc,
            screenX - (int)(mMyUsername.size() * 4),
            screenY - PLAYER_SIZE / 2 - 16,
            mMyUsername.c_str(), (int)mMyUsername.size());
    }

    // Level at top-left corner of player rect
    SetTextColor(hdc, RGB(255, 255, 80));
    char lvBuf[16];
    sprintf_s(lvBuf, "Lv.%d", (int)mLevel);
    TextOutA(hdc, screenX - PLAYER_SIZE / 2, screenY - PLAYER_SIZE / 2,
        lvBuf, static_cast<int>(strlen(lvBuf)));

    RenderObjects(hdc);
    RenderStats(hdc);
    RenderPartyPanel(hdc);
    RenderPartyUI(hdc);
}

void Player::AddObject(int objectId, const std::string& objName, int visualId,
    int x, int y, int hp, int max_hp, unsigned long long exp, unsigned char level)
{
    RenderObject obj;
    obj.object_id = objectId;
    obj.obj_name  = objName;
    obj.visual_id = visualId;
    obj.x = obj.targetX = (float)x;
    obj.y = obj.targetY = (float)y;
    obj.isMoving  = false;
    obj.facingLeft = false;
    obj.hp = hp;  obj.max_hp = max_hp;
    obj.exp = exp; obj.level = level;

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
        RenderObject& obj = it->second;
        if (x != (int)obj.targetX)
            obj.facingLeft = (x < obj.x);
        obj.targetX  = (float)x;
        obj.targetY  = (float)y;
        obj.isMoving = true;
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
    // Keep party panel HP in sync
    for (auto& m : mPartyMembers) {
        if (m.player_id == objectId) {
            m.hp    = hp;
            m.max_hp = max_hp;
            m.level  = level;
            break;
        }
    }
}

void Player::RenderObjects(HDC hdc)
{
    Camera* camera = GAME.GetCamera();

    for (auto& pair : mRenderList)
    {
        const RenderObject& obj = pair.second;
        int screenX, screenY;
        camera->WorldToScreen((int)obj.x, (int)obj.y, screenX, screenY);

        if (obj.object_id >= NPC_ID_START) {
            // Monster: use move animation while sliding, idle when stopped
            int monFrame = (GetTickCount() / 150) % MON_IDLE_FRAMES;
            SpriteManager::DrawMonster(hdc, obj.visual_id, obj.isMoving, monFrame,
                screenX, screenY, PLAYER_SIZE, PLAYER_SIZE, obj.facingLeft);
        } else {
            // Player: draw their chosen character sprite
            int objFrame = (GetTickCount() / 150) % SPRITE_IDLE_FRAMES;
            SpriteManager::DrawSprite(hdc, obj.visual_id, false, objFrame,
                screenX, screenY, PLAYER_SIZE, PLAYER_SIZE, obj.facingLeft);
        }

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

void Player::SendAoeAttackPacket()
{
    extern void SendAoeAttackToServer();
    SendAoeAttackToServer();
}

void Player::SendAttackPacket()
{
    extern void SendAttackToServer();
    SendAttackToServer();
}

// ---------------------------------------------------------------------------
// Party network
// ---------------------------------------------------------------------------

void Player::SendPartyCreate()
{
    extern void SendPartyCreateToServer();
    SendPartyCreateToServer();
}

void Player::SendPartyJoin(int partyId)
{
    extern void SendPartyJoinToServer(int id);
    SendPartyJoinToServer(partyId);
}

void Player::SendPartyLeave()
{
    extern void SendPartyLeaveToServer();
    SendPartyLeaveToServer();
}

void Player::SendPartyListReq()
{
    extern void SendPartyListReqToServer();
    SendPartyListReqToServer();
}

void Player::OnPartyUpdate(int partyId, int memberCount, PartyMemberInfo* members)
{
    mPartyId = partyId;
    mPartyMembers.clear();
    if (partyId < 0) return;
    for (int i = 0; i < memberCount; ++i) {
        PartyMember m;
        m.player_id = members[i].player_id;
        strncpy_s(m.name, members[i].name, MAX_NAME_LEN - 1);
        m.name[MAX_NAME_LEN - 1] = '\0';
        m.hp     = members[i].hp;
        m.max_hp = members[i].max_hp;
        m.level  = members[i].level;
        mPartyMembers.push_back(m);
    }
}

void Player::OnPartyList(int count, PartyListEntry* entries)
{
    mPartyList.clear();
    for (int i = 0; i < count; ++i) {
        PartyUIEntry e;
        e.party_id     = entries[i].party_id;
        e.member_count = entries[i].member_count;
        strncpy_s(e.leader_name, entries[i].leader_name, MAX_NAME_LEN - 1);
        e.leader_name[MAX_NAME_LEN - 1] = '\0';
        mPartyList.push_back(e);
    }
    if (mPartyUISelection >= (int)mPartyList.size())
        mPartyUISelection = (int)mPartyList.size() - 1;
}

// ---------------------------------------------------------------------------
// Party panel (always visible when in a party, below minimap)
// ---------------------------------------------------------------------------

void Player::RenderPartyPanel(HDC hdc)
{
    const int PX = 10, PY = 170;
    const int PW = 160;

    if (mPartyId < 0) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(120, 120, 120));
        const char* hint = "[P] Party";
        TextOutA(hdc, PX, PY, hint, (int)strlen(hint));
        return;
    }

    int PH = 20 + (int)mPartyMembers.size() * 32;

    HBRUSH bgBrush  = CreateSolidBrush(RGB(15, 15, 40));
    HPEN   borPen   = CreatePen(PS_SOLID, 1, RGB(60, 80, 180));
    HBRUSH oldBr    = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN   oldPen   = (HPEN)SelectObject(hdc, borPen);
    Rectangle(hdc, PX, PY, PX + PW, PY + PH);
    SelectObject(hdc, oldBr);  SelectObject(hdc, oldPen);
    DeleteObject(bgBrush);     DeleteObject(borPen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(100, 160, 255));
    char title[32];
    sprintf_s(title, "Party #%d", mPartyId);
    TextOutA(hdc, PX + 4, PY + 2, title, (int)strlen(title));

    for (int i = 0; i < (int)mPartyMembers.size(); ++i) {
        const PartyMember& m = mPartyMembers[i];
        int rowY = PY + 20 + i * 32;

        bool isSelf = (m.player_id == playerID);
        SetTextColor(hdc, isSelf ? RGB(255, 255, 80) : RGB(200, 200, 200));
        char nameText[32];
        sprintf_s(nameText, "%s Lv%d", m.name, (int)m.level);
        TextOutA(hdc, PX + 4, rowY, nameText, (int)strlen(nameText));

        // HP bar
        if (m.max_hp > 0) {
            int barW = PW - 10;
            int hpW  = barW * (m.hp > 0 ? m.hp : 0) / m.max_hp;
            HPEN nullPen = (HPEN)GetStockObject(NULL_PEN);
            HBRUSH barBg = CreateSolidBrush(RGB(60, 15, 15));
            HBRUSH barFg = CreateSolidBrush(isSelf ? RGB(80, 200, 80) : RGB(200, 60, 60));
            HBRUSH ob = (HBRUSH)SelectObject(hdc, barBg);
            HPEN   op = (HPEN)SelectObject(hdc, nullPen);
            Rectangle(hdc, PX + 4, rowY + 14, PX + 4 + barW, rowY + 22);
            SelectObject(hdc, barFg);
            if (hpW > 0) Rectangle(hdc, PX + 4, rowY + 14, PX + 4 + hpW, rowY + 22);
            SelectObject(hdc, ob); SelectObject(hdc, op);
            DeleteObject(barBg); DeleteObject(barFg);
        }
    }
}

// ---------------------------------------------------------------------------
// Party UI modal (toggled by P key)
// ---------------------------------------------------------------------------

void Player::RenderPartyUI(HDC hdc)
{
    if (!mShowPartyUI) return;

    const int MX = 170, MY = 80, MW = 430, MH = 380;

    HBRUSH bg     = CreateSolidBrush(RGB(8, 8, 28));
    HPEN   border = CreatePen(PS_SOLID, 2, RGB(80, 120, 220));
    HBRUSH ob     = (HBRUSH)SelectObject(hdc, bg);
    HPEN   op     = (HPEN)SelectObject(hdc, border);
    Rectangle(hdc, MX, MY, MX + MW, MY + MH);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(bg); DeleteObject(border);

    SetBkMode(hdc, TRANSPARENT);

    // Title
    SetTextColor(hdc, RGB(120, 180, 255));
    const char* title = "=== Party System ===   [P/ESC: close]";
    TextOutA(hdc, MX + 10, MY + 10, title, (int)strlen(title));

    int y = MY + 38;

    // Create / Leave button
    if (mPartyId < 0) {
        bool sel = (mPartyUISelection == -1);
        SetTextColor(hdc, sel ? RGB(80, 255, 80) : RGB(60, 180, 60));
        const char* btn = sel ? "> [Create Party]" : "  [Create Party]";
        TextOutA(hdc, MX + 10, y, btn, (int)strlen(btn));
    } else {
        char curInfo[48];
        sprintf_s(curInfo, "  In Party #%d", mPartyId);
        SetTextColor(hdc, RGB(100, 200, 100));
        TextOutA(hdc, MX + 10, y, curInfo, (int)strlen(curInfo));
        y += 18;
        bool sel = (mPartyUISelection == -2);
        SetTextColor(hdc, sel ? RGB(255, 80, 80) : RGB(180, 60, 60));
        const char* btn = sel ? "> [Leave Party]" : "  [Leave Party]";
        TextOutA(hdc, MX + 10, y, btn, (int)strlen(btn));
    }
    y += 28;

    // Separator
    HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(50, 50, 100));
    HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
    MoveToEx(hdc, MX + 8, y, nullptr);
    LineTo(hdc, MX + MW - 8, y);
    SelectObject(hdc, oldSep);
    DeleteObject(sepPen);
    y += 8;

    SetTextColor(hdc, RGB(160, 160, 200));
    TextOutA(hdc, MX + 10, y, "Available Parties:", 18);
    y += 18;

    if (mPartyList.empty()) {
        SetTextColor(hdc, RGB(100, 100, 100));
        TextOutA(hdc, MX + 20, y, "(none)  Create one!", 19);
    } else {
        for (int i = 0; i < (int)mPartyList.size(); ++i) {
            const PartyUIEntry& e = mPartyList[i];
            bool sel = (mPartyUISelection == i);
            SetTextColor(hdc, sel ? RGB(255, 255, 80) : RGB(180, 180, 180));
            char row[80];
            sprintf_s(row, "%s Party #%-3d  Leader: %-12s  [%d/4]",
                sel ? ">" : " ",
                e.party_id, e.leader_name, (int)e.member_count);
            TextOutA(hdc, MX + 10, y, row, (int)strlen(row));
            y += 20;
        }
    }

    // Controls hint at bottom
    SetTextColor(hdc, RGB(90, 90, 90));
    const char* hint = "Up/Down: navigate    Space: confirm";
    TextOutA(hdc, MX + 10, MY + MH - 22, hint, (int)strlen(hint));
}