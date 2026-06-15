#include "Player.h"
#include "Input.h"
#include "Game.h"
#include "Camera.h"
#include "Time.h"
#include "SpriteManager.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

// Forward declaration ??defined later in this file
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

    // Block all input while dead; advance die animation
    if (mIsDying) {
        constexpr float DIE_FRAME_DUR = 0.2f;
        mDieTimer += Time::DeltaTime();
        if (mDieTimer >= DIE_FRAME_DUR) {
            mDieTimer -= DIE_FRAME_DUR;
            if (mDieFrame < SPRITE_DIE_FRAMES - 1) mDieFrame++;
        }
        return;
    }

    // P key: toggle party interface
    if (Input::GetKeyDown(eKeyCode::P)) {
        mShowPartyUI = !mShowPartyUI;
        if (mShowPartyUI) {
            mPartyUISelection = (mPartyId >= 0) ? -2 : -1;
            SendPartyListReq();
        }
    }

    // F key: open/close NPC UI (Ability, Shop, or Quest)
    if (Input::GetKeyDown(eKeyCode::F)) {
        if (mShowAbilityUI) {
            mShowAbilityUI = false;
        } else if (mShowShopUI) {
            mShowShopUI = false;
        } else if (IsNearAbilityNpc()) {
            mShowAbilityUI = true;
        } else if (IsNearShopNpc()) {
            mShowShopUI = true;
        } else if (IsNearQuestNpc()) {
            SendQuestInteractPacket();
        }
    }

    // Shop UI modal: ESC closes, LButton buys item or enhances weapon
    if (mShowShopUI) {
        if (Input::GetKeyDown(eKeyCode::ESC)) {
            mShowShopUI = false;
            return;
        }
        if (Input::GetKeyDown(eKeyCode::LButton)) {
            Vector2 mp = Input::GetMousePosition();

            // Item buy buttons: centered at colCX, y=300
            const int colCX[2]  = { 250, 550 };
            const int btnY = 300, btnW = 120, btnH = 36;
            for (int i = 0; i < 2; ++i) {
                int bx = colCX[i] - btnW / 2;
                if (mp.x >= bx && mp.x <= bx + btnW &&
                    mp.y >= btnY && mp.y <= btnY + btnH) {
                    if (i == ITEM_HP_POTION && mPotionCooldown > 0.0f) break;
                    SendBuyItemPacket((ITEM_TYPE)i);
                    break;
                }
            }

            // Enhance button: centered at x=400, y=500, w=150, h=36
            {
                const int enhBtnX = 400 - 75, enhBtnY = 500;
                const int enhBtnW = 150,       enhBtnH = 36;
                if (mWeaponEnhance < 25 &&
                    mp.x >= enhBtnX && mp.x <= enhBtnX + enhBtnW &&
                    mp.y >= enhBtnY && mp.y <= enhBtnY + enhBtnH) {
                    SendEnhancePacket();
                }
            }
        }

        // Tick enhance message timer
        if (mEnhanceMsgTimer > 0.0f) {
            mEnhanceMsgTimer -= Time::DeltaTime();
            if (mEnhanceMsgTimer < 0.0f) mEnhanceMsgTimer = 0.0f;
        }
        return;
    }

    // Ability UI modal: ESC closes, LButton invests stat
    if (mShowAbilityUI) {
        if (Input::GetKeyDown(eKeyCode::ESC)) {
            mShowAbilityUI = false;
            return;
        }
        if (Input::GetKeyDown(eKeyCode::LButton) && mStatPoints > 0 && IsInSafeZone()) {
            // Plus button rects (x, y, w, h) ??must match RenderAbilityUI layout
            // Panel: x=100, y=90, w=600, h=400
            // 4 columns centered at x = 175, 325, 475, 625
            // Plus button top-y = 330, size 40x40
            const int colCX[4]  = { 175, 325, 475, 625 };
            const int btnY = 330, btnS = 40;
            Vector2 mp = Input::GetMousePosition();
            for (int i = 0; i < 4; ++i) {
                int bx = colCX[i] - btnS / 2;
                int by = btnY;
                if (mp.x >= bx && mp.x <= bx + btnS &&
                    mp.y >= by && mp.y <= by + btnS) {
                    SendStatInvestPacket((STAT_TYPE)i);
                    break;
                }
            }
        }
        return;
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

    // Cooldown countdown
    if (mAttackCooldown > 0.0f)  mAttackCooldown  -= Time::DeltaTime();
    if (mAoeCooldown    > 0.0f)  mAoeCooldown     -= Time::DeltaTime();
    if (mPotionCooldown > 0.0f)  mPotionCooldown  -= Time::DeltaTime();

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

    // G key: enter dungeon (party leader near Quest NPC) or exit dungeon
    if (Input::GetKeyDown(eKeyCode::G))
    {
        if (mIsInDungeon) {
            SendDungeonEnterPacket();
        } else if (IsNearQuestNpc() && mPartyId >= 0) {
            SendDungeonEnterPacket();
        }
    }

    // 1 key: use HP Potion from inventory
    if (Input::GetKeyDown(eKeyCode::Key1) && mPotionCount > 0 && mPotionCooldown <= 0.0f)
        SendUseItemPacket(ITEM_HP_POTION);

    // 2 key: use Teleport Scroll from inventory
    if (Input::GetKeyDown(eKeyCode::Key2) && mScrollCount > 0)
        SendUseItemPacket(ITEM_TELEPORT_SCROLL);

    float dt = Time::DeltaTime();

    // 4 tiles/s = 200 px/s (TILE_SIZE=50)
    // Accumulate fractional pixels so speed is frame-rate independent
    bool anyMove = Input::GetKey(eKeyCode::A) || Input::GetKey(eKeyCode::D) ||
                   Input::GetKey(eKeyCode::W) || Input::GetKey(eKeyCode::S);
    if (anyMove) {
        // DEX speed: +5 px/s per DEX above base(5), cap at 400 px/s (8 tiles/s)
        const int effSpeed = min(PLAYER_SPEED + (mDex - 5) * 5, 400);
        mMoveAccum += effSpeed * dt;
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

    // Dungeon uses local coordinate space: tiles 0..DUNGEON_SIZE-1
    if (mIsInDungeon && mDungeonInstanceId >= 0) {
        int dMinX = 0;
        int dMaxX = (DUNGEON_SIZE - 1) * TILE_SIZE + TILE_SIZE / 2;
        int dMinY = 0;
        int dMaxY = (DUNGEON_SIZE - 1) * TILE_SIZE + TILE_SIZE / 2;
        int cx = GetX(), cy = GetY();
        if (cx < dMinX) SetPosition(dMinX, GetY());
        else if (cx > dMaxX) SetPosition(dMaxX, GetY());
        cy = GetY();
        if (cy < dMinY) SetPosition(GetX(), dMinY);
        else if (cy > dMaxY) SetPosition(GetX(), dMaxY);
    } else {
        const int MAP_MAX = 100000; // 2000 tiles * 50 px
        int x = GetX();
        int y = GetY();
        if (x < 0) SetPosition(0, y);
        else if (x > MAP_MAX) SetPosition(MAP_MAX, y);
        if (y < 0) SetPosition(x, 0);
        else if (y > MAP_MAX) SetPosition(x, MAP_MAX);
    }

    // Sprite animation ??track movement & facing direction
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

    // Tick self-damage numbers (damage taken from monsters)
    for (auto& sd : mSelfDamages) {
        sd.timeLeft -= dt;
        sd.offsetY  -= 28.0f * dt;
    }
    mSelfDamages.erase(
        std::remove_if(mSelfDamages.begin(), mSelfDamages.end(),
            [](const SelfDamage& d) { return d.timeLeft <= 0.0f; }),
        mSelfDamages.end());

    // Tick attack animation timers + die animation on render objects
    for (auto& [id, obj] : mRenderList) {
        if (obj.isAttacking) {
            obj.attackTimer -= dt;
            if (obj.attackTimer <= 0.0f) {
                obj.isAttacking = false;
                obj.attackTimer = 0.0f;
            }
        }
        if (obj.isDying) {
            constexpr float DIE_FRAME_DUR = 0.2f;
            obj.dieTimer += dt;
            if (obj.dieTimer >= DIE_FRAME_DUR) {
                obj.dieTimer -= DIE_FRAME_DUR;
                if (obj.dieFrame < SPRITE_DIE_FRAMES - 1) obj.dieFrame++;
            }
        }
    }

    // Boss hand: smooth lerp movement + attack frame advance
    DWORD nowMs = GetTickCount();
    for (auto& [id, obj] : mRenderList) {
        if (obj.visual_id != VISUAL_BOSS_BELIAL_HAND_L &&
            obj.visual_id != VISUAL_BOSS_BELIAL_HAND_R) continue;

        if (obj.handLerping) {
            float t = (float)(nowMs - obj.handLerpStart) / (float)obj.handLerpDurMs;
            if (t >= 1.0f) {
                t = 1.0f;
                obj.handLerping = false;
            }
            obj.x = obj.handLerpFromX + (obj.targetX - obj.handLerpFromX) * t;
            obj.y = obj.handLerpFromY + (obj.targetY - obj.handLerpFromY) * t;
        }

        if (obj.handAnimState == 1) {
            obj.handAttackTimer += dt;
            if (obj.handAttackTimer >= 0.083f) {  // ~12 fps for 18 frames over 1.5 s
                obj.handAttackTimer = 0.0f;
                obj.handAttackFrame = (obj.handAttackFrame + 1) % BOSS_BELIAL_HAND_ATTACK_FRAMES;
            }
        } else {
            obj.handAttackFrame = 0;
            obj.handAttackTimer = 0.0f;
        }
    }

    // Tick attack effects
    for (auto& fx : mAttackEffects)
        fx.timeLeft -= dt;
    mAttackEffects.erase(
        std::remove_if(mAttackEffects.begin(), mAttackEffects.end(),
            [](const AttackEffect& fx) { return fx.timeLeft <= 0.0f; }),
        mAttackEffects.end());

    // Smooth interpolation for render objects (monsters & other players)
    const float MOVE_SPEED = 200.0f; // pixels per second ??matches player's 4 tiles/s
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

// Layer 0 ? background: dungeon floor overlay drawn before any entity
void Player::RenderLayer0(HDC hdc)
{
    RenderDungeonOverlay(hdc);
}

// Layer 1 ? entities: player sprite, other players/monsters, effects, floating numbers
void Player::RenderLayer1(HDC hdc)
{
    Camera* camera = GAME.GetCamera();
    int screenX, screenY;
    camera->WorldToScreen(GetX(), GetY(), screenX, screenY);

    if (mIsDying) {
        SpriteManager::DrawDie(hdc, mVisualId, mDieFrame,
            screenX, screenY, PLAYER_SIZE, PLAYER_SIZE, mFacingLeft);
    } else {
        SpriteManager::DrawSprite(hdc, mVisualId, mIsMoving, mAnimFrame,
            screenX, screenY, PLAYER_SIZE, PLAYER_SIZE, mFacingLeft);
    }

    SetBkMode(hdc, TRANSPARENT);

    RenderAttackEffects(hdc);

    // Damage numbers from monster attacks (float near our avatar)
    if (!mSelfDamages.empty()) {
        SetBkMode(hdc, TRANSPARENT);
        for (const auto& sd : mSelfDamages) {
            char numBuf[16];
            sprintf_s(numBuf, "-%d", sd.amount);
            int tx = screenX - (int)(strlen(numBuf) * 4);
            int ty = screenY - PLAYER_SIZE / 2 - 30 + (int)sd.offsetY;
            SetTextColor(hdc, RGB(255, 80, 80));
            TextOutA(hdc, tx, ty, numBuf, (int)strlen(numBuf));
        }
    }

    RenderObjects(hdc);  // other players, monsters, HP bars, damage numbers

    // Boss laser — each hand renders its own beam independently.
    // Visible when handAnimState==1 && handAttackFrame >= 10 && laserCenterY is set.
    // The beam extends from the hand's screen X to the opposite wall (off-screen edge).
    // DrawLaser is called with an off-screen "other" endpoint so the body tiles the
    // full visible width while only this hand's head sprite is drawn on-screen.
    for (auto& [id, obj] : mRenderList) {
        if (obj.visual_id != VISUAL_BOSS_BELIAL_HAND_L &&
            obj.visual_id != VISUAL_BOSS_BELIAL_HAND_R) continue;
        if (obj.handAnimState != 1 || obj.handAttackFrame < 10) continue;
        if (obj.laserCenterY < 0) continue;

        int laserFrame = min(obj.handAttackFrame - 10, 6);
        int laserSX, laserSY;
        camera->WorldToScreen(0, obj.laserCenterY * TILE_SIZE + TILE_SIZE / 2,
                              laserSX, laserSY);
        int hsx, hsy;
        camera->WorldToScreen((int)obj.x, (int)obj.y, hsx, hsy);

        if (obj.visual_id == VISUAL_BOSS_BELIAL_HAND_L) {
            // Left hand: normal head at hsx, body extends right (right "head" far off-screen)
            SpriteManager::DrawLaser(hdc, laserSY, hsx, hsx + 4000, TILE_SIZE, laserFrame);
        } else {
            // Right hand: body from far left (off-screen), flipped head at hsx
            SpriteManager::DrawLaser(hdc, laserSY, hsx - 4000, hsx, TILE_SIZE, laserFrame);
        }
    }

    // Phase 2: vertical swords falling at x = 3, 6, 9, ...
    if (mSwordFallActive) {
        DWORD elapsed = GetTickCount() - mSwordFallStartMs;
        if (elapsed >= (DWORD)mSwordFallDurationMs) {
            mSwordFallActive = false;
        } else {
            float progress = (float)elapsed / (float)mSwordFallDurationMs;
            float swordTileY = progress * (DUNGEON_SIZE - 1);
            int swordWorldCenterY = (int)(swordTileY * TILE_SIZE + TILE_SIZE / 2);

            const int SWORD_DRAW_W = TILE_SIZE;
            const int SWORD_DRAW_H = TILE_SIZE * 3;

            for (int tileX = SWORD_X_STEP; tileX < DUNGEON_SIZE; tileX += SWORD_X_STEP) {
                int swordWorldCenterX = tileX * TILE_SIZE + TILE_SIZE / 2;
                int screenX, screenY;
                camera->WorldToScreen(swordWorldCenterX, swordWorldCenterY, screenX, screenY);
                SpriteManager::DrawSword(hdc, screenX, screenY, SWORD_DRAW_W, SWORD_DRAW_H);
            }
        }
    }

    // Phase 2: horizontal swords sweeping rightward at y = 3, 6, 9, ...
    if (mSwordHFallActive) {
        DWORD elapsed = GetTickCount() - mSwordHFallStartMs;
        if (elapsed >= (DWORD)mSwordHFallDurationMs) {
            mSwordHFallActive = false;
        } else {
            float progress = (float)elapsed / (float)mSwordHFallDurationMs;
            float swordTileX = progress * (DUNGEON_SIZE - 1);
            int swordWorldCenterX = (int)(swordTileX * TILE_SIZE + TILE_SIZE / 2);

            const int SWORD_DRAW_W = TILE_SIZE * 3;
            const int SWORD_DRAW_H = TILE_SIZE;

            for (int tileY = SWORD_X_STEP; tileY < DUNGEON_SIZE; tileY += SWORD_X_STEP) {
                int swordWorldCenterY = tileY * TILE_SIZE + TILE_SIZE / 2;
                int screenX, screenY;
                camera->WorldToScreen(swordWorldCenterX, swordWorldCenterY, screenX, screenY);
                SpriteManager::DrawSwordH(hdc, screenX, screenY, SWORD_DRAW_W, SWORD_DRAW_H);
            }
        }
    }
}

// Layer 2 ? HUD/UI: fixed overlays drawn on top of everything
void Player::RenderLayer2(HDC hdc)
{
    SetBkMode(hdc, TRANSPARENT);

    // HP bar ? fixed top-left corner
    {
        const int BAR_X = 10, BAR_Y = 10, BAR_W = 200, BAR_H = 16;
        int hp = GetHp(), maxHp = (mMaxHp > 0 ? mMaxHp : 1);
        int fillW = BAR_W * max(0, hp) / maxHp;

        HPEN nullPen = (HPEN)GetStockObject(NULL_PEN);
        HBRUSH barBg = CreateSolidBrush(RGB(50, 10, 10));
        HBRUSH barFg = CreateSolidBrush(RGB(210, 40, 40));
        HPEN   barPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));

        HBRUSH ob = (HBRUSH)SelectObject(hdc, barBg);
        HPEN   op = (HPEN)SelectObject(hdc, nullPen);
        Rectangle(hdc, BAR_X, BAR_Y, BAR_X + BAR_W, BAR_Y + BAR_H);

        if (fillW > 0) {
            SelectObject(hdc, barFg);
            Rectangle(hdc, BAR_X, BAR_Y, BAR_X + fillW, BAR_Y + BAR_H);
        }

        SelectObject(hdc, barPen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, BAR_X, BAR_Y, BAR_X + BAR_W, BAR_Y + BAR_H);

        SelectObject(hdc, ob); SelectObject(hdc, op);
        DeleteObject(barBg); DeleteObject(barFg); DeleteObject(barPen);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        char hpBuf[32];
        sprintf_s(hpBuf, "HP  %d / %d", hp, maxHp);
        TextOutA(hdc, BAR_X + 4, BAR_Y + 1, hpBuf, (int)strlen(hpBuf));

        // Name + level
        const int INFO_Y = BAR_Y + BAR_H + 4;
        const int INFO_LH = 16;
        if (!mMyUsername.empty()) {
            char nameBuf[72];
            sprintf_s(nameBuf, "%s   Lv.%d", mMyUsername.c_str(), (int)mLevel);
            SetTextColor(hdc, RGB(255, 255, 200));
            TextOutA(hdc, BAR_X, INFO_Y, nameBuf, (int)strlen(nameBuf));
        }

        // EXP / next level EXP
        {
            char expBuf[64];
            if (mLevel >= 100) {
                sprintf_s(expBuf, "EXP: MAX");
            } else {
                unsigned long long nextExp =
                    (unsigned long long)mLevel * mLevel * 20ULL;
                sprintf_s(expBuf, "EXP: %llu / %llu", mExp, nextExp);
            }
            SetTextColor(hdc, RGB(120, 220, 120));
            TextOutA(hdc, BAR_X, INFO_Y + INFO_LH, expBuf, (int)strlen(expBuf));
        }
    }

    // Boss HP bar — shown at the top center while inside the dungeon
    if (mIsInDungeon) {
        // Find the boss head in the render list
        int bossHp = 0, bossMaxHp = 1;
        bool bossFound = false;
        for (auto& [id, obj] : mRenderList) {
            if (obj.visual_id == VISUAL_BOSS_BELIAL) {
                bossHp    = obj.hp;
                bossMaxHp = obj.max_hp > 0 ? obj.max_hp : 1;
                bossFound = true;
                break;
            }
        }
        if (bossFound) {
            // Use clipping rect to determine window width
            RECT rc = {};
            GetClipBox(hdc, &rc);
            int winW = rc.right - rc.left;

            const int BAR_H  = 22;
            const int BAR_W  = winW / 2;              // half the screen width
            const int BAR_X  = (winW - BAR_W) / 2;   // centered
            const int BAR_Y  = 35;  // 10px below player HP bar (player bar at y=10)

            int fillW = BAR_W * max(0, bossHp) / bossMaxHp;

            HPEN nullPen  = (HPEN)GetStockObject(NULL_PEN);
            HBRUSH bgBr   = CreateSolidBrush(RGB(30, 5, 5));
            HBRUSH fillBr = CreateSolidBrush(RGB(180, 20, 20));
            HBRUSH brdBr  = CreateSolidBrush(RGB(200, 160, 60));  // gold border

            // Border (slightly larger rect)
            HBRUSH ob = (HBRUSH)SelectObject(hdc, brdBr);
            HPEN   op = (HPEN)SelectObject(hdc, nullPen);
            Rectangle(hdc, BAR_X - 2, BAR_Y - 2, BAR_X + BAR_W + 2, BAR_Y + BAR_H + 2);

            // Background
            SelectObject(hdc, bgBr);
            Rectangle(hdc, BAR_X, BAR_Y, BAR_X + BAR_W, BAR_Y + BAR_H);

            // Fill
            if (fillW > 0) {
                SelectObject(hdc, fillBr);
                Rectangle(hdc, BAR_X, BAR_Y, BAR_X + fillW, BAR_Y + BAR_H);
            }

            SelectObject(hdc, ob); SelectObject(hdc, op);
            DeleteObject(bgBr); DeleteObject(fillBr); DeleteObject(brdBr);

            // Label: "Belial  HP / MaxHP"
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 220, 80));
            char buf[48];
            sprintf_s(buf, "Belial   %d / %d", bossHp, bossMaxHp);
            SIZE ts; GetTextExtentPoint32A(hdc, buf, (int)strlen(buf), &ts);
            TextOutA(hdc, BAR_X + (BAR_W - ts.cx) / 2, BAR_Y + (BAR_H - ts.cy) / 2,
                     buf, (int)strlen(buf));
        }
    }

    RenderQuestPanel(hdc);
    RenderStats(hdc);
    RenderPartyPanel(hdc);
    RenderPartyUI(hdc);
    RenderAbilityUI(hdc);
    RenderShopUI(hdc);
}

