#pragma once
#include "framework.h"
#include <deque>

class ChatSystem {
public:
    struct ChatMessage {
        std::string sender;
        std::string text;
    };

    ChatSystem();

    void AddMessage(const std::string& sender, const std::string& text);
    void Render(HDC hdc, const std::wstring& inputText);

    bool IsChatMode() const { return mChatMode; }
    void SetChatMode(bool mode) { mChatMode = mode; }

private:
    static const int MAX_MESSAGES = 7;
    static const int CHAT_X      = 5;
    static const int CHAT_WIDTH  = 395;
    static const int MSG_START_Y = 450;
    static const int LINE_HEIGHT = 17;
    static const int INPUT_Y     = 572;

    std::deque<ChatMessage> mMessages;
    bool mChatMode;
};
