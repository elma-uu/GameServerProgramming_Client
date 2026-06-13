#pragma once
#include "framework.h"

// Forward declaration to avoid circular dependency
class Game;

extern Game GAME;

enum class eKeyState
{
    Down,
    Pressed,
    Up,
    None,
};

enum class eKeyCode
{
    Q, W, E, R, T, Y, U, I, O, P,
    A, S, D, F, G, H, J, K, L,
    Z, X, C, V, B, N, M,
    Left, Right, Down, Up,
    LButton, MButton, RButton, SPACE, ESC, BACK,
    End
};

class Input
{
public:
    struct Key
    {
        eKeyCode keyCode;
        eKeyState state;
        bool bPressed;
    };

    static void Initialize();
    static void Update();

    static bool GetKeyDown(eKeyCode code) { return Keys[(UINT)code].state == eKeyState::Down; }
    static bool GetKeyUp(eKeyCode code) { return Keys[(UINT)code].state == eKeyState::Up; }
    static bool GetKey(eKeyCode code) { return Keys[(UINT)code].state == eKeyState::Pressed; }
    static Vector2 GetMousePosition() { return mMousePosition; }
    static void ProcessChar(WPARAM wParam); // WM_CHAR 메시지 처리
    static std::wstring GetInputText() { return mInputText; }
    static void ClearInputText() { mInputText.clear(); }

    static bool IsChatMode() { return mChatMode; }
    static void SetChatMode(bool mode) { mChatMode = mode; }
    static bool ConsumeChatSubmit();

private:
    static void createKeys();
    static void updateKeys();
    static void updateKey(Input::Key& key);
    static bool isKeyDown(eKeyCode code);
    static void updateKeyDown(Input::Key& key);
    static void updateKeyUp(Input::Key& key);
    static void getMousePositionByWindow();
    static void clearKeys();

private:
    static std::vector<Key> Keys;
    static Vector2 mMousePosition;
    static std::wstring mInputText;
    static bool mChatMode;
    static bool mChatSubmit;
};