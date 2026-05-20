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
};

