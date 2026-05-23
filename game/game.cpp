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
	mNetwork->Connect("127.0.0.1", PORT);

	// 카메라 초기화: 맵(2000x2000 타일), 타일 크기(50), 뷰(40x22 타일)
	// 맵 총 크기: 2000*50 = 100,000 픽셀
	mCamera.Initialize(2000, 2000, 50, 40, 22);

	// 맵 초기화: 2000x2000 타일, 타일 크기 50
	mMap.Initialize(2000, 2000, 50);
	// 미니맵 초기화: 2000x2000 타일, 타일 크기 50
	mMiniMap.Initialize(2000, 2000, 50);

	// 플레이어 초기 위치 설정 (월드 좌표, 타일 단위)
	// 2000x2000 타일의 중앙 = 1000,1000
	mAvatar.SetPosition(1000, 1000);
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
	Rectangle(mBackHdc, -1, -1, 1921, 1081);
	SelectObject(mBackHdc, oldBrush);
	DeleteObject(blackBrush);
}

void Game::copyRenderTarget(HDC source, HDC dest)
{
	// 백버퍼(source)를 화면(dest)에 복사 (1920x1080)
	BitBlt(dest, 0, 0, 1920, 1080, source, 0, 0, SRCCOPY);
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
	// 백버퍼 크기를 게임 해상도(1920x1080)로 설정
	mBackBitmap = CreateCompatibleBitmap(mHdc, 1920, 1080);

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
		movePacket.dir = UP;
		movePacket.move_time = 0;
		network->Send(&movePacket);
	}
}