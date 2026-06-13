#pragma once
#pragma once

#include "Input.h"
#include "Time.h"
#include "Camera.h"
#include "Player.h"
#include "Map.h"
#include "MiniMap.h"
#include "ChatSystem.h"

class Network;  // Forward declaration

enum class GameState { LOGIN, PLAYING };

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
	ChatSystem* GetChatSystem() { return &mChatSystem; }
	void SendChatPacket(const std::string& msg);

	// Login state
	GameState GetGameState() const { return mState; }
	void OnLoginSuccess();
	void SetLoginMessage(const std::string& msg) { mLoginMessage = msg; }

private:
	void clearRenderTarget();
	void copyRenderTarget(HDC source, HDC dest);
	void adjustWindowRect(HWND hwnd, UINT width, UINT height);
	void createBuffer(UINT width, UINT height);
	void initializeEtc();
	void RenderLoginScreen(HDC hdc);
	void SendLoginPacket(const std::string& id, const std::string& pw);

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
	ChatSystem mChatSystem;
	Network* mNetwork;

	GameState   mState;
	std::string mLoginMessage;
};



