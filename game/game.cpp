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

	// 카메라 초기화: 맵(20x20 타일), 타일 크기(100), 뷰(20x20 타일)
	mCamera.Initialize(20, 20, 100, 12, 12);
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
	mCamera.Update();
}
void Game::LateUpdate()
{

}
void Game::Render()
{
	clearRenderTarget();

	copyRenderTarget(mBackHdc, mHdc);
}

void Game::clearRenderTarget()
{

	Rectangle(mBackHdc, -1, -1, 1921, 1081);
}

void Game::copyRenderTarget(HDC source, HDC dest)
{
	StretchBlt(dest, 0, 0, 1920, 1080, source, 0, 0, 1280, 720, SRCCOPY);
	//BitBlt(dest, 0, 0, mWidth, mHeight, source, 0, 0, SRCCOPY);
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

	mBackBitmap = CreateCompatibleBitmap(mHdc, width, height);

	mBackHdc = CreateCompatibleDC(mHdc);

	HBITMAP oldBitmap = (HBITMAP)SelectObject(mBackHdc, mBackBitmap);
	DeleteObject(oldBitmap);
}

void Game::initializeEtc()
{
	Input::Initialize();
	Time::Initialize();
}