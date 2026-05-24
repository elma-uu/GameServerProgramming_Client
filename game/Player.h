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
	std::unordered_map<int, RenderObject> mRenderList;

	// 이전 위치 추적 (위치 변경 시 서버에 알림)
	int mLastSentX;
	int mLastSentY;
	DIRECTION mLastDirection; // 마지막 이동 방향
};

