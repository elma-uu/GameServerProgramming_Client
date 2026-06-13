#pragma once
#pragma once

#include "Object.h"
#include "protocol_2026.h"
#include <string>

class Player : public Object
{
public:
	Player();
	~Player();
	virtual void Update() override;
	virtual void LateUpdate() override;
	virtual void Render(HDC hdc) override;

	// Setter for Packet Process
	void SetPlayerInfo
	(const int& id, const int& x, const int& y, const int& hp
		, const int& max_hp, const unsigned long long& exp, const unsigned char& level)
	{
		playerID = id;
		SetPosition(x, y);
		SetHp(hp);
		mMaxHp = max_hp;
		mExp = exp;
		mLevel = level;
	}

	// Object management for render list
	void AddObject(int objectId, const std::string& objName, int visualId, 
		int x, int y, int hp, int max_hp, unsigned long long exp, unsigned char level);
	void RemoveObject(int objectId);
	void UpdateObjectPosition(int objectId, int x, int y);
	void UpdateObjectStatus(int objectId, int hp, int max_hp, unsigned long long exp, unsigned char level);
	void RenderObjects(HDC hdc);
	void SendMoveToServer(int tileX, int tileY);

	int GetPlayerID() const { return playerID; }
	DIRECTION GetLastDirection() const { return mLastDirection; }
	std::string GetObjectName(int objectId) const;

	void SetStatInfo(unsigned char str, unsigned char intl,
		unsigned char dex, unsigned char luk, unsigned char points)
	{
		mStr = str; mIntl = intl; mDex = dex; mLuk = luk; mStatPoints = points;
	}
	void RenderStats(HDC hdc);
	void SendStatInvestPacket(STAT_TYPE statType);

private:
	struct RenderObject
	{
		int object_id;
		std::string obj_name;
		int visual_id;
		int x;
		int y;
		int hp;
		int max_hp;
		unsigned long long exp;
		unsigned char level;
	};

	int playerID;
	int mVisualId; // for future use (different visual appearances)
	int mX;
	int mY;
	int mHp;
	int mMaxHp;
	unsigned long long mExp;
	unsigned char mLevel;
	unsigned char mStr;
	unsigned char mIntl;
	unsigned char mDex;
	unsigned char mLuk;
	unsigned char mStatPoints;
	std::unordered_map<int, RenderObject> mRenderList;

	// ���� ��ġ ���� (��ġ ���� �� ������ �˸�)
	int mLastSentX;
	int mLastSentY;
	DIRECTION mLastDirection; // ������ �̵� ����
};

