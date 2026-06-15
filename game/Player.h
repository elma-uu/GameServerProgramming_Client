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

    // Shop
    void SetGold(int g) { mGold = g; }
    void OnBuyResult(unsigned char success, ITEM_TYPE item, int gold, int newHp, short newX, short newY);
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
    void      RenderAbilityUI(HDC hdc);
    bool      IsNearAbilityNpc() const;
    void      RenderShopUI(HDC hdc);
    bool      IsNearShopNpc() const;
    bool      IsInSafeZone() const;

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
    float mPotionCooldown    = 0.0f;

    // Party state
    int                       mPartyId          = -1;
    std::vector<PartyMember>  mPartyMembers;
    bool                      mShowPartyUI      = false;
    int                       mPartyUISelection = -1;
    std::vector<PartyUIEntry> mPartyList;
    std::string               mMyUsername;
};
