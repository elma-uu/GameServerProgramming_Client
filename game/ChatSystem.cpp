#include "ChatSystem.h"

ChatSystem::ChatSystem() : mChatMode(false) {}

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

    // chat background
    HBRUSH bgBrush  = CreateSolidBrush(RGB(20, 20, 20));
    HPEN   nullPen  = (HPEN)GetStockObject(NULL_PEN);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN   oldPen   = (HPEN)SelectObject(hdc, nullPen);
    Rectangle(hdc, CHAT_X - 1, MSG_START_Y - 1, CHAT_X + CHAT_WIDTH + 1, bgBottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(bgBrush);

    // render messages
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);

    int i = 0;
    for (const auto& msg : mMessages) {
        std::string line = msg.sender + ": " + msg.text;
        TextOutA(hdc, CHAT_X + 4, MSG_START_Y + i * LINE_HEIGHT,
                 line.c_str(), static_cast<int>(line.size()));
        ++i;
    }

    // input box (visible only in chat mode)
    if (mChatMode) {
        HPEN   borderPen  = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HBRUSH inputBrush = CreateSolidBrush(RGB(40, 40, 40));
        SelectObject(hdc, borderPen);
        SelectObject(hdc, inputBrush);
        Rectangle(hdc, CHAT_X, INPUT_Y, CHAT_X + CHAT_WIDTH, INPUT_Y + LINE_HEIGHT + 2);
        DeleteObject(borderPen);
        DeleteObject(inputBrush);

        // build "> text_" prompt (ASCII only)
        std::string display = "> ";
        for (wchar_t c : inputText) {
            if (c >= 32 && c < 128)
                display += static_cast<char>(c);
        }
        display += '_';

        SetTextColor(hdc, RGB(255, 255, 100));
        TextOutA(hdc, CHAT_X + 4, INPUT_Y + 2,
                 display.c_str(), static_cast<int>(display.size()));
        SetTextColor(hdc, RGB(255, 255, 255));
    }
}
