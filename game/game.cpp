#include "Game.h"
#include "Game.h"
#include "Input.h"
#include "Network.h"
#include "SpriteManager.h"


Game::Game()
	: mHwnd(nullptr)
	, mHdc(nullptr)
	, mWidth(0)
	, mHeight(0)
	, mBackHdc(NULL)
	, mBackBitmap(NULL)
	, mNetwork(nullptr)
	, mState(GameState::LOGIN)
	, mCharSelectIdx(0)
{
}

Game::~Game()
{
	SpriteManager::Shutdown();
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
	SpriteManager::Init();

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
				std::string idStr, pwStr;
				for (wchar_t c : id) idStr += static_cast<char>(c);
				for (wchar_t c : pw) pwStr += static_cast<char>(c);
				SendLoginPacket(idStr, pwStr);
				mLoginMessage = L"접속 중...";
			}
			else
			{
				mLoginMessage = L"아이디와 비밀번호를 입력하세요.";
			}
		}
		return; // skip game update while in login screen
	}

	if (mState == GameState::CHAR_SELECT)
	{
		if (Input::GetKeyDown(eKeyCode::Left)  && mCharSelectIdx > 0)            mCharSelectIdx--;
		if (Input::GetKeyDown(eKeyCode::Right) && mCharSelectIdx < CHAR_COUNT-1) mCharSelectIdx++;
		if (Input::GetKeyDown(eKeyCode::Key1)) mCharSelectIdx = 0;
		if (Input::GetKeyDown(eKeyCode::Key2)) mCharSelectIdx = 1;
		if (Input::GetKeyDown(eKeyCode::Key3)) mCharSelectIdx = 2;
		if (Input::GetKeyDown(eKeyCode::Key4)) mCharSelectIdx = 3;
		if (Input::GetKeyDown(eKeyCode::Key5)) mCharSelectIdx = 4;
		if (Input::GetKeyDown(eKeyCode::SPACE)) OnCharSelected(mCharSelectIdx);
		return;
	}

	// PLAYING state
	mAvatar.Update();
	mCamera.Update();

	if (Input::ConsumeChatSubmit())
	{
		std::wstring inputText = Input::GetInputText();
		if (!inputText.empty())
		{
			// Convert wide string to UTF-8 for the chat packet
			int utf8Len = WideCharToMultiByte(CP_UTF8, 0, inputText.c_str(), -1, nullptr, 0, nullptr, nullptr);
			std::string msg(utf8Len > 0 ? utf8Len - 1 : 0, '\0');
			if (utf8Len > 0)
				WideCharToMultiByte(CP_UTF8, 0, inputText.c_str(), -1, &msg[0], utf8Len, nullptr, nullptr);
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

	if (mState == GameState::CHAR_SELECT)
	{
		RenderCharSelectScreen(mBackHdc);
		copyRenderTarget(mBackHdc, mHdc);
		return;
	}

	// Layer 0: background
	mMap.Render(mBackHdc);
	mAvatar.RenderLayer0(mBackHdc);

	// Layer 1: entities (player, monsters, effects)
	mAvatar.RenderLayer1(mBackHdc);

	// Layer 2: HUD / UI panels
	mAvatar.RenderLayer2(mBackHdc);
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

	RECT rect = { 0, 0, (LONG)width, (LONG)height };
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
	// New user — show character selection screen
	mState = GameState::CHAR_SELECT;
	mCharSelectIdx = 0;
	Input::SetLoginMode(false);
	mLoginMessage.clear();
}

void Game::OnLoginDirect()
{
	// Existing user — visual_id arrives via S2C_AVATAR_INFO, go straight to PLAYING
	mState = GameState::PLAYING;
	Input::SetLoginMode(false);
	mLoginMessage.clear();
}

void Game::OnCharSelected(int charId)
{
	mAvatar.SetMyVisualId(charId);
	SendCharSelectPacket(charId);
	mState = GameState::PLAYING;
}

void Game::RenderCharSelectScreen(HDC hdc)
{
	// Background
	HBRUSH bgBrush = CreateSolidBrush(RGB(8, 10, 30));
	HPEN nullPen    = (HPEN)GetStockObject(NULL_PEN);
	HBRUSH ob = (HBRUSH)SelectObject(hdc, bgBrush);
	HPEN   op = (HPEN)SelectObject(hdc, nullPen);
	Rectangle(hdc, 0, 0, 800, 600);
	SelectObject(hdc, ob); SelectObject(hdc, op);
	DeleteObject(bgBrush);

	SetBkMode(hdc, TRANSPARENT);

	HFONT titleFont = CreateFontW(20, 0, 0, 0, FW_BOLD, 0, 0, 0,
		HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
	HFONT hintFont  = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
	HFONT nameFont  = CreateFontW(14, 0, 0, 0, FW_BOLD, 0, 0, 0,
		HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
	HFONT oldFont   = (HFONT)SelectObject(hdc, titleFont);

	// Title
	SetTextColor(hdc, RGB(120, 180, 255));
	const wchar_t* title = L"== 캐릭터 선택 ==";
	TextOutW(hdc, 290, 30, title, (int)wcslen(title));

	// Sub-hint
	SelectObject(hdc, hintFont);
	SetTextColor(hdc, RGB(90, 90, 130));
	const wchar_t* sub = L"← →  또는  1-4키로 이동     Space: 선택 확인";
	TextOutW(hdc, 190, 555, sub, (int)wcslen(sub));

	static const wchar_t* kCharNames[CHAR_COUNT] = {
		L"기본", L"앨리스", L"철갑전사", L"광부", L"홍련"
	};

	// 5 character slots evenly across 800 px
	const int previewW = 80, previewH = 80;
	const int centerY  = 280;
	const int slotW    = 800 / CHAR_COUNT;   // 160 px per slot

	for (int i = 0; i < CHAR_COUNT; ++i) {
		int cx = slotW / 2 + i * slotW;
		int cy = centerY;

		bool selected = (i == mCharSelectIdx);

		// Selection border
		HPEN   selPen = CreatePen(PS_SOLID, selected ? 3 : 1,
		                          selected ? RGB(255, 220, 60) : RGB(50, 60, 100));
		HBRUSH selBg  = CreateSolidBrush(selected ? RGB(30, 30, 80) : RGB(15, 15, 40));
		HBRUSH obr = (HBRUSH)SelectObject(hdc, selBg);
		HPEN   opn = (HPEN)SelectObject(hdc, selPen);
		Rectangle(hdc, cx - previewW/2 - 6, cy - previewH/2 - 6,
		               cx + previewW/2 + 6, cy + previewH/2 + 6);
		SelectObject(hdc, obr); SelectObject(hdc, opn);
		DeleteObject(selPen); DeleteObject(selBg);

		// Character sprite preview
		SpriteManager::DrawPreview(hdc, i, cx, cy, previewW);

		// Character name
		SelectObject(hdc, nameFont);
		SetTextColor(hdc, selected ? RGB(255, 220, 60) : RGB(160, 160, 200));
		const wchar_t* name = kCharNames[i];
		int nameX = cx - (int)(wcslen(name) * 7);
		TextOutW(hdc, nameX, cy + previewH/2 + 14, name, (int)wcslen(name));

		// Number hint below name
		SelectObject(hdc, hintFont);
		SetTextColor(hdc, RGB(70, 70, 100));
		wchar_t numHint[6];
		if (i < 4) {
			swprintf_s(numHint, L"[%d]", i + 1);
			TextOutW(hdc, cx - 10, cy + previewH/2 + 30, numHint, (int)wcslen(numHint));
		}
	}

	SelectObject(hdc, oldFont);
	DeleteObject(titleFont);
	DeleteObject(hintFont);
	DeleteObject(nameFont);
}

void Game::SendCharSelectPacket(int charId)
{
	if (mNetwork == nullptr) return;
	C2S_CharSelect pkt;
	pkt.size      = sizeof(C2S_CharSelect);
	pkt.type      = C2S_CHAR_SELECT;
	pkt.visual_id = static_cast<unsigned char>(charId);
	mNetwork->Send(&pkt);
}

void Game::RenderLoginScreen(HDC hdc)
{
	const int CX = 400, CY = 300;
	const int BOX_W = 320, BOX_H = 210;
	const int BX = CX - BOX_W / 2, BY = CY - BOX_H / 2;

	// Background panel
	HBRUSH panelBrush = CreateSolidBrush(RGB(20, 20, 40));
	HPEN   borderPen  = CreatePen(PS_SOLID, 2, RGB(80, 120, 200));
	HBRUSH oldBr = (HBRUSH)SelectObject(hdc, panelBrush);
	HPEN   oldPn = (HPEN)SelectObject(hdc, borderPen);
	Rectangle(hdc, BX, BY, BX + BOX_W, BY + BOX_H);
	SelectObject(hdc, oldBr); SelectObject(hdc, oldPn);
	DeleteObject(panelBrush); DeleteObject(borderPen);

	SetBkMode(hdc, TRANSPARENT);

	HFONT titleFont = CreateFontW(18, 0, 0, 0, FW_BOLD, 0, 0, 0,
		HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
	HFONT labelFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
	HFONT inputFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		HANGUL_CHARSET, 0, 0, 0, 0, L"NanumBarunGothic");
	HFONT oldFont = (HFONT)SelectObject(hdc, titleFont);

	// Title
	SetTextColor(hdc, RGB(120, 180, 255));
	const wchar_t* title = L"심플 MMORPG";
	TextOutW(hdc, CX - 50, BY + 12, title, (int)wcslen(title));

	// ID / PW labels
	SelectObject(hdc, labelFont);
	SetTextColor(hdc, RGB(180, 180, 180));
	TextOutW(hdc, BX + 16, BY + 52, L"아이디 :", 5);
	TextOutW(hdc, BX + 16, BY + 82, L"비밀번호 :", 6);

	// Input boxes
	bool focusId = Input::IsLoginFocusId();
	auto drawBox = [&](int x, int y, bool focused) {
		HPEN   pen = CreatePen(PS_SOLID, 1, focused ? RGB(100, 200, 100) : RGB(80, 80, 80));
		HBRUSH br  = CreateSolidBrush(RGB(30, 30, 50));
		HBRUSH ob  = (HBRUSH)SelectObject(hdc, br);
		HPEN   op  = (HPEN)SelectObject(hdc, pen);
		Rectangle(hdc, x, y, x + 170, y + 22);
		SelectObject(hdc, ob); SelectObject(hdc, op);
		DeleteObject(pen); DeleteObject(br);
	};
	drawBox(BX + 106, BY + 48, focusId);
	drawBox(BX + 106, BY + 78, !focusId);

	// Input values (ASCII — login is always English)
	std::wstring idW  = Input::GetLoginId();
	std::wstring pwW  = Input::GetLoginPw();
	std::wstring pwMask(pwW.size(), L'*');

	SelectObject(hdc, inputFont);
	SetTextColor(hdc, RGB(220, 220, 100));
	std::wstring idDisp  = focusId  ? (idW   + L"_") : idW;
	std::wstring pwDisp  = !focusId ? (pwMask + L"_") : pwMask;
	TextOutW(hdc, BX + 110, BY + 52, idDisp.c_str(),  (int)idDisp.size());
	TextOutW(hdc, BX + 110, BY + 82, pwDisp.c_str(),  (int)pwDisp.size());

	// Hint
	SelectObject(hdc, inputFont);
	SetTextColor(hdc, RGB(120, 120, 120));
	TextOutW(hdc, BX + 16, BY + 118, L"Tab: 전환   Enter: 로그인/회원가입", 20);

	// Server/validation message
	if (!mLoginMessage.empty())
	{
		bool isError = (mLoginMessage.find(L"오류") != std::wstring::npos ||
		                mLoginMessage.find(L"틀렸") != std::wstring::npos ||
		                mLoginMessage.find(L"입력") != std::wstring::npos);
		SetTextColor(hdc, isError ? RGB(255, 80, 80) : RGB(80, 255, 80));
		TextOutW(hdc, BX + 10, BY + 152, mLoginMessage.c_str(), (int)mLoginMessage.size());
	}

	SelectObject(hdc, oldFont);
	DeleteObject(titleFont);
	DeleteObject(labelFont);
	DeleteObject(inputFont);
}

void SendAttackToServer(DIRECTION dir)
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_Attack packet;
	packet.size = sizeof(C2S_Attack);
	packet.type = C2S_ATTACK;
	packet.dir  = dir;
	network->Send(&packet);
}

void SendAoeAttackToServer(DIRECTION dir)
{
	Network* network = GAME.GetNetwork();
	if (network == nullptr) return;
	C2S_AoeAttack packet;
	packet.size = sizeof(C2S_AoeAttack);
	packet.type = C2S_AOE_ATTACK;
	packet.dir  = dir;
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
		movePacket.x = static_cast<short>(tileX);
		movePacket.y = static_cast<short>(tileY);
		movePacket.dir = GAME.GetAvatar()->GetLastDirection();
		movePacket.move_time = 0;
		network->Send(&movePacket);
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