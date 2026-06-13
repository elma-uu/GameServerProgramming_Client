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

	// Network �ʱ�ȭ �� ���� ����
	mNetwork = new Network();
	if (mNetwork->Connect("127.0.0.1", PORT))
	{
		// �α��� ��Ŷ ����
		C2S_Login loginPacket;
		loginPacket.size = sizeof(C2S_Login);
		loginPacket.type = C2S_LOGIN;
		strcpy_s(loginPacket.username, MAX_NAME_LEN, "Player");
		mNetwork->Send(&loginPacket);
	}

	// ī�޶� �ʱ�ȭ: ��(2000x2000 Ÿ��), Ÿ�� ũ��(50), ��(16x12 Ÿ�� = 800x600 �ȼ�)
	mCamera.Initialize(2000, 2000, 50, 16, 12);

	// �� �ʱ�ȭ: 2000x2000 Ÿ��, Ÿ�� ũ�� 50
	mMap.Initialize(2000, 2000, 50);
	// �̴ϸ� �ʱ�ȭ: 2000x2000 Ÿ��, Ÿ�� ũ�� 50
	mMiniMap.Initialize(2000, 2000, 50);

	// �÷��̾� �ʱ� ��ġ ���� (Ÿ�� ���� ��ǥ 1000, 1000 �߽� = �ȼ� 50025, 50025)
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

	// receive and process packets from server
	if (mNetwork != nullptr)
	{
		mNetwork->ReceiveAndProcessPackets();
	}

	// handle chat submit (Enter pressed while in chat mode)
	if (Input::ConsumeChatSubmit())
	{
		std::wstring inputText = Input::GetInputText();
		if (!inputText.empty())
		{
			std::string msg(inputText.begin(), inputText.end());
			SendChatPacket(msg);
		}
		Input::ClearInputText();
		Input::SetChatMode(false);
		mChatSystem.SetChatMode(false);
	}
	// sync ChatSystem mode with Input mode
	mChatSystem.SetChatMode(Input::IsChatMode());
}
void Game::LateUpdate()
{
	mAvatar.LateUpdate();
}
void Game::Render()
{
	clearRenderTarget();

	// �� �׸��� ������
	mMap.Render(mBackHdc);

	// �÷��̾� ������
	mAvatar.Render(mBackHdc);

	// �̴ϸ� ������
	mMiniMap.Render(mBackHdc);

	// FPS
	Time::Render(mBackHdc);

	// chat window (bottom-left)
	mChatSystem.Render(mBackHdc, Input::GetInputText());

	copyRenderTarget(mBackHdc, mHdc);
}

void Game::clearRenderTarget()
{
	// ���� ������� ä���
	HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
	HBRUSH oldBrush = (HBRUSH)SelectObject(mBackHdc, blackBrush);
	Rectangle(mBackHdc, -1, -1, 801, 601);
	SelectObject(mBackHdc, oldBrush);
	DeleteObject(blackBrush);
}

void Game::copyRenderTarget(HDC source, HDC dest)
{
	// �����(source)�� ȭ��(dest)�� ���� (800x600)
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
	// ����� ũ�⸦ ���� �ػ�(800x600)�� ����
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

void Game::SendChatPacket(const std::string& msg)
{
	if (mNetwork == nullptr) return;
	C2S_Chat chatPacket;
	chatPacket.size = sizeof(C2S_Chat);
	chatPacket.type = C2S_CHAT;
	strncpy_s(chatPacket.message, msg.c_str(), MAX_CHAT_MSG_LEN - 1);
	chatPacket.message[MAX_CHAT_MSG_LEN - 1] = '\0';
	mNetwork->Send(&chatPacket);
}

void SendStatInvest(STAT_TYPE st)
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_StatInvest packet;
	packet.size = sizeof(C2S_StatInvest);
	packet.type = C2S_STAT_INVEST;
	packet.stat_type = st;
	network->Send(&packet);
}

void SendAttackToServer()
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_Attack packet;
	packet.size = sizeof(C2S_Attack);
	packet.type = C2S_ATTACK;
	network->Send(&packet);
}

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
		movePacket.dir = GAME.GetAvatar()->GetLastDirection();
		movePacket.move_time = 0;
		network->Send(&movePacket);
		printf("Move packet sent to server - Tile: (%d, %d), Direction: %d\n", tileX, tileY, movePacket.dir);
	}
}