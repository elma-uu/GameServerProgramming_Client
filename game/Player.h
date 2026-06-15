#pragma once

#include "Object.h"
#include "protocol_2026.h"
#include <string>
#include <vector>
#include <unordered_map>

class Player : public Object
{
public:
    Player();
    ~Player();
    virtual void Update()          override;
    virtual void LateUpdate()      override;
    virtual void Render(HDC hdc)   override;

    void RenderLayer0(HDC hdc);  // Layer 0: background (dungeon overlay)
    void RenderLayer1(HDC hdc);  // Layer 1: player + monsters + effects
    void RenderLayer2(HDC hdc);  // Layer 2: HUD / UI panels

    // Initial player info from server
    void SetPlayerInfo(const int& id, const int& x, const int& y,
        const int& hp, const int& max_hp,
        const unsigned long long& exp, const unsigned char& level)
    {
        playerID = id;
        SetPosition(x, y);
        SetHp(hp);
        mMaxHp  = max_hp;
        mExp    = exp;
        mLevel  = level;
    }

    // Render-list management
    void AddObject(int objectId, const std::string& objName, int visualId,
        int x, int y, int hp, int max_hp,
        unsigned long long exp, unsigned char level);
    void RemoveObject(int objectId);
    void UpdateObjectPosition(int objectId, int x, int y);
    void UpdateObjectStatus(int objectId, int hp, int max_hp,
        unsigned long long exp, unsigned char level);
    void RenderObjects(HDC hdc);

    // Network sends
    void SendMoveToServer(int tileX, int tileY);
    void SendAttackPacket(DIRECTION dir);
    void SendAoeAttackPacket(DIRECTION dir);
    void SendStatInvestPacket(STAT_TYPE statType);
    void SendBuyItemPacket(ITEM_TYPE itemType);

    // Getters
    int       GetPlayerID()       const { return playerID; }
    DIRECTION GetLastDirection()  const { return mLastDirection; }
    std::string GetObjectName(int objectId) const;

    void SetStatInfo(unsigned char str, unsigned char intl,
        unsigned char dex, unsigned char luk, unsigned char points)
    {
        mStr = str; mIntl = intl; mDex = dex; mLuk = luk; mStatPoints = points;
    }
    void RenderStats(HDC hdc);

    // Visual / sprite
    void SetMyVisualId(int id) { mVisualId = id; }

    // In-game stat update (no position snap)
    void UpdateAvatarStats(int hp, int maxHp,
        unsigned long long exp, unsigned char level)
    {
        SetHp(hp); mMaxHp = maxHp; mExp = exp; mLevel = level;
    }

    // First AVATAR_INFO sets position; subsequent ones only update stats
    bool IsAvatarPositionSet() const  { return mAvatarPositionSet; }
    void MarkAvatarPositionSet()       { mAvatarPositionSet = true; }

    // Floating damage numbers + system chat message
    void AddDamageNumber(int attackerId, int objectId, int damage, bool isCrit);

    // Called when the server respawns us after death
    void Respawn(int hp, int maxHp, short tileX, short tileY);

    // Party
    void SetMyUsername(const std::string& name) { mMyUsername = name; }
    void OnPartyUpdate(int partyId, int memberCount, PartyMemberInfo* members);
    void OnPartyList(int count, PartyListEntry* entries);
    void RenderPartyPanel(HDC hdc);
    void RenderPartyUI(HDC hdc);

    // Shop / inventory
    void SetGold(int g) { mGold = g; }
    // OnBuyResult: new_hp is reused as item_count after purchase
    void OnBuyResult(unsigned char success, ITEM_TYPE item, int gold, int itemCount, short unused_x, short unused_y);
    void SendUseItemPacket(ITEM_TYPE itemType);
    void OnUseItemResult(unsigned char success, ITEM_TYPE item, int itemCount,
                         int newHp, short newX, short newY);

    // Quest
    void OnQuestUpdate(unsigned char questId, unsigned char state,
                       unsigned char progress, unsigned char goal);
    void RenderQuestPanel(HDC hdc) const;

    // Dungeon
    void SendDungeonEnterPacket();
    void DungeonEnter(unsigned char entered, int instance_id, short tileX, short tileY);
    bool IsInDungeon() const { return mIsInDungeon; }

    // Boss pattern events
    void OnHandMoveTo(int objId, short targetX, short targetY, int moveMs);
    void OnLaserFire(int objectId, short centerY, int durationMs);
    void OnHandAnimState(int objId, unsigned char animState);
    void OnSwordFall(int fallDurationMs);

    void SendPartyCreate();
    void SendPartyJoin(int partyId);
    void SendPartyLeave();
    void SendPartyListReq();

private:
    // --- nested types ---
    struct AttackEffect
    {
        int       worldX;
        int       worldY;
        DIRECTION dir;
        bool      isAoe;
        float     timeLeft;
        float     totalTime;
    };

