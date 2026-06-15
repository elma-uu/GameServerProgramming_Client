#pragma once

constexpr short PORT = 3500;
constexpr int WORLD_WIDTH  = 16000; // extended for dungeon instances
constexpr int WORLD_HEIGHT = 2000;

// Instance dungeon
constexpr int DUNGEON_BASE_X        = 2500;
constexpr int DUNGEON_BASE_Y        = 100;
constexpr int DUNGEON_SIZE          = 30;
constexpr int DUNGEON_STRIDE        = 50;
constexpr int SWORD_X_STEP          = 3;    // sword column spacing (tiles 3,6,9,...)
constexpr int MAX_DUNGEON_INSTANCES = 250;
constexpr int MAX_PLAYERS = 10000;
constexpr int NUM_NPCS = 200000;
constexpr int NPC_ID_START = 1000000;
constexpr int NPC_MOVE_INTERVAL = 500; // in milliseconds
constexpr int TOWN_NPC_ID_START = NPC_ID_START + NUM_NPCS;

// Town NPC visual IDs (monster visual IDs are 0-3)
constexpr int VISUAL_TOWN_ABILITY     = 4;
constexpr int VISUAL_TOWN_RESTAURANT  = 5;
constexpr int VISUAL_TOWN_SHOP        = 6;
constexpr int VISUAL_BOSS_BELIAL        = 7;
constexpr int VISUAL_BOSS_BELIAL_HAND_L = 8;
constexpr int VISUAL_BOSS_BELIAL_HAND_R = 9;
constexpr int DUNGEON_BOSS_ID_START     = TOWN_NPC_ID_START + 100;
constexpr int DUNGEON_HAND_L_ID_START   = DUNGEON_BOSS_ID_START + MAX_DUNGEON_INSTANCES;
constexpr int DUNGEON_HAND_R_ID_START   = DUNGEON_BOSS_ID_START + MAX_DUNGEON_INSTANCES * 2;
constexpr int MAX_NAME_LEN = 20;
constexpr int MAX_CHAT_MSG_LEN = 200;
constexpr int MAX_BUF_SIZE = 1024;

enum DIRECTION { UP, DOWN, LEFT, RIGHT };

enum PACKET_TYPE {
	C2S_LOGIN,			// Client to Server: Login request
	// ����� �̸��� ������ �α��� ��û ��Ŷ	
	C2S_MOVE,			// Client to Server: Move request
	// �̵� ����� �̵� �ð��� ������ �̵� ��û ��Ŷ
	C2S_CHAT,			// Client to Server: Chat message
	// ä�� �޽����� ������ ä�� ��û ��Ŷ
	C2S_ATTACK,			// Client to Server: Attack request
	// ���� ��û ��Ŷ (4 ���� ���� ����)
	C2S_AOE_ATTACK,		// Client to Server: AoE attack (8 surrounding tiles)
	C2S_TELEPORT,		// Client to Server: Teleport request
	// �ڷ���Ʈ ��û ��Ŷ (������ ��ǥ ����)
	// STRESS TEST������ �߰��� ��Ŷ�Դϴ�. ���� ������ ������ ���� ����.
	C2S_LOGOUT,			// Client to Server: Logout request

	S2C_LOGIN_RESULT,	//	Server to Client: Login result
	// �α��� ��� ��Ŷ (���� ���ο� �޽��� ����)
	S2C_AVATAR_INFO,	//	Server to Client: Avatar information
	S2C_ADD_OBJECT,		//	Server to Client: Add player or NPC		
	S2C_REMOVE_OBJECT,	//	Server to Client: Remove player or NPC
	S2C_MOVE_OBJECT,	//	Server to Client: Move player or NPC
	S2C_CHAT_MESSAGE,	//	Server to Client: Chat message
	S2C_STATUS_CHANGE,	//	Server to Client: Update player or NPC status (e.g., health, buffs)
	C2S_STAT_INVEST,	//	Client to Server: invest one stat point
	S2C_STAT_INFO,		//	Server to Client: current stat values and available points

	C2S_PARTY_CREATE,	//	Client to Server: create a new party
	C2S_PARTY_JOIN,		//	Client to Server: join existing party
	C2S_PARTY_LEAVE,	//	Client to Server: leave current party
	C2S_PARTY_LIST_REQ,	//	Client to Server: request party list

	S2C_PARTY_LIST,		//	Server to Client: list of all parties
	S2C_PARTY_UPDATE,	//	Server to Client: current party membership/state

	C2S_CHAR_SELECT,	//	Client to Server: choose character visual (sent before entering world)

	S2C_DAMAGE_NUMBER,	//	Server to Client: show floating damage number near an object

	C2S_BUY_ITEM,		//	Client to Server: buy item from shop NPC
	S2C_BUY_RESULT,		//	Server to Client: purchase result (success, gold, effect)
	S2C_GOLD_UPDATE,	//	Server to Client: gold balance changed (kill reward etc.)

