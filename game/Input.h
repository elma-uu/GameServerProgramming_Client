#pragma once
#include "framework.h"
#include "Game.h"

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
    static std::wstring GetInputText() { return mInputText; } // 입력된 텍스트 반환
    static void ClearInputText() { mInputText.clear(); } // 입력 텍스트 초기화

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
    static std::wstring mInputText; // 입력된 문자열 저장
};