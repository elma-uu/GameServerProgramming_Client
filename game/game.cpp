#include "Game.h"
#include "Game.h"
#include "Input.h"
#include "Network.h"


Game::Game()
	: mHwnd(nullptr)
	, mHdc(nullptr)
	, mWidth(0)
	, mHeight(0)
	, mBackHdc(NULL)
	, mBackBitmap(NULL)
	, mNetwork(nullptr)
{

}

Game::~Game()
{
	if (mNetwork != nullptr)
	{
		delete mNetwork;
		mNetwork = nullptr;
	}
}
void Game::Initialize(HWND hwnd, UINT width, UINT height)
{
	adjustWindowRect(hwnd, width, height);
	createBuffer(width, height);
	initializeEtc();

	// Network 초기화 및 서버 연결
	mNetwork = new Network();
	if (mNetwork->Connect("127.0.0.1", PORT))
	{
		// 로그인 패킷 전송
		C2S_Login loginPacket;
		loginPacket.size = sizeof(C2S_Login);
		loginPacket.type = C2S_LOGIN;
		strcpy_s(loginPacket.username, MAX_NAME_LEN, "Player");
		mNetwork->Send(&loginPacket);
	}

	// 카메라 초기화: 맵(2000x2000 타일), 타일 크기(50), 뷰(16x12 타일 = 800x600 픽셀)
	mCamera.Initialize(2000, 2000, 50, 16, 12);

	// 맵 초기화: 2000x2000 타일, 타일 크기 50
	mMap.Initialize(2000, 2000, 50);
	// 미니맵 초기화: 2000x2000 타일, 타일 크기 50
	mMiniMap.Initialize(2000, 2000, 50);

	// 플레이어 초기 위치 설정 (타일 기준 좌표 1000, 1000 중심 = 픽셀 50025, 50025)
	mAvatar.SetPosition(50025, 50025);
}
void Game::Run()
{
	Update();
	LateUpdate();
	Render();
}
void Game::Update()
{
	Input::Update();
	Time::Update();
	mAvatar.Update();
	mCamera.Update();

	// 서버로부터 패킷 수신 및 처리
	if (mNetwork != nullptr)
	{
		mNetwork->ReceiveAndProcessPackets();
	}
}
void Game::LateUpdate()
{
	mAvatar.LateUpdate();
}
void Game::Render()
{
	clearRenderTarget();

	// 맵 그리드 렌더링
	mMap.Render(mBackHdc);

	// 플레이어 렌더링
	mAvatar.Render(mBackHdc);

	// 미니맵 렌더링
	mMiniMap.Render(mBackHdc);

	// FPS 표시
	Time::Render(mBackHdc);

	copyRenderTarget(mBackHdc, mHdc);
}

void Game::clearRenderTarget()
{
	// 검은 배경으로 채우기
	HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
	HBRUSH oldBrush = (HBRUSH)SelectObject(mBackHdc, blackBrush);
	Rectangle(mBackHdc, -1, -1, 801, 601);
	SelectObject(mBackHdc, oldBrush);
	DeleteObject(blackBrush);
}

void Game::copyRenderTarget(HDC source, HDC dest)
{
	// 백버퍼(source)를 화면(dest)에 복사 (800x600)
	BitBlt(dest, 0, 0, 800, 600, source, 0, 0, SRCCOPY);
}

void Game::adjustWindowRect(HWND hwnd, UINT width, UINT height)
{
	mHwnd = hwnd;
	mHdc = GetDC(hwnd);

	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

	mWidth = rect.right - rect.left;
	mHeight = rect.bottom - rect.top;

	SetWindowPos(hwnd, nullptr, 0, 0, mWidth, mHeight, 0);
	ShowWindow(hwnd, true);
}

void Game::createBuffer(UINT width, UINT height)
{
	// 백버퍼 크기를 게임 해상도(800x600)로 설정
	mBackBitmap = CreateCompatibleBitmap(mHdc, 800, 600);

	mBackHdc = CreateCompatibleDC(mHdc);

	HBITMAP oldBitmap = (HBITMAP)SelectObject(mBackHdc, mBackBitmap);
	DeleteObject(oldBitmap);
}

void Game::initializeEtc()
{
	Input::Initialize();
	Time::Initialize();
}

// 플레이어 이동 패킷을 서버에 전송하는 함수
void SendPlayerMovePacket(int tileX, int tileY)
{
	Network* network = GAME.GetNetwork();
	if (network != nullptr)
	{
		C2S_Move movePacket;
		movePacket.size = sizeof(C2S_Move);
		movePacket.type = C2S_MOVE;
		movePacket.x = tileX;
		movePacket.y = tileY;
		movePacket.dir = GAME.GetAvatar()->GetLastDirection();  // 마지막 이동 방향 사용
		movePacket.move_time = 0;
		network->Send(&movePacket);
		printf("Move packet sent to server - Tile: (%d, %d), Direction: %d\n", tileX, tileY, movePacket.dir);
	}
}