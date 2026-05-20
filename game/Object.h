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
	int GetHp() const { return hp; }
	void SetHp(int h) { hp = h; }

private:
	int mX;      // int로 변경 (short는 최대 32,767이라 오버플로우 발생)
	int mY;      // int로 변경
	int hp;      // int로 변경
	int level;   // int로 변경
	int exp;     // int로 변경
	std::unordered_map<int, Object> mViewList;
};

