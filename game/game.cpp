#include "Game.h"
#include "Game.h"
#include "Input.h"

Game GAME;

Game::Game()
	: mHwnd(nullptr)
	, mHdc(nullptr)
	, mWidth(0)
	, mHeight(0)
	, mBackHdc(NULL)
	, mBackBitmap(NULL)
{

}

Game::~Game()
{

}
void Game::Initialize(HWND hwnd, UINT width, UINT height)
{
	adjustWindowRect(hwnd, width, height);
	createBuffer(width, height);
	initializeEtc();

	// 카메라 초기화: 맵(20x20 타일), 타일 크기(100), 뷰(12x12 타일)
	mCamera.Initialize(20, 20, 100, 12, 12);

	// 맵 초기화: 20x20 타일, 타일 크기 100
	mMap.Initialize(20, 20, 100);

	// 플레이어 초기 위치 설정
	mPlayer.SetPosition(1000, 1000);
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
	mPlayer.Update();
	mCamera.Update();
}
void Game::LateUpdate()
{
	mPlayer.LateUpdate();
}
void Game::Render()
{
	clearRenderTarget();

	// 맵 그리드 렌더링
	mMap.Render(mBackHdc);

	// 플레이어 렌더링
	mPlayer.Render(mBackHdc);

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