#pragma once
#include "Windows.h"
#include "WinSock2.h"
#include "protocol_2026.h"

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
};

