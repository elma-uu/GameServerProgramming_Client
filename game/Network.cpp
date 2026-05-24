#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Network.h"
#include "Game.h"
#include <string>

Network::Network()
{
	mSocket = INVALID_SOCKET;
	ZeroMemory(&mServerAddr, sizeof(mServerAddr));

	// WSAStartup 초기화
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

Network::~Network()
{
	if (mSocket != INVALID_SOCKET)
	{
		closesocket(mSocket);
	}
	WSACleanup();
}

bool Network::Connect(const char* ip, int port)
{
	mServerAddr.sin_family = AF_INET;
	mServerAddr.sin_port = htons(port);
	mServerAddr.sin_addr.s_addr = inet_addr(ip);
	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if (mSocket == INVALID_SOCKET)
	{
		int error = WSAGetLastError();
		printf("Socket creation failed: %d\n", error);
		return false;
	}
	int result = WSAConnect(mSocket, (SOCKADDR*)&mServerAddr, sizeof(mServerAddr), NULL, NULL, NULL, NULL);
	if (result == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		printf("Connection failed: %d\n", error);
		return false;
	}
	printf("Connected to server successfully\n");
	return true;
}

void Network::Send(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	unsigned char packetSize = *p;
	DWORD sent = 0;
	WSABUF wsaBuf;
	wsaBuf.buf = (char*)p;
	wsaBuf.len = packetSize;
	WSASend(mSocket, &wsaBuf, 1, &sent, 0, NULL, NULL);
}

int Network::Receive(char* buffer, int bufferSize)
{
	DWORD received = 0;
	DWORD flags = 0;
	WSABUF wsaBuf;
	wsaBuf.buf = buffer;
	wsaBuf.len = bufferSize;
	int result = WSARecv(mSocket, &wsaBuf, 1, &received, &flags, NULL, NULL);
	return (result == SOCKET_ERROR) ? -1 : static_cast<int>(received);
}

void Network::ProcessPacket(char* recv_packet)
{
	switch (static_cast<PACKET_TYPE>(recv_packet[1]))
	{
	case S2C_LOGIN_RESULT:
	{
		S2C_LoginResult* packet = reinterpret_cast<S2C_LoginResult*>(recv_packet);
		if (packet->success)
		{
			// 로그인 성공 처리
		}
		else
		{
			// 로그인 실패 처리
		}
	}
	break;
	case S2C_AVATAR_INFO:
	{
		S2C_AvatarInfo* packet = reinterpret_cast<S2C_AvatarInfo*>(recv_packet);
		// 서버에서 받은 타일 좌표(x, y)를 픽셀 좌표로 변환 (타일 크기: 50)
		int pixelX = packet->x * 50;
		int pixelY = packet->y * 50;
		GAME.GetAvatar()
			->SetPlayerInfo(packet->playerId, pixelX, pixelY, packet->hp, packet->max_hp, packet->exp, packet->level);
	}
	break;
	case S2C_ADD_OBJECT:
	{
		S2C_AddObject* packet = reinterpret_cast<S2C_AddObject*>(recv_packet);
		GAME.GetAvatar()->AddObject(packet->object_id, std::string(packet->obj_name), 
			packet->visual_id, packet->x, packet->y, packet->hp, packet->max_hp, 
			packet->exp, packet->level);
	}
	break;
	case S2C_REMOVE_OBJECT:
	{
		S2C_RemoveObject* packet = reinterpret_cast<S2C_RemoveObject*>(recv_packet);
		GAME.GetAvatar()->RemoveObject(packet->object_id);
	}
	break;
	case S2C_MOVE_OBJECT:
	{
		S2C_MoveObject* packet = reinterpret_cast<S2C_MoveObject*>(recv_packet);
		GAME.GetAvatar()->UpdateObjectPosition(packet->object_id, packet->x, packet->y);
	}
	break;
	case S2C_CHAT_MESSAGE:
	{
		S2C_ChatMessage* packet = reinterpret_cast<S2C_ChatMessage*>(recv_packet);
		// 채팅 메시지 처리
	}
	break;
	case S2C_STATUS_CHANGE:
	{
		S2C_StatusChange* packet = reinterpret_cast<S2C_StatusChange*>(recv_packet);
		GAME.GetAvatar()->UpdateObjectStatus(packet->object_id, packet->hp, packet->max_hp, 
			packet->exp, packet->level);
	}
	break;

	default:
		// Unknown Packet Type
		break;
	}
}

void Network::ReceiveAndProcessPackets()
{
	int receivedBytes = Receive(mBuf, MAX_BUF_SIZE);
	if (receivedBytes > 0)
	{
		// 받은 데이터를 패킷 단위로 처리
		// 첫 번째 바이트는 패킷 크기
		int offset = 0;
		while (offset < receivedBytes)
		{
			if (offset + 1 > receivedBytes)
				break;

			int packetSize = static_cast<unsigned char>(mBuf[offset]);

			if (packetSize <= 0 || offset + packetSize > receivedBytes)
				break;

			ProcessPacket(&mBuf[offset]);
			offset += packetSize;
		}
	}
}