void Player::Render(HDC hdc)
{
    RenderLayer0(hdc);
    RenderLayer1(hdc);
    RenderLayer2(hdc);
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
            if (obj.visual_id == VISUAL_BOSS_BELIAL) {
                // --- Belial Head: 9×9 tiles (450px) ---
                const int BOSS_DRAW = TILE_SIZE * 9;
                int bossFrame = (int)(GetTickCount() / 120) % BOSS_BELIAL_IDLE_FRAMES;
                SpriteManager::DrawBoss(hdc, bossFrame, screenX, screenY, BOSS_DRAW, BOSS_DRAW);

                // Boss name and HP are shown in the dedicated boss HP bar (RenderLayer2).
                // No floating name / HP bar is drawn above the sprite.

            } else if (obj.visual_id == VISUAL_BOSS_BELIAL_HAND_L ||
                       obj.visual_id == VISUAL_BOSS_BELIAL_HAND_R) {
                // --- Belial Hands: 4×4 tiles (200px), right hand is flipped ---
                const int HAND_DRAW  = TILE_SIZE * 4;
                bool      flip       = (obj.visual_id == VISUAL_BOSS_BELIAL_HAND_R);
                bool      isAtk      = (obj.handAnimState == 1);
                int       handFrame  = isAtk
                    ? obj.handAttackFrame
                    : (int)(GetTickCount() / 120) % BOSS_BELIAL_HAND_FRAMES;
                SpriteManager::DrawHand(hdc, handFrame, screenX, screenY,
                                        HAND_DRAW, HAND_DRAW, flip, isAtk);

            } else if (obj.visual_id >= NPC_ABILITY) {
                // --- Town NPC (Ability / Restaurant / Shop) ---
                int npcFrame = (int)((GetTickCount() / 200) % 6); // max frames among all types
                SpriteManager::DrawTownNpc(hdc, obj.visual_id, npcFrame,
                    screenX, screenY, PLAYER_SIZE, PLAYER_SIZE);

                // Name above sprite in white
                SetTextColor(hdc, RGB(255, 255, 255));
                const std::string& nm = obj.obj_name;
                TextOutA(hdc, screenX - (int)(nm.size() * 4),
                    screenY - PLAYER_SIZE / 2 - 18, nm.c_str(), (int)nm.size());
            } else {
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

            int monFrame = (int)(GetTickCount() / 150);  // DrawMonster wraps via % totalFrames
            SpriteManager::DrawMonster(hdc, obj.visual_id, obj.isMoving, obj.isAttacking,
                monFrame, screenX, monCenterY, PLAYER_SIZE, drawH, obj.facingLeft);

            // HP bar just above the sprite top
            DrawHpBar(hdc, screenX, spriteTop - 8, obj.hp, obj.max_hp);

            // Monster name above the HP bar
            SetTextColor(hdc, RGB(255, 200, 100));
            const std::string& nm = obj.obj_name;
            TextOutA(hdc, screenX - (int)(nm.size() * 4),
                spriteTop - 20, nm.c_str(), (int)nm.size());
            }

        } else {
            // --- Other Player ---
            if (obj.isDying || obj.hp <= 0) {
                // Dead / dying: render die pose (last frame when animation is done)
                int dieF = obj.isDying ? obj.dieFrame : (SPRITE_DIE_FRAMES - 1);
                SpriteManager::DrawDie(hdc, obj.visual_id, dieF,
                    screenX, screenY, PLAYER_SIZE, PLAYER_SIZE, obj.facingLeft);
            } else {
                int runFrames = obj.isMoving ? SPRITE_RUN_FRAMES : SPRITE_IDLE_FRAMES;
                int objFrame  = (int)((GetTickCount() / 150) % runFrames);
                SpriteManager::DrawSprite(hdc, obj.visual_id, obj.isMoving, objFrame,
                    screenX, screenY, PLAYER_SIZE, PLAYER_SIZE, obj.facingLeft);
            }

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
        // Big_Normal sprites are 2 tiles tall ??float damage above the full sprite
        bool isBigMon = (it->second.visual_id == MON_BIG_NORMAL);
        int  dmgBaseY = isBigMon ? sy - PLAYER_SIZE * 3 / 2 : sy - PLAYER_SIZE / 2;
        int ty = dmgBaseY - 30 + (int)dn.offsetY;

        SetTextColor(hdc, dn.isCrit ? RGB(255, 230, 0) : RGB(255, 255, 255));
        TextOutA(hdc, tx, ty, numBuf, (int)strlen(numBuf));
    }
}