	S2C_RESPAWN,		//	Server to Client: player died and respawned at town

	C2S_QUEST_INTERACT,	//	Client to Server: player pressed F near Quest NPC
	S2C_QUEST_UPDATE,	//	Server to Client: quest state changed

	C2S_DUNGEON_ENTER,	//	party leader enters; any member exits when inside
	S2C_DUNGEON_ENTER,	//	server teleports player into / out of dungeon

	C2S_USE_ITEM,		//	use item from inventory
	S2C_USE_ITEM_RESULT,//	item use result (effect + remaining count)

	S2C_HAND_MOVE_TO,	//	boss hand smooth movement
	S2C_LASER_FIRE,		//	laser fires at dungeon row y
	S2C_HAND_ANIM_STATE,//	switch boss hand anim state (0=idle 1=attack)
	S2C_SWORD_FALL,		//	phase 2 vertical swords fall (x=3,6,9,...)
	S2C_SWORD_FALL_H,	//	phase 2 horizontal swords sweep (y=3,6,9,...)
	S2C_PLAYER_DIE,		//	player died — play die animation

	C2S_ENHANCE_WEAPON,	//	request weapon enhancement
	S2C_ENHANCE_RESULT,	//	enhancement result (success/fail/reset + new level + gold)
};

enum STAT_TYPE : unsigned char {
	STAT_STR = 0,
	STAT_INT = 1,
	STAT_DEX = 2,
	STAT_LUK = 3,
};

enum ITEM_TYPE : unsigned char {
	ITEM_HP_POTION       = 0,
	ITEM_TELEPORT_SCROLL = 1,
};
constexpr int SHOP_POTION_PRICE   = 200;
constexpr int SHOP_TELEPORT_PRICE = 500;

enum LOGIN_RESULT : unsigned char {
	LOGIN_SUCCESS  = 0,
	LOGIN_NEW_USER = 1,
	LOGIN_WRONG_PW = 2,
	LOGIN_DB_ERROR = 3,
};

#pragma pack(push, 1) // Ensure no padding between struct members
struct C2S_Login {
	unsigned char size;
	PACKET_TYPE   type;
	char username[MAX_NAME_LEN];
	char password[MAX_NAME_LEN];
};

struct C2S_Move {
	unsigned char size;
	PACKET_TYPE   type;
	DIRECTION dir;
	short x;  // tile coordinate
	short y;  // tile coordinate
	int move_time;
};

struct C2S_Chat {
	unsigned char size;
	PACKET_TYPE   type;
	char message[MAX_CHAT_MSG_LEN];
};

struct C2S_Attack {
	unsigned char size;
	PACKET_TYPE   type;
	DIRECTION     dir;  // mouse-based attack direction
};

struct C2S_AoeAttack {
	unsigned char size;
	PACKET_TYPE   type;
	DIRECTION     dir;  // mouse-based attack direction
};

struct C2S_Teleport {
	unsigned char size;
	PACKET_TYPE   type;
	short x;
	short y;
};

struct C2S_Logout {
	unsigned char size;
	PACKET_TYPE   type;
};

struct S2C_LoginResult {
	unsigned char size;
	PACKET_TYPE   type;
	LOGIN_RESULT  result;
	char message[50];
};

struct S2C_AvatarInfo {
	unsigned char size;
	PACKET_TYPE   type;
	int playerId;
	int visualId; // for future use (different visual appearances)
	short x;
	short y;
	int hp;
	int max_hp;
	unsigned long long exp;
	unsigned char level;
};

struct S2C_AddObject {
	unsigned char size;
	PACKET_TYPE   type;
	int object_id;
	int visual_id; // for future use (different visual appearances)
	char obj_name[MAX_NAME_LEN];
	short x;
	short y;
	int hp;
	int max_hp;
	unsigned long long exp;
	unsigned char level;
};

struct S2C_RemoveObject {
	unsigned char size;
	PACKET_TYPE   type;
	int object_id;
};

struct S2C_MoveObject {
	unsigned char size;
	PACKET_TYPE   type;
	int object_id;
	short x;  // tile coordinate
	short y;  // tile coordinate
	int move_time;
};

struct S2C_ChatMessage {
	unsigned char size;
	PACKET_TYPE   type;
	int object_id;
	char message[MAX_CHAT_MSG_LEN];
};

struct S2C_StatusChange {
	unsigned char size;
	PACKET_TYPE   type;
	int object_id;
	int hp;
	int max_hp;
	unsigned long long exp;
	unsigned char level;
};

struct C2S_StatInvest {
	unsigned char size;
	PACKET_TYPE   type;
	STAT_TYPE     stat_type;
};

