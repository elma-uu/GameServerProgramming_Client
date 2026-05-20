#pragma once
#include "winsock.h"

class Network
{
private:
	SOCKET mSocket;
	SOCKADDR_IN mServerAddr;

public:
	Network();
	~Network();
	bool Connect(const char* ip, int port);
	void Send(const char* data, int length);
	int Receive(char* buffer, int bufferSize);
};