void Player::AddDamageNumber(int attackerId, int objectId, int damage, bool isCrit)
{
    // Monster attacked us: show damage near our avatar and trigger attack animation
    if (objectId == playerID) {
        SelfDamage sd;
        sd.amount   = damage;
        sd.timeLeft = 0.8f;
        sd.offsetY  = 0.0f;
        mSelfDamages.push_back(sd);

        // Trigger attack sprite on the monster that hit us
        auto it = mRenderList.find(attackerId);
        if (it != mRenderList.end()) {
            it->second.isAttacking = true;
            it->second.attackTimer = 0.6f;
        }
        return;
    }

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

        // \uXXXX escapes are encoding-independent ??always compile to correct UTF-16
        // "[system] ?�레?�어 {id}가 {monster}??�? 공격?�서 {damage} ?��?지�??�혔?�니??"
        wchar_t wbuf[256];
        if (isCrit)
            swprintf_s(wbuf, 256,
                L"[system] Player %d attacked %S"
                L" for %d damage (CRITICAL)",
                attackerId, monName.c_str(), damage);
        else
            swprintf_s(wbuf, 256,
                L"[system] Player %d attacked %S"
                L" for %d damage",
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
    const int PW = 195;
    const int LH = 18;
    const int TOTAL_ROWS = 9;
    const int PY = 600 - 10 - TOTAL_ROWS * LH - 6;  // bottom-right (= 422)

    HBRUSH bgBrush = CreateSolidBrush(RGB(20, 20, 20));
    HPEN nullPen   = (HPEN)GetStockObject(NULL_PEN);
    HBRUSH oldBr   = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN oldPen    = (HPEN)SelectObject(hdc, nullPen);
    Rectangle(hdc, PX - 2, PY - 2, PX + PW + 2, PY + TOTAL_ROWS * LH + 4);
    SelectObject(hdc, oldBr);
    SelectObject(hdc, oldPen);
    DeleteObject(bgBrush);

    SetBkMode(hdc, TRANSPARENT);

    // ?�?� Stats ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
    SetTextColor(hdc, RGB(200, 220, 255));
    char buf[64];
    sprintf_s(buf, "STR:%3d   INT:%3d", (int)mStr, (int)mIntl);
    TextOutA(hdc, PX + 4, PY, buf, (int)strlen(buf));

    sprintf_s(buf, "DEX:%3d   LUK:%3d", (int)mDex, (int)mLuk);
    TextOutA(hdc, PX + 4, PY + LH, buf, (int)strlen(buf));

    SetTextColor(hdc, mStatPoints > 0 ? RGB(255, 255, 80) : RGB(150, 150, 150));
    sprintf_s(buf, "Points: %d", (int)mStatPoints);
    TextOutA(hdc, PX + 4, PY + LH * 2, buf, (int)strlen(buf));

    SetTextColor(hdc, RGB(255, 210, 60));
    sprintf_s(buf, "Gold: %d", mGold);
    TextOutA(hdc, PX + 4, PY + LH * 3, buf, (int)strlen(buf));

    // ?�?� Divider ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
    HPEN divPen = CreatePen(PS_SOLID, 1, RGB(60, 60, 80));
    HPEN oldDiv = (HPEN)SelectObject(hdc, divPen);
    MoveToEx(hdc, PX + 2, PY + LH * 4 + 4, nullptr);
    LineTo(hdc,   PX + PW - 2, PY + LH * 4 + 4);
    SelectObject(hdc, oldDiv);
    DeleteObject(divPen);

    // ?�?� Inventory ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
    int invY = PY + LH * 4 + 10;

    // Potion row
    bool canUsePotion = (mPotionCount > 0 && mPotionCooldown <= 0.0f);
    SetTextColor(hdc, canUsePotion ? RGB(100, 220, 100) : RGB(120, 120, 120));
    sprintf_s(buf, "[1] HP Potion  x%d", mPotionCount);
    TextOutA(hdc, PX + 4, invY, buf, (int)strlen(buf));

    if (mPotionCooldown > 0.0f) {
        SetTextColor(hdc, RGB(200, 100, 100));
        char cdBuf[16];
        sprintf_s(cdBuf, " %.1fs", mPotionCooldown);
        TextOutA(hdc, PX + 4, invY + LH - 2, cdBuf, (int)strlen(cdBuf));
        invY += LH - 2;
    }
    invY += LH;

    // Scroll row
    bool canUseScroll = (mScrollCount > 0);
    SetTextColor(hdc, canUseScroll ? RGB(120, 180, 255) : RGB(120, 120, 120));
    sprintf_s(buf, "[2] Teleport   x%d", mScrollCount);
    TextOutA(hdc, PX + 4, invY, buf, (int)strlen(buf));
    invY += LH;

    // Hint
    SetTextColor(hdc, RGB(100, 100, 140));
    TextOutA(hdc, PX + 4, invY + 2, "Buy at Shop NPC", 15);
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
            float progress = t;  // 0?? as effect plays
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

// ---------------------------------------------------------------------------
// Shop NPC proximity check (within 3 tiles)
// ---------------------------------------------------------------------------

bool Player::IsNearShopNpc() const
{
    const int myTileX = GetX() / TILE_SIZE;
    const int myTileY = GetY() / TILE_SIZE;
    for (const auto& pair : mRenderList) {
        const RenderObject& obj = pair.second;
        if (obj.visual_id != NPC_SHOP) continue;
        int npcTileX = (int)obj.x / TILE_SIZE;
        int npcTileY = (int)obj.y / TILE_SIZE;
        int dx = myTileX - npcTileX;
        int dy = myTileY - npcTileY;
        if (dx * dx + dy * dy <= 9)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Safe zone check ??matches server TOWN_SAFE_R = 30 tiles around (1000,1000)
// ---------------------------------------------------------------------------

bool Player::IsInSafeZone() const
{
    const int myTileX = GetX() / TILE_SIZE;
    const int myTileY = GetY() / TILE_SIZE;
    const int SAFE_R = 30;
    int dx = myTileX - 1000;
    int dy = myTileY - 1000;
    return dx * dx + dy * dy <= SAFE_R * SAFE_R;
}

// ---------------------------------------------------------------------------
// Respawn (called when killed by a monster)
// ---------------------------------------------------------------------------

void Player::OnPlayerDie(int objectId)
{
    if (objectId == playerID) {
        mIsDying  = true;
        mDieFrame = 0;
        mDieTimer = 0.0f;
        mIsMoving = false;
    } else {
        auto it = mRenderList.find(objectId);
        if (it != mRenderList.end()) {
            it->second.isDying  = true;
            it->second.dieFrame = 0;
            it->second.dieTimer = 0.0f;
        }
    }
}

void Player::Respawn(int hp, int maxHp, short tileX, short tileY)
{
    // Clear death state so normal rendering and input resume
    mIsDying  = false;
    mDieFrame = 0;
    mDieTimer = 0.0f;

    SetHp(hp);
    mMaxHp = maxHp;
    int pixelX = tileX * TILE_SIZE + TILE_SIZE / 2;
    int pixelY = tileY * TILE_SIZE + TILE_SIZE / 2;
    SetPosition(pixelX, pixelY);
    mLastSentX = tileX;
    mLastSentY = tileY;
    mSelfDamages.clear();
}

// ---------------------------------------------------------------------------
// Buy item packet
// ---------------------------------------------------------------------------

void Player::SendBuyItemPacket(ITEM_TYPE itemType)
{
    extern void SendBuyItemToServer(ITEM_TYPE t);
    SendBuyItemToServer(itemType);
}

void Player::SendEnhancePacket()
{
    extern void SendEnhanceWeaponToServer();
    SendEnhanceWeaponToServer();
}

void Player::OnEnhanceResult(unsigned char result, unsigned char newLevel, int gold)
{
    mGold          = gold;
    mWeaponEnhance = (int)newLevel;

    if (result == 1) {
        wchar_t buf[32];
        swprintf_s(buf, L"Success! Weapon +%d", (int)newLevel);
        mEnhanceMsg      = buf;
        mEnhanceMsgTimer = 3.0f;
    } else if (result == 2) {
        mEnhanceMsg      = L"Failed! Enhancement RESET to +0";
        mEnhanceMsgTimer = 3.0f;
    } else {
        wchar_t buf[32];
        swprintf_s(buf, L"Failed. Weapon stays at +%d", (int)newLevel);
        mEnhanceMsg      = buf;
        mEnhanceMsgTimer = 3.0f;
    }
}

void Player::SendUseItemPacket(ITEM_TYPE itemType)
{
    extern void SendUseItemToServer(ITEM_TYPE t);
    SendUseItemToServer(itemType);
}

// ---------------------------------------------------------------------------
// Handle buy result from server
// ---------------------------------------------------------------------------

// new_hp (itemCount) carries the new inventory count after purchase.
void Player::OnBuyResult(unsigned char success, ITEM_TYPE item, int gold, int itemCount, short /*unused_x*/, short /*unused_y*/)
{
    mGold = gold;
    if (!success) return;

    if (item == ITEM_HP_POTION) {
        mPotionCount = itemCount;
    } else if (item == ITEM_TELEPORT_SCROLL) {
        mScrollCount = itemCount;
        mShowShopUI  = false;
    }
}

// Called when the server confirms a C2S_USE_ITEM result.
void Player::OnUseItemResult(unsigned char success, ITEM_TYPE item, int itemCount,
                              int newHp, short newX, short newY)
{
    if (!success) return;

    if (item == ITEM_HP_POTION) {
        mPotionCount    = itemCount;
        SetHp(newHp);
        mPotionCooldown = 7.0f;
    } else if (item == ITEM_TELEPORT_SCROLL) {
        mScrollCount = itemCount;
        // Handle dungeon exit or regular teleport
        if (mIsInDungeon) {
            mIsInDungeon       = false;
            mDungeonInstanceId = -1;
            mRenderList.clear();
        }
        SetPosition(newX * TILE_SIZE + TILE_SIZE / 2,
                    newY * TILE_SIZE + TILE_SIZE / 2);
        mLastSentX = newX;
        mLastSentY = newY;
    }
}

// ---------------------------------------------------------------------------
// Shop UI
// ---------------------------------------------------------------------------

void Player::RenderShopUI(HDC hdc)
{
    if (!mShowShopUI) return;

    const int PX = 100, PY = 90, PW = 600, PH = 510;

    HBRUSH bgBrush  = CreateSolidBrush(RGB(20, 30, 20));
    HPEN   bordPen  = CreatePen(PS_SOLID, 2, RGB(100, 180, 100));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN   oldPen   = (HPEN)SelectObject(hdc, bordPen);
    Rectangle(hdc, PX, PY, PX + PW, PY + PH);
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen);
    DeleteObject(bgBrush); DeleteObject(bordPen);

    SetBkMode(hdc, TRANSPARENT);

    // Title
    HFONT titleFont = CreateFontW(22, 0, 0, 0, FW_BOLD, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);
    SetTextColor(hdc, RGB(120, 220, 120));
        const wchar_t* title = L"[ Stat Investment ]";
    TextOutW(hdc, PX + PW / 2 - 50, PY + 12, title, (int)wcslen(title));

    // Gold & close hint
    HFONT smFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    SelectObject(hdc, smFont);
    wchar_t goldBuf[48];
    swprintf_s(goldBuf, L"Gold: %d G", mGold);
    SetTextColor(hdc, RGB(255, 210, 60));
    TextOutW(hdc, PX + 12, PY + 14, goldBuf, (int)wcslen(goldBuf));
    SetTextColor(hdc, RGB(130, 130, 130));
        const wchar_t* hint = L"F / ESC: Close";
    TextOutW(hdc, PX + PW - 110, PY + 14, hint, (int)wcslen(hint));
    DeleteObject(SelectObject(hdc, oldFont));

    // Item columns
    struct ShopItem {
        ITEM_TYPE     id;
        const wchar_t* name;
        int            price;
        const wchar_t* desc1;
        const wchar_t* desc2;
    };
    const ShopItem items[2] = {
        { ITEM_HP_POTION,       L"HP Potion",       SHOP_POTION_PRICE,
          L"Use '1' key  *  adds to inventory",  L"Restores 33pct of max HP (min 50)"  },
        { ITEM_TELEPORT_SCROLL, L"Teleport Scroll", SHOP_TELEPORT_PRICE,
          L"Use '2' key  *  adds to inventory",  L"Exit dungeon, return to town"      },
    };

    const int colCX[2] = { 250, 550 };
    const int btnY = 300, btnW = 120, btnH = 36;

    HFONT nameFont = CreateFontW(18, 0, 0, 0, FW_BOLD, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    HFONT descFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");

    for (int i = 0; i < 2; ++i) {
        int cx = colCX[i];

        // Divider
        if (i == 1) {
            HPEN div = CreatePen(PS_SOLID, 1, RGB(60, 100, 60));
            HPEN odp = (HPEN)SelectObject(hdc, div);
            MoveToEx(hdc, PX + PW / 2, PY + 50, nullptr);
            LineTo(hdc, PX + PW / 2, PY + PH - 20);
            SelectObject(hdc, odp); DeleteObject(div);
        }

        // Item icon
        int iconX = cx - 40, iconY = PY + 65;
        SpriteManager::DrawItemImage(hdc, i, iconX, iconY, 80, 80);

        // Item name
        oldFont = (HFONT)SelectObject(hdc, nameFont);
        SetTextColor(hdc, RGB(200, 230, 200));
        int nw = (int)wcslen(items[i].name) * 9;
        TextOutW(hdc, cx - nw / 2, PY + 158, items[i].name, (int)wcslen(items[i].name));
        SelectObject(hdc, oldFont);   // restore

        // Description
        oldFont = (HFONT)SelectObject(hdc, descFont);
        SetTextColor(hdc, RGB(160, 200, 160));
        int dw1 = (int)wcslen(items[i].desc1) * 7;
        TextOutW(hdc, cx - dw1 / 2, PY + 186, items[i].desc1, (int)wcslen(items[i].desc1));
        SetTextColor(hdc, RGB(120, 160, 120));
        int dw2 = (int)wcslen(items[i].desc2) * 7;
        TextOutW(hdc, cx - dw2 / 2, PY + 206, items[i].desc2, (int)wcslen(items[i].desc2));

        // Price
        wchar_t priceBuf[32];
        swprintf_s(priceBuf, L"%d G", items[i].price);
        SetTextColor(hdc, RGB(255, 210, 60));
        int pw2 = (int)wcslen(priceBuf) * 9;
        TextOutW(hdc, cx - pw2 / 2, PY + 262, priceBuf, (int)wcslen(priceBuf));
        SelectObject(hdc, oldFont);   // restore

        // Buy button
        bool canBuy  = (mGold >= items[i].price);
        HBRUSH btnBr = CreateSolidBrush(canBuy ? RGB(40, 160, 40) : RGB(80, 80, 80));
        HBRUSH oldBtn = (HBRUSH)SelectObject(hdc, btnBr);
        int bx = cx - btnW / 2;
        Rectangle(hdc, bx, btnY, bx + btnW, btnY + btnH);
        SelectObject(hdc, oldBtn); DeleteObject(btnBr);

        oldFont = (HFONT)SelectObject(hdc, smFont);
        SetTextColor(hdc, canBuy ? RGB(255, 255, 255) : RGB(140, 140, 140));
        const wchar_t* btnBuf = L"Buy";
        int blw = (int)wcslen(btnBuf) * 9;
        TextOutW(hdc, cx - blw / 2, btnY + 9, btnBuf, (int)wcslen(btnBuf));
        SelectObject(hdc, oldFont);   // restore
    }

    // ── Weapon Enhancement Section ───────────────────────────────────────────
    // Divider
    {
        HPEN div = CreatePen(PS_SOLID, 1, RGB(60, 100, 60));
        HPEN odp = (HPEN)SelectObject(hdc, div);
        MoveToEx(hdc, PX + 20, PY + 360, nullptr);
        LineTo(hdc, PX + PW - 20, PY + 360);
        SelectObject(hdc, odp); DeleteObject(div);
    }

    // Section title
    {
        HFONT secFont = CreateFontW(18, 0, 0, 0, FW_BOLD, 0, 0, 0,
            HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
        oldFont = (HFONT)SelectObject(hdc, secFont);
        SetTextColor(hdc, RGB(255, 220, 80));
        const wchar_t* sec = L"[ Weapon Enhancement ]";
        TextOutW(hdc, PX + PW / 2 - 95, PY + 372, sec, (int)wcslen(sec));
        SelectObject(hdc, oldFont); DeleteObject(secFont);
    }

    // Level design tables (client mirrors server tables — display only)
    static const int ENH_COST[25] = {
        500,   500,   500,   500,   500,
        1500,  1500,  1500,  1500,  1500,
        5000,  5000,  5000,  5000,  5000,
        15000, 15000, 15000, 15000, 15000,
        50000, 50000, 50000, 50000, 50000,
    };
    static const int ENH_SUCCESS[25] = {
        90, 85, 80, 75, 70,
        65, 60, 55, 50, 45,
        40, 35, 30, 28, 25,
        22, 20, 18, 15, 13,
        11,  9,  7,  5,  3,
    };
    static const int ENH_RESET[25] = {
         0,  0,  0,  0,  0,
         0,  0,  0,  0,  0,
         0,  0,  5,  8, 12,
        16, 20, 25, 30, 35,
        40, 50, 60, 70, 80,
    };

    HFONT infoFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    HFONT boldFont = CreateFontW(28, 0, 0, 0, FW_BOLD, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");

    // Current level badge (left)
    {
        oldFont = (HFONT)SelectObject(hdc, boldFont);
        wchar_t lvBuf[16];
        if (mWeaponEnhance == 25)
            swprintf_s(lvBuf, L"+25 MAX");
        else
            swprintf_s(lvBuf, L"+%d", mWeaponEnhance);
        COLORREF lvColor = (mWeaponEnhance == 0)   ? RGB(180, 180, 180) :
                           (mWeaponEnhance < 12)   ? RGB(100, 220, 255) :
                           (mWeaponEnhance < 20)   ? RGB(255, 160, 50)  :
                                                     RGB(255, 80,  80);
        SetTextColor(hdc, lvColor);
        TextOutW(hdc, PX + 40, PY + 405, lvBuf, (int)wcslen(lvBuf));
        SelectObject(hdc, oldFont);
    }

    // Stats block (center)
    if (mWeaponEnhance < 25)
    {
        int lv = mWeaponEnhance;
        oldFont = (HFONT)SelectObject(hdc, infoFont);

        wchar_t buf[64];
        swprintf_s(buf, L"Cost:     %d G", ENH_COST[lv]);
        SetTextColor(hdc, RGB(255, 210, 60));
        TextOutW(hdc, PX + 170, PY + 400, buf, (int)wcslen(buf));

        swprintf_s(buf, L"Success:  %d%%", ENH_SUCCESS[lv]);
        SetTextColor(hdc, RGB(100, 220, 100));
        TextOutW(hdc, PX + 170, PY + 422, buf, (int)wcslen(buf));

        if (ENH_RESET[lv] > 0) {
            swprintf_s(buf, L"Reset on fail: %d%%", ENH_RESET[lv]);
            SetTextColor(hdc, RGB(255, 90, 90));
            TextOutW(hdc, PX + 170, PY + 444, buf, (int)wcslen(buf));
        }

        swprintf_s(buf, L"Damage bonus: +%d", mWeaponEnhance * 10);
        SetTextColor(hdc, RGB(180, 180, 255));
        TextOutW(hdc, PX + 170, PY + 466, buf, (int)wcslen(buf));

        SelectObject(hdc, oldFont);
    }

    // Enhance button
    {
        const int enhBtnX = 400 - 75, enhBtnY = 500;
        const int enhBtnW = 150,       enhBtnH = 36;
        bool canEnhance = (mWeaponEnhance < 25) &&
                          (mGold >= (mWeaponEnhance < 25 ? ENH_COST[mWeaponEnhance] : 0));
        HBRUSH btnBr = CreateSolidBrush(
            (mWeaponEnhance >= 25) ? RGB(60, 60, 60) :
            canEnhance             ? RGB(160, 80, 20) :
                                     RGB(80, 80, 80));
        HBRUSH oldBtn = (HBRUSH)SelectObject(hdc, btnBr);
        Rectangle(hdc, enhBtnX, enhBtnY, enhBtnX + enhBtnW, enhBtnY + enhBtnH);
        SelectObject(hdc, oldBtn); DeleteObject(btnBr);

        oldFont = (HFONT)SelectObject(hdc, smFont);
        SetTextColor(hdc, RGB(255, 255, 255));
        const wchar_t* btnLbl = (mWeaponEnhance >= 25) ? L"MAX" : L"Enhance +1";
        int blw = (int)wcslen(btnLbl) * 9;
        TextOutW(hdc, enhBtnX + enhBtnW / 2 - blw / 2, enhBtnY + 9,
                 btnLbl, (int)wcslen(btnLbl));
        SelectObject(hdc, oldFont);
    }

    // Result message (fades out over 3s)
    if (mEnhanceMsgTimer > 0.0f && !mEnhanceMsg.empty()) {
        oldFont = (HFONT)SelectObject(hdc, infoFont);
        SetTextColor(hdc, RGB(255, 255, 120));
        int mw = (int)mEnhanceMsg.size() * 9;
        TextOutW(hdc, PX + PW / 2 - mw / 2, PY + 542,
                 mEnhanceMsg.c_str(), (int)mEnhanceMsg.size());
        SelectObject(hdc, oldFont);
    }

    DeleteObject(infoFont);
    DeleteObject(boldFont);
    DeleteObject(nameFont);
    DeleteObject(descFont);
    DeleteObject(smFont);
    DeleteObject(titleFont);
}

// ---------------------------------------------------------------------------
// Ability NPC proximity check (within 3 tiles)
// ---------------------------------------------------------------------------

bool Player::IsNearAbilityNpc() const
{
    const int myTileX = GetX() / TILE_SIZE;
    const int myTileY = GetY() / TILE_SIZE;
    for (const auto& pair : mRenderList) {
        const RenderObject& obj = pair.second;
        if (obj.visual_id != NPC_ABILITY) continue;
        int npcTileX = (int)obj.x / TILE_SIZE;
        int npcTileY = (int)obj.y / TILE_SIZE;
        int dx = myTileX - npcTileX;
        int dy = myTileY - npcTileY;
        if (dx * dx + dy * dy <= 9) // within 3 tiles
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Quest NPC proximity check and interaction
// ---------------------------------------------------------------------------

bool Player::IsNearQuestNpc() const
{
    const int myTileX = GetX() / TILE_SIZE;
    const int myTileY = GetY() / TILE_SIZE;
    for (const auto& pair : mRenderList) {
        const RenderObject& obj = pair.second;
        if (obj.visual_id != NPC_RESTAURANT) continue;
        int npcTileX = (int)obj.x / TILE_SIZE;
        int npcTileY = (int)obj.y / TILE_SIZE;
        int dx = myTileX - npcTileX;
        int dy = myTileY - npcTileY;
        if (dx * dx + dy * dy <= 9)
            return true;
    }
    return false;
}

void Player::SendQuestInteractPacket()
{
    extern void SendQuestInteractToServer();
    SendQuestInteractToServer();
}

void Player::OnQuestUpdate(unsigned char questId, unsigned char state,
                           unsigned char progress, unsigned char goal)
{
    if (questId > 1) return;
    mQuests[questId].state    = state;
    mQuests[questId].progress = progress;
    mQuests[questId].goal     = goal;
}

// ---------------------------------------------------------------------------
// Quest panel ??rendered above the minimap (bottom-right area)
// Minimap is at (640, 440) size 150x150 per MiniMap.h
// ---------------------------------------------------------------------------

void Player::RenderQuestPanel(HDC hdc) const
{
    // Names displayed to the player (hardcoded Korean; logic/rewards live in quests.lua)
        static const wchar_t* kQuestNames[2]  = { L"[Tutorial] Find the NPC", L"[Hunt] Monster Slayer" };
        static const wchar_t* kQuestGoalDesc[2] = { L"Talk to Quest NPC", L"Kill 5 monsters" };

    // Check if any quest is active (state 1 or 2)
    bool anyActive = false;
    for (int i = 0; i < 2; ++i)
        if (mQuests[i].state == 1 || mQuests[i].state == 2) { anyActive = true; break; }
    if (!anyActive) return;

    // Panel position: just above minimap (minimap top-left is 640,440)
    const int PANEL_W = 200;
    const int PANEL_X = 800 - PANEL_W - 10;  // 590 ??align with minimap right edge
    int panelH = 10;
    for (int i = 0; i < 2; ++i)
        if (mQuests[i].state >= 1 && mQuests[i].state <= 2) panelH += 42;
    const int PANEL_Y = 10 + 150 + 6;        // 6px gap below minimap (minimap top-right y=10, h=150)

    // Background
    HBRUSH bgBr = CreateSolidBrush(RGB(10, 10, 30));
    HPEN   bgPn = CreatePen(PS_SOLID, 1, RGB(80, 120, 200));
    HBRUSH ob   = (HBRUSH)SelectObject(hdc, bgBr);
    HPEN   op   = (HPEN)SelectObject(hdc, bgPn);
    Rectangle(hdc, PANEL_X, PANEL_Y, PANEL_X + PANEL_W, PANEL_Y + panelH);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(bgBr); DeleteObject(bgPn);

    SetBkMode(hdc, TRANSPARENT);

    HFONT titleFont = CreateFontW(13, 0, 0, 0, FW_BOLD, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    HFONT descFont  = CreateFontW(12, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);

    int rowY = PANEL_Y + 6;
    for (int i = 0; i < 2; ++i) {
        if (mQuests[i].state < 1 || mQuests[i].state > 2) continue;

        bool complete = (mQuests[i].state == 2);

        // Quest name
        SelectObject(hdc, titleFont);
        SetTextColor(hdc, complete ? RGB(100, 255, 100) : RGB(200, 200, 255));
        TextOutW(hdc, PANEL_X + 6, rowY, kQuestNames[i], (int)wcslen(kQuestNames[i]));
        rowY += 16;

        // Progress line
        SelectObject(hdc, descFont);
        if (i == 0) {
            // Tutorial: just "?�료! NPC?�게 ?�아가�? when complete
            const wchar_t* txt = complete ? L"Done! Return to NPC [F]" : kQuestGoalDesc[i];
            SetTextColor(hdc, complete ? RGB(80, 220, 80) : RGB(160, 160, 200));
            TextOutW(hdc, PANEL_X + 8, rowY, txt, (int)wcslen(txt));
        } else {
            // Kill quest: show X/Y progress bar
            wchar_t progBuf[48];
            if (complete)
                swprintf_s(progBuf, L"Done! Return to NPC [F]");
            else
                                swprintf_s(progBuf, L"%d / %d killed", (int)mQuests[i].progress, (int)mQuests[i].goal);
            SetTextColor(hdc, complete ? RGB(80, 220, 80) : RGB(200, 180, 100));
            TextOutW(hdc, PANEL_X + 8, rowY, progBuf, (int)wcslen(progBuf));

            if (!complete && mQuests[i].goal > 0) {
                // Small progress bar
                const int BAR_W = PANEL_W - 16, BAR_H = 4;
                int fillW = BAR_W * (int)mQuests[i].progress / (int)mQuests[i].goal;
                HPEN   np  = (HPEN)GetStockObject(NULL_PEN);
                HBRUSH barbg = CreateSolidBrush(RGB(40, 40, 80));
                HBRUSH barfg = CreateSolidBrush(RGB(100, 200, 100));
                HBRUSH oob = (HBRUSH)SelectObject(hdc, barbg);
                HPEN   oop = (HPEN)SelectObject(hdc, np);
                Rectangle(hdc, PANEL_X + 8, rowY + 14, PANEL_X + 8 + BAR_W, rowY + 14 + BAR_H);
                SelectObject(hdc, barfg);
                if (fillW > 0) Rectangle(hdc, PANEL_X + 8, rowY + 14, PANEL_X + 8 + fillW, rowY + 14 + BAR_H);
                SelectObject(hdc, oob); SelectObject(hdc, oop);
                DeleteObject(barbg); DeleteObject(barfg);
            }
        }
        rowY += 26;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);
    DeleteObject(descFont);
}

// ---------------------------------------------------------------------------
// Ability NPC stat investment UI
// ---------------------------------------------------------------------------

void Player::RenderAbilityUI(HDC hdc)
{
    if (!mShowAbilityUI) return;

    // Panel dimensions
    const int PX = 100, PY = 90, PW = 600, PH = 400;

    // Draw panel background
    HBRUSH bgBrush  = CreateSolidBrush(RGB(20, 20, 40));
    HPEN   bordPen  = CreatePen(PS_SOLID, 2, RGB(180, 160, 100));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN   oldPen   = (HPEN)SelectObject(hdc, bordPen);
    Rectangle(hdc, PX, PY, PX + PW, PY + PH);
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen);
    DeleteObject(bgBrush); DeleteObject(bordPen);

    SetBkMode(hdc, TRANSPARENT);

    // Title
    HFONT titleFont = CreateFontW(22, 0, 0, 0, FW_BOLD, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);
    SetTextColor(hdc, RGB(255, 220, 80));
        const wchar_t* title = L"[ Stat Investment ]";
    TextOutW(hdc, PX + PW / 2 - 80, PY + 12, title, (int)wcslen(title));
    DeleteObject(SelectObject(hdc, oldFont));

    // Stat points remaining
    HFONT ptFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    oldFont = (HFONT)SelectObject(hdc, ptFont);
    wchar_t ptBuf[64];
    swprintf_s(ptBuf, L"Stat Points: %d", (int)mStatPoints);
    SetTextColor(hdc, mStatPoints > 0 ? RGB(100, 255, 100) : RGB(180, 80, 80));
    TextOutW(hdc, PX + PW / 2 - 85, PY + 40, ptBuf, (int)wcslen(ptBuf));

    // Close hint
    SetTextColor(hdc, RGB(150, 150, 150));
        const wchar_t* hint = L"F / ESC: Close";
    TextOutW(hdc, PX + PW - 100, PY + 12, hint, (int)wcslen(hint));
    DeleteObject(SelectObject(hdc, oldFont));

    // Per-stat data
    struct StatInfo {
        const wchar_t* name;
        unsigned char  value;
        const wchar_t* effect1;
        const wchar_t* effect2;
    };
    const StatInfo stats[4] = {
                { L"STR", mStr,  L"Melee ATK+3",        L"Increases melee attack damage"   },
        { L"INT", mIntl, L"Skill DMG+4",        L"Boosts AOE attack damage"        },
        { L"DEX", mDex,  L"Move spd+5px/s",     L"Cap 400px/s (8 tiles/s)"        },
                { L"LUK", mLuk,  L"Crit chance+1%",       L"Crit multiplier x1.5"       },
    };

    // Column centers
    const int colCX[4] = { 175, 325, 475, 625 };
    const int iconY     = 75;   // top of icon relative to PY
    const int iconSize  = 80;
    const int btnY      = 330;  // absolute Y of plus button top
    const int btnSize   = 40;

    HFONT nameFont = CreateFontW(17, 0, 0, 0, FW_BOLD, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
    HFONT descFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");

    for (int i = 0; i < 4; ++i) {
        int cx = colCX[i];

        // Divider line between columns
        if (i > 0) {
            HPEN divPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 100));
            HPEN odp = (HPEN)SelectObject(hdc, divPen);
            MoveToEx(hdc, cx - 75, PY + 65, nullptr);
            LineTo(hdc, cx - 75, PY + PH - 20);
            SelectObject(hdc, odp); DeleteObject(divPen);
        }

        // Stat icon (uiIdx 0-3)
        SpriteManager::DrawUiImage(hdc, i, cx - iconSize / 2, PY + iconY, iconSize, iconSize);

        // Stat name
        oldFont = (HFONT)SelectObject(hdc, nameFont);
        SetTextColor(hdc, RGB(220, 200, 100));
        TextOutW(hdc, cx - 15, PY + iconY + iconSize + 8, stats[i].name, (int)wcslen(stats[i].name));
        DeleteObject(SelectObject(hdc, oldFont));

        // Current value
        oldFont = (HFONT)SelectObject(hdc, descFont);
        wchar_t valBuf[16];
        swprintf_s(valBuf, L"%d", (int)stats[i].value);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, cx - 8, PY + iconY + iconSize + 30, valBuf, (int)wcslen(valBuf));

        // Effect lines
        SetTextColor(hdc, RGB(160, 220, 160));
        int ew1 = (int)wcslen(stats[i].effect1) * 7;
        TextOutW(hdc, cx - ew1 / 2, PY + iconY + iconSize + 52, stats[i].effect1, (int)wcslen(stats[i].effect1));
        SetTextColor(hdc, RGB(130, 170, 130));
        int ew2 = (int)wcslen(stats[i].effect2) * 7;
        TextOutW(hdc, cx - ew2 / 2, PY + iconY + iconSize + 70, stats[i].effect2, (int)wcslen(stats[i].effect2));
        DeleteObject(SelectObject(hdc, oldFont));

        // Plus button
        bool canInvest = (mStatPoints > 0);
        if (canInvest) {
            SpriteManager::DrawUiImage(hdc, 4,
                cx - btnSize / 2, btnY, btnSize, btnSize);
        } else {
            // Greyed-out rectangle when no points
            HBRUSH gb  = CreateSolidBrush(RGB(60, 60, 60));
            HBRUSH ogb = (HBRUSH)SelectObject(hdc, gb);
            Rectangle(hdc, cx - btnSize/2, btnY, cx + btnSize/2, btnY + btnSize);
            SelectObject(hdc, ogb); DeleteObject(gb);
        }
    }

    DeleteObject(nameFont);
    DeleteObject(descFont);
}

// ---------------------------------------------------------------------------
// Dungeon: send entry / exit request to server
// ---------------------------------------------------------------------------

void Player::SendDungeonEnterPacket()
{
    extern void SendDungeonEnterToServer();
    SendDungeonEnterToServer();
}

// Called when server confirms dungeon entry or exit.
void Player::DungeonEnter(unsigned char entered, int instance_id, short tileX, short tileY)
{
    mIsInDungeon       = (entered == 1);
    mDungeonInstanceId = instance_id;

    int pixelX = tileX * TILE_SIZE + TILE_SIZE / 2;
    int pixelY = tileY * TILE_SIZE + TILE_SIZE / 2;
    SetPosition(pixelX, pixelY);
    mLastSentX = tileX;
    mLastSentY = tileY;

    // Clear all rendered objects ??the server will re-send what's visible in the new zone.
    mRenderList.clear();
    mSelfDamages.clear();
    mAttackEffects.clear();
}

void Player::OnHandMoveTo(int objId, short targetX, short targetY, int moveMs)
{
    auto it = mRenderList.find(objId);
    if (it == mRenderList.end()) return;
    RenderObject& obj = it->second;

    float destPixX = targetX * TILE_SIZE + TILE_SIZE / 2.0f;
    float destPixY = targetY * TILE_SIZE + TILE_SIZE / 2.0f;

    obj.handLerpFromX  = obj.x;
    obj.handLerpFromY  = obj.y;
    obj.targetX        = destPixX;
    obj.targetY        = destPixY;
    obj.handLerpStart  = GetTickCount();
    obj.handLerpDurMs  = moveMs > 0 ? moveMs : 1;
    obj.handLerping    = true;
    obj.isMoving       = false;  // disable the generic lerp for hands
}

void Player::OnLaserFire(int objectId, short centerY, int /*durationMs*/)
{
    // Store the target row on the firing hand's RenderObject.
    // Laser visibility is gated by hand attack animation frame >= 10.
    auto it = mRenderList.find(objectId);
    if (it != mRenderList.end())
        it->second.laserCenterY = centerY;
}

void Player::OnSwordFall(int fallDurationMs)
{
    mSwordFallActive     = true;
    mSwordFallStartMs    = GetTickCount();
    mSwordFallDurationMs = fallDurationMs;
}

void Player::OnSwordFallH(int fallDurationMs)
{
    mSwordHFallActive     = true;
    mSwordHFallStartMs    = GetTickCount();
    mSwordHFallDurationMs = fallDurationMs;
}

void Player::OnHandAnimState(int objId, unsigned char animState)
{
    auto it = mRenderList.find(objId);
    if (it == mRenderList.end()) return;
    RenderObject& obj = it->second;

    obj.handAnimState   = animState;
    obj.handAttackFrame = 0;
    obj.handAttackTimer = 0.0f;
    if (animState == 0) obj.laserCenterY = -1;  // laser off when hand goes idle
}

// ---------------------------------------------------------------------------
// Dungeon overlay: dark stone floor + border grid + HUD + hints
// ---------------------------------------------------------------------------

void Player::RenderDungeonOverlay(HDC hdc)
{
    if (!mIsInDungeon || mDungeonInstanceId < 0) {
        // Show entry hint when near Quest NPC and in a party (not yet inside)
        if (IsNearQuestNpc() && mPartyId >= 0) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(180, 255, 180));
            const char* hint = "[G] Enter Dungeon";
            TextOutA(hdc, 10, 440, hint, (int)strlen(hint));
        }
        return;
    }

    Camera* camera = GAME.GetCamera();

    // Dungeon uses local coordinate space (0..DUNGEON_SIZE*TILE_SIZE)
    int worldMinX = 0;
    int worldMinY = 0;
    int worldMaxX = DUNGEON_SIZE * TILE_SIZE;
    int worldMaxY = DUNGEON_SIZE * TILE_SIZE;

    int sMinX, sMinY, sMaxX, sMaxY;
    camera->WorldToScreen(worldMinX, worldMinY, sMinX, sMinY);
    camera->WorldToScreen(worldMaxX, worldMaxY, sMaxX, sMaxY);

    // Floor fill ??dark stone colour
    HBRUSH floorBr = CreateSolidBrush(RGB(50, 48, 44));
    HPEN   nullPen  = (HPEN)GetStockObject(NULL_PEN);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, floorBr);
    HPEN   op = (HPEN)SelectObject(hdc, nullPen);
    Rectangle(hdc, sMinX, sMinY, sMaxX, sMaxY);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(floorBr);

    // Tile grid lines
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(70, 68, 64));
    HPEN og = (HPEN)SelectObject(hdc, gridPen);
    for (int tx = 0; tx <= DUNGEON_SIZE; ++tx) {
        int wx = worldMinX + tx * TILE_SIZE;
        int sx1, sy1, sx2, sy2;
        camera->WorldToScreen(wx, worldMinY, sx1, sy1);
        camera->WorldToScreen(wx, worldMaxY, sx2, sy2);
        MoveToEx(hdc, sx1, sy1, nullptr);
        LineTo(hdc, sx2, sy2);
    }
    for (int ty = 0; ty <= DUNGEON_SIZE; ++ty) {
        int wy = worldMinY + ty * TILE_SIZE;
        int sx1, sy1, sx2, sy2;
        camera->WorldToScreen(worldMinX, wy, sx1, sy1);
        camera->WorldToScreen(worldMaxX, wy, sx2, sy2);
        MoveToEx(hdc, sx1, sy1, nullptr);
        LineTo(hdc, sx2, sy2);
    }
    SelectObject(hdc, og);
    DeleteObject(gridPen);

    // Border wall ??bright stone edge
    HPEN borderPen = CreatePen(PS_SOLID, 3, RGB(160, 140, 100));
    HPEN obp = (HPEN)SelectObject(hdc, borderPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, sMinX, sMinY, sMaxX, sMaxY);
    SelectObject(hdc, obp);
    DeleteObject(borderPen);

    // HUD: dungeon info at top-left
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 200, 80));
    char dunBuf[48];
    sprintf_s(dunBuf, "[Dungeon #%d]   G: Exit", mDungeonInstanceId);
    TextOutA(hdc, 10, 440, dunBuf, (int)strlen(dunBuf));
}