#include "framework.h"
#include "Game.h"

#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")

#define IDB_BITMAP1 129
#define MAX_LOADSTRING 100


// 전역 변수:
extern const UINT width = 1280;
extern const UINT height = 720;

HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 전역 문자열을 초기화합니다.
    wcscpy_s(szTitle, L"Client");
    wcscpy_s(szWindowClass, L"GameWindow");
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, 0);

    MSG msg;

    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
             // 시작
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 기본 애플리케이션 아이콘
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION); // 기본 작은 아이콘
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW); // 기본 커서
    wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wcex.lpszMenuName = nullptr; // 메뉴바 제거
    wcex.lpszClassName = szWindowClass;

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP,
        0, 0, 1920, 1080, nullptr, nullptr, hInstance, nullptr);
    GAME.Initialize(hWnd, 1920, 1080); // 애플리케이션 초기화

    if (!hWnd)
    {
        return FALSE;
    }

    SetWindowPos(hWnd, HWND_TOP, 0, 0, 1920, 1080, SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOZORDER);
    ShowWindow(hWnd, SW_MAXIMIZE);
    UpdateWindow(hWnd);
    // LoadScenes();

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        ShowCursor(true);
        
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CHAR:
    {
        Input::ProcessChar(wParam);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    }

    return (INT_PTR)FALSE;
}