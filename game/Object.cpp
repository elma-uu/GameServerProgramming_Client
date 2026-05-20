#include "Object.h"
#include "Object.h"

Object::Object()
	: mX(0)
	, mY(0)
	, hp(100)
	, level(1)
	, exp(0)
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