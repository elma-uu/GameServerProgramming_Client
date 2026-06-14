#include "Player.h"
#include "Input.h"
#include "Game.h"
#include "Camera.h"
#include "Time.h"
#include "SpriteManager.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

// Forward declaration — defined later in this file
static void DrawHpBar(HDC hdc, int cx, int top, int hp, int maxHp);

// �� �����Ӵ� �̵� �Ÿ� (�׽�Ʈ��)
// 60FPS ����: 200�ȼ�/�� �� 60fps = �� 3.33�ȼ�/������
const int PLAYER_SIZE = 50;    // player sprite size
const int TILE_SIZE = PLAYER_SIZE;  // �� ��ĭ ũ�� = �÷��̾� ũ��
const float MOVE_TIME = 0.25f; // 0.25s per tile = 4 tiles/s
const int PLAYER_SPEED = (int)(TILE_SIZE / MOVE_TIME);  // 200 px/s
const int FRAME_MOVE = 2;      // �׽�Ʈ: �����Ӵ� 2�ȼ� �̵�

Player::Player()
{
    playerID  = -1;
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
    mMoveAccum        = 0.0f;
    mSendTimer        = 0.0f;
    mAvatarPositionSet = false;
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

    // Attack: left click, 0.5s cooldown
    if (Input::GetKeyDown(eKeyCode::LButton) && mAttackCooldown <= 0.0f)
    {
        DIRECTION dir = GetMouseDirection();
        mLastDirection = dir;
        mFacingLeft    = (dir == LEFT);
        SendAttackPacket(dir);
        mAttackEffects.push_back({ GetX(), GetY(), dir, false, 0.25f, 0.25f });
        mAttackCooldown = 1.0f;
    }

    // AoE Attack: E key, 3s cooldown
    if (Input::GetKeyDown(eKeyCode::E) && mAoeCooldown <= 0.0f)
    {
        DIRECTION dir = GetMouseDirection();
        mLastDirection = dir;
        mFacingLeft    = (dir == LEFT);
        SendAoeAttackPacket(dir);
        mAttackEffects.push_back({ GetX(), GetY(), dir, true, 0.4f, 0.4f });
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

    float dt = Time::DeltaTime();

    // 4 tiles/s = 200 px/s (TILE_SIZE=50)
    // Accumulate fractional pixels so speed is frame-rate independent
    bool anyMove = Input::GetKey(eKeyCode::A) || Input::GetKey(eKeyCode::D) ||
                   Input::GetKey(eKeyCode::W) || Input::GetKey(eKeyCode::S);
    if (anyMove) {
        mMoveAccum += PLAYER_SPEED * dt;
        int px = (int)mMoveAccum;
        mMoveAccum -= (float)px;
        if (px > 0) {
            if (Input::GetKey(eKeyCode::A)) { SetPosition(GetX() - px, GetY()); mLastDirection = LEFT; }
            if (Input::GetKey(eKeyCode::D)) { SetPosition(GetX() + px, GetY()); mLastDirection = RIGHT; }
            if (Input::GetKey(eKeyCode::W)) { SetPosition(GetX(), GetY() - px); mLastDirection = UP; }
            if (Input::GetKey(eKeyCode::S)) { SetPosition(GetX(), GetY() + px); mLastDirection = DOWN; }
        }
    } else {
        mMoveAccum = 0.0f;
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
    bool movingNow = Input::GetKey(eKeyCode::A) || Input::GetKey(eKeyCode::D) ||
                     Input::GetKey(eKeyCode::W) || Input::GetKey(eKeyCode::S);
    mIsMoving = movingNow;

    if (Input::GetKey(eKeyCode::A)) mFacingLeft = true;
    if (Input::GetKey(eKeyCode::D)) mFacingLeft = false;

    int numFrames = mIsMoving ? SPRITE_RUN_FRAMES : SPRITE_IDLE_FRAMES;
    mAnimTimer += dt;
    if (mAnimTimer >= 0.1f) {
        mAnimTimer = 0.0f;
        mAnimFrame = (mAnimFrame + 1) % numFrames;
    }

    // Tick damage numbers
    for (auto& dn : mDamageNumbers) {
        dn.timeLeft -= dt;
        dn.offsetY  -= 28.0f * dt;  // drift upward
    }
    mDamageNumbers.erase(
        std::remove_if(mDamageNumbers.begin(), mDamageNumbers.end(),
            [](const DamageNumber& d) { return d.timeLeft <= 0.0f; }),
        mDamageNumbers.end());

    // Tick attack effects
    for (auto& fx : mAttackEffects)
        fx.timeLeft -= dt;
    mAttackEffects.erase(
        std::remove_if(mAttackEffects.begin(), mAttackEffects.end(),
            [](const AttackEffect& fx) { return fx.timeLeft <= 0.0f; }),
        mAttackEffects.end());

    // Smooth interpolation for render objects (monsters & other players)
    const float MOVE_SPEED = 200.0f; // pixels per second — matches player's 4 tiles/s
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
    GAME.GetCamera()->SetTarget(GetX(), GetY());

    float dt = Time::DeltaTime();
    int currentTileX = GetX() / TILE_SIZE;
    int currentTileY = GetY() / TILE_SIZE;
    bool tileChanged = (currentTileX != mLastSentX || currentTileY != mLastSentY);

    // Sync tile position 10 times/s: on tile change or every 100 ms while moving
    mSendTimer += dt;
    if (tileChanged || (mIsMoving && mSendTimer >= 0.1f))
    {
        SendMoveToServer(currentTileX, currentTileY);
        mLastSentX = currentTileX;
        mLastSentY = currentTileY;
        mSendTimer = 0.0f;
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

    // HP bar - top left corner
    {
        const int BAR_X = 10, BAR_Y = 10, BAR_W = 200, BAR_H = 16;
        int hp = GetHp(), maxHp = (mMaxHp > 0 ? mMaxHp : 1);
        int fillW = BAR_W * max(0, hp) / maxHp;

        HPEN nullPen = (HPEN)GetStockObject(NULL_PEN);
        HBRUSH barBg = CreateSolidBrush(RGB(50, 10, 10));
        HBRUSH barFg = CreateSolidBrush(RGB(210, 40, 40));
        HPEN   barPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));

        // background
        HBRUSH ob = (HBRUSH)SelectObject(hdc, barBg);
        HPEN   op = (HPEN)SelectObject(hdc, nullPen);
        Rectangle(hdc, BAR_X, BAR_Y, BAR_X + BAR_W, BAR_Y + BAR_H);

        // fill
        if (fillW > 0) {
            SelectObject(hdc, barFg);
            Rectangle(hdc, BAR_X, BAR_Y, BAR_X + fillW, BAR_Y + BAR_H);
        }

        // border
        SelectObject(hdc, barPen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, BAR_X, BAR_Y, BAR_X + BAR_W, BAR_Y + BAR_H);

        SelectObject(hdc, ob); SelectObject(hdc, op);
        DeleteObject(barBg); DeleteObject(barFg); DeleteObject(barPen);

        // HP text centered on bar
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        char hpBuf[32];
        sprintf_s(hpBuf, "HP  %d / %d", hp, maxHp);
        TextOutA(hdc, BAR_X + 4, BAR_Y + 1, hpBuf, (int)strlen(hpBuf));
    }

    // Username above head
    if (!mMyUsername.empty()) {
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutA(hdc,
            screenX - (int)(mMyUsername.size() * 4),
            screenY - PLAYER_SIZE / 2 - 15,
            mMyUsername.c_str(), (int)mMyUsername.size());
    }

    // Level text + HP bar below feet
    {
        const int footY = screenY + PLAYER_SIZE / 2 + 2;
        SetTextColor(hdc, RGB(255, 255, 80));
        char lvBuf[16];
        sprintf_s(lvBuf, "Lv.%d", (int)mLevel);
        TextOutA(hdc, screenX - (int)(strlen(lvBuf) * 4), footY,
            lvBuf, static_cast<int>(strlen(lvBuf)));
        DrawHpBar(hdc, screenX, footY + 14, GetHp(), mMaxHp > 0 ? mMaxHp : 1);
    }

    RenderAttackEffects(hdc);
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
    // Update self HP/level when server sends our own status change
    if (objectId == playerID) {
        SetHp(hp);
        mMaxHp  = max_hp;
        mExp    = exp;
        mLevel  = level;
        return;
    }

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

// Draw a small HP bar above the given screen position.
static void DrawHpBar(HDC hdc, int cx, int top, int hp, int maxHp)
{
    const int BAR_W = 46, BAR_H = 5;
    int x = cx - BAR_W / 2;
    int y = top;
    int fillW = (maxHp > 0) ? (BAR_W * max(0, hp) / maxHp) : 0;

    // background
    HPEN   nullPen = (HPEN)GetStockObject(NULL_PEN);
    HBRUSH barBg   = CreateSolidBrush(RGB(50, 10, 10));
    HBRUSH ob = (HBRUSH)SelectObject(hdc, barBg);
    HPEN   op = (HPEN)SelectObject(hdc, nullPen);
    Rectangle(hdc, x, y, x + BAR_W, y + BAR_H);

    // fill
    if (fillW > 0) {
        HBRUSH barFg = CreateSolidBrush(RGB(50, 200, 50));
        SelectObject(hdc, barFg);
        Rectangle(hdc, x, y, x + fillW, y + BAR_H);
        DeleteObject(barFg);
    }

    // border
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
    SelectObject(hdc, borderPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, y, x + BAR_W, y + BAR_H);

    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(barBg); DeleteObject(borderPen);
}

void Player::RenderObjects(HDC hdc)
{
    Camera* camera = GAME.GetCamera();
    SetBkMode(hdc, TRANSPARENT);

    for (auto& pair : mRenderList)
    {
        const RenderObject& obj = pair.second;
        int screenX, screenY;
        camera->WorldToScreen((int)obj.x, (int)obj.y, screenX, screenY);

        const int half = PLAYER_SIZE / 2;
        const int headTop  = screenY - half;  // top edge of sprite
        const int footBot  = screenY + half;  // bottom edge of sprite

        if (obj.object_id >= NPC_ID_START) {
            // --- Monster ---
            // Big_Normal renders 2 tiles tall; sprite top is TILE_SIZE above the
            // normal head, bottom aligns with the actual position tile's bottom edge.
            // All other measurements (hitbox, server position) remain at the 1-tile center.
            const bool isBig = (obj.visual_id == MON_BIG_NORMAL);
            const int  drawH  = isBig ? PLAYER_SIZE * 2 : PLAYER_SIZE;
            // Shift the draw-center up so that destY = spriteTop = screenY - drawH/2
            // places the sprite bottom at screenY + PLAYER_SIZE/2 (foot of the tile).
            // drawCenter = screenY - PLAYER_SIZE/2 + drawH/2 - drawH
            //            = screenY - PLAYER_SIZE/2 - drawH/2
            // Simplifies to: for Big: screenY - 25 - 50 = screenY - 75   (sprite top)
            //                         via DrawMonster: destY = centerY - drawH/2
            // We want destY = screenY - 75, drawH=100  => centerY = screenY - 75 + 50 = screenY - 25
            const int  monCenterY = isBig ? screenY - PLAYER_SIZE / 2 : screenY;
            const int  spriteTop  = monCenterY - drawH / 2;  // top pixel of the sprite

            int monFrame = (GetTickCount() / 150) % MON_IDLE_FRAMES;
            SpriteManager::DrawMonster(hdc, obj.visual_id, obj.isMoving, monFrame,
                screenX, monCenterY, PLAYER_SIZE, drawH, obj.facingLeft);

            // HP bar just above the sprite top
            DrawHpBar(hdc, screenX, spriteTop - 8, obj.hp, obj.max_hp);

            // Monster name above the HP bar
            SetTextColor(hdc, RGB(255, 200, 100));
            const std::string& nm = obj.obj_name;
            TextOutA(hdc, screenX - (int)(nm.size() * 4),
                spriteTop - 20, nm.c_str(), (int)nm.size());

        } else {
            // --- Other Player ---
            int runFrames = obj.isMoving ? SPRITE_RUN_FRAMES : SPRITE_IDLE_FRAMES;
            int objFrame  = (GetTickCount() / 150) % runFrames;
            SpriteManager::DrawSprite(hdc, obj.visual_id, obj.isMoving, objFrame,
                screenX, screenY, PLAYER_SIZE, PLAYER_SIZE, obj.facingLeft);

            // Name above head
            SetTextColor(hdc, RGB(180, 220, 255));
            const std::string& nm = obj.obj_name;
            TextOutA(hdc, screenX - (int)(nm.size() * 4),
                headTop - 15, nm.c_str(), (int)nm.size());

            // Level text + HP bar below feet
            SetTextColor(hdc, RGB(255, 255, 80));
            char lvBuf[16];
            sprintf_s(lvBuf, "Lv.%d", (int)obj.level);
            TextOutA(hdc, screenX - (int)(strlen(lvBuf) * 4),
                footBot + 2, lvBuf, (int)strlen(lvBuf));
            DrawHpBar(hdc, screenX, footBot + 16, obj.hp, obj.max_hp > 0 ? obj.max_hp : 1);
        }
    }

    // Floating damage numbers
    for (const auto& dn : mDamageNumbers) {
        auto it = mRenderList.find(dn.objectId);
        if (it == mRenderList.end()) continue;
        const RenderObject& obj = it->second;
        int sx, sy;
        camera->WorldToScreen((int)obj.x, (int)obj.y, sx, sy);

        char numBuf[16];
        sprintf_s(numBuf, "%d", dn.amount);
        int tx = sx - (int)(strlen(numBuf) * 4);
        // Big_Normal sprites are 2 tiles tall — float damage above the full sprite
        bool isBigMon = (it->second.visual_id == MON_BIG_NORMAL);
        int  dmgBaseY = isBigMon ? sy - PLAYER_SIZE * 3 / 2 : sy - PLAYER_SIZE / 2;
        int ty = dmgBaseY - 30 + (int)dn.offsetY;

        SetTextColor(hdc, dn.isCrit ? RGB(255, 230, 0) : RGB(255, 255, 255));
        TextOutA(hdc, tx, ty, numBuf, (int)strlen(numBuf));
    }
}

void Player::AddDamageNumber(int attackerId, int objectId, int damage, bool isCrit)
{
    DamageNumber dn;
    dn.objectId = objectId;
    dn.amount   = damage;
    dn.isCrit   = isCrit;
    dn.timeLeft = 0.5f;
    dn.offsetY  = 0.0f;
    mDamageNumbers.push_back(dn);

    // System message only for the local player's hits
    if (attackerId == playerID)
    {
        std::string monName = "Monster";
        auto it = mRenderList.find(objectId);
        if (it != mRenderList.end())
            monName = it->second.obj_name;

        // \uXXXX escapes are encoding-independent — always compile to correct UTF-16
        // "[system] 플레이어 {id}가 {monster}을(를) 공격해서 {damage} 데미지를 입혔습니다."
        wchar_t wbuf[256];
        if (isCrit)
            swprintf_s(wbuf, 256,
                L"[system] 플레이어 %d가 %S"
                L"을(를) 공격해서 %d "
                L"데미지를 입혔습니다!"
                L" (크리티컬)",
                attackerId, monName.c_str(), damage);
        else
            swprintf_s(wbuf, 256,
                L"[system] 플레이어 %d가 %S"
                L"을(를) 공격해서 %d "
                L"데미지를 입혔습니다.",
                attackerId, monName.c_str(), damage);

        // Convert wide string to UTF-8 (matches /utf-8 project encoding)
        char buf[512];
        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), nullptr, nullptr);
        GAME.GetChatSystem()->AddMessage("", std::string(buf));
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

void Player::SendAoeAttackPacket(DIRECTION dir)
{
    extern void SendAoeAttackToServer(DIRECTION dir);
    SendAoeAttackToServer(dir);
}

void Player::SendAttackPacket(DIRECTION dir)
{
    extern void SendAttackToServer(DIRECTION dir);
    SendAttackToServer(dir);
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

// ---------------------------------------------------------------------------
// Mouse direction helper
// ---------------------------------------------------------------------------

DIRECTION Player::GetMouseDirection() const
{
    Vector2 mouse = Input::GetMousePosition();
    Camera* cam = GAME.GetCamera();
    int screenX, screenY;
    cam->WorldToScreen(GetX(), GetY(), screenX, screenY);
    float dx = mouse.x - (float)screenX;
    float dy = mouse.y - (float)screenY;
    if (fabsf(dx) >= fabsf(dy))
        return dx >= 0.0f ? RIGHT : LEFT;
    else
        return dy >= 0.0f ? DOWN : UP;
}

// ---------------------------------------------------------------------------
// Attack effect rendering (client-side only, GDI+)
// ---------------------------------------------------------------------------

void Player::RenderAttackEffects(HDC hdc) const
{
    if (mAttackEffects.empty()) return;

    Camera* cam = GAME.GetCamera();
    Gdiplus::Graphics g(hdc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    for (const auto& fx : mAttackEffects)
    {
        float t = 1.0f - fx.timeLeft / fx.totalTime;  // 0=fresh, 1=expired
        int   alpha = (int)(240 * (1.0f - t));
        if (alpha <= 0) continue;

        int cx, cy;
        cam->WorldToScreen(fx.worldX, fx.worldY, cx, cy);

        if (!fx.isAoe)
        {
            // --- Single attack: three concentric slash arcs ---
            // GDI+ DrawArc: startAngle 0=right, 90=down, counter-clockwise
            // We want arc facing the attack direction, 90° sweep.
            float startAngle = 0.0f;
            switch (fx.dir) {
            case RIGHT: startAngle = -45.0f; break;
            case DOWN:  startAngle =  45.0f; break;
            case LEFT:  startAngle = 135.0f; break;
            case UP:    startAngle = 225.0f; break;
            }

            for (int ring = 0; ring < 3; ++ring)
            {
                int r = 28 + ring * 16;
                int pw = 3 - ring;
                BYTE a = (BYTE)(alpha * (3 - ring) / 3);
                Gdiplus::Pen pen(Gdiplus::Color(a, 180, 220, 255), (Gdiplus::REAL)pw);
                g.DrawArc(&pen,
                    (Gdiplus::REAL)(cx - r), (Gdiplus::REAL)(cy - r),
                    (Gdiplus::REAL)(r * 2),  (Gdiplus::REAL)(r * 2),
                    startAngle, 90.0f);
            }

            // Direction streak lines (5 lines spread ±20°)
            float dirRad = 0.0f;
            switch (fx.dir) {
            case RIGHT: dirRad =  0.0f;                 break;
            case LEFT:  dirRad =  3.14159f;             break;
            case UP:    dirRad = -3.14159f / 2.0f;      break;
            case DOWN:  dirRad =  3.14159f / 2.0f;      break;
            }
            Gdiplus::Pen streakPen(Gdiplus::Color((BYTE)alpha, 220, 240, 255), 2.0f);
            for (int s = -2; s <= 2; ++s)
            {
                float a = dirRad + s * (3.14159f / 9.0f);  // ±40° spread
                int x1 = cx + (int)(12 * cosf(a));
                int y1 = cy + (int)(12 * sinf(a));
                int x2 = cx + (int)(55 * cosf(a));
                int y2 = cy + (int)(55 * sinf(a));
                g.DrawLine(&streakPen, x1, y1, x2, y2);
            }
        }
        else
        {
            // --- AOE: expanding ring + 8 radial slashes ---
            float progress = t;  // 0→1 as effect plays
            int   radius   = (int)(80 * progress);

            // Expanding ring
            Gdiplus::Pen ringPen(Gdiplus::Color((BYTE)alpha, 255, 180, 40), 2.5f);
            if (radius > 1)
                g.DrawEllipse(&ringPen, cx - radius, cy - radius, radius * 2, radius * 2);

            // 8 radial slash lines growing outward
            for (int i = 0; i < 8; ++i)
            {
                float angle  = i * (3.14159f / 4.0f);
                int   minLen = 10 + (int)(20 * progress);
                int   maxLen = 40 + (int)(45 * progress);
                int   x1 = cx + (int)(minLen * cosf(angle));
                int   y1 = cy + (int)(minLen * sinf(angle));
                int   x2 = cx + (int)(maxLen * cosf(angle));
                int   y2 = cy + (int)(maxLen * sinf(angle));
                Gdiplus::Pen slashPen(Gdiplus::Color((BYTE)alpha, 255, 210, 80), 2.0f);
                g.DrawLine(&slashPen, x1, y1, x2, y2);
            }

            // Inner burst star (fresh part only)
            if (t < 0.3f)
            {
                int br = (int)(35 * (1.0f - t / 0.3f));
                Gdiplus::Pen burstPen(Gdiplus::Color((BYTE)(alpha), 255, 255, 160), 3.0f);
                g.DrawEllipse(&burstPen, cx - br, cy - br, br * 2, br * 2);
            }
        }
    }
}