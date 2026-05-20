#pragma once
#include "Windows.h"
#include <unordered_map>

class Object
{
public:
	Object();
	~Object();
	virtual void Update();
	virtual void LateUpdate();
	virtual void Render(HDC hdc);

private:
	short mX;
	short mY;
	short hp;
	short level;
	short exp;
	std::unordered_map<int, Object> mViewList;
};

