#include "Input.h"

std::vector<Input::Key> Input::Keys = {};
Vector2 Input::mMousePosition = Vector2::One;
std::wstring Input::mInputText;

int ASCII[(UINT)eKeyCode::End] =
{
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M',
    VK_LEFT, VK_RIGHT, VK_DOWN, VK_UP,
    VK_LBUTTON, VK_MBUTTON, VK_RBUTTON, VK_SPACE, VK_ESCAPE, VK_BACK,
};

void Input::ProcessChar(WPARAM wParam)
{
    if (wParam == VK_BACK)
    {
        if (!mInputText.empty())
        {
            mInputText.pop_back();
        }
    }
    else if (wParam >= 32 && wParam <= 126 && mInputText.length() < 20) // 아스키 문자 범위
    {
        mInputText += static_cast<wchar_t>(wParam);
    }
}

void Input::Initialize()
{
    createKeys();
    ClearInputText();
}

void Input::Update()
{
    updateKeys();
}

void Input::createKeys()
{
    for (size_t i = 0; i < (UINT)eKeyCode::End; i++)
    {
        Key key = {};
        key.bPressed = false;
        key.state = eKeyState::None;
        key.keyCode = (eKeyCode)i;

        Keys.push_back(key);
    }
}

void Input::updateKeys()
{
    std::for_each(Keys.begin(), Keys.end(),
        [](Key& key) -> void
        {
            updateKey(key);
        });
}

void Input::updateKey(Input::Key& key)
{
    if (GetFocus())
    {
        if (isKeyDown(key.keyCode))
            updateKeyDown(key);
        else
            updateKeyUp(key);

        getMousePositionByWindow();
    }
    else
    {
        clearKeys();
    }
}

bool Input::isKeyDown(eKeyCode code)
{
    return GetAsyncKeyState(ASCII[(UINT)code]) & 0x8000;
}

void Input::updateKeyDown(Input::Key& key)
{
    if (key.bPressed == true)
        key.state = eKeyState::Pressed;
    else
        key.state = eKeyState::Down;

    key.bPressed = true;
}

void Input::updateKeyUp(Input::Key& key)
{
    if (key.bPressed == true)
        key.state = eKeyState::Up;
    else
        key.state = eKeyState::None;

    key.bPressed = false;
}

void Input::getMousePositionByWindow()
{
    POINT mousePos = {};
    if (!GetCursorPos(&mousePos))
    {
        OutputDebugStringW(L"Error: GetCursorPos failed\n");
        return;
    }

    HWND hWnd = GAME.GetHwnd();
    if (hWnd == nullptr)
    {
        OutputDebugStringW(L"Error: HWND is nullptr\n");
        return;
    }

    if (!ScreenToClient(hWnd, &mousePos))
    {
        OutputDebugStringW(L"Error: ScreenToClient failed\n");
        return;
    }

    mMousePosition.x = static_cast<float>(mousePos.x);
    mMousePosition.y = static_cast<float>(mousePos.y);
}

void Input::clearKeys()
{
    for (Key& key : Keys)
    {
        if (key.state == eKeyState::Down || key.state == eKeyState::Pressed)
            key.state = eKeyState::Up;
        else if (key.state == eKeyState::Up)
            key.state = eKeyState::None;

        key.bPressed = false;
    }
}