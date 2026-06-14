#include "ChatSystem.h"
#pragma comment(lib, "Msimg32.lib")

// Convert a UTF-8 multibyte string to UTF-16
// (project uses /utf-8, so all char strings are UTF-8)
static std::wstring ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

ChatSystem::ChatSystem() : mChatMode(false)
{
    // Load private Korean font so CreateFont can reference it by name
    AddFontResourceExA("./Resource/Font/NanumBarunGothic.ttf", FR_PRIVATE, nullptr);
}

ChatSystem::~ChatSystem()
{
    RemoveFontResourceExA("./Resource/Font/NanumBarunGothic.ttf", FR_PRIVATE, nullptr);
}

void ChatSystem::AddMessage(const std::string& sender, const std::string& text)
{
    mMessages.push_back({ sender, text });
    while (static_cast<int>(mMessages.size()) > MAX_MESSAGES)
        mMessages.pop_front();
}

void ChatSystem::Render(HDC hdc, const std::wstring& inputText)
{
    int bgBottom = mChatMode
        ? (INPUT_Y + LINE_HEIGHT + 4)
        : (MSG_START_Y + MAX_MESSAGES * LINE_HEIGHT + 2);

    // Semi-transparent chat background via AlphaBlend
    {
        int bgX = CHAT_X - 1;
        int bgY = MSG_START_Y - 1;
        int bgW = CHAT_WIDTH + 2;
        int bgH = bgBottom - bgY;

        HDC      memDC  = CreateCompatibleDC(hdc);
        HBITMAP  memBmp = CreateCompatibleBitmap(hdc, bgW, bgH);
        HBITMAP  oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        HBRUSH fill = CreateSolidBrush(RGB(10, 10, 25));
        RECT   rc   = { 0, 0, bgW, bgH };
        FillRect(memDC, &rc, fill);
        DeleteObject(fill);

        BLENDFUNCTION bf = {};
        bf.BlendOp             = AC_SRC_OVER;
        bf.SourceConstantAlpha = 160;
        bf.AlphaFormat         = 0;
        AlphaBlend(hdc, bgX, bgY, bgW, bgH, memDC, 0, 0, bgW, bgH, bf);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
    }

    // NanumBarunGothic with HANGUL_CHARSET for correct Korean glyph mapping
    HFONT hFont = CreateFontW(
        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        HANGUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"NanumBarunGothic");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);

    int i = 0;
    for (const auto& msg : mMessages)
    {
        std::wstring wline;
        if (msg.sender.empty())
            wline = ToWide(msg.text);
        else
            wline = ToWide(msg.sender) + L": " + ToWide(msg.text);

        TextOutW(hdc, CHAT_X + 4, MSG_START_Y + i * LINE_HEIGHT,
            wline.c_str(), static_cast<int>(wline.size()));
        ++i;
    }

    // Input box (visible only in chat mode)
    if (mChatMode)
    {
        HPEN   borderPen  = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HBRUSH inputBrush = CreateSolidBrush(RGB(40, 40, 40));
        SelectObject(hdc, borderPen);
        SelectObject(hdc, inputBrush);
        Rectangle(hdc, CHAT_X, INPUT_Y, CHAT_X + CHAT_WIDTH, INPUT_Y + LINE_HEIGHT + 2);
        DeleteObject(borderPen);
        DeleteObject(inputBrush);

        // inputText is already wstring — Korean renders directly
        std::wstring display = L"> " + inputText + L"_";
        SetTextColor(hdc, RGB(255, 255, 100));
        TextOutW(hdc, CHAT_X + 4, INPUT_Y + 2,
            display.c_str(), static_cast<int>(display.size()));
        SetTextColor(hdc, RGB(255, 255, 255));
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}
