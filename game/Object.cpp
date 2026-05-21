#include "Object.h"
#include "Object.h"

Object::Object()
	: mX(0)
	, mY(0)
	, mHp(100)
	, mLevel(1)
	, mExp(0)
{
}

Object::~Object()
{
}

void Object::Update()
{
}

void Object::LateUpdate()
{
}

void Object::Render(HDC hdc)
{
}