struct S2C_StatInfo {
	unsigned char size;
	PACKET_TYPE   type;
	int           object_id;
	unsigned char str;
	unsigned char intl;  // INT (int is a reserved keyword)
	unsigned char dex;
	unsigned char luk;
	unsigned char stat_points;
};

struct PartyMemberInfo {
	int           player_id;
	char          name[MAX_NAME_LEN];
	int           hp;
	int           max_hp;
	unsigned char level;
};

struct S2C_PartyUpdate {
	unsigned char   size;
	PACKET_TYPE     type;
	int             party_id;       // -1 = left / no party
	unsigned char   member_count;
	PartyMemberInfo members[4];
};

struct PartyListEntry {
	int           party_id;
	char          leader_name[MAX_NAME_LEN];
	unsigned char member_count;
};

struct S2C_PartyList {
	unsigned char  size;
	PACKET_TYPE    type;
	unsigned char  party_count;
	PartyListEntry entries[8];
};

struct C2S_PartyCreate {
	unsigned char size;
	PACKET_TYPE   type;
};

struct C2S_PartyJoin {
	unsigned char size;
	PACKET_TYPE   type;
	int           party_id;
};

struct C2S_PartyLeave {
	unsigned char size;
	PACKET_TYPE   type;
};

struct C2S_PartyListReq {
	unsigned char size;
	PACKET_TYPE   type;
};

struct C2S_CharSelect {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char visual_id; // 0=base 1=alice 2=metalPlate 3=pickax 4=redLotus
};

struct S2C_DamageNumber {
	unsigned char size;
	PACKET_TYPE   type;
	int           attacker_id; // player who dealt the damage
	int           object_id;   // target (monster)
	int           damage;
	unsigned char is_crit;
};

struct C2S_BuyItem {
	unsigned char size;
	PACKET_TYPE   type;
	ITEM_TYPE     item_type;
};

struct S2C_BuyResult {
	unsigned char  size;
	PACKET_TYPE    type;
	unsigned char  success;
	ITEM_TYPE      item_type;
	int            gold;
	int            new_hp;
	short          new_x;
	short          new_y;
};

struct S2C_GoldUpdate {
	unsigned char size;
	PACKET_TYPE   type;
	int           gold;
};

struct S2C_Respawn {
	unsigned char size;
	PACKET_TYPE   type;
	int           hp;
	int           max_hp;
	short         x;
	short         y;
};

struct C2S_QuestInteract {
	unsigned char size;
	PACKET_TYPE   type;
};

// quest_state: 0=none 1=in_progress 2=complete(ready_to_hand_in) 3=done
struct S2C_QuestUpdate {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char quest_id;
	unsigned char quest_state;
	unsigned char progress;
	unsigned char goal;
};

struct C2S_UseItem {
	unsigned char size;
	PACKET_TYPE   type;
	ITEM_TYPE     item_type;
};

struct S2C_UseItemResult {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char success;
	ITEM_TYPE     item_type;
	int           item_count;
	int           new_hp;
	short         new_x;
	short         new_y;
};

struct C2S_DungeonEnter {
	unsigned char size;
	PACKET_TYPE   type;
};

struct S2C_DungeonEnter {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char entered;      // 1 = entered dungeon, 0 = exited to town
	int           instance_id;  // -1 if exited
	short         x;            // spawn tile X
	short         y;            // spawn tile Y
};

struct S2C_HandMoveTo {
	unsigned char size;
	PACKET_TYPE   type;
	int           object_id;
	short         target_x;
	short         target_y;
	int           move_ms;
};

struct S2C_LaserFire {
	unsigned char size;
	PACKET_TYPE   type;
	int           object_id;   // which hand fired
	short         center_y;
	int           duration_ms;
};

struct S2C_HandAnimState {
	unsigned char size;
	PACKET_TYPE   type;
	int           object_id;
	unsigned char anim_state;  // 0=idle  1=attack
};

// Phase 2: swords fall at x=3,6,9,... from row 0 to DUNGEON_SIZE-1
struct S2C_SwordFall {
	unsigned char size;
	PACKET_TYPE   type;
	int           fall_duration_ms;
};

struct S2C_SwordFallH {
	unsigned char size;
	PACKET_TYPE   type;
	int           fall_duration_ms;
};

struct S2C_PlayerDie {
	unsigned char size;
	PACKET_TYPE   type;
	int           object_id;
};

struct C2S_EnhanceWeapon {
	unsigned char size;
	PACKET_TYPE   type;
};

// result: 0=fail  1=success  2=fail+reset(level→0)
struct S2C_EnhanceResult {
	unsigned char size;
	PACKET_TYPE   type;
	unsigned char result;    // 0=fail 1=success 2=fail+reset
	unsigned char new_level; // weapon enhance level after result
	int           gold;      // updated gold balance
};

#pragma pack(pop) // Restore default packing
