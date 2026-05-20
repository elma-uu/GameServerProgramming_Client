#include "Network.h"

Network::Network()
{
	mSocket = INVALID_SOCKET;
	ZeroMemory(&mServerAddr, sizeof(mServerAddr));
}

Network::~Network()
{
	if (mSocket != INVALID_SOCKET)
	{
		closesocket(mSocket);
	}
}

bool Network::Connect(const char* ip, int port)
{
	mServerAddr.sin_port = htons(port);
	mServerAddr.sin_addr.s_addr = inet_addr(ip);
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if (mSocket == INVALID_SOCKET)
	{
		return false;
	}
	int result = WSAConnect(mSocket, (SOCKADDR*)&mServerAddr, sizeof(mServerAddr), NULL, NULL, NULL, NULL);
	return result != SOCKET_ERROR;
}

void Network::Send(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	LPDWORD sent{ 0 };
	WSASend(mSocket, reinterpret_cast<WSABUF*>(&p), 1, sent, 0, NULL, NULL);
}

int Network::Receive(char* buffer, int bufferSize)
{
	LPDWORD received{ 0 };
	WSARecv(mSocket, reinterpret_cast<WSABUF*>(&buffer), 1, received, NULL, NULL, NULL);
}