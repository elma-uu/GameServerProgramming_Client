#pragma once
#pragma once

#include "Object.h"

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

private:
	int playerID;
	int mVisualId; // for future use (different visual appearances)
	int mX;
	int mY;
	int mHp;
	int mMaxHp;
	unsigned long long mExp;
	unsigned char mLevel;
	std::unordered_map<int, Object> mViewList;
};

