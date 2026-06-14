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
	, mState(GameState::LOGIN)
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

	mCamera.Initialize(2000, 2000, 50, 16, 12);
	mMap.Initialize(2000, 2000, 50);
	mMiniMap.Initialize(2000, 2000, 50);
	mAvatar.SetPosition(50025, 50025);

	// Connect to server but do NOT send login yet — wait for the login screen
	mNetwork = new Network();
	mNetwork->Connect("127.0.0.1", PORT);

	mState = GameState::LOGIN;
	Input::SetLoginMode(true);
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

	if (mNetwork != nullptr)
		mNetwork->ReceiveAndProcessPackets();

	if (mState == GameState::LOGIN)
	{
		// Submit login when Enter is pressed
		if (Input::ConsumeLoginSubmit())
		{
			std::wstring id = Input::GetLoginId();
			std::wstring pw = Input::GetLoginPw();
			if (!id.empty() && !pw.empty())
			{
				std::string idStr(id.begin(), id.end());
				std::string pwStr(pw.begin(), pw.end());
				SendLoginPacket(idStr, pwStr);
				mLoginMessage = "Connecting...";
			}
			else
			{
				mLoginMessage = "ID and Password required.";
			}
		}
		return; // skip game update while in login screen
	}

	// PLAYING state
	mAvatar.Update();
	mCamera.Update();

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
	mChatSystem.SetChatMode(Input::IsChatMode());
}
void Game::LateUpdate()
{
	if (mState == GameState::PLAYING)
		mAvatar.LateUpdate();
}
void Game::Render()
{
	clearRenderTarget();

	if (mState == GameState::LOGIN)
	{
		RenderLoginScreen(mBackHdc);
		copyRenderTarget(mBackHdc, mHdc);
		return;
	}

	mMap.Render(mBackHdc);
	mAvatar.Render(mBackHdc);
	mMiniMap.Render(mBackHdc);
	Time::Render(mBackHdc);
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

void Game::SendLoginPacket(const std::string& id, const std::string& pw)
{
	if (mNetwork == nullptr) return;
	mAvatar.SetMyUsername(id);
	C2S_Login packet;
	packet.size = sizeof(C2S_Login);
	packet.type = C2S_LOGIN;
	strncpy_s(packet.username, id.c_str(), MAX_NAME_LEN - 1);
	packet.username[MAX_NAME_LEN - 1] = '\0';
	strncpy_s(packet.password, pw.c_str(), MAX_NAME_LEN - 1);
	packet.password[MAX_NAME_LEN - 1] = '\0';
	mNetwork->Send(&packet);
}

void Game::OnLoginSuccess()
{
	mState = GameState::PLAYING;
	Input::SetLoginMode(false);
	mLoginMessage.clear();
}

void Game::RenderLoginScreen(HDC hdc)
{
	const int CX = 400, CY = 300; // screen center
	const int BOX_W = 300, BOX_H = 200;
	const int BX = CX - BOX_W / 2, BY = CY - BOX_H / 2;

	// Background panel
	HBRUSH panelBrush = CreateSolidBrush(RGB(20, 20, 40));
	HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(80, 120, 200));
	HBRUSH oldBr = (HBRUSH)SelectObject(hdc, panelBrush);
	HPEN   oldPn = (HPEN)SelectObject(hdc, borderPen);
	Rectangle(hdc, BX, BY, BX + BOX_W, BY + BOX_H);
	SelectObject(hdc, oldBr); SelectObject(hdc, oldPn);
	DeleteObject(panelBrush); DeleteObject(borderPen);

	SetBkMode(hdc, TRANSPARENT);

	// Title
	SetTextColor(hdc, RGB(120, 180, 255));
	const char* title = "SIMPLEST MMORPG";
	TextOutA(hdc, CX - 60, BY + 12, title, static_cast<int>(strlen(title)));

	// ID field
	bool focusId = Input::IsLoginFocusId();
	std::wstring idW  = Input::GetLoginId();
	std::wstring pwW  = Input::GetLoginPw();
	std::string  idS(idW.begin(), idW.end());
	std::string  pwMask(pwW.size(), '*');

	SetTextColor(hdc, RGB(180, 180, 180));
	TextOutA(hdc, BX + 20, BY + 50, "ID :", 4);
	TextOutA(hdc, BX + 20, BY + 80, "PW :", 4);

	// Input box highlight
	auto drawBox = [&](int x, int y, bool focused) {
		HPEN pen = CreatePen(PS_SOLID, 1, focused ? RGB(100, 200, 100) : RGB(80, 80, 80));
		HBRUSH br = CreateSolidBrush(RGB(30, 30, 50));
		HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
		HPEN   op = (HPEN)SelectObject(hdc, pen);
		Rectangle(hdc, x, y, x + 180, y + 20);
		SelectObject(hdc, ob); SelectObject(hdc, op);
		DeleteObject(pen); DeleteObject(br);
	};

	drawBox(BX + 55, BY + 47, focusId);
	drawBox(BX + 55, BY + 77, !focusId);

	SetTextColor(hdc, RGB(220, 220, 100));
	char idBuf[24], pwBuf[24];
	sprintf_s(idBuf, "%s_", idS.c_str());
	sprintf_s(pwBuf, "%s_", pwMask.c_str());
	TextOutA(hdc, BX + 58, BY + 49, focusId  ? idBuf : idS.c_str(),
		static_cast<int>(focusId ? strlen(idBuf) : idS.size()));
	TextOutA(hdc, BX + 58, BY + 79, !focusId ? pwBuf : pwMask.c_str(),
		static_cast<int>(!focusId ? strlen(pwBuf) : pwMask.size()));

	// Hints
	SetTextColor(hdc, RGB(120, 120, 120));
	TextOutA(hdc, BX + 20, BY + 112, "Tab: switch   Enter: login/register", 35);

	// Server message
	if (!mLoginMessage.empty())
	{
		bool isError = (mLoginMessage.find("Wrong") != std::string::npos ||
		                mLoginMessage.find("error") != std::string::npos ||
		                mLoginMessage.find("required") != std::string::npos);
		SetTextColor(hdc, isError ? RGB(255, 80, 80) : RGB(80, 255, 80));
		TextOutA(hdc, BX + 10, BY + 145, mLoginMessage.c_str(),
			static_cast<int>(mLoginMessage.size()));
	}
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

void SendAoeAttackToServer()
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_AoeAttack packet;
	packet.size = sizeof(C2S_AoeAttack);
	packet.type = C2S_AOE_ATTACK;
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

void SendPartyCreateToServer()
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_PartyCreate pkt;
	pkt.size = sizeof(C2S_PartyCreate);
	pkt.type = C2S_PARTY_CREATE;
	network->Send(&pkt);
}

void SendPartyJoinToServer(int partyId)
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_PartyJoin pkt;
	pkt.size = sizeof(C2S_PartyJoin);
	pkt.type = C2S_PARTY_JOIN;
	pkt.party_id = partyId;
	network->Send(&pkt);
}

void SendPartyLeaveToServer()
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_PartyLeave pkt;
	pkt.size = sizeof(C2S_PartyLeave);
	pkt.type = C2S_PARTY_LEAVE;
	network->Send(&pkt);
}

void SendPartyListReqToServer()
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_PartyListReq pkt;
	pkt.size = sizeof(C2S_PartyListReq);
	pkt.type = C2S_PARTY_LIST_REQ;
	network->Send(&pkt);
}