    struct DamageNumber
    {
        int   objectId;
        int   amount;
        bool  isCrit;
        float timeLeft;
        float offsetY;
    };

    struct RenderObject
    {
        int                object_id   = 0;
        std::string        obj_name;
        int                visual_id   = 0;
        float              x           = 0.0f;
        float              y           = 0.0f;
        float              targetX     = 0.0f;
        float              targetY     = 0.0f;
        bool               isMoving    = false;
        bool               facingLeft  = false;
        bool               isAttacking = false;
        float              attackTimer = 0.0f;
        int                hp          = 0;
        int                max_hp      = 0;
        unsigned long long exp         = 0;
        unsigned char      level       = 1;

        // Boss hand: smooth movement lerp
        bool               handLerping     = false;
        float              handLerpFromX   = 0.0f;
        float              handLerpFromY   = 0.0f;
        DWORD              handLerpStart   = 0;
        int                handLerpDurMs   = 0;

        // Boss hand: attack animation
        int                handAnimState   = 0;   // 0=idle  1=attack
        int                handAttackFrame = 0;
        float              handAttackTimer = 0.0f;

        // Boss hand: laser target row (-1 = no active laser)
        int                laserCenterY    = -1;
    };

    struct SelfDamage
    {
        int   amount;
        float timeLeft;
        float offsetY;
    };

    struct PartyMember
    {
        int   player_id = 0;
        char  name[MAX_NAME_LEN];
        int   hp     = 0;
        int   max_hp = 0;
        unsigned char level = 1;
    };

    struct PartyUIEntry
    {
        int   party_id     = 0;
        char  leader_name[MAX_NAME_LEN];
        unsigned char member_count = 0;
    };

    // --- private helpers ---
    DIRECTION GetMouseDirection()      const;
    void      RenderAttackEffects(HDC hdc) const;
    void      RenderDungeonOverlay(HDC hdc);
    void      RenderAbilityUI(HDC hdc);
    bool      IsNearAbilityNpc() const;
    void      RenderShopUI(HDC hdc);
    bool      IsNearShopNpc() const;
    bool      IsNearQuestNpc() const;
    bool      IsInSafeZone() const;
    void      SendQuestInteractPacket();

    // --- data members ---
    int                playerID  = -1;
    int                mVisualId =  0;
    int                mX        =  0;
    int                mY        =  0;
    int                mHp       =  100;
    int                mMaxHp    =  100;
    unsigned long long mExp      =  0;
    unsigned char      mLevel    =  1;
    unsigned char      mStr      =  5;
    unsigned char      mIntl     =  5;
    unsigned char      mDex      =  5;
    unsigned char      mLuk      =  5;
    unsigned char      mStatPoints = 0;

    std::unordered_map<int, RenderObject> mRenderList;
    std::vector<DamageNumber>             mDamageNumbers;
    std::vector<SelfDamage>               mSelfDamages;
    std::vector<AttackEffect>             mAttackEffects;

    // Movement / position tracking
    int       mLastSentX    = 1000;
    int       mLastSentY    = 1000;
    DIRECTION mLastDirection = UP;
    float     mAttackCooldown = 0.0f;
    float     mAoeCooldown    = 0.0f;
    float     mSendTimer      = 0.0f;

    // Sprite animation
    int   mAnimFrame  = 0;
    float mAnimTimer  = 0.0f;
    bool  mFacingLeft = false;
    bool  mIsMoving   = false;
    float mMoveAccum  = 0.0f;

    // State flags
    bool mAvatarPositionSet = false;
    bool mShowAbilityUI     = false;
    bool mShowShopUI        = false;

    // Economy
    int   mGold              = 0;
    int   mPotionCount       = 0;   // HP potions in inventory
    int   mScrollCount       = 0;   // teleport scrolls in inventory
    float mPotionCooldown    = 0.0f; // use cooldown (not buy cooldown)

    // Quest state (mirrors server QuestEntry)
    struct QuestEntry {
        unsigned char state    = 0;
        unsigned char progress = 0;
        unsigned char goal     = 0;
    };
    QuestEntry mQuests[2];  // [0]=tutorial  [1]=kill quest

    // Dungeon state
    bool  mIsInDungeon       = false;
    int   mDungeonInstanceId = -1;

    // Phase 2 sword fall visual
    bool  mSwordFallActive     = false;
    DWORD mSwordFallStartMs    = 0;
    int   mSwordFallDurationMs = 0;

    // Party state
    int                       mPartyId          = -1;
    std::vector<PartyMember>  mPartyMembers;
    bool                      mShowPartyUI      = false;
    int                       mPartyUISelection = -1;
    std::vector<PartyUIEntry> mPartyList;
    std::string               mMyUsername;
};
