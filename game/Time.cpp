#include "Time.h"


LARGE_INTEGER Time::CpuFrequency = {};
LARGE_INTEGER Time::PrevFrequency = {};
LARGE_INTEGER Time::CurrentFrequency = {};
float Time::DeltaTimeValue = 0.0f;
bool Time::TimeStop;
void Time::Initialize()
{
    // Cpu ���� ������
    QueryPerformanceFrequency(&CpuFrequency);

    // ���α׷��� ���� ���� �� ���� ������
    QueryPerformanceCounter(&PrevFrequency);
}

void Time::Update()
{

    QueryPerformanceCounter(&CurrentFrequency);

    float differenceFrequency
        = static_cast<float>(CurrentFrequency.QuadPart - PrevFrequency.QuadPart);
    DeltaTimeValue = differenceFrequency / static_cast<float>(CpuFrequency.QuadPart);
    PrevFrequency.QuadPart = CurrentFrequency.QuadPart;


}
void Time::Render(HDC hdc)
{
    static float time = 0.0f;

    time += DeltaTimeValue;
    float fps = 1.0f / DeltaTimeValue;

    wchar_t str[50] = L"";
    swprintf_s(str, 50, L"FPS : %d", (int)fps);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT); // ����� �����ϰ� ����
    HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"EXO 2");

    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    TextOut(hdc, 1280 / 2, 0, str, (int)wcslen(str));


    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

}
void Time::SetTimeStop(bool a)
{
    TimeStop = a;
}
float Time::DeltaTime() { return TimeStop ? 0.0f : DeltaTimeValue; }
