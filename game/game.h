#pragma once
#pragma once

#include "Input.h"
#include "Time.h"
#include "Camera.h"
#include "Player.h"
#include "Map.h"
#include "MiniMap.h"

class Network;  // Forward declaration

class Game
{
public:
	Game();
	~Game();

	void Initialize(HWND hwnd, UINT width, UINT height);
	void Run();

	void Update();
	void LateUpdate();
	void Render();
	HWND GetHwnd() { return mHwnd; }
	Camera* GetCamera() { return &mCamera; }
	Player* GetAvatar() { return &mAvatar; }
	Network* GetNetwork() { return mNetwork; }

private:
	void clearRenderTarget();
	void copyRenderTarget(HDC source, HDC dest);
	void adjustWindowRect(HWND hwnd, UINT width, UINT height);
	void createBuffer(UINT width, UINT height);
	void initializeEtc();

private:
	HWND mHwnd;
	HDC mHdc;

	HDC mBackHdc;
	HBITMAP mBackBitmap;

	UINT mWidth;
	UINT mHeight;

	Camera mCamera;
	Player mAvatar;
	Map mMap;
	MiniMap mMiniMap;
	Network* mNetwork;
};



