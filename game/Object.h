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
	void SetPosition(short x, short y) { mX = x; mY = y; }

	// Getter
	short GetX() const { return mX; }
	short GetY() const { return mY; }
	short GetHp() const { return hp; }
	void SetHp(short h) { hp = h; }

private:
	short mX;
	short mY;
	short hp;
	short level;
	short exp;
	std::unordered_map<int, Object> mViewList;
};

