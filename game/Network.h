#pragma once
#pragma once

#include "protocol_2026.h"
#include "WinSock2.h"
#include "Windows.h"

class Game;  // Forward declaration

class Network
{
private:
	SOCKET mSocket;
	SOCKADDR_IN mServerAddr;
	char mBuf[MAX_BUF_SIZE];

public:
	Network();
	~Network();
	bool Connect(const char* ip, int port);
	void Send(void* packet);
	int Receive(char* buffer, int bufferSize);
	void ProcessPacket(char* packet);
	void ReceiveAndProcessPackets();  // 새로 추가: 패킷 수신 및 처리
};


