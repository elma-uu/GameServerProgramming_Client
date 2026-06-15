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

enum class GameState { LOGIN, CHAR_SELECT, PLAYING };

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

	// State accessors
	GameState GetGameState() const { return mState; }
	void OnLoginSuccess();           // new user: auth ok → show char-select screen
	void OnLoginDirect();            // existing user: auth ok → PLAYING immediately
	void OnCharSelected(int charId); // char chosen → send packet → PLAYING
	void SetLoginMessage(const std::wstring& msg) { mLoginMessage = msg; }

private:
	void clearRenderTarget();
	void copyRenderTarget(HDC source, HDC dest);
	void adjustWindowRect(HWND hwnd, UINT width, UINT height);
	void createBuffer(UINT width, UINT height);
	void initializeEtc();
	void RenderLoginScreen(HDC hdc);
	void RenderCharSelectScreen(HDC hdc);
	void SendLoginPacket(const std::string& id, const std::string& pw);
	void SendCharSelectPacket(int charId);

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
	std::wstring mLoginMessage;
	int         mCharSelectIdx;   // 0-4, current selection on char-select screen
};



