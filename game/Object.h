#pragma once
#pragma once

#include "Windows.h"
#include <unordered_map>

// Forward declaration
class Camera;

class Object
{
public:
	Object();
	~Object();
	virtual void Update();
	virtual void LateUpdate();
	virtual void Render(HDC hdc);

	// Setter
	void SetPosition(int x, int y) { mX = x; mY = y; }

	// Getter
	int GetX() const { return mX; }
	int GetY() const { return mY; }
	int GetHp() const { return mHp; }
	void SetHp(int h) { mHp = h; }

private:
	int mVisualId; // for future use (different visual appearances)
	int mX;
	int mY;
	int mHp;
	int mMaxHp;
	unsigned long long mExp;
	unsigned char mLevel;
};

